// license:BSD-3-Clause
// copyright-holders:Ariane Fugmann

/*
NOTE: this is a fake device that mimics other vunit systems
*/

#include "emu.h"
#include "midvunit_comm.h"

#include "emuopts.h"

#define VERBOSE 0
#include "logmacro.h"

DEFINE_DEVICE_TYPE(MIDWAY_VUNIT_COMM, midway_vunit_comm_device, "vunit_comm", "Midway V-Unit COMM")

midway_vunit_comm_device::midway_vunit_comm_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, MIDWAY_VUNIT_COMM, tag, owner, clock),
	m_acceptor(m_ioctx),
	m_sock_rx(m_ioctx),
	m_sock_tx(m_ioctx),
	m_tx_timeout(m_ioctx)
{
}

void midway_vunit_comm_device::device_start()
{
}

void midway_vunit_comm_device::device_reset()
{
	m_tx_state = 0;
	m_rx_state = 0;
	comm_start();

	m_linkid = 0;
	m_linkstate = 0;

	m_data = 0;
	m_flags = 0;
}

void midway_vunit_comm_device::device_stop()
{
	comm_stop();
}

void midway_vunit_comm_device::set_linktype(uint8_t linktype)
{
	m_linktype = linktype;

	switch (m_linktype)
	{
		case 1:
			// Cruis'n USA
			osd_printf_verbose("VUNIT_COMM: set mode 'Cruis'n USA'\n");
		for (int i = 0; i < 2; i++)
		{
			m_link_offset[i] = 2;
			m_link_length[i] = 19;

			m_link_buffer[i][0] = i + 1;
			m_link_buffer[i][1] = 19;
			m_link_buffer[i][2] = 0x06;
			m_link_buffer[i][3] = 0xff;
			m_link_buffer[i][4] = 0xf9;
			m_link_buffer[i][5] = 0x01;
			m_link_buffer[i][6] = 0x22;
			m_link_buffer[i][7] = 0x00;
			m_link_buffer[i][8] = 0x00;
			m_link_buffer[i][9] = 0x00;
			m_link_buffer[i][10] = 0x00;
			m_link_buffer[i][11] = 0x00;
			m_link_buffer[i][12] = 0x00;
			m_link_buffer[i][13] = 0x00;
			m_link_buffer[i][14] = 0xff;
			m_link_buffer[i][15] = 0x64;
			m_link_buffer[i][16] = 0x00;
			m_link_buffer[i][17] = 0x01;
			m_link_buffer[i][18] = 0x86;
		}
			break;
		case 2:
			// Cruis'n World
			osd_printf_verbose("VUNIT_COMM: set mode 'Cruis'n World'\n");
			for (int i = 0; i < 4; i++)
			{
				m_link_offset[i] = 2;
				m_link_length[i] = 3;

				m_link_buffer[i][0] = i + 1;
				m_link_buffer[i][1] = 3;
				m_link_buffer[i][2] = 0xa5;
			}
			break;
		case 3:
			// Off Road Challenge
			osd_printf_verbose("VUNIT_COMM: set mode 'Off Road Challenge'\n");
			break;
		default:
			logerror("VUNIT_COMM-set_linktype: unknown linktype %d\n", m_linktype);
			break;
	}
}

uint32_t midway_vunit_comm_device::data_r()
{
	if (machine().side_effects_disabled())
		return 0;

	comm_tick();

	switch (m_linktype)
	{
		case 1:
			return data_r_crusnusa();

		case 2:
			return data_r_crusnwld();

		default:
			return 0;
	}
}

void midway_vunit_comm_device::data_w(uint32_t data)
{
	switch (m_linktype)
	{
		case 1:
			data_w_crusnusa(data);
			break;

		case 2:
			data_w_crusnwld(data);
			break;

		default:
			logerror("VUNIT_COMM-data_w: unknown linktype %d\n", m_linktype);
			break;
	}

	m_data = data >> 16;
}

uint32_t midway_vunit_comm_device::flags_r()
{
	// does that even get called?
	if (!machine().side_effects_disabled())
		logerror("flags_r() got called.");
	return 0;
}

void midway_vunit_comm_device::flags_w(uint32_t data)
{
	switch (m_linktype)
	{
		case 1:
			flags_w_crusnusa(data);
			break;

		case 2:
			flags_w_crusnwld(data);
			break;

		default:
			logerror("VUNIT_COMM-flags_w: unknown linktype %d\n", m_linktype);
			break;
	}

	m_flags = data >> 24;
}

uint32_t midway_vunit_comm_device::data_r_crusnusa()
{
	uint8_t offset = 0;
	uint32_t result = 0;
	switch (m_linkstate)
	{
		case 0x10:
			// comms idle
			return 0x01000000;

		case 0x11:
			// ready to send
			return 0x00000000;
			
		case 0x12:
			// prep send
			return 0x02000000;

		case 0x13:
			osd_printf_verbose("VUNIT_COMM: 13 to 14.\n");
			m_linkstate = 0x14;
			return 0x00000000;

		case 0x15:
			// start receive
			m_readcount++;
			if (m_readcount == 3)
			{
				m_readcount = 0;
				m_link_offset[1] = 2;
				osd_printf_verbose("VUNIT_COMM: 15 to 16.\n");
				m_linkstate = 0x16;
			}
			return 0x02000000;

		case 0x16:
			offset = m_link_offset[1];
			osd_printf_verbose("VUNIT_COMM: read %02x.@ %d\n", m_link_buffer[1][offset], offset);
			result = m_link_buffer[1][offset] << 16;
			if ((offset % 2))
				result |= 0x04000000;
			m_readcount++;
			if (m_readcount == 2)
			{
				m_readcount = 0;
				m_link_offset[1] += 1;
			}
			if (m_link_offset[1] >= m_link_length[1])
			{
				osd_printf_verbose("VUNIT_COMM: 16 to 17.\n");
				m_linkstate = 0x17;
			}
			return result;

		case 0x17:
			osd_printf_verbose("VUNIT_COMM: 17 to 18.\n");
			m_linkstate = 0x18;
			return 0x02000000;

		case 0x18:
			osd_printf_verbose("VUNIT_COMM: 18 to 10.\n");
			m_linkstate = 0x10;
			return 0x00000000;

		case 0x20:
			// comms idle
			return 0x00000000;

		case 0x21:
			// ready to receive
			return 0x04000000;

		case 0x22:
			// prep receive
			return 0x00000000;

		case 0x23:
			// start receive
			m_readcount++;
			if (m_readcount == 3)
			{
				m_readcount = 0;
				m_link_offset[0] = 2;
				osd_printf_verbose("VUNIT_COMM: 23 to 24.\n");
				m_linkstate = 0x24;
			}
			return 0x04000000;

		case 0x24:
			offset = m_link_offset[0];
			osd_printf_verbose("VUNIT_COMM: read %02x.@ %d\n", m_link_buffer[0][offset], offset);
			result = m_link_buffer[0][offset] << 16;
			if ((offset % 2))
				result |= 0x04000000;
			m_readcount++;
			if (m_readcount == 2)
			{
				m_readcount = 0;
				m_link_offset[0] += 1;
			}
			if (m_link_offset[0] >= m_link_length[0])
			{
				osd_printf_verbose("VUNIT_COMM: 24 to 25.\n");
				m_linkstate = 0x25;
			}
			return result;

		case 0x26:
			return 0x04000000;

		case 0x27:
			osd_printf_verbose("VUNIT_COMM: 27 to 28.\n");
			m_linkstate = 0x28;
			return 0x00000000;

		default:
			break;
	}
	return 0;
}

void midway_vunit_comm_device::data_w_crusnusa(uint32_t data)
{
	uint8_t ctrl = (data >> 24) & 0xff;
	uint8_t payload = (data >> 16) & 0xff;

	// check if comms enabled
	if (!(m_flags & 0x20))
		return;

	if (m_linkstate == 0x00)
	{
		if ((ctrl & 0xf0) == 0xc0)
		{
			m_linkid = 1;
			m_linkstate = 0x10;
			osd_printf_verbose("VUNIT_COMM: we are MASTER.\n");
		}
		else if ((ctrl & 0xf0) == 0x30)
		{
			m_linkid = 2;
			m_linkstate = 0x20;
			osd_printf_verbose("VUNIT_COMM: we are SLAVE.\n");
		}
	}

	if (ctrl == 0xc8)
	{
		osd_printf_verbose("VUNIT_COMM: VSYNC?.\n");
		m_linkstate = 0x10;
	}

	uint8_t offset = 0;
	switch (m_linkstate)
	{
		case 0x10:
			if (ctrl == 0xc4)
			{
				osd_printf_verbose("VUNIT_COMM: 10 to 11.\n");
				m_linkstate = 0x11;
			}
			break;

		case 0x11:
			if (ctrl == 0xc0)
			{
				osd_printf_verbose("VUNIT_COMM: 11 to 12.\n");
				m_linkstate = 0x12;
			}
			break;

		case 0x12:
			if (ctrl == 0xc4)
			{
				osd_printf_verbose("VUNIT_COMM: 12 to 13.\n");
				m_linkstate = 0x13;
				m_link_offset[0] = 0x02;
				m_link_length[0] = 0x02;
			}
			break;

		case 0x13:
			// send
			offset = m_link_offset[0];
			osd_printf_verbose("VUNIT_COMM: send %02x @ %d\n", payload, offset);
			m_link_buffer[0][offset] = payload;
			offset++;
			m_link_offset[0] = m_link_length[0] = offset;
			break;

		case 0x14:
			if (ctrl == 0xc4)
			{
				osd_printf_verbose("VUNIT_COMM: 14 to 15.\n");
				m_linkstate = 0x15;
				m_link_offset[1] = 0x02;
			}
			break;

		case 0x20:
			if (ctrl == 0x31)
			{
				// comms idle -> ready to receive
				osd_printf_verbose("VUNIT_COMM: 20 to 21.\n");
				m_linkstate = 0x21;
			}
			else
			{
				osd_printf_verbose("VUNIT_COMM-data_w_crusnusa: %04x.\n", data >> 16);
			}
			break;

		case 0x21:
			if (ctrl == 0x30)
			{
				// ready to receive -> prep receive
				osd_printf_verbose("VUNIT_COMM: 21 to 22.\n");
				m_linkstate = 0x22;
			}
			break;

		case 0x22:
			if (ctrl == 0x32)
			{
				// prep receive -> start receive
				osd_printf_verbose("VUNIT_COMM: 22 to 23.\n");
				m_readcount = 0;
				m_linkstate = 0x23;
			}
			break;

		case 0x24:
			if (ctrl == 0x30)
			{
				osd_printf_verbose("VUNIT_COMM: 24 to 20.\n");
				m_linkstate = 0x20;
			}
			break;

		case 0x25:
			if (ctrl == 0x30)
			{
				// end receive
				osd_printf_verbose("VUNIT_COMM: 25 to 26.\n");
				m_linkstate = 0x26;
			}
			break;

		case 0x26:
			if (ctrl == 0x32)
			{
				// prepare to send
				osd_printf_verbose("VUNIT_COMM: 26 to 27.\n");
				m_link_offset[1] = 0x02;
				m_link_length[1] = 0x02;
				m_linkstate = 0x27;
			}
			break;

		case 0x27:
			// send
			offset = m_link_offset[1];
			osd_printf_verbose("VUNIT_COMM: send %02x @ %d\n", payload, offset);
			m_link_buffer[1][offset] = payload;
			offset++;
			m_link_offset[1] = m_link_length[1] = offset;
			break;

		case 0x28:
			if (ctrl == 0x30)
			{
				osd_printf_verbose("VUNIT_COMM: 28 to 20.\n");
				m_linkstate = 0x20;
			}
			break;

		default:
			break;
	}
}

void midway_vunit_comm_device::flags_w_crusnusa(uint32_t data)
{
	uint8_t newflags = (data >> 24) & 0xff;
	if (newflags != m_flags)
	{
		switch (newflags)
		{
			case 0x20:
				osd_printf_verbose("VUNIT_COMM: we are READING.\n");
				break;
			
			case 0x60:
				osd_printf_verbose("VUNIT_COMM: we are WRITING.\n");
				break;
		}
	}
}

uint32_t midway_vunit_comm_device::data_r_crusnwld()
{
		uint8_t offset = 0;
	uint32_t result = 0;
	switch (m_linkstate)
	{
		case 0x10:
			return 0x0eff0000;

		case 0x11:
			return 0x00ff0000;

		case 0x12:
			if (m_readcount % 2)
				return 0x0eff0000;
			else
				return 0x00ff0000;

		case 0x15:
			return 0x0eff0000;

		case 0x16:
			return 0x02ff0000;

		case 0x20:
			return 0x00ff0000;

		case 0x21:
			// 7807
			return 0x01ff0000;

		case 0x22:
			// 781a
			m_linkstate = 0x23;
			return 0x00ff0000;

		case 0x23:
			if (m_readcount % 2)
				return 0x01ff0000;
			else
				return 0x00ff0000;

		case 0x24:
			m_linkstate = 0x25;
			return 0x00a50000;

		case 0x25:
			m_linkstate = 0x26;
			return 0x00a50000;

		case 0x26:
			m_linkstate = 0x27;
			return 0x00ff0000;

		case 0x27:
			m_linkstate = 0x20;
			return 0x01ff0000;

		default:
			return 0;
	}
}

void midway_vunit_comm_device::data_w_crusnwld(uint32_t data)
{
	uint8_t ctrl = (data >> 24) & 0xff;
	uint8_t payload = (data >> 16) & 0xff;

	// check if comms enabled
	if (!(m_flags & 0x20))
		return;

	if (m_linkstate == 0x00)
	{
		switch (ctrl & 0xf0)
		{
			case 0x10:
				m_linkid = 1;
				m_linkstate = 0x10;
				osd_printf_verbose("VUNIT_COMM: we are MASTER.\n");
				break;

			case 0x20:
				m_linkid = 2;
				m_linkstate = 0x20;
				osd_printf_verbose("VUNIT_COMM: we are SLAVE.\n");
				break;

			case 0x40:
				m_linkid = 3;
				m_linkstate = 0x30;
				osd_printf_verbose("VUNIT_COMM: we are SLAVE.\n");
				break;

			case 0x80:
				m_linkid = 4;
				m_linkstate = 0x40;
				osd_printf_verbose("VUNIT_COMM: we are SLAVE.\n");
				break;
		}
	}

	uint8_t offset = 0;
	switch (m_linkstate)
	{
		case 0x10:
			if (ctrl == 0x11)
			{
				osd_printf_verbose("VUNIT_COMM: 10 to 11.\n");
				m_linkstate = 0x11;
			}
			break;

		case 0x11:
			if (ctrl == 0x10)
			{
				osd_printf_verbose("VUNIT_COMM: 11 to 12.\n");
				m_linkstate = 0x12;
				m_readcount = 0x01;
			}
			break;

		case 0x12:
			if ((m_readcount % 2) && (ctrl == 0x11))
			{
				m_readcount++;
				osd_printf_verbose("VUNIT_COMM: 1 m_readcount++ : %d\n", m_readcount);
			}
			else if (!(m_readcount % 2) && (ctrl == 0x10))
			{
				m_readcount++;
				osd_printf_verbose("VUNIT_COMM: 2 m_readcount++ : %d\n", m_readcount);
			}

			if (m_readcount >= 8)
			{
				m_readcount = 0;
				osd_printf_verbose("VUNIT_COMM: 12 to 13.\n");
				m_linkstate = 0x13;
			}
			break;

		case 0x13:
			if (ctrl == 0x10)
			{
				osd_printf_verbose("VUNIT_COMM: send : %02x.\n", payload);
				m_linkstate = 0x14;
			}
			break;

		case 0x14:
			if (ctrl == 0x10)
			{
				osd_printf_verbose("VUNIT_COMM: 14 to 15.\n");
				m_linkstate = 0x15;
			}
			break;

		case 0x15:
			if (ctrl == 0x11)
			{
				osd_printf_verbose("VUNIT_COMM: 15 to 16.\n");
				m_linkstate = 0x16;
			}
			break;

		case 0x20:
			if (ctrl == 0x22)
			{
				// 77f9
				osd_printf_verbose("VUNIT_COMM: 20 to 21.\n");
				m_linkstate = 0x21;
				m_readcount = 0x01;
			}
			break;

		case 0x21:
			if (ctrl == 0x20)
			{
				// 7813
				osd_printf_verbose("VUNIT_COMM: 21 to 22.\n");
				m_linkstate = 0x22;
				m_readcount = 0x00;
			}
			break;

		case 0x23:
			m_readcount++;
			osd_printf_verbose("VUNIT_COMM: 1 m_readcount++ : %d\n", m_readcount);

			if (m_readcount >= 8)
			{
				m_readcount = 0;
				osd_printf_verbose("VUNIT_COMM: 23 to 24.\n");
				m_linkstate = 0x24;
			}
			break;

		default:
			break;
	}
}

void midway_vunit_comm_device::flags_w_crusnwld(uint32_t data)
{
	uint8_t newflags = (data >> 24) & 0xff;
	if (newflags != m_flags)
	{
		switch (newflags)
		{
			case 0x20:
				osd_printf_verbose("VUNIT_COMM: we are READING.\n");
				break;
			
			case 0x60:
				osd_printf_verbose("VUNIT_COMM: we are WRITING.\n");
				break;
		}
	}
}

void midway_vunit_comm_device::check_sockets()
{
	// if async operation in progress, poll context
	if ((m_rx_state == 1) || (m_tx_state == 1))
		m_ioctx.poll();

	// start acceptor if needed
	if (m_localaddr && m_rx_state == 0)
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
					osd_printf_verbose("VUNIT_COMM: RX listen on %s\n", *m_localaddr);
					m_acceptor.async_accept(
							[this] (std::error_code const &err, asio::ip::tcp::socket sock)
							{
								if (err)
								{
									LOG("VUNIT_COMM: RX error accepting - %d %s\n", err.value(), err.message());
									std::error_code e;
									m_acceptor.close(e);
									m_rx_state = 0;
								}
								else
								{
									LOG("VUNIT_COMM: RX connection from %s\n", sock.remote_endpoint());
									std::error_code e;
									m_acceptor.close(e);
									m_sock_rx = std::move(sock);
									m_sock_rx.non_blocking(true);
									m_sock_rx.set_option(asio::socket_base::receive_buffer_size(524288));
									m_sock_rx.set_option(asio::socket_base::keep_alive(true));
									m_rx_state = 2;
								}
							});
					m_rx_state = 1;
				}
			}
		}
		if (err)
		{
			LOG("VUNIT_COMM: RX failed - %d %s\n", err.value(), err.message());
		}
	}

	// connect socket if needed
	if (m_remoteaddr && m_tx_state == 0)
	{
		std::error_code err;
		if (m_sock_tx.is_open())
			m_sock_tx.close(err);
		m_sock_tx.open(m_remoteaddr->protocol(), err);
		if (!err)
		{
			m_sock_tx.non_blocking(true);
			m_sock_tx.set_option(asio::ip::tcp::no_delay(true));
			m_sock_tx.set_option(asio::socket_base::send_buffer_size(65536));
			m_sock_tx.set_option(asio::socket_base::keep_alive(true));
			osd_printf_verbose("VUNIT_COMM: TX connecting to %s\n", *m_remoteaddr);
			m_tx_timeout.expires_after(std::chrono::seconds(10));
			m_tx_timeout.async_wait(
					[this] (std::error_code const &err)
					{
						if (!err && m_tx_state == 1)
						{
							osd_printf_verbose("VUNIT_COMM: TX connect timed out\n");
							std::error_code e;
							m_sock_tx.close(e);
							m_tx_state = 0;
						}
					});
			m_sock_tx.async_connect(
					*m_remoteaddr,
					[this] (std::error_code const &err)
					{
						m_tx_timeout.cancel();
						if (err)
						{
							osd_printf_verbose("VUNIT_COMM: TX connect error - %d %s\n", err.value(), err.message());
							std::error_code e;
							m_sock_tx.close(e);
							m_tx_state = 0;
						}
						else
						{
							LOG("VUNIT_COMM: TX connection established\n");
							m_tx_state = 2;
						}
					});
			m_tx_state = 1;
		}
	}
}

void midway_vunit_comm_device::comm_start()
{
	auto const &opts = mconfig().options();
	std::error_code err;
	asio::ip::tcp::resolver resolver(m_ioctx);

	for (auto &&resolveIte : resolver.resolve(opts.comm_localhost(), opts.comm_localport(), asio::ip::tcp::resolver::flags::address_configured, err))
	{
		m_localaddr = resolveIte.endpoint();
		LOG("VUNIT_COMM: localhost = %s\n", *m_localaddr);
	}
	if (err) {
		LOG("VUNIT_COMM: localhost resolve error: %s\n", err.message());
	}

	for (auto &&resolveIte : resolver.resolve(opts.comm_remotehost(), opts.comm_remoteport(), asio::ip::tcp::resolver::flags::address_configured, err))
	{
		m_remoteaddr = resolveIte.endpoint();
		LOG("VUNIT_COMM: remotehost = %s\n", *m_remoteaddr);
	}
	if (err) {
		LOG("VUNIT_COMM: remotehost resolve error: %s\n", err.message());
	}
}

void midway_vunit_comm_device::comm_stop()
{
	std::error_code err;
	if (m_acceptor.is_open())
		m_acceptor.close(err);
	if (m_sock_rx.is_open())
		m_sock_rx.close(err);
	if (m_sock_tx.is_open())
		m_sock_tx.close(err);
	m_tx_timeout.cancel();
}

void midway_vunit_comm_device::comm_tick()
{
	check_sockets();

	if (m_rx_state == 2 && m_tx_state == 2)
	{
		unsigned data_size = 0x0080;
		unsigned recv = read_frame(data_size);
		while (recv > 0)
		{
			// check if valid id
			uint8_t idx = m_buffer0[0];
			if (idx > 0 && idx <= 4)
			{
				// if not own message
				if (idx != m_linkid)
				{
					// save message to buffer
					unsigned buffer = idx - 1;
					for (unsigned j = 0x00 ; j < data_size ; j++)
					{
						m_link_buffer[buffer][j] = m_buffer0[j];
					}
					m_link_length[buffer] = m_buffer0[1];
					osd_printf_verbose("VUNIT_COMM: recv m_link_length[%d] = %d.\n", buffer, m_link_length[buffer]);

					// forward message to other nodes
					send_frame(data_size);
				}
			}
			recv = read_frame(data_size);
		}

		if (m_linkid > 0x00 && m_linkid <= 4)
		{
			unsigned buffer = m_linkid - 1;
			if (m_link_length[buffer] > 0x00)
			{
				for (unsigned j = 0x00 ; j < data_size ; j++)
				{
					m_buffer0[j] = m_link_buffer[buffer][j];
				}
				m_link_buffer[buffer][0] = m_linkid;
				m_link_buffer[buffer][1] = m_link_length[buffer];
				osd_printf_verbose("VUNIT_COMM: send m_link_length[%d] = %d.\n", buffer, m_link_length[buffer]);
				m_link_length[buffer] = 0;
				send_frame(data_size);
			}
		}
	}
}

unsigned midway_vunit_comm_device::read_frame(unsigned data_size)
{
	if (m_rx_state != 2)
		return 0;

	std::error_code err;
	std::size_t bytes_read = m_sock_rx.receive(asio::buffer(&m_buffer0[0], data_size), asio::socket_base::message_peek, err);
	if (err == asio::error::would_block)
		return 0;

	if (bytes_read == data_size)
		bytes_read = m_sock_rx.receive(asio::buffer(&m_buffer0[0], data_size), 0, err);

	if (err)
	{
		osd_printf_verbose("VUNIT_COMM: RX error receiving - %d %s\n", err.value(), err.message());
		m_sock_rx.close(err);
		m_rx_state = 0;
		return 0;
	}

	if (bytes_read == data_size)
		return bytes_read;
	return 0;
}

void midway_vunit_comm_device::send_frame(unsigned data_size){
	if (m_tx_state != 2)
		return;

	std::error_code err;
	std::size_t bytes_sent = m_sock_tx.send(asio::buffer(&m_buffer0[0], data_size), 0, err);
	if (err || bytes_sent != data_size)
	{
		osd_printf_verbose("VUNIT_COMM: TX error sending - %d %s\n", err.value(), err.message());
		m_sock_tx.close(err);
		m_tx_state = 0;
	}
}