// license:BSD-3-Clause
// copyright-holders:Ariane Fugmann

/*
There are in fact 3 boards.
Exact differences are unknown.

COM-LINK PCB BR-8956
COM-LINK PCB CH-9074
it seems f-1 grand prix star (II) has this included on the PCB.
*/

#include "emu.h"
#include "cischeat_comlink.h"

#include "emuopts.h"

#include "asio.h"

#include <iostream>

#define VERBOSE 0
#include "logmacro.h"

class jaleco_cischeat_comlink_device::context
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
			LOG("COM-LINK: localhost = %s\n", *m_localaddr);
		}
		if (err)
		{
			LOG("COM-LINK: localhost resolve error: %s\n", err.message());
		}

		for (auto &&resolveIte : resolver.resolve(remotehost, remoteport, asio::ip::tcp::resolver::flags::address_configured, err))
		{
			m_remoteaddr = resolveIte.endpoint();
			LOG("COM-LINK: remotehost = %s\n", *m_remoteaddr);
		}
		if (err)
		{
			LOG("COM-LINK: remotehost resolve error: %s\n", err.message());
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
			LOG("COM-LINK: TX buffer overflow\n");
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
					osd_printf_verbose("COM-LINK: RX listen on %s\n", *m_localaddr);
					m_acceptor.async_accept(
							[this] (std::error_code const &err, asio::ip::tcp::socket sock)
							{
								if (err)
								{
									LOG("COM-LINK: RX error accepting - %d %s\n", err.value(), err.message());
									std::error_code e;
									m_acceptor.close(e);
									m_state_rx.store(0);
								}
								else
								{
									LOG("COM-LINK: RX connection from %s\n", sock.remote_endpoint());
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
			LOG("COM-LINK: RX failed - %d %s\n", err.value(), err.message());
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
			osd_printf_verbose("COM-LINK: TX connecting to %s\n", *m_remoteaddr);
			m_timeout_tx.expires_after(std::chrono::seconds(10));
			m_timeout_tx.async_wait(
					[this] (std::error_code const &err)
					{
						if (!err && m_state_tx.load() == 1)
						{
							osd_printf_verbose("COM-LINK: TX connect timed out\n");
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
							osd_printf_verbose("COM-LINK: TX connect error - %d %s\n", err.value(), err.message());
							std::error_code e;
							m_sock_tx.close(e);
							m_state_tx.store(0);
						}
						else
						{
							LOG("COM-LINK: TX connection established\n");
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
						LOG("COM-LINK: TX connection error: %s\n", err.message().c_str());
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
							LOG("COM-LINK: RX connection error: %s\n", err.message());
						else
							LOG("COM-LINK: RX connection lost\n");
						m_sock_rx.close();
						m_state_rx.store(0);
						m_fifo_rx.clear();
					}
					else
					{
						if (UINT_MAX == m_fifo_rx.write(&m_buffer_rx[0], length))
						{
							LOG("COM-LINK: RX buffer overflow\n");
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

DEFINE_DEVICE_TYPE(JALECO_CISCOHEAT_COMLINK, jaleco_cischeat_comlink_device, "comlink", "Jaleco COM-LINK PCB")

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

jaleco_cischeat_comlink_device::jaleco_cischeat_comlink_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, JALECO_CISCOHEAT_COMLINK, tag, owner, clock)
{
	m_framesize = 0x0200;
	m_framesync = mconfig.options().comm_framesync() ? 0x01 : 0x00;
	m_linkmax = 0x04;
}

void jaleco_cischeat_comlink_device::device_start()
{
	auto ctx = std::make_unique<context>();
	m_context = std::move(ctx);
	m_context->start();
}

void jaleco_cischeat_comlink_device::device_reset()
{
	std::fill(std::begin(m_shared), std::end(m_shared), 0);
	std::fill(std::begin(m_buffer), std::end(m_buffer), 0);

	auto const &opts = mconfig().options();
	m_context->reset(opts.comm_localhost(), opts.comm_localport(), opts.comm_remotehost(), opts.comm_remoteport());
}

void jaleco_cischeat_comlink_device::device_stop()
{
	m_context->stop();
	m_context.reset();
}

uint8_t jaleco_cischeat_comlink_device::share_r(offs_t offset)
{
	return m_shared[offset];
}

void jaleco_cischeat_comlink_device::share_w(offs_t offset, uint8_t data)
{
	if ((offset % m_framesize) == 0)
		m_linkid = (offset / m_framesize);
	m_shared[offset] = data;
}

void jaleco_cischeat_comlink_device::handle_vint_irq()
{
	comm_tick();
}

void jaleco_cischeat_comlink_device::setup_comlink(uint16_t frame_size, uint8_t link_max)
{
	m_framesize = frame_size;
	m_linkmax = link_max;
}

void jaleco_cischeat_comlink_device::comm_tick()
{
	m_context->check_sockets();

	// if both sockets are connected transmit data
	if (m_context->connected())
	{
		bool is_master = m_linkid == 0;
		bool is_slave = !is_master;
		unsigned data_size = m_framesize + 1;

		do
		{
			// try to read a message
			unsigned recv = read_frame(data_size);
			while (recv > 0)
			{
				// check if valid id
				uint8_t idx = m_buffer[m_framesize];
				if (idx >= 0 && idx < m_linkmax)
				{
					// if not own message
					if (idx != m_linkid)
					{
						// save message to "ring buffer"
						std::copy_n(&m_buffer[0], m_framesize, &m_shared[idx * m_framesize]);

						// forward message to other nodes
						send_frame(data_size);
					}
				}
				else
				{
					if (idx == 0xff)
					{
						// 0xFF - VSYNC
						m_linkwait = 0x00;

						if (is_slave)
							// forward message to other nodes
							send_frame(data_size);
					}
				}

				// try to read another message
				recv = read_frame(data_size);
			}
		}
		while (m_linkwait == 0x01);

		if (m_linkid < m_linkmax)
		{
			// enable wait for vsync
			m_linkwait = m_framesync;

			// send local data
			send_data(m_linkid, m_linkid * m_framesize, m_framesize, data_size);
		}

		if (is_master)
		{
			// send vsync
			m_buffer[m_framesize] = 0xff;
			send_frame(data_size);
		}
	}
}

unsigned jaleco_cischeat_comlink_device::read_frame(unsigned data_size)
{
	unsigned bytes_read = m_context->receive(&m_buffer[0], data_size);
	if (bytes_read == UINT_MAX)
	{
		m_linkwait = 0x00;
		return 0;
	}
	return bytes_read;
}

void jaleco_cischeat_comlink_device::send_data(uint8_t frame_type, unsigned frame_start, unsigned frame_size, unsigned data_size)
{
	m_buffer[m_framesize] = frame_type;
	std::copy_n(&m_shared[frame_start], frame_size, &m_buffer[0]);
	send_frame(data_size);
}

void jaleco_cischeat_comlink_device::send_frame(unsigned data_size)
{
	unsigned bytes_sent = m_context->send(&m_buffer[0], data_size);
	if (bytes_sent == UINT_MAX)
	{
		m_linkwait = 0x00;
	}
}