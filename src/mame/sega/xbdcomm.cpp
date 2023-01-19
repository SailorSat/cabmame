// license:BSD-3-Clause
// copyright-holders:Ariane Fugmann

/*
Top    : 834-6780
Sticker: 834-7112
|---------| |--| |----------------------|
|         RX   TX            315-5336   |
|             315-5337                  |
|                                       |
|            16MHz      6264            |
|                    EPR-12587.14       |
| MB89372P-SH     Z80E        MB8421    |
|---------------------------------------|
Notes:
      315-5337 - PAL16L8
      315-5336 - PAL16L8
      Z80 clock: 8.000MHz [16/2]
      6264     : 8k x8 SRAM
      MB8421   : Fujitsu 2k x8 Dual-Port SRAM (SDIP52)
      MB89372  : Fujitsu Multi-Protocol Controller (SDIP64)
      EPR-12587: 27C256 EPROM
*/

#include "emu.h"
#include "emuopts.h"
#include "xbdcomm.h"

#define Z80_TAG     "commcpu"

#define VERBOSE 0
#include "logmacro.h"

/*************************************
 *  XBDCOMM Memory Map
 *************************************/
void xbdcomm_device::xbdcomm_mem(address_map &map)
{
	map(0x0000, 0x1fff).rom();
	map(0x2000, 0x3fff).ram();
	map(0x4000, 0x47ff).rw(m_dpram, FUNC(mb8421_device::left_r), FUNC(mb8421_device::left_w));
}

/*************************************
 *  XBDCOMM I/O Map
 *************************************/
void xbdcomm_device::xbdcomm_io(address_map &map)
{
	map.global_mask(0xff);
	map(0x20, 0x27).w(FUNC(xbdcomm_device::dma_reg_w));
	map(0x40, 0x40).rw(FUNC(xbdcomm_device::z80_stat_r), FUNC(xbdcomm_device::z80_debug_w));
	map(0x80, 0x80).w(FUNC(xbdcomm_device::z80_stat_w));
}

ROM_START( xbdcomm )
	ROM_REGION( 0x10000, Z80_TAG, ROMREGION_ERASEFF )
	ROM_DEFAULT_BIOS("epr12587")

	// found on Super Monaco GP
	ROM_SYSTEM_BIOS( 0, "epr12587", "EPR-12587" )
	ROMX_LOAD( "epr-12587.14", 0x00000, 0x08000, CRC(2afe648b) SHA1(b5bf86f3acbcc23c136185110acecf2c971294fa), ROM_BIOS(0) )
ROM_END

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

DEFINE_DEVICE_TYPE(XBDCOMM, xbdcomm_device, "xbdcomm", "Sega X-Board Communication Board")

//-------------------------------------------------
//  device_add_mconfig - add device configuration
//-------------------------------------------------

void xbdcomm_device::device_add_mconfig(machine_config &config)
{
	Z80(config, m_cpu, 8000000); // 16 MHz / 2
	m_cpu->set_memory_map(&xbdcomm_device::xbdcomm_mem);
	m_cpu->set_io_map(&xbdcomm_device::xbdcomm_io);

	MB8421(config, m_dpram).intl_callback().set(FUNC(xbdcomm_device::dpram_int5_w));
}

//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------
const tiny_rom_entry *xbdcomm_device::device_rom_region() const
{
	return ROM_NAME( xbdcomm );
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  xbdcomm_device - constructor
//-------------------------------------------------

xbdcomm_device::xbdcomm_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, XBDCOMM, tag, owner, clock),
	m_cpu(*this, Z80_TAG),
	m_dpram(*this, "dpram")
{
	std::fill(std::begin(m_dma_reg), std::end(m_dma_reg), 0);

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
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void xbdcomm_device::device_start()
{
	m_ex_page = 0;
	m_xbd_stat = 0;
	m_z80_stat = 0;
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void xbdcomm_device::device_reset()
{
}

void xbdcomm_device::device_reset_after_children()
{
	m_cpu->set_input_line(INPUT_LINE_RESET, ASSERT_LINE);
}

uint8_t xbdcomm_device::ex_r(offs_t offset)
{
	switch (offset)
	{
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
			return m_dpram->right_r(m_ex_page << 3 | offset);

		case 0x08:
			// page latch
			return m_ex_page;

		case 0x10:
			// status register?
			LOG("xbdcomm-ex_r: %02x %02x\n", offset, m_z80_stat);
#ifdef XBDCOMM_SIMULATION
			comm_tick();
#endif
			return m_z80_stat;

		default:
			logerror("xbdcomm-ex_r: %02x\n", offset);
			return 0xff;
}	}


void xbdcomm_device::ex_w(offs_t offset, uint8_t data)
{
	switch (offset)
	{
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
			m_dpram->right_w(m_ex_page << 3 | offset, data);
			break;

		case 0x08:
			// page latch
			m_ex_page = data;
#ifndef XBDCOMM_SIMULATION
			comm_tick();
#endif
			break;

		case 0x10:
			// status register?
			// bit 7 = on/off toggle
			// bit 1 = test flag?
			// bit 0 = ready to send?
			m_xbd_stat = data;
#ifndef XBDCOMM_SIMULATION
			if (m_xbd_stat & 0x80)
			{
				m_cpu->set_input_line(INPUT_LINE_RESET, CLEAR_LINE);
			}
			else
			{
				device_reset();
				device_reset_after_children();
			}
			LOG("xbdcomm-ex_w: %02x %02x\n", offset, data);
#else
			if (m_xbd_stat & 0x80)
			{
				// link active
				if (!m_linkenable)
				{
					// init command
					osd_printf_verbose("XBDCOMM: board enabled\n");
					m_linkenable = 0x01;
					m_linkid = 0x00;
					m_linkalive = 0x00;
					m_linkcount = 0x00;
					m_linktimer = 0x00e8; // 58 fps * 4s
				}
			}
			else
			{
				if (m_linkenable)
				{
					// reset command
					osd_printf_verbose("XBDCOMM: board disabled\n");
					m_linkenable = 0x00;
				}
			}
			comm_tick();
#endif
			break;

		case 0x19:
		case 0x1d:
			// unknown registers
			// 19 - 11 byte writes (smgp)
			// 1d - completes write cycle (smgp)
			break;

		default:
			logerror("xbdcomm-ex_w: %02x %02x\n", offset, data);
	}
}

WRITE_LINE_MEMBER(xbdcomm_device::dpram_int5_w)
{
	m_cpu->set_input_line_and_vector(0, state ? ASSERT_LINE : CLEAR_LINE, 0xef); // Z80 INT5
}

WRITE_LINE_MEMBER(xbdcomm_device::dlc_int7_w)
{
	logerror("dlc_int7_w: %02x\n", state);
	m_cpu->set_input_line_and_vector(0, state ? ASSERT_LINE : CLEAR_LINE, 0xff); // Z80 INT7
}

void xbdcomm_device::dma_reg_w(offs_t offset, uint8_t data)
{
	m_dma_reg[offset] = data;
	logerror("dma_reg_w: %02x %02x\n", offset, data);
	update_dma();
}

void xbdcomm_device::update_dma()
{
	// check if both channels are active to begin with
	if ((m_dma_reg[0x03] & 0x80) && (m_dma_reg[0x07] & 0x80))
	{
		int rx_dma_config = (m_dma_reg[0x02] << 16) | (m_dma_reg[0x01] << 8) | m_dma_reg[0x00];
		int tx_dma_config = (m_dma_reg[0x06] << 16) | (m_dma_reg[0x05] << 8) | m_dma_reg[0x04];

		if (rx_dma_config && tx_dma_config)
		{
			int rx_size = (dma_mem_r(rx_dma_config + 1) << 8) | dma_mem_r(rx_dma_config);
			int tx_size = (dma_mem_r(tx_dma_config + 1) << 8) | dma_mem_r(tx_dma_config);

			if (rx_size == tx_size)
			{
				int rx_addr = (dma_mem_r(rx_dma_config + 4) << 16) | (dma_mem_r(rx_dma_config + 3) << 8) | dma_mem_r(rx_dma_config + 2);
				int tx_addr = (dma_mem_r(tx_dma_config + 4) << 16) | (dma_mem_r(tx_dma_config + 3) << 8) | dma_mem_r(tx_dma_config + 2);

				int rx_flag = dma_mem_r(rx_dma_config + 5);
				int tx_flag = dma_mem_r(tx_dma_config + 5);

				for (int i = 0 ; i < rx_size ; i++)
				{
					dma_mem_w(rx_addr + i, dma_mem_r(tx_addr + i));
				}

				rx_flag |= 0x60;
				tx_flag |= 0x60;

				dma_mem_w(rx_dma_config + 5, rx_flag);
				dma_mem_w(tx_dma_config + 5, tx_flag);
				logerror("dma magic: size %04x from %04x to %04x\n", rx_size, tx_addr, rx_addr);

				if (rx_flag & 0x10)
				{
					m_dma_reg[0x00] = dma_mem_r(rx_dma_config + 9);
					m_dma_reg[0x01] = dma_mem_r(rx_dma_config + 10);
					m_dma_reg[0x02] = dma_mem_r(rx_dma_config + 11);
				}
				if (tx_flag & 0x10)
				{
					m_dma_reg[0x04] = dma_mem_r(tx_dma_config + 9);
					m_dma_reg[0x05] = dma_mem_r(tx_dma_config + 10);
					m_dma_reg[0x06] = dma_mem_r(tx_dma_config + 11);
				}

				if ((rx_flag & 0x10) && (tx_flag & 0x10))
				{
					update_dma();
				}
			}
			else
			{
				logerror("rx size != tx size\n");
			}
		}
	}
}

uint8_t xbdcomm_device::dma_mem_r(offs_t offset)
{
	return m_cpu->space(AS_PROGRAM).read_byte(offset);
}

void xbdcomm_device::dma_mem_w(offs_t offset, uint8_t data)
{
	m_cpu->space(AS_PROGRAM).write_byte(offset, data);
}

uint8_t xbdcomm_device::z80_stat_r()
{
	return m_xbd_stat;
}

void xbdcomm_device::z80_stat_w(uint8_t data)
{
	m_z80_stat = data;
}

void xbdcomm_device::z80_debug_w(uint8_t data)
{
	m_ex_page = data;
	m_z80_stat = 0;
}

void xbdcomm_device::comm_tick()
{
	if (m_linkenable == 0x01)
	{
		uint8_t cabIdx = dma_mem_r(0x4000);
		uint8_t cabCnt = dma_mem_r(0x4001);

		int frameStartTx = 0x4010;
		int frameStartRx = 0x4310;
		int frameSize;
		switch (cabCnt)
		{
			case 1:
				frameSize = 0x300;
				break;
			case 2:
				frameSize = 0x180;
				break;
			case 3:
				frameSize = 0x110;
				break;
			case 4:
				frameSize = 0x0c0;
				break;
			case 5:
			case 6:
				frameSize = 0x080;
				break;
			case 7:
			case 8:
				frameSize = 0x060;
				break;
			default:
				frameSize = 0x000;
				break;
		}
		int frameOffset = frameStartTx + (frameSize * (cabIdx - 1));

		int dataSize = frameSize + 1;
		int recv = 0;
		int idx = 0;

		bool isMaster = (cabIdx == 0x01);
		bool isSlave = (cabIdx > 0x01);
		bool isRelay = (cabIdx == 0x00);

		if (m_linkalive == 0x02)
		{
			// link failed... (guesswork)
			m_z80_stat = 0xff;
			return;
		}
		else if (m_linkalive == 0x00)
		{
			// link not yet established... (guesswork)
			m_z80_stat = 0x01;

			// check rx socket
			if (!m_line_rx)
			{
				osd_printf_verbose("XBDCOMM: listen on %s\n", m_localhost);
				uint64_t filesize; // unused
				osd_file::open(m_localhost, OPEN_FLAG_CREATE, m_line_rx, filesize);
			}

			// check tx socket
			if (!m_line_tx)
			{
				osd_printf_verbose("XBDCOMM: connect to %s\n", m_remotehost);
				uint64_t filesize; // unused
				osd_file::open(m_remotehost, 0, m_line_tx, filesize);
			}

			// if both sockets are there check ring
			if (m_line_rx && m_line_tx)
			{
				// try to read one message
				recv = read_frame(dataSize);
				while (recv > 0)
				{
					// check message id
					idx = m_buffer0[0];

					// 0xFF - link id
					if (idx == 0xff)
					{
						if (isMaster)
						{
							// master gets first id and starts next state
							m_linkid = 0x01;
							m_linkcount = m_buffer0[1];
							m_linktimer = 0x00;
						}
						else
						{
							// slave get own id, relay does nothing
							if (isSlave)
							{
								m_buffer0[1]++;
								m_linkid = m_buffer0[1];
							}

							// forward message to other nodes
							send_frame(dataSize);
						}
					}

					// 0xFE - link size
					else if (idx == 0xfe)
					{
						if (isSlave || isRelay)
						{
							m_linkcount = m_buffer0[1];

							// forward message to other nodes
							send_frame(dataSize);
						}

						// consider it done
						osd_printf_verbose("XBDCOMM: link established - id %02x of %02x\n", m_linkid, m_linkcount);
						m_linkalive = 0x01;

						// write to shared mem
						m_z80_stat = 0x00;
					}


					if (m_linkalive == 0x00)
						recv = read_frame(dataSize);
					else
						recv = 0;
				}

				// if we are master and link is not yet established
				if (isMaster && (m_linkalive == 0x00))
				{
					// send first packet
					if (m_linktimer == 0x01)
					{
						m_buffer0[0] = 0xff;
						m_buffer0[1] = 0x01;
						send_frame(dataSize);
					}

					// send second packet
					else if (m_linktimer == 0x00)
					{
						m_buffer0[0] = 0xfe;
						m_buffer0[1] = m_linkcount;
						send_frame(dataSize);

						// consider it done
						osd_printf_verbose("XBDCOMM: link established - id %02x of %02x\n", m_linkid, m_linkcount);
						m_linkalive = 0x01;

						// write to shared mem
						m_z80_stat = 0x00;
					}

					else if (m_linktimer > 0x01)
					{
						// decrease delay timer
						m_linktimer--;
					}
				}
			}
		}

		if (m_linkalive == 0x01)
		{
			// link established
			// try to read a message
			recv = read_frame(dataSize);
			while (recv > 0)
			{
				// check if valid id
				idx = m_buffer0[0];
				if (idx > 0 && idx <= m_linkcount)
				{
					// save message to "ring buffer"
					frameOffset = frameStartTx + ((idx - 1) * frameSize);
					for (int j = 0x00 ; j < frameSize ; j++)
					{
						dma_mem_w(frameOffset + j, m_buffer0[1 + j]);
					}

					// if not own message
					if (idx != cabIdx)
					{
						// forward message to other nodes
						send_frame(dataSize);
					}
					else
					{
						m_z80_stat = 0x00;
					}
				}

				// try to read another message
				recv = read_frame(dataSize);
			}

			// update buffers... guesswork
			for (int j = 0x00 ; j < 0x300 ; j++)
			{
				dma_mem_w(frameStartRx + j, dma_mem_r(frameStartTx + j));
			}


			// update "ring buffer" if link established
			// live relay does not send data
			if (cabIdx != 0x00)
			{
				// check ready-to-send flag
				if (m_xbd_stat & 0x01)
				{
					frameOffset = frameStartTx + ((cabIdx - 1) * frameSize);
					send_data(cabIdx, frameOffset, frameSize, dataSize);
					m_z80_stat = 0x01;
				}
			}

			// clear ready-to-send flag
			m_xbd_stat &= 0xFE;
		}
	}
}

int xbdcomm_device::read_frame(int dataSize)
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
				else if (!filerr && recv == 0)
				{
					togo = 0;
				}
			}
		}
	}
	else if (!filerr && recv == 0)
	{
		if (m_linkalive == 0x01)
		{
			osd_printf_verbose("XBDCOMM: rx connection lost\n");
			m_linkalive = 0x02;
			m_linktimer = 0x00;

			m_z80_stat = 0xff;

			m_line_rx.reset();
			m_line_tx.reset();
		}
	}
	return recv;
}

void xbdcomm_device::send_data(uint8_t frameType, int frameOffset, int frameSize, int dataSize)
{
	m_buffer0[0] = frameType;
	for (int i = 0x00 ; i < frameSize ; i++)
	{
		m_buffer0[1 + i] = dma_mem_r(frameOffset + i);
	}
	send_frame(dataSize);
}

void xbdcomm_device::send_frame(int dataSize){
	if (!m_line_tx)
		return;

	std::error_condition filerr;
	std::uint32_t written;

	filerr = m_line_tx->write(&m_buffer0, 0, dataSize, written);
	if (filerr)
	{
		if (m_linkalive == 0x01)
		{
			osd_printf_verbose("XBDCOMM: tx connection lost\n");
			m_linkalive = 0x02;
			m_linktimer = 0x00;

			m_z80_stat = 0xff;

			m_line_rx.reset();
			m_line_tx.reset();
		}
	}
}