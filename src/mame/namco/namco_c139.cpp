// license:BSD-3-Clause
// copyright-holders:Angelo Salese
/***************************************************************************

    Namco C139 - Serial I/F Controller


    TODO:
    - Make this to actually work!
    - Is RAM shared with a specific CPU other than master/slave?
    - is this another MCU with internal ROM?

***************************************************************************/

#include "emu.h"
#include "namco_c139.h"
#include "emuopts.h"

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************
#define REG_0_STATUS 0
#define REG_1_CONTROL 1
#define REG_2_START 2
#define REG_3_MODE 3
#define REG_4_RXWORDS 4
#define REG_5_TXWORDS 5
#define REG_6_RXOFFSET 6
#define REG_7_TXOFFSET 7

// device type definition
DEFINE_DEVICE_TYPE(NAMCO_C139, namco_c139_device, "namco_c139", "Namco C139 Serial")


//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

void namco_c139_device::data_map(address_map &map)
{
	map(0x0000, 0x3fff).rw(FUNC(namco_c139_device::ram_r),FUNC(namco_c139_device::ram_w));
}

void namco_c139_device::regs_map(address_map &map)
{
	map(0x00, 0x0f).rw(FUNC(namco_c139_device::reg_r), FUNC(namco_c139_device::reg_w));
}

//-------------------------------------------------
//  namco_c139_device - constructor
//-------------------------------------------------

namco_c139_device::namco_c139_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, NAMCO_C139, tag, owner, clock)
	, m_irq_cb(*this)
{
#ifdef C139_SIMULATION
	// prepare localhost "filename"
	m_localhost[0] = 0;
	strcat(m_localhost, "socket.");
	strcat(m_localhost, mconfig.options().comm_localhost());
	strcat(m_localhost, ":");
	strcat(m_localhost, mconfig.options().comm_localport());

	// prepare remotehost "filename"
	m_remotehost[0] = 0;
	strcat(m_remotehost, "socket.");
	strcat(m_remotehost, mconfig.options().comm_remotehost());
	strcat(m_remotehost, ":");
	strcat(m_remotehost, mconfig.options().comm_remoteport());

	m_linkid = 0;
	for (int x = 0; x < sizeof(m_localhost) && m_localhost[x] != 0; x++)
	{

		m_linkid ^= m_localhost[x];
	}

	osd_printf_verbose("C139: ID byte = %02d\n", m_linkid);
#endif
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void namco_c139_device::device_start()
{
	m_irq_cb.resolve_safe();
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void namco_c139_device::device_reset()
{
	m_linktimer = 0x0000;
}


//**************************************************************************
//  READ/WRITE HANDLERS
//**************************************************************************

uint16_t namco_c139_device::ram_r(offs_t offset)
{
	/*if (!machine().side_effects_disabled())
		osd_printf_verbose("C139: ram_r %02x = %04x\n", offset, m_ram[offset]);*/

	return m_ram[offset] & 0x1ff;
}

void namco_c139_device::ram_w(offs_t offset, uint16_t data, uint16_t mem_mask)
{
	/*if (!machine().side_effects_disabled())
		osd_printf_verbose("C139: ram_w %02x = %04x, %04x\n", offset, data, mem_mask);*/

	COMBINE_DATA(&m_ram[offset]);
}

uint16_t namco_c139_device::reg_r(offs_t offset)
{
	uint16_t result = m_reg[offset];

	if (!machine().side_effects_disabled())
		osd_printf_verbose("C139: reg_r[%02x] = %04x\n", offset, result);

	return result;
}

void namco_c139_device::reg_w(offs_t offset, uint16_t data, uint16_t mem_mask)
{
	if (!machine().side_effects_disabled())
		osd_printf_verbose("C139: reg_w[%02x] = %04x\n", offset, data);

	m_reg[offset] = data;

	if (offset == 0 && data == 0)
		m_reg[offset] = 4;

	if (offset == 1 && data == 1)
		m_linkcount = 0;
}

void namco_c139_device::vblank_irq_trigger()
{
#ifdef C139_SIMULATION
	comm_tick();
	comm_tx();
#endif
}

void namco_c139_device::check_rx()
{
#ifdef C139_SIMULATION
	comm_tick();
	comm_rx();
#endif
}

#ifdef C139_SIMULATION
void namco_c139_device::comm_tick()
{
	if (m_linktimer > 0x0000)
		m_linktimer--;

	if (m_linktimer == 0x0000)
	{
		std::error_condition filerr;

		// check rx socket
		if (!m_line_rx)
		{
			osd_printf_verbose("C139: listen on %d\n", m_localhost);
			uint64_t filesize; // unused
			filerr = osd_file::open(m_localhost, OPEN_FLAG_CREATE, m_line_rx, filesize);
			if (filerr.value() != 0)
			{
				osd_printf_verbose("C139: rx connection failed\n");
				m_line_tx.reset();
				m_linktimer = 0x0100;
			}
		}

		// check tx socket
		if (!m_line_tx)
		{
			osd_printf_verbose("C139: connect to %s\n", m_remotehost);
			uint64_t filesize; // unused
			filerr = osd_file::open(m_remotehost, 0, m_line_tx, filesize);
			if (filerr.value() != 0)
			{
				osd_printf_verbose("C139: tx connection failed\n");
				m_line_tx.reset();
				m_linktimer = 0x0100;
			}
		}
	}
}

void namco_c139_device::comm_tx()
{
	// if both sockets are there try to send data
	if (m_line_rx && m_line_tx)
	{
		// link established
		int dataSize = 0x200;

		// send data
		switch (m_reg[REG_1_CONTROL])
		{
			case 0x09:
				// suzuka8h, acedrive, winrungp, cybrcycc
				// 0b1001 - auto-send with sync bit?
				m_reg[REG_2_START] |= 0x03;
				break;

			case 0x0c:
				break;

			case 0x0d:
				// final lap
				// 0b1011 - auto-send with register?
				if (m_reg[REG_5_TXWORDS] > 0x00)
					m_reg[REG_2_START] |= 0x01;
				break;

			case 0x0f:
				// init?
				return;
		}

		if (m_reg[REG_1_CONTROL] & 0x08)
			if (m_reg[REG_2_START] & 0x01)
				send_data(dataSize);
	}
}

void namco_c139_device::comm_rx()
{
	// if both sockets are there check for received data
	if (m_line_rx && m_line_tx)
	{
		// link established
		int dataSize = 0x200;
		int rxSize = 0;
		int rxOffset = 0;
		int bufOffset = 0;
		int recv = 0;

		if (m_reg[REG_0_STATUS] != 0x02)
		{
			// try to read a message
			recv = read_frame(dataSize);
			if (recv > 0)
			{
				// save message to "rx buffer"
				rxSize = m_buffer0[2] << 8 | m_buffer0[1];
				rxOffset = 0; //m_reg[REG_6_RXOFFSET]; // rx offset in words
				osd_printf_verbose("C139: rxOffset = %04x, rxSize == %02x\n", rxOffset, rxSize);
				bufOffset = 3;
				for (int j = 0x00 ; j < rxSize ; j++)
				{
					m_ram[0x1000 + (rxOffset & 0x0fff)] = m_buffer0[bufOffset + 1] << 8 | m_buffer0[bufOffset];
					rxOffset++;
					bufOffset += 2;
				}

				/*
				// relay messages
				if (m_buffer0[0] != m_linkid)
					send_frame(dataSize);
				*/

				// update regs
				m_reg[REG_0_STATUS] = 0x02;
				m_reg[REG_4_RXWORDS] = rxSize;
				m_reg[REG_6_RXOFFSET] = rxSize; //rxOffset & 0x0fff;

				// fire interrupt
				m_irq_cb(ASSERT_LINE);
			}
			else
			{
				// update regs
				m_reg[REG_0_STATUS] = 0x04;
			}
		}
	}
}

int namco_c139_device::find_sync_bit()
{
	// hack to find sync bit in data area
	int txOffset = m_reg[REG_7_TXOFFSET] >> 1; // tx offset in bytes
	for (int j = 0; j < 0x200; j++)
	{
		if (m_ram[txOffset + j] & 0x0100)
		{
			return j + 1;
		}
	}
	return 0;
}

int namco_c139_device::read_frame(int dataSize)
{
	if (!m_line_rx)
		return 0;

	// try to read a message
	std::uint32_t recv = 0;
	std::error_condition filerr = m_line_rx->read(m_buffer0, 0, dataSize, recv);
	if (recv > 0)
	{
		// check if message complete
		if (recv != dataSize)
		{
			// only part of a message - read on
			std::uint32_t togo = dataSize - recv;
			int offset = recv;
			while (togo > 0)
			{
				filerr = m_line_rx->read(m_buffer1, 0, togo, recv);
				if (recv > 0)
				{
					for (int i = 0 ; i < recv ; i++)
					{
						m_buffer0[offset + i] = m_buffer1[i];
					}
					togo -= recv;
					offset += recv;
				}
				if ((!filerr && recv == 0) || (filerr && std::errc::operation_would_block != filerr))
					togo = 0;
			}
		}
	}
	if ((!filerr && recv == 0) || (filerr && std::errc::operation_would_block != filerr))
	{
		osd_printf_verbose("C139: rx connection error\n");
		m_line_rx.reset();
		m_linktimer = 0x0100;
	}
	return recv;
}

void namco_c139_device::send_data(int dataSize)
{
	int txSize = m_reg[REG_5_TXWORDS];
	int txOffset = m_reg[REG_7_TXOFFSET] >> 1; // tx offset in bytes
	int bufOffset = 3;

	if (m_reg[REG_2_START] & 0x02)
	{
		txSize = find_sync_bit();
	}

	osd_printf_verbose("C139: txOffset = %04x, txSize == %02x\n", txOffset, txSize);
	if (txSize == 0)
		return;

	m_buffer0[0] = m_linkid;
	m_buffer0[1] = txSize & 0xff;
	m_buffer0[2] = (txSize & 0xff00) >> 8;

	for (int j = 0x00 ; j < txSize ; j++)
	{
		m_buffer0[bufOffset] = m_ram[txOffset] & 0xff;
		m_buffer0[bufOffset + 1] = (m_ram[txOffset] & 0xff00) >> 8;

		txOffset++;
		bufOffset += 2;
	}

	// set bit-8
	m_buffer0[bufOffset -1] |= 0x01;

	// reset tx flag, tx words, tx offset
	m_reg[REG_2_START] ^= 0x01;
	//m_reg[REG_5_TXWORDS] = 0;
	//m_reg[REG_7_TXOFFSET] = 0;

	send_frame(dataSize);
}

void namco_c139_device::send_frame(int dataSize)
{
	if (!m_line_tx)
		return;

	std::uint32_t written;
	std::error_condition filerr = m_line_tx->write(&m_buffer0, 0, dataSize, written);
	if (filerr)
	{
		osd_printf_verbose("C139: tx connection error\n");
		m_line_tx.reset();
		m_linktimer = 0x0100;
	}
}
#endif
