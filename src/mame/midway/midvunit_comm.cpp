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
	m_dsw(*this, "^DSW"),
	m_acceptor(m_ioctx),
	m_sock_rx(m_ioctx),
	m_sock_tx(m_ioctx),
	m_tx_timeout(m_ioctx)
{
	m_framesync = mconfig.options().comm_framesync() ? 0x01 : 0x00;
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
				m_link_offset[i] = 0;
				m_link_length[i] = 19;
				m_link_ready[i] = 0;
				m_link_alive[i] = 0;

				m_link_buffer[i][0] = i + 1; // link id
				m_link_buffer[i][1] = 0x00;  // head.len-h
				m_link_buffer[i][2] = 0x06;  // head.len-l
				m_link_buffer[i][3] = 0xff;  // head.chk-h
				m_link_buffer[i][4] = 0xf9;  // head.chk-l
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
			for (int i = 0; i < 4; i++)
			{
				m_link_offset[i] = 0;
				m_link_length[i] = 19;
				m_link_ready[i] = 0;
				m_link_alive[i] = 0;

				m_link_buffer[i][0] = i + 1; // link id
				m_link_buffer[i][1] = 0x00;  // head.len-h
				m_link_buffer[i][2] = 0x06;  // head.len-l
				m_link_buffer[i][3] = 0xff;  // head.chk-h
				m_link_buffer[i][4] = 0xf9;  // head.chk-l
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
			for (int i = 0; i < 4; i++)
			{
				m_link_offset[i] = 0;
				m_link_length[i] = 41;
				m_link_ready[i] = 0;
				m_link_alive[i] = 0;

				m_link_buffer[i][0] = i + 1; // link id
				m_link_buffer[i][1] = 0x00;
				m_link_buffer[i][2] = 0x00;
				m_link_buffer[i][3] = 0x00;
				m_link_buffer[i][4] = 0x08;
				m_link_buffer[i][5] = 0x00;
				m_link_buffer[i][6] = 0x00;
				m_link_buffer[i][7] = 0x00;
				m_link_buffer[i][8] = 0x00;
				m_link_buffer[i][9] = 0x00;
				m_link_buffer[i][10] = 0x00;
				m_link_buffer[i][11] = 0x00;
				m_link_buffer[i][12] = 0x00;
				m_link_buffer[i][13] = 0x00;
				m_link_buffer[i][14] = 0x00;
				m_link_buffer[i][15] = 0x00;
				m_link_buffer[i][16] = 0x00;
				m_link_buffer[i][17] = 0x00;
				m_link_buffer[i][18] = 0x00;
				m_link_buffer[i][19] = 0x00;
				m_link_buffer[i][20] = 0x00;
				m_link_buffer[i][21] = 0x00;
				m_link_buffer[i][22] = 0x00;
				m_link_buffer[i][23] = 0x00;
				m_link_buffer[i][24] = 0x00;
				m_link_buffer[i][25] = 0x00;
				m_link_buffer[i][26] = 0x00;
				m_link_buffer[i][27] = 0x00;
				m_link_buffer[i][28] = 0x00;
				m_link_buffer[i][29] = 0x00;
				m_link_buffer[i][30] = 0x00;
				m_link_buffer[i][31] = 0x00;
				m_link_buffer[i][32] = 0x00;
				m_link_buffer[i][33] = 0x00;
				m_link_buffer[i][34] = 0x00;
				m_link_buffer[i][35] = 0x00;
				m_link_buffer[i][36] = 0x00;
				m_link_buffer[i][37] = 0x00;
				m_link_buffer[i][38] = 0x00;
				m_link_buffer[i][39] = 0x00;
				m_link_buffer[i][40] = 0x08;
			}
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

		case 3:
			result = data_r_offroadc();
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

		case 3:
			data_w_offroadc(data);
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
	switch (m_linktype)
	{
		case 1:
			flags_w_crusnusa(data);
			break;

		case 2:
			flags_w_crusnwld(data);
			break;

		case 3:
			flags_w_offroadc(data);
			break;

		default:
			logerror("VUNIT_COMM-flags_w: unknown linktype %d\n", m_linktype);
			break;
	}

	osd_printf_verbose("flags_w: %08x @ %08x.\n", data, m_maincpu->pc());

	m_flags = data >> 24;
}

// ------------------------------------------------

uint32_t midway_vunit_comm_device::data_r_crusnusa()
{
	switch (m_linkstate)
	{
		case 0x105:
		case 0x203:
			// do NOT recv data while reading from buffer.
			break;

		case 0x103:
		case 0x205:
			// do NOT send data while writing to buffer.
			break;

		default:
			comm_tick();
			break;
	}

	uint16_t offset = 0;
	uint32_t result = 0;
	switch (m_linkstate)
	{
		case 0x100:
			// comms idle - signal slave idle
			return 0x01000000;

		case 0x101:
			// ready to send - signal slave ready to recv
			return 0x00000000;
			
		case 0x102:
			// prepare to send
			return 0x02000000;

		case 0x103:
			// end of send - prepare to recv
			set_linkstate(0x104);
			comm_tick();
			return 0x00000000;

		case 0x105:
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
				set_linkstate(0x106);
			}
			return result;

		case 0x106:
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
			// recv
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
				set_linkstate(0x204);
			}
			return result;

		case 0x204:
			set_linkstate(0x205);
			m_link_offset[1] = 0x01;
			m_link_length[1] = 0x01;
			return 0x04000000;

		case 0x205:
			set_linkstate(0x206);
			comm_tick();
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

	uint16_t offset = 0;
	switch (m_linkstate)
	{
		case 0x100:
			// comms idle
			if (ctrl == 0xc4)
			{
				// ready to send
				set_linkstate(0x101);
			}
			break;

		case 0x101:
			// ready to send
			if (ctrl == 0xc0)
			{
				// prepare to send
				set_linkstate(0x102);
			}
			break;

		case 0x102:
			// prepare to send
			if (ctrl == 0xc0)
			{
				// send
				set_linkstate(0x103);
				m_link_offset[0] = 1;
				m_link_length[0] = 1;
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
			// start recv
			set_linkstate(0x105);
			m_readcount = 0;
			m_link_offset[1] = 1;
			if (m_framesync && m_link_alive[1])
				wait_recv_ready(1);
			break;

		case 0x106:
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
				m_link_offset[0] = 1;
				if (m_framesync && m_link_alive[0])
					wait_recv_ready(0);
			}
			break;

		case 0x203:
			if (ctrl == 0x30)
			{
				set_linkstate(0x204);
			}
			break;

		case 0x205:
			// send
			offset = m_link_offset[1];
			osd_printf_verbose("VUNIT_COMM: send %02x @ %u\n", payload, offset);
			m_link_buffer[1][offset] = payload;
			offset++;
			m_link_offset[1] = m_link_length[1] = offset;
			break;

		case 0x206:
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

// ------------------------------------------------

uint32_t midway_vunit_comm_device::data_r_crusnwld()
{
	switch (m_linkstate)
	{
		case 0x110:
		case 0x120:
		case 0x130:
		case 0x140:
		case 0x210:
		case 0x220:
		case 0x230:
		case 0x240:
		case 0x310:
		case 0x320:
		case 0x330:
		case 0x340:
		case 0x410:
		case 0x420:
		case 0x430:
		case 0x440:
			// do NOT send/recv data while accessing a buffer.
			break;

		default:
			comm_tick();
			break;
	}

	uint16_t offset = 0;
	uint32_t result = 0;
	switch (m_linkstate)
	{
		case 0x100:
			// start of communication
			return get_linkmask_crusnwld();

		case 0x101:
			// handshake
			// status is ANDed with a pattern @ 00773c+
			osd_printf_verbose("VUNIT_COMM: sync-1 step-%u\n", m_readcount);
			switch (m_readcount)
			{
				case 0:
					return 0x00ff0000;

				case 1:
					return get_linkmask_crusnwld() & 0x0c000000;

				case 2:
					return get_linkmask_crusnwld() & 0x09000000;

				case 3:
					return get_linkmask_crusnwld() & 0x03000000;

				case 4:
					return get_linkmask_crusnwld() & 0x06000000;

				case 5:
					return get_linkmask_crusnwld() & 0x01000000;

				case 6:
					return get_linkmask_crusnwld() & 0x02000000;

				case 7:
					return get_linkmask_crusnwld() & 0x04000000;

				case 8:
					set_linkstate(0x102);
					return get_linkmask_crusnwld() & 0x08000000;

				default:
					return 0;
			}
			break;

		case 0x102:
			// after handshake
			return get_linkmask_crusnwld();

		case 0x110:
			// send
			m_readcount++;
			if (m_readcount == 2)
			{
				set_linkstate(0x111);
				m_readcount = 0;
				comm_tick();
				return get_linkmask_crusnwld();
			}
			return 0x00ff0000;

		case 0x111:
			// after send - prepare recv 2
			if (m_readcount > 0)
				return 0x005a0000;
			return get_linkmask_crusnwld();

		case 0x120:
			// recv 2
			offset = m_link_offset[1];
			osd_printf_verbose("VUNIT_COMM: read-1.2 %02x.@ %u\n", m_link_buffer[1][offset], offset);
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
				set_linkstate(0x121);
			}
			return result;

		case 0x121:
			// after recv 2
			if (m_readcount > 0)
				return 0x005a0000;
			m_readcount++;
			return 0x02ff0000;

		case 0x130:
			// recv 3
			offset = m_link_offset[2];
			osd_printf_verbose("VUNIT_COMM: read-1.3 %02x.@ %u\n", m_link_buffer[2][offset], offset);
			result = m_link_buffer[2][offset] << 16;
			if ((offset % 2))
				result |= 0x04000000;
			m_readcount++;
			if (m_readcount == 2)
			{
				m_readcount = 0;
				m_link_offset[2] += 1;
			}
			if (m_link_offset[2] >= m_link_length[2])
			{
				set_linkstate(0x131);
			}
			return result;

		case 0x131:
			// after recv 3
			if (m_readcount > 0)
				return 0x005a0000;
			m_readcount++;
			return 0x04ff0000;

		case 0x140:
			// recv 4
			offset = m_link_offset[3];
			osd_printf_verbose("VUNIT_COMM: read-1.4 %02x.@ %u\n", m_link_buffer[3][offset], offset);
			result = m_link_buffer[3][offset] << 16;
			if ((offset % 2))
				result |= 0x08000000;
			m_readcount++;
			if (m_readcount == 2)
			{
				m_readcount = 0;
				m_link_offset[3] += 1;
			}
			if (m_link_offset[3] >= m_link_length[3])
			{
				set_linkstate(0x141);
			}
			return result;

		case 0x141:
			// after recv 4
			if (m_readcount > 0)
				return 0x00000000;
			m_readcount++;
			return 0x08ff0000;

// -----------------------------------

		case 0x200:
			// start of communication
			return 0x00ff0000;

		case 0x201:
			// handshake
			if (m_readcount % 2)
				return 0x00ff0000;
			else
				return 0x01ff0000;

		case 0x202:
			// after handshake
			// 2pl c6 0c
			// 3pl 8e 28
			// 4pl 1e c3
			switch (get_linkcount_crusnwld())
			{
				case 2:
					if (m_readcount > 1)
						return 0x010c0000;
					m_readcount++;
					return 0x00c60000;

				case 3:
					if (m_readcount > 1)
						return 0x01280000;
					m_readcount++;
					return 0x008e0000;

				case 4:
					if (m_readcount > 1)
						return 0x01c30000;
					m_readcount++;
					return 0x001e0000;

				default:
					return 0;
			}

		case 0x203:
			// prepare recv 1
			return 0x005a0000;

		case 0x210:
			// recv 1
			offset = m_link_offset[0];
			osd_printf_verbose("VUNIT_COMM: read-2.1 %02x.@ %u\n", m_link_buffer[0][offset], offset);
			result = m_link_buffer[0][offset] << 16;
			if ((offset % 2))
				result |= 0x01000000;
			m_readcount++;
			if (m_readcount == 2)
			{
				m_readcount = 0;
				m_link_offset[0] += 1;
			}
			if (m_link_offset[0] >= m_link_length[0])
			{
				set_linkstate(0x211);
			}
			return result;

		case 0x211:
			// after recv 1 - before send
			if (m_readcount > 0)
				return 0x01ff0000;
			return 0x00ff0000;

		case 0x220:
			// send
			if (m_readcount > 0)
			{
				set_linkstate(0x221);
				m_readcount = 0;
				comm_tick();
				switch (get_linkcount_crusnwld())
				{
					case 2:
						return 0x01ff0000;

					case 3:
						return 0x05ff0000;

					case 4:
						return 0x0dff0000;

					default:
						return 0;
				}
			}
			m_readcount++;
			return 0x00ff0000;

		case 0x221:
			// after send - prepare recv 3
			m_readcount++;
			return 0x005a0000;

		case 0x230:
			// recv 3
			offset = m_link_offset[2];
			osd_printf_verbose("VUNIT_COMM: read-2.3 %02x.@ %u\n", m_link_buffer[2][offset], offset);
			result = m_link_buffer[2][offset] << 16;
			if ((offset % 2))
				result |= 0x04000000;
			m_readcount++;
			if (m_readcount == 2)
			{
				m_readcount = 0;
				m_link_offset[2] += 1;
			}
			if (m_link_offset[2] >= m_link_length[2])
			{
				set_linkstate(0x231);
			}
			return result;

		case 0x231:
			// after recv 3 - before recv 4
			m_readcount++;
			if (m_readcount > 1)
				return 0x005a0000;
			return 0x04ff0000;

		case 0x240:
			// recv 4
			offset = m_link_offset[3];
			osd_printf_verbose("VUNIT_COMM: read-2.4 %02x.@ %u\n", m_link_buffer[2][offset], offset);
			result = m_link_buffer[3][offset] << 16;
			if ((offset % 2))
				result |= 0x08000000;
			m_readcount++;
			if (m_readcount == 2)
			{
				m_readcount = 0;
				m_link_offset[3] += 1;
			}
			if (m_link_offset[3] >= m_link_length[3])
			{
				set_linkstate(0x241);
			}
			return result;

		case 0x241:
			// after recv 4
			if (m_readcount > 0)
				return 0x00000000;
			m_readcount++;
			return 0x08ff0000;

// -----------------------------------

		case 0x300:
			// start of communication
			return 0x00ff0000;

		case 0x301:
			// handshake
			if (m_readcount % 2)
				return 0x00ff0000;
			else
				return 0x01ff0000;

		case 0x302:
			// after handshake
			// 2pl c6 0c
			// 3pl 8e 28
			// 4pl 1e c3
			switch (get_linkcount_crusnwld())
			{
				case 2:
					if (m_readcount > 1)
						return 0x010c0000;
					m_readcount++;
					return 0x00c60000;

				case 3:
					if (m_readcount > 1)
						return 0x01280000;
					m_readcount++;
					return 0x008e0000;

				case 4:
					if (m_readcount > 1)
						return 0x01c30000;
					m_readcount++;
					return 0x001e0000;

				default:
					return 0;
			}

		case 0x303:
			// prepare recv 1
			return 0x005a0000;

		case 0x310:
			// recv 1
			offset = m_link_offset[0];
			osd_printf_verbose("VUNIT_COMM: read-3.1 %02x.@ %u\n", m_link_buffer[0][offset], offset);
			result = m_link_buffer[0][offset] << 16;
			if ((offset % 2))
				result |= 0x01000000;
			m_readcount++;
			if (m_readcount == 2)
			{
				m_readcount = 0;
				m_link_offset[0] += 1;
			}
			if (m_link_offset[0] >= m_link_length[0])
			{
				set_linkstate(0x311);
			}
			return result;

		case 0x311:
			// after recv 1 - before recv 2
			m_readcount++;
			if (m_readcount > 1)
				return 0x005a0000;
			return 0x01ff0000;

		case 0x320:
			// recv 2
			offset = m_link_offset[1];
			osd_printf_verbose("VUNIT_COMM: read-3.2 %02x.@ %u\n", m_link_buffer[1][offset], offset);
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
				set_linkstate(0x321);
			}
			return result;

		case 0x321:
			// after recv 2 - before send
			if (m_readcount > 0)
				return 0x02ff0000;
			return 0x00ff0000;

		case 0x330:
			// send
			if (m_readcount > 0)
			{
				set_linkstate(0x331);
				m_readcount = 0;
				comm_tick();
				switch (get_linkcount_crusnwld())
				{
					case 2:
						return 0x03ff0000;

					case 3:
						return 0x03ff0000;

					case 4:
						return 0x0bff0000;

					default:
						return 0;
				}
			}
			m_readcount++;
			return 0x00ff0000;

		case 0x331:
			// after send - prepare recv 4
			m_readcount++;
			return 0x005a0000;

		case 0x340:
			// recv 4
			offset = m_link_offset[3];
			osd_printf_verbose("VUNIT_COMM: read-3.4 %02x.@ %u\n", m_link_buffer[2][offset], offset);
			result = m_link_buffer[3][offset] << 16;
			if ((offset % 2))
				result |= 0x08000000;
			m_readcount++;
			if (m_readcount == 2)
			{
				m_readcount = 0;
				m_link_offset[3] += 1;
			}
			if (m_link_offset[3] >= m_link_length[3])
			{
				set_linkstate(0x341);
			}
			return result;

		case 0x341:
			// after recv 4
			if (m_readcount > 0)
				return 0x00000000;
			m_readcount++;
			return 0x08ff0000;

// -----------------------------------

		case 0x400:
			// start of communication
			return 0x00ff0000;

		case 0x401:
			// handshake
			if (m_readcount % 2)
				return 0x00ff0000;
			else
				return 0x01ff0000;

		case 0x402:
			// after handshake
			// 2pl c6 0c
			// 3pl 8e 28
			// 4pl 1e c3
			switch (get_linkcount_crusnwld())
			{
				case 2:
					if (m_readcount > 1)
						return 0x010c0000;
					m_readcount++;
					return 0x00c60000;

				case 3:
					if (m_readcount > 1)
						return 0x01280000;
					m_readcount++;
					return 0x008e0000;

				case 4:
					if (m_readcount > 1)
						return 0x01c30000;
					m_readcount++;
					return 0x001e0000;

				default:
					return 0;
			}

		case 0x403:
			// prepare recv 1
			return 0x005a0000;

		case 0x410:
			// recv 1
			offset = m_link_offset[0];
			osd_printf_verbose("VUNIT_COMM: read-4.1 %02x.@ %u\n", m_link_buffer[0][offset], offset);
			result = m_link_buffer[0][offset] << 16;
			if ((offset % 2))
				result |= 0x01000000;
			m_readcount++;
			if (m_readcount == 2)
			{
				m_readcount = 0;
				m_link_offset[0] += 1;
			}
			if (m_link_offset[0] >= m_link_length[0])
			{
				set_linkstate(0x411);
			}
			return result;

		case 0x411:
			// after recv 1 - before recv 2
			m_readcount++;
			if (m_readcount > 1)
				return 0x005a0000;
			return 0x01ff0000;

		case 0x420:
			// recv 2
			offset = m_link_offset[1];
			osd_printf_verbose("VUNIT_COMM: read-4.2 %02x.@ %u\n", m_link_buffer[1][offset], offset);
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
				set_linkstate(0x421);
			}
			return result;

		case 0x421:
			// after recv 2 - before recv 3
			m_readcount++;
			if (m_readcount > 1)
				return 0x005a0000;
			return 0x02ff0000;

		case 0x430:
			// recv 3
			offset = m_link_offset[2];
			osd_printf_verbose("VUNIT_COMM: read-4.3 %02x.@ %u\n", m_link_buffer[2][offset], offset);
			result = m_link_buffer[2][offset] << 16;
			if ((offset % 2))
				result |= 0x04000000;
			m_readcount++;
			if (m_readcount == 2)
			{
				m_readcount = 0;
				m_link_offset[2] += 1;
			}
			if (m_link_offset[2] >= m_link_length[2])
			{
				set_linkstate(0x431);
			}
			return result;

		case 0x431:
			// after recv 3 - before send
			if (m_readcount > 0)
				return 0x04ff0000;
			return 0x00ff0000;

		case 0x440:
			// send
			if (m_readcount > 0)
			{
				set_linkstate(0x441);
				m_readcount = 0;
				comm_tick();
				switch (get_linkcount_crusnwld())
				{
					case 2:
						return 0x03ff0000;

					case 3:
						return 0x07ff0000;

					case 4:
						return 0x07ff0000;

					default:
						return 0;
				}
			}
			m_readcount++;
			return 0x00ff0000;

		case 0x441:
			// after send
			return 0x00ff0000;

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

	uint16_t offset = 0;
	switch (m_linkstate)
	{
		case 0x100:
			// start of communication
			if (ctrl == 0x11)
			{
				// handshake
				set_linkstate(0x101);
				m_readcount = 0x00;
			}
			break;

		case 0x101:
			// handshake
			m_readcount++;
			break;

		case 0x102:
			// after handshake
			if (ctrl == 0x10 && payload == 0xa5)
			{
				// handshake error
				set_linkstate(0x100);
			}
			else if (ctrl == 0x10 && payload == 0x5a)
			{
				// handshake ok
				set_linkstate(0x110);
				m_link_offset[0] = 0x01;
				m_link_length[0] = 0x01;
				m_readcount = 0x00;
			}
			break;

		case 0x110:
			// send
			offset = m_link_offset[0];
			osd_printf_verbose("VUNIT_COMM: send-1 %02x @ %u\n", payload, offset);
			m_link_buffer[0][offset] = payload;
			offset++;
			m_link_offset[0] = m_link_length[0] = offset;
			break;

		case 0x111:
			// after send - prepare recv 2
			if (ctrl == 0x11)
			{
				m_readcount = 1;
			}
			else if (ctrl == 0x10)
			{
				set_linkstate(0x120);
				m_readcount = 0;
				m_link_offset[1] = 0x01;
				if (m_framesync && m_link_alive[1])
					wait_recv_ready(1);
			}
			break;

		case 0x120:
			// recv 2
			if (ctrl == 0x10 && payload == 0xa5)
				set_linkstate(0x100);
			if (ctrl == 0x11)
			{
				set_linkstate(0x121);
			}
			break;

		case 0x121:
			// after recv 2 - prepare recv 3
			if (ctrl == 0x10)
			{
				if (payload == 0xa5)
					set_linkstate(0x100);
				else
				{
					set_linkstate(0x130);
					m_readcount = 0;
					m_link_offset[2] = 0x01;
					if (m_framesync && m_link_alive[2])
						wait_recv_ready(2);
				}
			}
			break;

		case 0x130:
			// recv 3
			if (ctrl == 0x10 && payload == 0xa5)
				set_linkstate(0x100);
			if (ctrl == 0x11)
			{
				set_linkstate(0x131);
			}
			break;

		case 0x131:
			// after recv 3
			if (ctrl == 0x10)
			{
				if (payload == 0xa5)
					set_linkstate(0x100);
				else
				{
					set_linkstate(0x140);
					m_readcount = 0;
					m_link_offset[3] = 0x01;
					if (m_framesync && m_link_alive[3])
						wait_recv_ready(3);
				}
			}
			break;

		case 0x140:
			// recv 4
			if (ctrl == 0x10 && payload == 0xa5)
				set_linkstate(0x100);
			if (ctrl == 0x11)
			{
				set_linkstate(0x141);
			}
			break;

		case 0x141:
			// after recv 4
			if (ctrl == 0x10)
				set_linkstate(0x100);
			break;

// ------------------------------------------------

		case 0x200:
			if (ctrl == 0x22)
			{
				set_linkstate(0x201);
				m_readcount = 0;
			}
			break;

		case 0x201:
			// handshake
			osd_printf_verbose("VUNIT_COMM: sync-2 step-%u\n", m_readcount);
			m_readcount++;
			if (m_readcount == 9)
			{
				set_linkstate(0x202);
				m_readcount = 0;
			}
			break;

		case 0x202:
			// after handshake
			if (ctrl == 0x22)
			{
				set_linkstate(0x203);
			}
			break;

		case 0x203:
			// prepare recv 1
			if (ctrl == 0x20)
			{
				set_linkstate(0x210);
				m_readcount = 0;
				m_link_offset[0] = 0x01;
				if (m_framesync && m_link_alive[0])
					wait_recv_ready(0);
			}
			break;

		case 0x210:
			// recv 1
			if (ctrl == 0x22)
			{
				set_linkstate(0x211);
				m_readcount = 0;
			}
			break;

		case 0x211:
			// after recv - before send
			if (ctrl == 0x22)
			{
				m_readcount = 1;
			}
			else if (ctrl == 0x20 && payload == 0x5a)
			{
				set_linkstate(0x220);
				m_link_offset[1] = 0x01;
				m_link_length[1] = 0x01;
				m_readcount = 0x00;
			}
			break;

		case 0x220:
			// send
			offset = m_link_offset[1];
			osd_printf_verbose("VUNIT_COMM: send-2 %02x @ %u\n", payload, offset);
			m_link_buffer[1][offset] = payload;
			offset++;
			m_link_offset[1] = m_link_length[1] = offset;
			break;

		case 0x221:
			// after send - prepare recv 3
			if (ctrl == 0x20)
			{
				if (m_readcount == 2)
				{
					set_linkstate(0x230);
					m_readcount = 0;
					m_link_offset[2] = 0x01;
					if (m_framesync && m_link_alive[2])
						wait_recv_ready(2);
				}
				else
				{
					set_linkstate(0x200);
				}
			}
			break;

		case 0x230:
			// recv 3
			if (ctrl == 0x22)
			{
				set_linkstate(0x231);
				m_readcount = 0;
			}
			break;

		case 0x231:
			// after recv 3 - before recv 4
			if (ctrl == 0x20)
			{
				if (m_readcount > 1)
				{
					set_linkstate(0x240);
					m_readcount = 0;
					m_link_offset[3] = 0x01;
					if (m_framesync && m_link_alive[3])
						wait_recv_ready(3);
				}
				else
				{
					set_linkstate(0x200);
				}
			}
			break;

		case 0x240:
			// recv 4
			if (ctrl == 0x22)
			{
				set_linkstate(0x241);
				m_readcount = 0;
			}
			break;

		case 0x241:
			// after recv 4
			if (ctrl == 0x20)
				set_linkstate(0x200);
			break;

// ------------------------------------------------

		case 0x300:
			if (ctrl == 0x44)
			{
				set_linkstate(0x301);
				m_readcount = 0;
			}
			break;

		case 0x301:
			// handshake
			osd_printf_verbose("VUNIT_COMM: sync-3 step-%u\n", m_readcount);
			m_readcount++;
			if (m_readcount == 9)
			{
				set_linkstate(0x302);
				m_readcount = 0;
			}
			break;

		case 0x302:
			// after handshake
			if (ctrl == 0x44)
			{
				set_linkstate(0x303);
			}
			break;

		case 0x303:
			// prepare recv 1
			if (ctrl == 0x40)
			{
				set_linkstate(0x310);
				m_readcount = 0;
				m_link_offset[0] = 0x01;
				if (m_framesync && m_link_alive[0])
					wait_recv_ready(0);
			}
			break;

		case 0x310:
			// recv 1
			if (ctrl == 0x44)
			{
				set_linkstate(0x311);
				m_readcount = 0;
			}
			break;

		case 0x311:
			// after recv 1 - before recv 2
			if (ctrl == 0x40)
			{
				if (m_readcount > 1)
				{
					set_linkstate(0x320);
					m_readcount = 0;
					m_link_offset[1] = 0x01;
					if (m_framesync && m_link_alive[1])
						wait_recv_ready(1);
				}
				else
				{
					set_linkstate(0x300);
				}
			}
			break;

		case 0x320:
			// recv 2
			if (ctrl == 0x44)
			{
				set_linkstate(0x321);
				m_readcount = 0;
			}
			break;

		case 0x321:
			// after recv 2 - before send
			if (ctrl == 0x44)
			{
				m_readcount = 1;
			}
			else if (ctrl == 0x40 && payload == 0x5a)
			{
				set_linkstate(0x330);
				m_link_offset[2] = 0x01;
				m_link_length[2] = 0x01;
				m_readcount = 0x00;
			}
			break;

		case 0x330:
			// send
			offset = m_link_offset[2];
			osd_printf_verbose("VUNIT_COMM: send-3 %02x @ %u\n", payload, offset);
			m_link_buffer[2][offset] = payload;
			offset++;
			m_link_offset[2] = m_link_length[2] = offset;
			break;

		case 0x331:
			// after send - prepare recv 4
			if (ctrl == 0x40)
			{
				if (m_readcount == 2)
				{
					set_linkstate(0x340);
					m_readcount = 0;
					m_link_offset[3] = 0x01;
					if (m_framesync && m_link_alive[3])
						wait_recv_ready(3);
				}
				else
				{
					set_linkstate(0x300);
				}
			}
			break;

		case 0x340:
			// recv 4
			if (ctrl == 0x44)
			{
				set_linkstate(0x341);
				m_readcount = 0;
			}
			break;

		case 0x341:
			// after recv 4
			if (ctrl == 0x40)
				set_linkstate(0x300);
			break;

// ------------------------------------------------

		case 0x400:
			if (ctrl == 0x88)
			{
				set_linkstate(0x401);
				m_readcount = 0;
			}
			break;

		case 0x401:
			// handshake
			osd_printf_verbose("VUNIT_COMM: sync-4 step-%u\n", m_readcount);
			m_readcount++;
			if (m_readcount == 9)
			{
				set_linkstate(0x402);
				m_readcount = 0;
			}
			break;

		case 0x402:
			// after handshake
			if (ctrl == 0x88)
			{
				set_linkstate(0x403);
			}
			break;

		case 0x403:
			// prepare recv 1
			if (ctrl == 0x80)
			{
				set_linkstate(0x410);
				m_readcount = 0;
				m_link_offset[0] = 0x01;
				if (m_framesync && m_link_alive[0])
					wait_recv_ready(0);
			}
			break;

		case 0x410:
			// recv 1
			if (ctrl == 0x88)
			{
				set_linkstate(0x411);
				m_readcount = 0;
			}
			break;

		case 0x411:
			// after recv 1 - before recv 2
			if (ctrl == 0x80)
			{
				if (m_readcount > 1)
				{
					set_linkstate(0x420);
					m_readcount = 0;
					m_link_offset[1] = 0x01;
					if (m_framesync && m_link_alive[1])
						wait_recv_ready(1);
				}
				else
				{
					set_linkstate(0x400);
				}
			}
			break;

		case 0x420:
			// recv 2
			if (ctrl == 0x88)
			{
				set_linkstate(0x421);
				m_readcount = 0;
			}
			break;

		case 0x421:
			// after recv 2 - before recv 3
			if (ctrl == 0x80)
			{
				if (m_readcount > 1)
				{
					set_linkstate(0x430);
					m_readcount = 0;
					m_link_offset[2] = 0x01;
					if (m_framesync && m_link_alive[2])
						wait_recv_ready(2);
				}
				else
				{
					set_linkstate(0x400);
				}
			}
			break;

		case 0x430:
			// recv 3
			if (ctrl == 0x88)
			{
				set_linkstate(0x431);
				m_readcount = 0;
			}
			break;

		case 0x431:
			// after recv 3 - before send
			if (ctrl == 0x88)
			{
				m_readcount = 1;
			}
			else if (ctrl == 0x80 && payload == 0x5a)
			{
				set_linkstate(0x440);
				m_link_offset[3] = 0x01;
				m_link_length[3] = 0x01;
				m_readcount = 0x00;
			}
			break;

		case 0x440:
			// send
			offset = m_link_offset[3];
			osd_printf_verbose("VUNIT_COMM: send-4 %02x @ %u\n", payload, offset);
			m_link_buffer[3][offset] = payload;
			offset++;
			m_link_offset[3] = m_link_length[3] = offset;
			break;

		case 0x441:
			// after send
			if (ctrl == 0x80)
				set_linkstate(0x400);
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

// ------------------------------------------------

uint32_t midway_vunit_comm_device::data_r_offroadc()
{
	if (!get_linkenabled_offroadc())
		return 0;

	switch (m_linkstate)
	{
		case 0x111:
		case 0x121:
		case 0x131:
		case 0x141:
		case 0x211:
		case 0x221:
		case 0x231:
		case 0x241:
		case 0x311:
		case 0x321:
		case 0x331:
		case 0x341:
		case 0x411:
		case 0x421:
		case 0x431:
		case 0x441:
			// do NOT send/recv data while accessing a buffer.
			break;

		default:
			comm_tick();
			break;
	}

	uint16_t offset = 0;
	uint32_t result = 0;
	switch (m_linkstate)
	{
		case 0x100:
			// start of communication
			// 2 players = 04;
			// 3 players = 06;
			// 4 players = 07;
			return 0x07000000;

		case 0x101:
			// handshake
			return 0x00000000;

		case 0x140:
			// pre recv 4
			m_readcount++;
			if (m_readcount == 0x2)
			{
				set_linkstate(0x141);
				m_readcount = 0;
				m_link_offset[3] = 0x01;
				if (m_framesync && m_link_alive[3])
					wait_recv_ready(3);
			}
			return 0x00ff0000;

		case 0x141:
			// recv 4
			offset = m_link_offset[3];
			osd_printf_verbose("VUNIT_COMM: read-1.4 %02x.@ %u\n", m_link_buffer[3][offset], offset);
			result = m_link_buffer[3][offset] << 16;
			if ((offset % 2))
				result |= 0x01000000;
			m_readcount++;
			if (m_readcount == 2)
			{
				m_readcount = 0;
				m_link_offset[3] += 1;
			}
			if (m_link_offset[3] >= m_link_length[3])
			{
				set_linkstate(0x142);
			}
			return result;

		case 0x142:
			// after recv 4
			m_readcount++;
			if (m_readcount == 0x2)
			{
				set_linkstate(0x130);
				m_readcount = 0;
			}
			return 0x01ff0000;

		case 0x130:
			// pre recv 3
			m_readcount++;
			if (m_readcount == 0x2)
			{
				set_linkstate(0x131);
				m_readcount = 0;
				m_link_offset[2] = 0x01;
				if (m_framesync && m_link_alive[2])
					wait_recv_ready(2);
			}
			return 0x00ff0000;

		case 0x131:
			// recv 3
			offset = m_link_offset[2];
			osd_printf_verbose("VUNIT_COMM: read-1.3 %02x.@ %u\n", m_link_buffer[2][offset], offset);
			result = m_link_buffer[2][offset] << 16;
			if ((offset % 2))
				result |= 0x01000000;
			m_readcount++;
			if (m_readcount == 2)
			{
				m_readcount = 0;
				m_link_offset[2] += 1;
			}
			if (m_link_offset[2] >= m_link_length[2])
			{
				set_linkstate(0x132);
			}
			return result;

		case 0x132:
			// after recv 3
			m_readcount++;
			if (m_readcount == 0x2)
			{
				set_linkstate(0x120);
				m_readcount = 0;
			}
			return 0x01ff0000;

		case 0x120:
			// pre recv 2
			m_readcount++;
			if (m_readcount == 0x2)
			{
				set_linkstate(0x121);
				m_readcount = 0;
				m_link_offset[1] = 0x01;
				if (m_framesync && m_link_alive[1])
					wait_recv_ready(1);
			}
			return 0x00ff0000;

		case 0x121:
			// recv 2
			offset = m_link_offset[1];
			osd_printf_verbose("VUNIT_COMM: read-1.2 %02x.@ %u\n", m_link_buffer[1][offset], offset);
			result = m_link_buffer[1][offset] << 16;
			if ((offset % 2))
				result |= 0x01000000;
			m_readcount++;
			if (m_readcount == 2)
			{
				m_readcount = 0;
				m_link_offset[1] += 1;
			}
			if (m_link_offset[1] >= m_link_length[1])
			{
				set_linkstate(0x122);
			}
			return result;

		case 0x122:
			// after recv 2
			m_readcount++;
			if (m_readcount == 0x2)
			{
				set_linkstate(0x110);
				m_readcount = 0;
			}
			return 0x01ff0000;

		case 0x110:
			// pre send 1
			return 0x00ff0000;

		case 0x150:
			return 0x07000000;

		case 0x151:
			// if 0x07000000 will send 0x80d50000
			// if 0x00000000 will send 0x802a0000
			return 0x00000000;

// ------------------------------------------------

		case 0x200:
			// comms idle
			return 0x00000000;

		case 0x201:
			// handshake
			return 0x00aa0000;

		case 0x202:
			// pre recv 4
			return 0x005f0000;

		case 0x240:
			// pre recv 4
			m_readcount++;
			if (m_readcount == 0x2)
			{
				set_linkstate(0x241);
				m_readcount = 0;
				m_link_offset[3] = 0x01;
				if (m_framesync && m_link_alive[3])
					wait_recv_ready(3);
			}
			return 0x00ff0000;

		case 0x241:
			// recv 4
			offset = m_link_offset[3];
			osd_printf_verbose("VUNIT_COMM: read-2.4 %02x.@ %u\n", m_link_buffer[3][offset], offset);
			result = m_link_buffer[3][offset] << 16;
			if ((offset % 2))
				result |= 0x01000000;
			m_readcount++;
			if (m_readcount == 2)
			{
				m_readcount = 0;
				m_link_offset[3] += 1;
			}
			if (m_link_offset[3] >= m_link_length[3])
			{
				set_linkstate(0x242);
			}
			return result;

		case 0x242:
			// after recv 4
			m_readcount++;
			if (m_readcount == 0x2)
			{
				set_linkstate(0x230);
				m_readcount = 0;
			}
			return 0x01ff0000;

		case 0x230:
			// pre recv 3
			m_readcount++;
			if (m_readcount == 0x2)
			{
				set_linkstate(0x231);
				m_readcount = 0;
				m_link_offset[2] = 0x01;
				if (m_framesync && m_link_alive[2])
					wait_recv_ready(2);
			}
			return 0x00ff0000;

		case 0x231:
			// recv 3
			offset = m_link_offset[2];
			osd_printf_verbose("VUNIT_COMM: read-2.3 %02x.@ %u\n", m_link_buffer[2][offset], offset);
			result = m_link_buffer[2][offset] << 16;
			if ((offset % 2))
				result |= 0x01000000;
			m_readcount++;
			if (m_readcount == 2)
			{
				m_readcount = 0;
				m_link_offset[2] += 1;
			}
			if (m_link_offset[2] >= m_link_length[2])
			{
				set_linkstate(0x232);
			}
			return result;

		case 0x232:
			// after recv 3
			m_readcount++;
			if (m_readcount == 0x2)
			{
				set_linkstate(0x220);
				m_readcount = 0;
			}
			return 0x01ff0000;

		case 0x220:
			// pre send 2
			return 0x00ff0000;

		case 0x210:
			// pre recv 1
			m_readcount++;
			if (m_readcount == 0x2)
			{
				set_linkstate(0x211);
				m_readcount = 0;
				m_link_offset[0] = 0x01;
				if (m_framesync && m_link_alive[0])
					wait_recv_ready(0);
			}
			return 0x00ff0000;

		case 0x211:
			// recv 1
			offset = m_link_offset[0];
			osd_printf_verbose("VUNIT_COMM: read-2.1 %02x.@ %u\n", m_link_buffer[0][offset], offset);
			result = m_link_buffer[0][offset] << 16;
			if ((offset % 2))
				result |= 0x01000000;
			m_readcount++;
			if (m_readcount == 2)
			{
				m_readcount = 0;
				m_link_offset[0] += 1;
			}
			if (m_link_offset[0] >= m_link_length[0])
			{
				set_linkstate(0x212);
			}
			return result;

		case 0x212:
			// after recv 1
			m_readcount++;
			if (m_readcount == 0x2)
			{
				set_linkstate(0x203);
				m_readcount = 0;
			}
			return 0x01ff0000;

		case 0x204:
			return 0x00850000;

		case 0x205:
			// if 0x002a0000 will send 0x40000000
			// if 0x00d50000 will send 0x44000000
			return 0x002a0000;

		default:
			return 0;
	}
}

void midway_vunit_comm_device::data_w_offroadc(uint32_t data)
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
			case 0x80:
				m_linkid = 1;
				set_linkstate(0x100);
				osd_printf_verbose("VUNIT_COMM: we are cab 1 - MASTER.\n");
				break;

			case 0x40:
				m_linkid = 2;
				set_linkstate(0x200);
				osd_printf_verbose("VUNIT_COMM: we are cab 2 - SLAVE.\n");
				break;

			case 0x20:
				m_linkid = 3;
				set_linkstate(0x300);
				osd_printf_verbose("VUNIT_COMM: we are cab 3 - SLAVE.\n");
				break;

			case 0x10:
				m_linkid = 4;
				set_linkstate(0x400);
				osd_printf_verbose("VUNIT_COMM: we are cab 4 - SLAVE.\n");
				break;
		}
	}

	uint16_t offset;
	switch (m_linkstate)
	{
		case 0x100:
			// start of communication
			// 0xaa = handshake
			if (ctrl == 0x80 && payload == 0xaa)
			{
				set_linkstate(0x101);
			}
			break;

		case 0x101:
			// handshake
			// 0x5c = 2 players
			// 0x5e = 3 players
			// 0x5f = 4 players
			if (ctrl == 0x80 && payload == 0x5f)
			{
				// recv 4
				set_linkstate(0x140);
				m_readcount = 0;
			}
			break;

		case 0x110:
			// pre send 1
			// 80 + 10
			if (ctrl == 0x90 && payload == 0xff)
			{
				set_linkstate(0x111);
				m_link_offset[0] = 1;
				m_link_length[0] = 1;
			}
			break;

		case 0x111:
			// send 1
			offset = m_link_offset[0];
			osd_printf_verbose("VUNIT_COMM: send-1 %02x @ %u\n", payload, offset);
			m_link_buffer[0][offset] = payload;
			offset++;
			m_link_offset[0] = m_link_length[0] = offset;
			if (offset == 41)
			{
				set_linkstate(0x112);
				comm_tick();
			}
			break;

		case 0x112:
			// after send 1
			if (ctrl == 0x80 && payload == 0xff)
			{
				set_linkstate(0x150);
			}
			break;

		case 0x150:
			if (ctrl == 0x80 && payload == 0x85)
			{
				set_linkstate(0x151);
			}
			break;

		case 0x151:
			if (ctrl == 0x80 && payload == 0x2a)
			{
				set_linkstate(0x100);
			}
			break;

// ------------------------------------------------

		case 0x200:
			// comms idle
			// 44ff0000 @ 0000ccd4.
			if (ctrl == 0x44)
			{
				set_linkstate(0x201);
			}
			break;

		case 0x201:
			// handshake 1
			if (ctrl == 0x40)
			{
				set_linkstate(0x202);
			}
			break;

		case 0x202:
			// handshake 2
			if (ctrl == 0x00)
			{
				set_linkstate(0x240);
				m_readcount = 0;
			}
			break;

		case 0x220:
			// pre send 2
			if (ctrl == 0x10 && payload == 0xff)
			{
				set_linkstate(0x221);
				m_link_offset[1] = 1;
				m_link_length[1] = 1;
			}
			break;

		case 0x221:
			// send 2
			offset = m_link_offset[1];
			osd_printf_verbose("VUNIT_COMM: send-2 %02x @ %u\n", payload, offset);
			m_link_buffer[1][offset] = payload;
			offset++;
			m_link_offset[1] = m_link_length[1] = offset;
			if (offset == 41)
			{
				set_linkstate(0x222);
				comm_tick();
			}
			break;

		case 0x222:
			// after send 2
			if (ctrl == 0x00 && payload == 0xff)
			{
				set_linkstate(0x210);
			}
			break;

		case 0x203:
			if (ctrl == 0x40)
			{
				set_linkstate(0x204);
			}
			break;

		case 0x204:
			if (ctrl == 0x40)
			{
				set_linkstate(0x205);
				m_readcount = 0;
			}
			break;

		case 0x205:
			if (ctrl == 0x40)
			{
				set_linkstate(0x200);
			}
			break;

		default:
			if (ctrl == 0x88)
				set_linkstate(0x100);
			break;
	}
}

void midway_vunit_comm_device::flags_w_offroadc(uint32_t data)
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

uint8_t midway_vunit_comm_device::get_linkcount_crusnwld()
{
	return ((m_dsw->read() >> 3) & 0x0003) + 1;
}

uint32_t midway_vunit_comm_device::get_linkmask_crusnwld()
{
	uint8_t linkcount = get_linkcount_crusnwld();
	switch (linkcount)
	{
		case 2:
			return 0x02000000;

		case 3:
			return 0x06000000;

		case 4:
			return 0x0e000000;

		default:
			return 0x00000000;
	}
}

bool midway_vunit_comm_device::get_linkenabled_offroadc()
{
	return m_dsw->read() && 0x0020;
}

void midway_vunit_comm_device::set_linkstate(uint16_t newstate)
{
	if (m_linkstate != newstate)
	{
		osd_printf_verbose("VUNIT_COMM: linkstate %x to %x.\n", m_linkstate, newstate);
		m_linkstate = newstate;
	}
}

void midway_vunit_comm_device::wait_recv_ready(uint8_t idx)
{
	while(m_link_ready[idx] == 0)
	{
		if (m_rx_state == 2 && m_tx_state == 2)
			comm_tick();
		else
			return;
	}
	m_link_ready[idx] = 0;
}

void midway_vunit_comm_device::send_vsync(uint8_t state)
{
	unsigned data_size = 0x0300;
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
		unsigned data_size = 0x0300;
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
					m_link_length[buffer] = m_buffer0[0x2fe] << 8 | m_buffer0[0x2ff]; //((m_buffer0[2] * 2) + 5;
					osd_printf_verbose("VUNIT_COMM: recv m_link_length[%d] = %u.\n", buffer, m_link_length[buffer]);
					m_link_ready[buffer] = 1;
					m_link_alive[buffer] = 1;

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
				for (unsigned j = 0x00 ; j < data_size ; j++)
				{
					m_buffer0[j] = m_link_buffer[buffer][j];
				}
				m_buffer0[0x2fe] = m_link_length[buffer] >> 8;
				m_buffer0[0x2ff] = m_link_length[buffer] & 0xff;

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