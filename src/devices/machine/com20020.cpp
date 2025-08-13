// license:BSD-3-Clause
// copyright-holders:Ariane Fugmann
/**************************************************************************************************

Standard Microsystems Corp. COM20020 Universal Local Area Netowrk Controller (ULANC)

Skeleton implementation based on datasheet DS00002704C (08-10-20)

Notes:
  on setting node id, mem 0 should read 0xD1 and mem 1 should be the node id.

**************************************************************************************************/

#include "emu.h"
#include "com20020.h"

#include "emuopts.h"
#include "multibyte.h"

#include "asio.h"

#include <iostream>

#define VERBOSE (0)
#include "logmacro.h"

class com20020_device::context
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

		m_thread = std::thread(
				[this] ()
				{
					LOG("COM20020: network thread started\n");
					try {
						m_ioctx.run();
					} catch (const std::exception& e) {
						LOG("COM20020: Exception in network thread: %s\n", e.what());
					} catch (...) { // Catch any other unknown exceptions
						LOG("COM20020: Unknown exception in network thread\n");
					}
					LOG("COM20020: network thread completed\n");
				});
	}

	void reset(std::string localhost, std::string localport, std::string remotehost, std::string remoteport)
	{
		std::error_code err;
		asio::ip::tcp::resolver resolver(m_ioctx);

		for (auto &&resolveIte : resolver.resolve(localhost, localport, asio::ip::tcp::resolver::flags::address_configured, err))
		{
			m_localaddr = resolveIte.endpoint();
			LOG("COM20020: localhost = %s\n", *m_localaddr);
		}
		if (err)
		{
			LOG("COM20020: localhost resolve error: %s\n", err.message());
		}

		for (auto &&resolveIte : resolver.resolve(remotehost, remoteport, asio::ip::tcp::resolver::flags::address_configured, err))
		{
			m_remoteaddr = resolveIte.endpoint();
			LOG("COM20020: remotehost = %s\n", *m_remoteaddr);
		}
		if (err)
		{
			LOG("COM20020: remotehost resolve error: %s\n", err.message());
		}

		m_ioctx.post(
				[this] ()
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
					start_accept();
					start_connect();
				});
	}

	void stop()
	{
		m_ioctx.post(
				[this] ()
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
				});
		m_work_guard.reset();
		if (m_thread.joinable()) {
			m_thread.join();
		}
	}

	void check_sockets()
	{
	}

	bool connected()
	{
		return m_state_rx.load() == 2 && m_state_tx.load() == 2;
	}

	unsigned receive(uint8_t *buffer, unsigned data_size)
	{
		if (m_state_rx.load() < 2)
			return UINT_MAX;

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
			LOG("COM20020: TX buffer overflow\n");
			return UINT_MAX;
		}

		bool const sending = m_fifo_tx.used();
		m_fifo_tx.write(&buffer[0], data_size);
		if (!sending)
			m_ioctx.post(
					[this] ()
					{
						start_send_tx();
					});
		return data_size;
	}

private:
	class fifo
	{
	public:
		unsigned write(uint8_t *buffer, unsigned data_size)
		{
			std::lock_guard<std::mutex> lock(m_mutex);
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
			std::lock_guard<std::mutex> lock(m_mutex);
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
			std::lock_guard<std::mutex> lock(m_mutex);
			m_rp = (m_rp + data_size) % m_buffer.size();
			m_used -= data_size;
		}

		unsigned used()
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			return m_used;
		}

		unsigned free()
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			return m_buffer.size() - m_used;
		}

		void clear()
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_wp = m_rp = m_used = 0;
		}


	private:
		unsigned m_wp = 0;
		unsigned m_rp = 0;
		unsigned m_used = 0;
		std::array<uint8_t, 0x80000> m_buffer;
		std::mutex m_mutex;
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
					osd_printf_verbose("COM20020: RX listen on %s\n", *m_localaddr);
					m_acceptor.async_accept(
							[this] (std::error_code const &err, asio::ip::tcp::socket sock)
							{
								if (err)
								{
									LOG("COM20020: RX error accepting - %d %s\n", err.value(), err.message());
									std::error_code e;
									m_acceptor.close(e);
									m_state_rx.store(0);
									start_accept();
								}
								else
								{
									LOG("COM20020: RX connection from %s\n", sock.remote_endpoint());
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
			LOG("COM20020: RX failed - %d %s\n", err.value(), err.message());
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
			osd_printf_verbose("COM20020: TX connecting to %s\n", *m_remoteaddr);
			m_timeout_tx.expires_after(std::chrono::seconds(10));
			m_timeout_tx.async_wait(
					[this] (std::error_code const &err)
					{
						if (!err && m_state_tx.load() == 1)
						{
							osd_printf_verbose("COM20020: TX connect timed out\n");
							std::error_code e;
							m_sock_tx.close(e);
							m_state_tx.store(0);
							start_connect();
						}
					});
			m_sock_tx.async_connect(
					*m_remoteaddr,
					[this] (std::error_code const &err)
					{
						m_timeout_tx.cancel();
						if (err)
						{
							osd_printf_verbose("COM20020: TX connect error - %d %s\n", err.value(), err.message());
							std::error_code e;
							m_sock_tx.close(e);
							m_state_tx.store(0);
							start_connect();
						}
						else
						{
							LOG("COM20020: TX connection established\n");
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
						LOG("COM20020: TX connection error: %s\n", err.message().c_str());
						m_sock_tx.close();
						m_state_tx.store(0);
						m_fifo_tx.clear();
						start_connect();
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
							LOG("COM20020: RX connection error: %s\n", err.message());
						else
							LOG("COM20020: RX connection lost\n");
						m_sock_rx.close();
						m_state_rx.store(0);
						m_fifo_rx.clear();
						start_accept();
					}
					else
					{
						if (UINT_MAX == m_fifo_rx.write(&m_buffer_rx[0], length))
						{
							LOG("COM20020: RX buffer overflow\n");
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

	std::thread m_thread;
	asio::io_context m_ioctx;
	asio::executor_work_guard<asio::io_context::executor_type> m_work_guard{m_ioctx.get_executor()};
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

#define REG_0_STATUS   0
#define REG_1_DIAG     1
#define REG_2_ADDR_H   2
#define REG_3_ADDR_L   3
#define REG_4_DATA     4
#define REG_5_SUBADDR  5
#define REG_6_CONFIG   6
#define REG_7_SUBREG   7
#define REG_7_0_TENTID 8
#define REG_7_1_NODEID 9
#define REG_7_2_SETUP1 10
#define REG_7_3_NEXTID 11
#define REG_7_4_SETUP2 12

#define REG_0_IRQMASK  13
#define REG_1_COMMAND  14
#define REG_7_3_TEST   15

DEFINE_DEVICE_TYPE(COM20020, com20020_device, "COM20020", "COM20020 ULANC")

com20020_device::com20020_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, COM20020, tag, owner, clock),
	m_ram(*this, "ulanc_ram", 0x800U, ENDIANNESS_BIG),
	m_irq_cb(*this)
{
}

void com20020_device::device_start()
{
	m_tick_timer = timer_alloc(FUNC(com20020_device::tick_timer_callback), this);
	m_tick_timer->adjust(attotime::never);

	auto ctx = std::make_unique<context>();
	m_context = std::move(ctx);
	m_context->start();

	// state saving
	save_item(NAME(m_reg));
	save_item(NAME(m_irq_state));
	save_item(NAME(m_cmd_tx));
	save_item(NAME(m_cmd_rx));
	save_item(NAME(m_cfg));
}

void com20020_device::device_reset()
{
	std::fill(std::begin(m_ram), std::end(m_ram), 0);
	std::fill(std::begin(m_reg), std::end(m_reg), 0);

	auto const &opts = mconfig().options();
	m_context->reset(opts.comm_localhost(), opts.comm_localport(), opts.comm_remotehost(), opts.comm_remoteport());

	m_reg[REG_0_STATUS]   = 0x91;
	m_reg[REG_1_DIAG]     = 0x00;
	m_reg[REG_2_ADDR_H]   = 0x00; // undefined to doc
	m_reg[REG_3_ADDR_L]   = 0x00; // undefined to doc
	m_reg[REG_4_DATA]     = 0x00; // undefined to doc
	m_reg[REG_5_SUBADDR]  = 0x00;
	m_reg[REG_6_CONFIG]   = 0x18;

	m_reg[REG_7_0_TENTID] = 0x00;
	m_reg[REG_7_1_NODEID] = 0x00;
	m_reg[REG_7_2_SETUP1] = 0x00;
	m_reg[REG_7_3_NEXTID] = 0x00;
	m_reg[REG_7_4_SETUP2] = 0x00;

	m_reg[REG_0_IRQMASK]  = 0x00;
	m_reg[REG_1_COMMAND]  = 0x00;

	m_irq_state = CLEAR_LINE;

	m_cmd_tx = 0x00;
	m_cmd_rx = 0x00;
	m_cfg = 0x01;
	m_txd = 0x00;

	m_tick_timer->adjust(attotime::from_hz(600),0,attotime::from_hz(600));
}

void com20020_device::device_stop()
{
	m_tick_timer->adjust(attotime::never);

	m_context->stop();
	m_context.reset();
}

void com20020_device::regs_map(address_map &map)
{
	map(0x00, 0x00).lrw8(
		NAME([this] (offs_t offset) {
			// REG_0_STATUS
			// 0x80 RI/TRI*
			// 0x40 x/RI*
			// 0x20 x/TA*
			// 0x10 POR
			// 0x08 TEST
			// 0x04 RECON
			// 0x02 TMA
			// 0x01 TA/TTA*
			return m_reg[REG_0_STATUS];
		}),
		NAME([this] (offs_t offset, u8 data) {
			// REG_0_IRQMASK
			m_reg[REG_0_IRQMASK] = data & 0x8f;
			update_irq();
		})
	);
	map(0x01, 0x01).lrw8(
		NAME([this] (offs_t offset) {
			// REG_1_DIAG
			// 0x80 MY-RECON
			// 0x40 DUPID
			// 0x20 RCVACT
			// 0x10 TOKEN
			// 0x08 EXCNAK
			// 0x04 TENTID
			// 0x02 NEWNEXTID
			// 0x01 x
			return m_reg[REG_1_DIAG];
		}),
		NAME([this] (offs_t offset, u8 data) {
			// REG_1_COMMAND
			uint8_t cmd_type = data & 0x07;
			switch (cmd_type)
			{
				case 0:
					// 0000 0000 - 0x00 - clear transmit interrupt
					// 0000 1000 - 0x08 - clear receive interrupt
					// 0001 1000 - 0x18 - (re)start internal operation
					break;
				case 1:
					// 0x01 - disable transmitter (cancels transmission)
					m_cmd_tx = 0x00;
					m_txd = 0x00;
					m_reg[REG_0_STATUS] |= 0x01; // set TA
					break;
				case 2:
					// 0x02 - disable receiver (cancels receive)
					m_cmd_rx = 0x00;
					break;
				case 3:
					// 00fn n011 - enable transmit from page fnn
					m_cmd_tx = data;
					m_txd = 0x00;
					m_reg[REG_0_STATUS] &= 0xfc; // clear TMA and TA
					break;
				case 4:
					// b0fn n100 - enable receive to page fnn (b = enable broadcasts)
					m_cmd_rx = data;
					m_reg[REG_0_STATUS] &= 0x7f; // clear RI
					break;
				case 5:
					// 0000 c101 - define configuration (c0 = only short packets, c1 = long and short packets)
					m_cfg = (data & 0x08) >> 3;
					break;
				case 6:
					// 000r p110 - clear flags (p = POR and EXCNAK, r = RECON)
					if (data & 0x04)
					{
						m_reg[REG_0_STATUS] &= 0xfb; // clear RECON
					}
					if (data & 0x10)
					{
						m_reg[REG_0_STATUS] &= 0xef; // clear POR
						m_reg[REG_1_DIAG] &= 0xf7; // clear EXCNAK
					}
					break;
				default:
					// not defined
					break;
			}
			m_reg[REG_1_COMMAND] = data;
			update_irq();
		})
	);
	map(0x02, 0x02).lrw8(
		NAME([this] (offs_t offset) {
			// REG_2_ADDR_H
			// 0x80 RDDATA
			// 0x40 AUTOINC
			// 0x20 x
			// 0x10 x
			// 0x08 x
			// 0x04 A10
			// 0x02 A9
			// 0x01 A8
			return m_reg[REG_2_ADDR_H];
		}),
		NAME([this] (offs_t offset, u8 data) {
			// REG_2_ADDR_H
			m_reg[REG_2_ADDR_H] = data & 0xc7;
		})
	);
	map(0x03, 0x03).lrw8(
		NAME([this] (offs_t offset) {
			// REG_3_ADDR_L
			// A7-A0
			return m_reg[REG_3_ADDR_L];
		}),
		NAME([this] (offs_t offset, u8 data) {
			// REG_3_ADDR_L
			m_reg[REG_3_ADDR_L] = data;
			if (m_reg[REG_2_ADDR_H] & 0x80)
			{
				// read access, update data register
				uint16_t addr = (m_reg[REG_2_ADDR_H] << 8 & 0x07) | m_reg[REG_3_ADDR_L];
				m_reg[REG_2_ADDR_H] = (m_reg[REG_2_ADDR_H] & 0xf8) | (addr >> 8 & 0x07);
				m_reg[REG_4_DATA] = m_ram[addr];
			}
		})
	);
	map(0x04, 0x04).lrw8(
		NAME([this] (offs_t offset) {
			// REG_4_DATA
			// D7-D0
			uint8_t result = m_reg[REG_4_DATA];
			if (!machine().side_effects_disabled())
				if (m_reg[REG_2_ADDR_H] & 0xc0)
				{
					// read AND auto increment, update data register
					uint16_t addr = (m_reg[REG_2_ADDR_H] << 8 & 0x0700) | m_reg[REG_3_ADDR_L];
					addr++;
					m_reg[REG_2_ADDR_H] = (m_reg[REG_2_ADDR_H] & 0xf8) | (addr >> 8 & 0x07);
					m_reg[REG_3_ADDR_L] = addr & 0xff;
					m_reg[REG_4_DATA] = m_ram[addr];
				}
			return result;
		}),
		NAME([this] (offs_t offset, u8 data) {
			// REG_4_DATA
			m_reg[REG_4_DATA] = data;

			if (m_reg[REG_2_ADDR_H] & 0x80)
				// does writing to the register in READ mode do anything?
				return;

			uint16_t addr = (m_reg[REG_2_ADDR_H] << 8 & 0x0700) | m_reg[REG_3_ADDR_L];
			m_ram[addr] = m_reg[REG_4_DATA];
			if (m_reg[REG_2_ADDR_H] & 0x40)
			{
				// auto increment
				addr++;
				m_reg[REG_2_ADDR_H] = (m_reg[REG_2_ADDR_H] & 0xf8) | (addr >> 8 & 0x07);
				m_reg[REG_3_ADDR_L] = addr & 0xff;
			}
		})
	);
	map(0x05, 0x05).lrw8(
		NAME([this] (offs_t offset) {
			// REG_5_SUBADDR
			// 0x80 R/W*
			// 0x40 x
			// 0x20 x
			// 0x10 x
			// 0x08 R/W*
			// 0x04 SUBAD2
			// 0x02 SUBAD1
			// 0x01 SUBAD0
			// R/W is used to determin chip revisions
			return m_reg[REG_5_SUBADDR];
		}),
		NAME([this] (offs_t offset, u8 data) {
			// REG_5_SUBADDR
			m_reg[REG_5_SUBADDR] = data & 0x8f;
			m_reg[REG_6_CONFIG] = (m_reg[REG_6_CONFIG] & 0xfc) | (data & 0x03);
			// does writing to the config register update the subaddr register?
		})
	);
	map(0x06, 0x06).lrw8(
		NAME([this] (offs_t offset) {
			// REG_6_CONFIG
			return m_reg[REG_6_CONFIG];
		}),
		NAME([this] (offs_t offset, u8 data) {
			// REG_6_CONFIG
			m_reg[REG_6_CONFIG] = data;
			m_reg[REG_5_SUBADDR] = data & 0x3;
			// writing the config register does update the subaddr register.
		})
	);
	map(0x07, 0x07).lrw8(
		NAME([this] (offs_t offset) {
			// REG_7_SUBREG
			uint8_t reg = REG_7_0_TENTID + m_reg[REG_5_SUBADDR];
			if (reg > REG_7_4_SETUP2)
			{
				uint8_t result = 0xff;
				return result;
			}
			if (reg == REG_7_3_NEXTID)
			{
				m_reg[REG_1_DIAG] &= 0xfd; // clear NEWNEXTID
				update_irq();
			}

			return m_reg[reg];
		}),
		NAME([this] (offs_t offset, u8 data) {
			// REG_7_SUBREG
			uint8_t reg = REG_7_0_TENTID + m_reg[REG_5_SUBADDR];
			if (reg > REG_7_4_SETUP2)
				return;

			switch (reg)
			{
				case REG_7_0_TENTID:
					m_reg[reg] = data;
					break;
				case REG_7_1_NODEID:
					m_reg[reg] = data;
					// signature byte AND node id
					m_ram[0] = 0xd1;
					m_ram[1] = data;
					// fake activity
					m_reg[REG_0_STATUS] |= 0x04; // RECON
					m_reg[REG_1_DIAG] = 0xb0; // MY-RECON, RCVACT & TOKEN
					break;
				case REG_7_2_SETUP1:
					m_reg[reg] = data & 0xdf;
					break;
				case REG_7_3_NEXTID:
					// actually REG_7_3_TEST
					m_reg[REG_7_3_TEST] = data & 0x00;
					break;
				case REG_7_4_SETUP2:
					m_reg[reg] = data & 0xbf;
					break;
				default:
					break;
			}
		})
	);
}

TIMER_CALLBACK_MEMBER(com20020_device::tick_timer_callback)
{
	comm_tick();
}

void com20020_device::update_irq()
{
	// 80 RI
	// 40 x
	// 20 x
	// 10 x
	// 08 EXCNAK
	// 04 RECON
	// 02 NEWNEXTID
	// 01 TA
	uint8_t irq = (m_reg[REG_0_STATUS] & 0x85) | (m_reg[REG_1_DIAG] & 0x0a);
	irq &= m_reg[REG_0_IRQMASK];

	int state = (irq == 0) ? CLEAR_LINE : ASSERT_LINE;
	if (state != m_irq_state)
	{
		osd_printf_verbose("COM20020: STATUS=%02x, DIAG=%02x, IRQMASK=%02x, IRQ=%02x.\n", m_reg[REG_0_STATUS], m_reg[REG_1_DIAG], m_reg[REG_0_IRQMASK], irq);
		m_irq_state = state;
		m_irq_cb(m_irq_state);
	}
}

void com20020_device::comm_tick()
{
	m_context->check_sockets();

	// check if we have a node id before doing ANYTHING
	if (m_reg[REG_7_1_NODEID] != 0x00)
	{
		// check if we shall transmit some data
		if ((m_cmd_tx != 0x00) && (m_txd == 0x00))
		{
			uint8_t page = (m_cmd_tx & 0x18) >> 3;
			uint16_t addr = page * 0x200;
			if (m_cmd_tx & 0x20)
				addr += 0x100;

			unsigned data_size = 0x200;
			unsigned frame_size;
			if (m_ram[addr + 2] == 0x00)
				frame_size = 0x200 - m_ram[addr + 3]; // long packet
			else
				frame_size = 0x100 - m_ram[addr + 2]; // short packet

			// TODO: check config if we are allowed to send long packets?
			// TODO: check for possible rollover

			const auto lanc_ram = (u8*)m_ram.target();
			std::copy_n(&lanc_ram[addr], frame_size, &m_buffer[0]);

			// send to line and consider it done.
			send_frame(data_size);

			m_cmd_tx = 0x00;
			m_txd = 0x01;
			osd_printf_verbose("COM20020: sent %u bytes from %03x.\n", frame_size, addr);
			update_irq();
		}

		// check if we shall receive some data
		if (m_cmd_rx != 0x00)
		{
			uint8_t page = (m_cmd_rx & 0x18) >> 3;
			uint16_t addr = page * 0x200;
			if (m_cmd_tx & 0x20)
				addr += 0x100;

			unsigned data_size = 0x200;
			unsigned recv = read_frame(data_size);
			if (recv > 0)
			{
				uint8_t src_id = m_buffer[0];
				uint8_t dst_id = m_buffer[1];
				unsigned frame_size;

				// check we are destination or broadcast (and enabled)
				if ((dst_id == m_reg[REG_7_1_NODEID]) || ((dst_id == 0x00) && (m_cmd_rx & 0x80)))
				{
					if (m_buffer[2] == 0x00)
						frame_size = 0x200 - m_buffer[3]; // long packet
					else
						frame_size = 0x100 - m_buffer[2]; // short packet

					// TODO: check config if we are allowed to recv long packets?
					// TODO: check for possible rollover

					const auto lanc_ram = (u8*)m_ram.target();
					std::copy_n(&m_buffer[0], frame_size, &lanc_ram[addr]);

					m_cmd_rx = 0x00;
					m_reg[REG_0_STATUS] |= 0x80; // set RI
					osd_printf_verbose("COM20020: recv %u bytes to %03x.\n", frame_size, addr);
				}

				// check source of packet
				if (src_id == m_reg[REG_7_1_NODEID])
				{
					m_reg[REG_0_STATUS] |= 0x01; // set TA
					// we sent out - broadcasts dont get acknowledged
					if (dst_id != 0x00)
						m_reg[REG_0_STATUS] |= 0x02; // set TMA
				}
				else
				{
					// someone else sent
					send_frame(data_size);
				}

				update_irq();
			}
		}
	}
}

unsigned com20020_device::read_frame(unsigned data_size)
{
	unsigned bytes_read = m_context->receive(&m_buffer[0], data_size);
	if (bytes_read == UINT_MAX)
	{
		// ignore errors for now
		return 0;
	}
	return bytes_read;
}

void com20020_device::send_frame(unsigned data_size)
{
	unsigned bytes_sent = m_context->send(&m_buffer[0], data_size);
	if (bytes_sent == UINT_MAX)
	{
		// ignore errors for now
	}
}