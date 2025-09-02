// license:BSD-3-Clause
// copyright-holders:Ariane Fugmann
/***************************************************************************

    Namco C345 - Serial I/F Controller

    used on SYSTEM-FL MAIN PCB at 21B
    connected to SRAM at 21A and 22A
    nearby traces pass markings JP11 (RXD1)
    as well as JP12 (TXD1B) and JP13 (TXD1A)

***************************************************************************/

#include "emu.h"
#include "namco_c345.h"

#include "emuopts.h"
#include "multibyte.h"

#include "asio.h"

#include <iostream>

#define VERBOSE 1
#include "logmacro.h"

class namco_c345_device::context
{
public:
	context() :
	m_acceptor(m_ioctx),
	m_sock_rx(m_ioctx),
	m_sock_tx(m_ioctx),
	m_timeout_tx(m_ioctx),
	m_state_rx(0U),
	m_state_tx(0U)
	{
	}

	void start()
	{
	}

	void reset(std::string localhost, std::string localport, std::string remotehost, std::string remoteport)
	{
		std::error_code err;
		if (m_acceptor.is_open())
			m_acceptor.close(err);
		if (m_sock_rx.is_open())
			m_sock_rx.close(err);
		if (m_sock_tx.is_open())
			m_sock_tx.close(err);
		m_timeout_tx.cancel();
		m_state_rx.store(0);
		m_state_tx.store(0);

		asio::ip::tcp::resolver resolver(m_ioctx);

		for (auto &&resolveIte : resolver.resolve(localhost, localport, asio::ip::tcp::resolver::flags::address_configured, err))
		{
			m_localaddr = resolveIte.endpoint();
			LOG("C345: localhost = %s\n", *m_localaddr);
		}
		if (err)
		{
			LOG("C345: localhost resolve error: %s\n", err.message());
		}

		for (auto &&resolveIte : resolver.resolve(remotehost, remoteport, asio::ip::tcp::resolver::flags::address_configured, err))
		{
			m_remoteaddr = resolveIte.endpoint();
			LOG("C345: remotehost = %s\n", *m_remoteaddr);
		}
		if (err)
		{
			LOG("C345: remotehost resolve error: %s\n", err.message());
		}
	}

	void stop()
	{
		std::error_code err;
		if (m_acceptor.is_open())
			m_acceptor.close(err);
		if (m_sock_rx.is_open())
			m_sock_rx.close(err);
		if (m_sock_tx.is_open())
			m_sock_tx.close(err);
		m_timeout_tx.cancel();
		m_state_rx.store(0);
		m_state_tx.store(0);
		m_ioctx.stop();
	}

	void check_sockets()
	{
		// if async operation in progress, poll context
		if ((m_state_rx > 0) || (m_state_tx > 0))
			m_ioctx.poll();

		// start acceptor if needed
		if (m_localaddr && m_state_rx.load() == 0)
		{
			start_accept();
		}

		// connect socket if needed
		if (m_remoteaddr && m_state_tx.load() == 0)
		{
			start_connect();
		}
	}

	bool connected()
	{
		return m_state_rx.load() == 2 && m_state_tx.load() == 2;
	}

	unsigned receive(uint8_t *buffer, unsigned data_size)
	{
		if (m_state_rx.load() < 2)
			return UINT_MAX;

		m_ioctx.poll();

		if (data_size > m_fifo_rx.used())
			return 0;

		return m_fifo_rx.read(&buffer[0], data_size, false);
	}

	unsigned send(uint8_t *buffer, unsigned data_size)
	{
		if (m_state_tx.load() < 2)
			return UINT_MAX;

		if (data_size > m_fifo_tx.free())
		{
			LOG("C345: TX buffer overflow\n");
			return UINT_MAX;
		}

		bool const sending = m_fifo_tx.used();
		m_fifo_tx.write(&buffer[0], data_size);
		if (!sending)
			start_send_tx();

		m_ioctx.poll();

		return data_size;
	}

private:
	class fifo
	{
	public:
		unsigned write(uint8_t *buffer, unsigned data_size)
		{
			unsigned used = 0;
			if (m_wp >= m_rp)
			{
				used = std::min<unsigned>(m_buffer.size() - m_wp, data_size);
				std::copy_n(&buffer[0], used, &m_buffer[m_wp]);
				m_wp = (m_wp + used) % m_buffer.size();
			}
			unsigned const block = std::min<unsigned>(data_size - used, m_rp - m_wp);
			if (block)
			{
				std::copy_n(&buffer[used], block, &m_buffer[m_wp]);
				used += block;
				m_wp += block;
			}
			m_used += used;
			return used;
		}

		unsigned read(uint8_t *buffer, unsigned data_size, bool peek)
		{
			unsigned rp = m_rp;
			unsigned used = 0;
			if (rp >= m_wp)
			{
				used = std::min<std::size_t>(m_buffer.size() - rp, data_size);
				std::copy_n(&m_buffer[rp], used, &buffer[0]);
				rp = (rp + used) % m_buffer.size();
			}
			unsigned const block = std::min<unsigned>(data_size - used, m_wp - rp);
			if (block)
			{
				std::copy_n(&m_buffer[rp], block, &buffer[used]);
				used += block;
				rp += block;
			}
			if (!peek)
			{
				m_rp = (m_rp + used) % m_buffer.size();
				m_used -= used;
			}
			return used;
		}

		void consume(unsigned data_size)
		{
			m_rp = (m_rp + data_size) % m_buffer.size();
			m_used -= data_size;
		}

		unsigned used()
		{
			return m_used;
		}

		unsigned free()
		{
			return m_buffer.size() - m_used;
		}

		void clear()
		{
			m_wp = m_rp = m_used = 0;
		}


	private:
		unsigned m_wp = 0;
		unsigned m_rp = 0;
		unsigned m_used = 0;
		std::array<uint8_t, 0x80000> m_buffer;
	};

	void start_accept()
	{
		std::error_code err;
		m_acceptor.open(m_localaddr->protocol(), err);
		m_acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
		if (!err)
		{
			m_acceptor.bind(*m_localaddr, err);
			if (!err)
			{
				m_acceptor.listen(1, err);
				if (!err)
				{
					osd_printf_verbose("C345: RX listen on %s\n", *m_localaddr);
					m_acceptor.async_accept(
							[this] (std::error_code const &err, asio::ip::tcp::socket sock)
							{
								if (err)
								{
									LOG("C345: RX error accepting - %d %s\n", err.value(), err.message());
									std::error_code e;
									m_acceptor.close(e);
									m_state_rx.store(0);
								}
								else
								{
									LOG("C345: RX connection from %s\n", sock.remote_endpoint());
									std::error_code e;
									m_acceptor.close(e);
									m_sock_rx = std::move(sock);
									m_sock_rx.set_option(asio::socket_base::keep_alive(true));
									m_state_rx.store(2);
									start_receive_rx();
								}
							});
					m_state_rx.store(1);
				}
			}
		}
		if (err)
		{
			LOG("C345: RX failed - %d %s\n", err.value(), err.message());
		}
	}

	void start_connect()
	{
		std::error_code err;
		if (m_sock_tx.is_open())
			m_sock_tx.close(err);
		m_sock_tx.open(m_remoteaddr->protocol(), err);
		if (!err)
		{
			m_sock_tx.set_option(asio::ip::tcp::no_delay(true));
			m_sock_tx.set_option(asio::socket_base::keep_alive(true));
			osd_printf_verbose("C345: TX connecting to %s\n", *m_remoteaddr);
			m_timeout_tx.expires_after(std::chrono::seconds(10));
			m_timeout_tx.async_wait(
					[this] (std::error_code const &err)
					{
						if (!err && m_state_tx.load() == 1)
						{
							osd_printf_verbose("C345: TX connect timed out\n");
							std::error_code e;
							m_sock_tx.close(e);
							m_state_tx.store(0);
						}
					});
			m_sock_tx.async_connect(
					*m_remoteaddr,
					[this] (std::error_code const &err)
					{
						m_timeout_tx.cancel();
						if (err)
						{
							osd_printf_verbose("C345: TX connect error - %d %s\n", err.value(), err.message());
							std::error_code e;
							m_sock_tx.close(e);
							m_state_tx.store(0);
						}
						else
						{
							LOG("C345: TX connection established\n");
							m_state_tx.store(2);
						}
					});
			m_state_tx.store(1);
		}
	}

	void start_send_tx()
	{
		unsigned used = m_fifo_tx.read(&m_buffer_tx[0], std::min<unsigned>(m_fifo_tx.used(), m_buffer_tx.size()), true);
		m_sock_tx.async_write_some(
				asio::buffer(&m_buffer_tx[0], used),
				[this] (std::error_code const &err, std::size_t length)
				{
					m_fifo_tx.consume(length);
					if (err)
					{
						LOG("C345: TX connection error: %s\n", err.message().c_str());
						m_sock_tx.close();
						m_state_tx.store(0);
						m_fifo_tx.clear();
					}
					else if (m_fifo_tx.used())
					{
						start_send_tx();
					}
				});
	}

	void start_receive_rx()
	{
		m_sock_rx.async_read_some(
				asio::buffer(m_buffer_rx),
				[this] (std::error_code const &err, std::size_t length)
				{
					if (err || !length)
					{
						if (err)
							LOG("C345: RX connection error: %s\n", err.message());
						else
							LOG("C345: RX connection lost\n");
						m_sock_rx.close();
						m_state_rx.store(0);
						m_fifo_rx.clear();
					}
					else
					{
						if (UINT_MAX == m_fifo_rx.write(&m_buffer_rx[0], length))
						{
							LOG("C345: RX buffer overflow\n");
							m_sock_rx.close();
							m_state_rx.store(0);
							m_fifo_rx.clear();
						}
						start_receive_rx();
					}
				});
	}

	template <typename Format, typename... Params>
	void logerror(Format &&fmt, Params &&... args) const
	{
		util::stream_format(
				std::cerr,
				"%s",
				util::string_format(std::forward<Format>(fmt), std::forward<Params>(args)...));
	}

	asio::io_context m_ioctx;
	std::optional<asio::ip::tcp::endpoint> m_localaddr;
	std::optional<asio::ip::tcp::endpoint> m_remoteaddr;
	asio::ip::tcp::acceptor m_acceptor;
	asio::ip::tcp::socket m_sock_rx;
	asio::ip::tcp::socket m_sock_tx;
	asio::steady_timer m_timeout_tx;
	std::atomic_uint m_state_rx;
	std::atomic_uint m_state_tx;
	fifo m_fifo_rx;
	fifo m_fifo_tx;
	std::array<uint8_t, 0x400> m_buffer_rx;
	std::array<uint8_t, 0x400> m_buffer_tx;
};

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

// device type definition
DEFINE_DEVICE_TYPE(NAMCO_C345, namco_c345_device, "namco_c345", "Namco C345 Serial")


//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

void namco_c345_device::data_map(address_map &map)
{
	map(0x0000, 0x3fff).rw(FUNC(namco_c345_device::ram_r),FUNC(namco_c345_device::ram_w));
}

void namco_c345_device::regs_map(address_map &map)
{
	map(0x00, 0x1f).rw(FUNC(namco_c345_device::reg_r), FUNC(namco_c345_device::reg_w));
}

namco_c345_device::namco_c345_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, NAMCO_C345, tag, owner, clock),
	m_irq_cb(*this)
{
	auto const &opts = mconfig.options();

	m_localhost = opts.comm_localhost();
	m_localport = opts.comm_localport();
	m_remotehost = opts.comm_remotehost();
	m_remoteport = opts.comm_remoteport();
	m_framesync = opts.comm_framesync() ? 0x01 : 0x00;

	// come up with some magic number for identification
	std::string remotehost = util::string_format("%s:%s", m_remotehost, m_remoteport);
	m_linkid = 0;
	for (int x = 0; x < sizeof(remotehost) && remotehost[x] != 0; x++)
	{
		m_linkid ^= remotehost[x];
	}

	LOG("C345: ID byte = %02d\n", m_linkid);

	std::fill(std::begin(m_buffer), std::end(m_buffer), 0);
}

void namco_c345_device::device_start()
{
	auto ctx = std::make_unique<context>();
	m_context = std::move(ctx);
	m_context->start();

	// state saving
	save_item(NAME(m_ram));
	save_item(NAME(m_reg));
	save_item(NAME(m_id));

	save_item(NAME(m_linkid));
	save_item(NAME(m_txblock));
}

void namco_c345_device::device_reset()
{
	std::fill(std::begin(m_ram), std::end(m_ram), 0);
	std::fill(std::begin(m_reg), std::end(m_reg), 0);
	std::fill(std::begin(m_id), std::end(m_id), 0);

	m_context->reset(m_localhost, m_localport, m_remotehost, m_remoteport);

	m_linkstate = 0x00;
	m_txblock = 0x00;
}

void namco_c345_device::device_stop()
{
	m_context->stop();
	m_context.reset();
}


//**************************************************************************
//  READ/WRITE HANDLERS
//**************************************************************************
void namco_c345_device::vblank_tick()
{
	comm_tick();
}

uint16_t namco_c345_device::ram_r(offs_t offset)
{
	return m_ram[offset];
}

void namco_c345_device::ram_w(offs_t offset, uint16_t data, uint16_t mem_mask)
{
	COMBINE_DATA(&m_ram[offset]);
}

uint16_t namco_c345_device::reg_r(offs_t offset)
{
	uint16_t result = m_reg[offset];
	if (!machine().side_effects_disabled())
	{
		LOG("C345: reg_r[%02x] = %04x\n", offset, result);
		if (offset == 1)
			return 0x007d;
		if (offset == 9)
			return 0;
		return 0x00ff;
	}

	return result;
}

void namco_c345_device::reg_w(offs_t offset, uint16_t data, uint16_t mem_mask)
{
	if (!machine().side_effects_disabled())
		LOG("C345: reg_w[%02x] = %04x\n", offset, data);
	m_reg[offset] = data;

	if (offset == 0x09 && data == 0x01)
	{
		if (m_reg[0x04] == 0x0001)
		{
			if (m_txblock == 0x00 && m_linkstate == 0x01)
			{
				unsigned data_size = 0x200;
				unsigned tx_size = m_reg[0x07];
				unsigned buf_offset = 0;
				for (unsigned j = 0x00; j < tx_size; j++)
				{
					uint16_t data = m_ram[j];
					put_u16be(&m_buffer[buf_offset], data);
					buf_offset += 2;
				}
				m_buffer[0x1fe] = m_linkid;
				m_buffer[0x1ff] = tx_size;
				send_frame(data_size);
				m_txblock = 0x01;
			}
		}
	}
}

void namco_c345_device::comm_tick()
{
	m_context->check_sockets();

	// if both sockets are connected
	if (m_context->connected())
	{
		// link established
		unsigned data_size = 0x200;

		if (m_reg[0x04] == 0x0001)
		{
			// clear buffer
			/*
			for (unsigned j = 0x0400; j < 0x2000; j++)
			{
				m_ram[j] = 0;
			}
			*/

			// try to read a message
			do
			{
				unsigned recv = read_frame(data_size);
				while (recv > 0)
				{
					unsigned rx_id = m_buffer[0x1fe];
					unsigned rx_size = m_buffer[0x1ff];

					// if not own message
					if (rx_id != m_linkid)
					{
						// if 0-size frame
						if (rx_size == 0)
						{
							m_buffer[0x000]++;
						}
						else
						{
							int id = comm_find_id(rx_id);
							if (id > 0)
							{
								// save message to buffer
								unsigned rx_offset = id * 0x0400;
								unsigned buf_offset = 0;
								for (unsigned j = 0x00; j < rx_size; j++)
								{
									uint16_t data = get_u16be(&m_buffer[buf_offset]);
									m_ram[rx_offset + j] = data;
									buf_offset += 2;
								}
							}
						}

						// forward message to other nodes
						send_frame(data_size);
					}
					else
					{
						if (m_linkstate == 0x00 && rx_size == 0)
						{
							m_linkstate = 0x01;
							osd_printf_verbose("C345: link online, %u nodes found.\n", m_buffer[0x000]);
						}
						m_txblock = 0x00;
					}

					// try to read another message
					recv = read_frame(data_size);
				}
			}
			while (m_framesync && m_txblock && m_linkstate);

			if (m_linkstate == 0x00)
			{
				// send 0-size-frame
				m_buffer[0x000] = 0x01;
				m_buffer[0x1fe] = m_linkid;
				m_buffer[0x1ff] = 0x00;
				send_frame(data_size);
				m_txblock = 0x01;
			}
		}
	}
}

int namco_c345_device::comm_find_id(uint8_t id)
{
	if (id == m_linkid)
		return 0;

	for (int j = 1; j < 8; j++)
	{
		if (m_id[j] == id)
			return j;
		if (m_id[j] == 0)
		{
			m_id[j] = id;
			return j;
		}
	}

	return -1;
}

unsigned namco_c345_device::read_frame(unsigned data_size)
{
	unsigned bytes_read = m_context->receive(&m_buffer[0], data_size);
	if (bytes_read == UINT_MAX)
	{
		// ignore errors
		if (m_linkstate == 0x01)
		{
			osd_printf_verbose("C345: link lost.\n");
			m_linkstate = 0x00;
		}
		m_txblock = 0x00;
		return 0;
	}
	return bytes_read;
}

void namco_c345_device::send_frame(unsigned data_size)
{
	unsigned bytes_sent = m_context->send(&m_buffer[0], data_size);
	if (bytes_sent == UINT_MAX)
	{
		// ignore errors
		if (m_linkstate == 0x01)
		{
			osd_printf_verbose("C345: link lost.\n");
			m_linkstate = 0x00;
		}
		m_txblock = 0x00;
	}
}