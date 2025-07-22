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
	m_maincpu(*this, "^maincpu"),
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

	m_intcount = 0;

	osd_printf_verbose("---\n");
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

			m_link_buffer[i][0] = i + 1; // link id
			m_link_buffer[i][1] = 19;    // message size
			m_link_buffer[i][2] = 0x06;  // head.words
			m_link_buffer[i][3] = 0xff;  // head.unk
			m_link_buffer[i][4] = 0xf9;  // head.checksum
			m_link_buffer[i][5] = 0x01;  // data 1a...
			m_link_buffer[i][6] = 0x22;  // data 1b...
			m_link_buffer[i][7] = 0x00;  // data 2a...
			m_link_buffer[i][8] = 0x00;  // data 2b...
			m_link_buffer[i][9] = 0x00;  // data 3a...
			m_link_buffer[i][10] = 0x00; // data 3b...
			m_link_buffer[i][11] = 0x00; // data 4a...
			m_link_buffer[i][12] = 0x00; // data 4b...
			m_link_buffer[i][13] = 0x00; // data 5a...
			m_link_buffer[i][14] = 0xff; // data 5b...
			m_link_buffer[i][15] = 0x64; // data 6a...
			m_link_buffer[i][16] = 0x00; // data 6b...
			m_link_buffer[i][17] = 0x01; // tail
			m_link_buffer[i][18] = 0x86; // tail
		}
			break;
		case 2:
			// Cruis'n World
			osd_printf_verbose("VUNIT_COMM: set mode 'Cruis'n World'\n");
			for (int i = 0; i < 2; i++)
			{
				m_link_offset[i] = 2;
				m_link_length[i] = 19;

				m_link_buffer[i][0] = i + 1; // link id
				m_link_buffer[i][1] = 19;    // message size
				m_link_buffer[i][2] = 0x06;  // head.words
				m_link_buffer[i][3] = 0xff;  // head.unk
				m_link_buffer[i][4] = 0xf9;  // head.checksum
				m_link_buffer[i][5] = 0x03;  // data 1a...
				m_link_buffer[i][6] = 0x01;  // data 1b...
				m_link_buffer[i][7] = 0x00;  // data 2a...
				m_link_buffer[i][8] = 0x00;  // data 2b...
				m_link_buffer[i][9] = 0x00;  // data 3a...
				m_link_buffer[i][10] = 0x00; // data 3b...
				m_link_buffer[i][11] = 0x00; // data 4a...
				m_link_buffer[i][12] = 0x00; // data 4b...
				m_link_buffer[i][13] = 0x00; // data 5a...
				m_link_buffer[i][14] = 0x2b; // data 5b...
				m_link_buffer[i][15] = 0x64; // data 6a...
				m_link_buffer[i][16] = 0x00; // data 6b...
				m_link_buffer[i][17] = 0x00; // tail
				m_link_buffer[i][18] = 0x93; // tail
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

	uint32_t result;
	switch (m_linktype)
	{
		case 1:
			result = data_r_crusnusa();
			break;

		case 2:
			result = data_r_crusnwld();
			break;

		default:
			result = 0;
			break;
	}

	osd_printf_verbose("data_r : %08x @ %08x.\n", result, m_maincpu->pc());
	return result;
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

	osd_printf_verbose("data_w : %08x @ %08x.\n", data, m_maincpu->pc());

	m_data = data >> 16;
}

uint32_t midway_vunit_comm_device::flags_r()
{
	if (machine().side_effects_disabled())
		return 0;

	osd_printf_verbose("flags_r: %08x @ %08x.\n", 0, m_maincpu->pc());

	// does that even get called?
	logerror("flags_r() got called.");
	machine().debug_break();
	return 0;
}

void midway_vunit_comm_device::flags_w(uint32_t data)
{
	uint8_t tmp = data >> 24;
	if (tmp == 0xf0)
	{
		osd_printf_verbose("fake INT2 assert\n");
		m_maincpu->set_input_line(2, ASSERT_LINE);
		return;
	}
	else if (tmp == 0x0f)
	{
		osd_printf_verbose("fake INT2 clear\n");
		m_maincpu->set_input_line(2, CLEAR_LINE);
		return;
	}

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

	osd_printf_verbose("flags_w: %08x @ %08x.\n", data, m_maincpu->pc());

	m_flags = data >> 24;
}

uint32_t midway_vunit_comm_device::data_r_crusnusa()
{
	switch (m_linkstate)
	{
		case 0x106:
		case 0x204:
			// do NOT recv data while reading from buffer.
			break;

		case 0x103:
		case 0x207:
			// do NOT send data while writing to buffer.
			break;

		default:
			comm_tick();
			break;
	}

	uint8_t offset = 0;
	uint32_t result = 0;
	switch (m_linkstate)
	{
		case 0x100:
			// comms idle
			return 0x01000000;

		case 0x101:
			// ready to send
			return 0x00000000;
			
		case 0x102:
			// prep send
			return 0x02000000;

		case 0x103:
			// end of send
			set_linkstate(0x104);
			return 0x00000000;

		case 0x105:
			// start receive
			m_readcount++;
			if (m_readcount == 3)
			{
				set_linkstate(0x106);
				m_readcount = 0;
				m_link_offset[1] = 2;
			}
			return 0x02000000;

		case 0x106:
			offset = m_link_offset[1];
			osd_printf_verbose("VUNIT_COMM: read %02x.@ %u\n", m_link_buffer[1][offset], offset);
			result = m_link_buffer[1][offset] << 16;
			if ((offset % 2))
				result |= 0x02000000;
			m_readcount++;
			if (m_readcount == 2)
			{
				m_readcount = 0;
				m_link_offset[1] += 1;
			}
			if (m_link_offset[1] >= m_link_length[1])
			{
				set_linkstate(0x107);
				m_linkstate = 0x107;
			}
			return result;

		case 0x107:
			set_linkstate(0x100);
			return 0x02000000;

		case 0x200:
			// comms idle
			return 0x00000000;

		case 0x201:
			// ready to receive
			return 0x04000000;

		case 0x202:
			// prep receive
			return 0x00000000;

		case 0x203:
			// start receive
			m_readcount++;
			if (m_readcount == 3)
			{
				m_readcount = 0;
				m_link_offset[0] = 2;
				set_linkstate(0x204);
			}
			return 0x04000000;

		case 0x204:
			offset = m_link_offset[0];
			osd_printf_verbose("VUNIT_COMM: read %02x.@ %u\n", m_link_buffer[0][offset], offset);
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
				set_linkstate(0x205);
			}
			return result;

		case 0x206:
			return 0x04000000;

		case 0x207:
			set_linkstate(0x208);
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
	uint8_t intr = ctrl & 0x88;

	// check if comms enabled
	if (!(m_flags & 0x20))
		return;

	if (m_linkstate == 0x00)
	{
		if ((ctrl & 0xf0) == 0xc0)
		{
			m_linkid = 1;
			set_linkstate(0x100);
			osd_printf_verbose("VUNIT_COMM: we are cab-1 MASTER.\n");
		}
		else if ((ctrl & 0xf0) == 0x30)
		{
			m_linkid = 2;
			set_linkstate(0x200);
			osd_printf_verbose("VUNIT_COMM: we are cab-2 SLAVE.\n");
		}
	}

	if (ctrl == 0xC8)
	{
		osd_printf_verbose("VUNIT_COMM: raise VSYNC.\n");
		send_vsync(1);
		m_linkstate = 0x100;
	}

	if (intr & 0x80)
	{
		m_maincpu->set_input_line(2, intr & 0x08 ? ASSERT_LINE : CLEAR_LINE);
	}

	uint8_t offset = 0;
	switch (m_linkstate)
	{
		case 0x100:
			if (ctrl == 0xc4)
			{
				set_linkstate(0x101);
			}
			break;

		case 0x101:
			if (ctrl == 0xc0)
			{
				set_linkstate(0x102);
			}
			break;

		case 0x102:
			if (ctrl == 0xc4)
			{
				set_linkstate(0x103);
				m_link_offset[0] = 0x02;
				m_link_length[0] = 0x02;
			}
			break;

		case 0x103:
			// send
			offset = m_link_offset[0];
			osd_printf_verbose("VUNIT_COMM: send %02x @ %u\n", payload, offset);
			m_link_buffer[0][offset] = payload;
			offset++;
			m_link_offset[0] = m_link_length[0] = offset;
			break;

		case 0x104:
			if (ctrl == 0xc4)
			{
				set_linkstate(0x105);
				m_link_offset[1] = 0x02;
			}
			break;

		case 0x107:
			set_linkstate(0x100);
			break;

		case 0x200:
			if (ctrl == 0x31)
			{
				// comms idle -> ready to receive
				set_linkstate(0x201);
				if (m_intcount == 1)
				{
					m_maincpu->set_input_line(2, ASSERT_LINE);
					m_intcount = 0;
				}
			}
			else
			{
				osd_printf_verbose("VUNIT_COMM-data_w_crusnusa: %04x.\n", data >> 16);
			}
			break;

		case 0x201:
			if (ctrl == 0x30)
			{
				// ready to receive -> prep receive
				set_linkstate(0x202);
				m_maincpu->set_input_line(2, CLEAR_LINE);
			}
			break;

		case 0x202:
			if (ctrl == 0x32)
			{
				// prep receive -> start receive
				set_linkstate(0x203);
				m_readcount = 0;
			}
			break;

		case 0x204:
			if (ctrl == 0x30)
			{
				set_linkstate(0x200);
			}
			break;

		case 0x205:
			if (ctrl == 0x30)
			{
				// end receive
				set_linkstate(0x206);
			}
			break;

		case 0x206:
			if (ctrl == 0x32)
			{
				// prepare to send
				set_linkstate(0x207);
				m_link_offset[1] = 0x02;
				m_link_length[1] = 0x02;
			}
			break;

		case 0x207:
			// send
			offset = m_link_offset[1];
			osd_printf_verbose("VUNIT_COMM: send %02x @ %u\n", payload, offset);
			m_link_buffer[1][offset] = payload;
			offset++;
			m_link_offset[1] = m_link_length[1] = offset;
			break;

		case 0x208:
			if (ctrl == 0x30)
			{
				set_linkstate(0x200);
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

	if (newflags == 0x20 && m_linkstate == 0x26)
	{
		set_linkstate(0x200);
	}
}

uint32_t midway_vunit_comm_device::data_r_crusnwld()
{
	comm_tick();

	uint8_t offset = 0;
	uint32_t result = 0;
	switch (m_linkstate)
	{
		case 0x100:
			// start of communication
			return 0x02ff0000; // 0eff0000

		case 0x101:
			// before handshake
			return 0x00ff0000;

		case 0x102:
			// during handshake
			// status is ANDed with a pattern @ 00773c+
			switch (m_readcount)
			{
				case 0:
					return 0x00ff0000; // & 0c000000

				case 1:
					return 0x00ff0000; // & 09000000

				case 2:
					return 0x02ff0000; // & 03000000

				case 3:
					return 0x02ff0000; // & 06000000

				case 4:
					return 0x00ff0000; // & 01000000

				case 5:
					return 0x02ff0000; // & 02000000

				case 6:
					return 0x00ff0000; // & 04000000

				case 7:
					return 0x00ff0000; // & 08000000

				default:
					if (m_readcount % 2)
						return 0x02ff0000;
					else
						return 0x00ff0000;
			}

		case 0x103:
			return 0x00ff0000;

		case 0x105:
			return 0x02ff0000;

		case 0x106:
			return 0x00ff0000;

		case 0x107:
			// after send
			set_linkstate(0x108);
			return 0x02ff0000;

		case 0x109:
			return 0x005a0000;

		case 0x10a:
			m_readcount++;
			if (m_readcount == 2)
			{
				set_linkstate(0x10b);
				m_readcount = 0;
				m_link_offset[1] = 0x02;
			}
			return 0x02000000;

		case 0x10b:
			// recv
			offset = m_link_offset[1];
			osd_printf_verbose("VUNIT_COMM: read %02x.@ %u\n", m_link_buffer[1][offset], offset);
			result = m_link_buffer[1][offset] << 16;
			if ((offset % 2))
				result |= 0x02000000;
			m_readcount++;
			if (m_readcount == 2)
			{
				m_readcount = 0;
				m_link_offset[1] += 1;
			}
			if (m_link_offset[1] >= m_link_length[1])
			{
				set_linkstate(0x10c);
			}
			return result;

		case 0x10d:
			return 0x02ff0000;

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
				set_linkstate(0x100);
				osd_printf_verbose("VUNIT_COMM: we are cab 1 - MASTER.\n");
				break;

			case 0x20:
				m_linkid = 2;
				set_linkstate(0x200);
				osd_printf_verbose("VUNIT_COMM: we are cab 2 - SLAVE.\n");
				break;

			case 0x40:
				m_linkid = 3;
				set_linkstate(0x300);
				osd_printf_verbose("VUNIT_COMM: we are cab 3 - SLAVE.\n");
				break;

			case 0x80:
				m_linkid = 4;
				set_linkstate(0x400);
				osd_printf_verbose("VUNIT_COMM: we are cab 4 - SLAVE.\n");
				break;
		}
	}

	uint8_t offset = 0;
	switch (m_linkstate)
	{
		case 0x100:
			// start of communication
			if (ctrl == 0x11)
			{
				set_linkstate(0x101);
			}
			break;

		case 0x101:
			// start handshake
			if (ctrl == 0x10)
			{
				set_linkstate(0x102);
				m_readcount = 0x00;
				osd_printf_verbose("VUNIT_COMM: sync step-%u\n", m_readcount);
			}
			break;

		case 0x102:
			// handshake
			if ((m_readcount % 2) && (ctrl == 0x10))
			{
				m_readcount++;
				osd_printf_verbose("VUNIT_COMM: sync step-%u\n", m_readcount);
			}
			else if (!(m_readcount % 2) && (ctrl == 0x11))
			{
				m_readcount++;
				osd_printf_verbose("VUNIT_COMM: sync step-%u\n", m_readcount);
			}

			if (m_readcount >= 8)
			{
				set_linkstate(0x103);
				m_readcount = 0;
				data_w_crusnwld(data);
			}
			break;

		case 0x103:
			if (ctrl == 0x10)
			{
				if (payload == 0xa5)
				{
					// handshake error
					set_linkstate(0x100);
				} else {
					set_linkstate(0x104);
				}
			}
			break;

		case 0x104:
			if (ctrl == 0x11)
			{
				set_linkstate(0x105);
			}
			break;

		case 0x105:
			if (ctrl == 0x10)
			{
				set_linkstate(0x106);
			}
			break;

		case 0x106:
			if (ctrl == 0x11)
			{
				set_linkstate(0x107);
				m_link_offset[0] = 0x02;
				m_link_length[0] = 0x02;
			}
			break;

		case 0x107:
			// send mode
			offset = m_link_offset[0];
			osd_printf_verbose("VUNIT_COMM: send %02x @ %u\n", payload, offset);
			m_link_buffer[0][offset] = payload;
			offset++;
			m_link_offset[0] = m_link_length[0] = offset;
			break;

		case 0x108:
			// after send
			if (ctrl == 0x11)
			{
				set_linkstate(0x109);
			}
			break;

		case 0x109:
			if (ctrl == 0x10)
			{
				set_linkstate(0x10a);
			}
			break;

		case 0x10c:
			// after recv
			if (ctrl == 0x11)
			{
				set_linkstate(0x10d);
			}
			break;

		case 0x10d:
			if (ctrl == 0x10)
			{
				set_linkstate(0x100);
			}
			break;


		case 0x200:
			if (ctrl == 0x22)
			{
				// 77f9
				set_linkstate(0x201);
				m_readcount = 0x01;
			}
			break;

		case 0x201:
			if (ctrl == 0x20)
			{
				set_linkstate(0x202);
				m_readcount = 0x00;
			}
			break;

		case 0x203:
			m_readcount++;
			osd_printf_verbose("VUNIT_COMM: 1 m_readcount++ : %u\n", m_readcount);

			if (m_readcount >= 8)
			{
				m_readcount = 0;
				set_linkstate(0x204);
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

void midway_vunit_comm_device::set_linkstate(uint16_t newstate)
{
	if (m_linkstate != newstate)
	{
		osd_printf_verbose("VUNIT_COMM: linkstate %x to %x.\n", m_linkstate, newstate);
		m_linkstate = newstate;
	}
}

void midway_vunit_comm_device::send_vsync(uint8_t state)
{
	unsigned data_size = 0x0100;
	m_buffer0[0] = 0xff;
	m_buffer0[1] = state;
	send_frame(data_size);
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
					osd_printf_verbose("VUNIT_COMM: recv m_link_length[%d] = %u.\n", buffer, m_link_length[buffer]);

					// forward message to other nodes
					send_frame(data_size);
				}
			}
			else if (idx == 0xff)
			{
				if (m_linkid != 1)
				{
					// VSYNC
					m_intcount = 1;

					// forward message to other nodes (if NOT master)
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
				m_link_buffer[buffer][0] = m_linkid;
				m_link_buffer[buffer][1] = m_link_length[buffer];
				for (unsigned j = 0x00 ; j < data_size ; j++)
				{
					m_buffer0[j] = m_link_buffer[buffer][j];
				}

				osd_printf_verbose("VUNIT_COMM: send m_link_length[%d] = %u.\n", buffer, m_link_length[buffer]);
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