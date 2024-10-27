// license:BSD-3-Clause
// copyright-holders:Fabio Priuli
/***************************************************************************

Konami IC 056230 (LANC)

Device Notes:
-The custom IC itself
-64k shared ram
-LS161 4-bit binary counter
-PAL(056787) for racinfrc's sub board and plygonet.cpp
-PAL(056787A) for zr107.cpp, gticlub.cpp and thunderh's I/O board
-HYC2485S RS485 transceiver

TODO: nearly everything

***************************************************************************/

#include "emu.h"
#include "k056230.h"
#include "emuopts.h"

#define LOG_REG_READS   (1 << 1U)
#define LOG_REG_WRITES  (1 << 2U)
#define LOG_RAM_READS   (1 << 3U)
#define LOG_RAM_WRITES  (1 << 4U)
#define LOG_UNKNOWNS    (1 << 5U)
#define LOG_ALL (LOG_REG_READS | LOG_REG_WRITES | LOG_RAM_READS | LOG_RAM_WRITES | LOG_UNKNOWNS)

#define VERBOSE (0)
#include "logmacro.h"

DEFINE_DEVICE_TYPE(K056230, k056230_device, "k056230", "K056230 LANC")


k056230_device::k056230_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
	: device_t(mconfig, K056230, tag, owner, clock)
	, m_ram(*this, "lanc_ram", 0x800U * 4, ENDIANNESS_BIG)
	, m_irq_cb(*this)
{
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

void k056230_device::device_start()
{
	m_irq_cb.resolve_safe();
	m_irq_state = CLEAR_LINE;

	save_item(NAME(m_irq_state));
	
	// todo save comm stuff
}

u8 k056230_device::regs_r(offs_t offset)
{
	u8 data = 0;

	switch (offset)
	{
		case 0:     // Status register
			data = 0x08 | m_status;
			LOGMASKED(LOG_REG_READS, "%s: regs_r: Status Register: %02x\n", machine().describe_context(), data);
			comm_tick();
			break;

		case 1:     // CRC Error register
			data = 0x00;
			LOGMASKED(LOG_REG_READS, "%s: regs_r: CRC Error Register: %02x\n", machine().describe_context(), data);
			break;

		default:
			LOGMASKED(LOG_REG_READS, "%s: regs_r: Unknown Register [%02x]: %02x\n", machine().describe_context(), offset, data);
			break;
	}

	return data;
}

void k056230_device::regs_w(offs_t offset, u8 data)
{
	switch (offset)
	{
		case 0:     // Mode register
			LOGMASKED(LOG_REG_WRITES, "%s: regs_w: Mode Register = %02x\n", machine().describe_context(), data);
			set_mode(data);
			break;

		case 1:     // Control register
		{
			LOGMASKED(LOG_REG_WRITES, "%s: regs_w: Control Register = %02x\n", machine().describe_context(), data);
			set_ctrl(data);
			// TODO: This is a literal translation of the previous device behaviour, and seems pretty likely to be incorrect.
			const int old_state = m_irq_state;
			if (BIT(data, 5))
			{
				LOGMASKED(LOG_REG_WRITES, "%s: regs_w: Asserting IRQ\n", machine().describe_context());
				m_irq_state = ASSERT_LINE;
			}
			if (!BIT(data, 0))
			{
				LOGMASKED(LOG_REG_WRITES, "%s: regs_w: Clearing IRQ\n", machine().describe_context());
				m_irq_state = CLEAR_LINE;
			}
			if (old_state != m_irq_state)
			{
				m_irq_cb(m_irq_state);
			}
			break;
		}

		case 2:     // Sub ID register
			LOGMASKED(LOG_REG_WRITES, "%s: regs_w: Sub ID Register = %02x\n", machine().describe_context(), data);
			break;

		default:
			LOGMASKED(LOG_REG_WRITES | LOG_UNKNOWNS, "%s: regs_w: Unknown Register [%02x] = %02x\n", machine().describe_context(), offset, data);
			break;
	}
}

u32 k056230_device::ram_r(offs_t offset, u32 mem_mask)
{
	const auto lanc_ram = util::big_endian_cast<const u32>(m_ram.target());
	u32 data = lanc_ram[offset & 0x7ff];
	LOGMASKED(LOG_RAM_READS, "%s: Network RAM read [%04x (%03x)]: %08x & %08x\n", machine().describe_context(), offset << 2, (offset & 0x7ff) << 2, data, mem_mask);
	return data;
}

void k056230_device::ram_w(offs_t offset, u32 data, u32 mem_mask)
{
	const auto lanc_ram = util::big_endian_cast<u32>(m_ram.target());
	LOGMASKED(LOG_RAM_WRITES, "%s: Network RAM write [%04x (%03x)] = %08x & %08x\n", machine().describe_context(), offset << 2, (offset & 0x7ff) << 2, data, mem_mask);
	COMBINE_DATA(&lanc_ram[offset & 0x7ff]);
}

void k056230_device::set_mode(u8 data)
{
	switch (data & 0xf0)
	{
		case 0x00:
		case 0x20:
			m_linkid = data & 0x0f;
			break;
	}
}

void k056230_device::set_ctrl(u8 data)
{
	
	m_linkenable = data && 0x10;

	switch (data)
	{
		case 0x10:
		case 0x18:
			// prepare for tx?
			break;

		case 0x14:
		case 0x1C:
			// prepare for rx? - clear bit5
			m_status = 0x00;
			break;

		case 0x36:
		case 0x3E:
			// plygonet, polynetw, gticlub = 36, midnrun 3E
			// tx mode?
			m_txmode = 0x01;
			comm_tick();
			break;

		case 0x37:
		case 0x3F:
			// rx mode? - set bit 5
			// plygonet, polynetw, gticlub = 37, midnrun 3F
			m_status = 0x20;
			m_txmode = 0x00;
			comm_tick();
			break;
	}
}

void k056230_device::comm_tick()
{
	if (m_linkenable == 0x01)
	{
		std::error_condition filerr;

		// check rx socket
		if (!m_line_rx)
		{
			osd_printf_verbose("k056230: listen on %s\n", m_localhost);
			uint64_t filesize; // unused
			filerr = osd_file::open(m_localhost, OPEN_FLAG_CREATE, m_line_rx, filesize);
			if (filerr.value() != 0)
			{
				osd_printf_verbose("k056230: rx connection failed\n");
				m_line_rx.reset();
			}
		}

		// check tx socket
		if ((!m_line_tx) && (m_txmode == 0x01))
		{
			osd_printf_verbose("k056230: connect to %s\n", m_remotehost);
			uint64_t filesize; // unused
			filerr = osd_file::open(m_remotehost, 0, m_line_tx, filesize);
			if (filerr.value() != 0)
			{
				osd_printf_verbose("k056230: tx connection failed\n");
				m_line_tx.reset();
			}
		}

		// if both sockets are there check ring
		if (m_line_rx && m_line_tx)
		{
			int frameStart = 0x0000;
			int frameSize = 0x0200;
			int dataSize = 0x201;

			int frameOffset = 0;
			u32 frameData = 0;

			int recv = 0;
			int idx = 0;

			if (m_txmode == 0x00)
			{
				// try to read one message
				recv = read_frame(dataSize);
				while (recv > 0)
				{
					// check if valid id
					idx = m_buffer0[0];
					if (idx <= 0x0E)
					{
						if (idx != m_linkid)
						{
							// save message to ram
							frameStart = (idx & 0x07) * 0x0100;
							frameSize = (idx & 0x08) ? 0x0200 : 0x0100;
							for (int j = 0x00 ; j < frameSize ; j += 4)
							{
								frameOffset = (frameStart + j) / 4;
								frameData = m_buffer0[4 + j] |  m_buffer0[3 + j] << 8 |  m_buffer0[2 + j] << 16 |  m_buffer0[1 + j] << 24;
								ram_w(frameOffset, frameData, 0xffffffff);
							}

							// forward message to other nodes
							send_frame(dataSize);
						}
					}

					// try to read another message
					recv = read_frame(dataSize);
				}
			}
			else if (m_txmode == 0x01)
			{
				// send local data to network (once)
				frameStart = (m_linkid & 0x07) * 0x100;
				frameSize = (m_linkid & 0x08) ? 0x200 : 0x100;

				frameOffset = 0;
				frameData = 0;

				m_buffer0[0] = m_linkid;
				for (int i = 0x00 ; i < frameSize ; i += 4)
				{
					frameOffset = (frameStart + i) / 4;
					frameData = ram_r(frameOffset, 0xffffffff);
					m_buffer0[4 + i] = frameData & 0xff;
					m_buffer0[3 + i] = (frameData >> 8) & 0xff;
					m_buffer0[2 + i] = (frameData >> 16) & 0xff;
					m_buffer0[1 + i] = (frameData >> 24) & 0xff;
				}

				send_frame(dataSize);
				m_txmode = 0x02;
			}
		}
	}
}

int k056230_device::read_frame(int dataSize)
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
		osd_printf_verbose("k056230: rx connection error\n");
		m_line_rx.reset();
	}
	return recv;
}

void k056230_device::send_frame(int dataSize)
{
	if (!m_line_tx)
		return;

	std::uint32_t written;
	std::error_condition filerr = m_line_tx->write(&m_buffer0, 0, dataSize, written);
	if (filerr)
	{
		osd_printf_verbose("k056230: tx connection error %08x %s\n");
		m_line_tx.reset();
	}
}
