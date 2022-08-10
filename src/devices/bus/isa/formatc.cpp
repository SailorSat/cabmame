// license:BSD-3-Clause
// copyright-holders:Ariane Fugmann

/*
FORMAT C 20-177-21 Rev C/D
Virtuality Enterntainment 1993

To-Do:
  ODIE registers
    MITSUMI CD-ROM interface
    SoundScape S-1000
  actual 80196/80198 emulation (for CTRL stuff)
  make ODIE a standalone device?

Notes:
  1x MC68EC000FN8   - Motorola 68000
  1x ES5506         - Ensoniq "OTTO" Wavetable Synth
  1x ES5706         - Ensoniq "ODIE" Host Interface
  1x CS4508-KL      - Crystal CS4508
  1x SIMM-4MB       - 30pin SIMM module (4MB)
  1x TC511664BJ-80  - Toshiba 64Kx16 DRAM / 80ns
  1x YMF262-M       - Yamaha OPL3

  1x N80C198        - Intel MCS-96 (8-bit, ROMless)
  1x IDT7132-SA100J - IDE 2Kx8 SRAM / 100ns
  2x N82C54-2       - Intel Programmable Timer
  1x M27C256B-12XF1 - 32Kx8 EPROM / 120ns

  SU2000: primary card jumpered to no IRQ, I/O 0x210, I/O 330, MEM 0xE0000
          secondary card jumpered to no IRQ, I/O 0x218, I/O 350, MEM 0xE0800
          seems to be hooked to DMA 1 and DMA 3

  attached drives seen: CRMC-LU005S, CRMC-FX001D

  Rev C: http://images.arianchen.de/virtuality/su2000_format1-01.jpg
  Rev D: http://images.arianchen.de/virtuality/su2000_format2-01.jpg
  http://www.arianchen.de/su2000/details_format-c.html

  https://elixir.bootlin.com/linux/1.3.100/source/drivers/cdrom/mcd.c
*/
#include "emu.h"
#include "formatc.h"

#include "speaker.h"

/*************************************
 *
 *  internal register definitions
 *
 *************************************/

#define MIDI_IF_EMU_CTRL_STAT   0x00
#define MIDI_IF_EMU_DATA        0x01
#define HOST_IF_CTRL_STAT       0x02
#define HOST_IF_DATA            0x03
#define MIDI_IF_CTRL_STAT       0x04
#define MIDI_IF_DATA            0x05
#define INT_CTRL                0x06
#define INT_STAT_OPL2_STAT      0x07
#define DMAA_CTRL               0x08
#define DMAA_TRIG_STAT          0x09
#define DMAA_ADDR_2             0x0a
#define DMAA_ADDR_3             0x0b
#define DMAA_ADDR_0             0x0c
#define DMAA_ADDR_1             0x0d
#define DMAB_CTRL               0x0e
#define DMAB_TRIG_STAT          0x0f
#define DMAB_ADDR_2             0x10
#define DMAB_ADDR_3             0x11
#define DMAB_ADDR_0             0x12
#define DMAB_ADDR_1             0x13
#define MIDI_CLOCK_COUNT_0      0x14
#define MIDI_CLOCK_COUNT_1      0x15
#define PWM0_COUNT              0x16
#define PWM1_COUNT              0x17
#define PWM2_COUNT              0x18
#define PWM3_COUNT              0x19
#define AD_CTRL                 0x1a
#define MISC_CTRL               0x1b
#define OBP_MISC_STAT           0x1c
#define TEST_MODE_CTRL_STAT     0x1d
#define SB_PCM_CTRL_STAT        0x1e
#define SB_PCM_DATA             0x1f

#define HOST_INT_STAT           0x20
#define HOST_INT_ENAB           0x21
#define HOST_DMAA_TRIG_STAT     0x22
#define HOST_DMAB_TRIG_STAT     0x23
#define HOST_INT_IF_CONF        0x24
#define HOST_DMA_IF_CONF        0x25
#define HOST_CDR_IF_CONF        0x26
#define MEM_CONFIG_A            0x27
#define MEM_CONFIG_B            0x28
#define HOST_MASTER_CTRL        0x29
#define OPL2_EMU_ADDR           0x2a
#define OPL2_EMU_DATA           0x2b
#define OPL2_EMU_STAT           0x2c

static INPUT_PORTS_START( formatc )
	PORT_START("FORMATC_IO_ODIE")
	PORT_CONFNAME(0x01, 0x00, "FORMAT-C ODIE I/O")
	PORT_CONFSETTING( 0x00, "330")
	PORT_CONFSETTING( 0x01, "350")

	PORT_START("FORMATC_IRQ_ODIE")
	PORT_CONFNAME(0x01, 0x00, "FORMAT-C ODIE IRQ")
	PORT_CONFSETTING( 0x00, "IRQ 12")
	PORT_CONFSETTING( 0x01, "IRQ 15")

	PORT_START("FORMATC_IO_CTRL")
	PORT_CONFNAME(0x01, 0x00, "FORMAT-C CTRL I/O")
	PORT_CONFSETTING( 0x00, "210")
	PORT_CONFSETTING( 0x01, "218")

	PORT_START("FORMATC_MEM_CTRL")
	PORT_CONFNAME(0x01, 0x00, "FORMAT-C CTRL MEM")
	PORT_CONFSETTING( 0x00, "e0000")
	PORT_CONFSETTING( 0x01, "e0800")
INPUT_PORTS_END

ROM_START( formatc )
	ROM_REGION(0x8000,"eprom", 0)

	ROM_DEFAULT_BIOS("v212")

	ROM_SYSTEM_BIOS( 0, "v212", "V2.12" )
	ROMX_LOAD( "wfc062_212.u62", 0x0000, 0x8000, CRC(9a6b553a) SHA1(7045f733446866ee3171e175e1b22d9384fda1b5), ROM_BIOS(0) )

	ROM_SYSTEM_BIOS( 1, "v300", "V3.00" )
	ROMX_LOAD( "wfc062_300.u62", 0x0000, 0x8000, CRC(843e0877) SHA1(612de2fbe58ca87cf28f32d526c23311d5f7c320), ROM_BIOS(1) )
ROM_END

ioport_constructor formatc_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(formatc);
}

const tiny_rom_entry *formatc_device::device_rom_region() const
{
	return ROM_NAME( formatc );
}

DEFINE_DEVICE_TYPE(ISA16_FORMATC, formatc_device, "formatc", "Virtuality FORMAT C")

void formatc_device::device_add_mconfig(machine_config &config)
{
	M68000(config, m_m68000, FMTC_SSCAPE_CLOCK1 / 4);
	m_m68000->set_addrmap(AS_PROGRAM, &formatc_device::sscape_map);

	SPEAKER(config, "lspeaker").front_left();
	SPEAKER(config, "rspeaker").front_right();

	ES5506(config, m_es5506, FMTC_SSCAPE_CLOCK1 / 2);
	m_es5506->set_region0("es5506.0");
	m_es5506->set_region1("es5506.1");
	m_es5506->set_region2("es5506.2");
	m_es5506->set_region3("es5506.3");
	m_es5506->set_channels(1);
	m_es5506->add_route(0, "lspeaker", 0.1);
	m_es5506->add_route(1, "rspeaker", 0.1);

	MCD2(config, m_mcd, 0);

	P8098(config, m_i80198, FMTC_CONTROL_CLOCK);
	m_i80198->set_addrmap(AS_PROGRAM, &formatc_device::ctrl_map);
}

formatc_device::formatc_device(const machine_config& mconfig, const char* tag, device_t* owner, uint32_t clock)
	: device_t(mconfig, ISA16_FORMATC, tag, owner, clock),
	device_isa16_card_interface(mconfig, *this),
	m_m68000(*this, "m68000"),
	m_es5506(*this, "es5506"),
	m_mcd(*this, "mcd"),
	m_i80198(*this, "i80198")
{
}

void formatc_device::device_start()
{
	set_isa_device();
}

void formatc_device::device_reset()
{
	map_io();
	map_ram();
	map_dma();

	m_odie_page = 0;
	memset(m_odie_regs, 0, 0x30);

	m_isa->drq1_w(CLEAR_LINE);
	m_isa->drq3_w(CLEAR_LINE);

	m_odie_dma_channel[0] = 0;
	m_odie_dma_channel[1] = 0;

	m_odie_dma_address[0] = 0;
	m_odie_dma_address[1] = 0;
}

void formatc_device::device_reset_after_children()
{
	m_m68000->set_input_line(INPUT_LINE_RESET, ASSERT_LINE);
	m_i80198->set_input_line(INPUT_LINE_RESET, ASSERT_LINE); // keep in reset for now...
}

/*************************************************************
 *
 * ISA BUS
 *
 *************************************************************/
 
void formatc_device::map_io()
{
	// soundscape part
	uint8_t io_odie = ioport("FORMATC_IO_ODIE")->read() & 0x1;
	switch (io_odie) {
		case 0:
			m_isa->install_device(0x0330, 0x033f, read8sm_delegate(*this, FUNC(formatc_device::isa_odie_r)), write8sm_delegate(*this, FUNC(formatc_device::isa_odie_w)));
			break;
		case 1:
			m_isa->install_device(0x0350, 0x035f, read8sm_delegate(*this, FUNC(formatc_device::isa_odie_r)), write8sm_delegate(*this, FUNC(formatc_device::isa_odie_w)));
			break;
	}

	// control part
	uint8_t io_ctrl = ioport("FORMATC_IO_CTRL")->read() & 0x1;
	switch (io_ctrl) {
		case 0:
			m_isa->install_device(0x0210, 0x0217, read8sm_delegate(*this, FUNC(formatc_device::isa_ctrl_r)), write8sm_delegate(*this, FUNC(formatc_device::isa_ctrl_w)));
			break;
		case 1:
			m_isa->install_device(0x0218, 0x021f, read8sm_delegate(*this, FUNC(formatc_device::isa_ctrl_r)), write8sm_delegate(*this, FUNC(formatc_device::isa_ctrl_w)));
			break;
	}
}

void formatc_device::map_ram()
{
	uint8_t mem_base = ioport("FORMATC_MEM_CTRL")->read() & 0x1;
	switch (mem_base) {
		case 0:
			m_isa->install_memory(0xe0000, 0xe07ff, read8sm_delegate(*this, FUNC(formatc_device::isa_mem_r)), write8sm_delegate(*this, FUNC(formatc_device::isa_mem_w)));
			break;
		case 1:
			m_isa->install_memory(0xe0800, 0xe0fff, read8sm_delegate(*this, FUNC(formatc_device::isa_mem_r)), write8sm_delegate(*this, FUNC(formatc_device::isa_mem_w)));
			break;
	}
}

void formatc_device::map_dma()
{
	m_isa->set_dma_channel(1, this, true);
	m_isa->set_dma_channel(3, this, true);
}

uint8_t formatc_device::dack_r(int line)
{
	// todo
	logerror("formatc: ISA - unhandled DMA read @ %02x\n", line);
	return 0xff;
}

void formatc_device::dack_w(int line, uint8_t data)
{
	// todo
	int which = -1;

	if (m_odie_dma_channel[0] == line)
		which = 0;
	if (m_odie_dma_channel[1] == line)
		which = 1;

	if (which == -1) {
		logerror("formatc: ISA - unhandled DMA %u write : %02x\n", line, data);
		return;
	}

	if (m_eop == 1) {
		m_odie_regs[HOST_DMAA_TRIG_STAT + which] |= 0x01;
		switch (line) {
			case 0:
				// should not happen on 8-bit dma?
				m_isa->drq0_w(CLEAR_LINE);
				break;
			case 1:
				m_isa->drq1_w(CLEAR_LINE);
				break;
			case 3:
				m_isa->drq3_w(CLEAR_LINE);
				break;
			case 5:
				// should not happen on 8-bit dma?
				m_isa->drq5_w(CLEAR_LINE);
				break;
			case 6:
				// should not happen on 8-bit dma?
				m_isa->drq6_w(CLEAR_LINE);
				break;
			case 7:
				// should not happen on 8-bit dma?
				m_isa->drq7_w(CLEAR_LINE);
				break;
		}
	}

	// TODO - write data to actual sscape memory
	logerror("formatc: ISA - handled DMA %u write @ %06x : %02x\n", line, m_odie_dma_address[which], data);
	m_odie_dma_address[which]++;
}

void formatc_device::eop_w(int state)
{
	m_eop = state;
}

void formatc_device::drq_w(int state, int source)
{
	// todo
	// m_isa->drq1_w(state);
	// m_isa->drq3_w(state);
}

void formatc_device::irq_w(int state)
{
	uint8_t mem_base = ioport("FORMATC_IRQ_ODIE")->read() & 0x1;
	switch (mem_base) {
		case 0:
			m_isa->irq12_w(state);
			break;
		case 1:
			m_isa->irq15_w(state);
			break;
	}
}

/*************************************************************
 *
 * Ensoniq SoundScape S-1000
 *
 *************************************************************/
 
uint8_t formatc_device::isa_odie_r(offs_t offset)
{
	switch (offset) {
		case 0:
			// MIDI Interface Emulation Control/Status
			return odie_reg_r(MIDI_IF_EMU_CTRL_STAT);
		case 1:
			// MIDI Interface Emulation Data
			return odie_reg_r(MIDI_IF_EMU_DATA);
		case 2:
			// HOST Interface Control/Status
			return odie_reg_r(HOST_IF_CTRL_STAT);
		case 3:
			// HOST Interface Data
			return odie_reg_r(HOST_IF_DATA);
		case 4:
			// ODIE internal address
			return m_odie_page;
		case 5:
			// ODIE internal data
			return odie_reg_r(0x20 | m_odie_page);
	}

	// CD-ROM Interface Mode-0 - +6 to +7
	// CD-ROM Interface Mode-1 - +8 to +15
	// CD-ROM Interface Mode-2 - +16 to +31
	// CD-ROM Interface Mode-3 - +16 to +47
	if ((m_odie_cd_mode == 1) && (offset & 8)) {
		return mcd_r(offset & 3);
	}

	logerror("formatc: ISA - unhandled ODIE read @ %02x\n", offset);
	return 0xff;
}

void formatc_device::isa_odie_w(offs_t offset, uint8_t data)
{
	switch (offset) {
		case 0x0:
			// MIDI Interface Emulation Control/Status
			odie_reg_w(MIDI_IF_EMU_CTRL_STAT, data);
			return;
		case 0x1:
			// MIDI Interface Emulation Data
			odie_reg_w(MIDI_IF_EMU_DATA, data);
			return;
		case 0x2:
			// HOST Interface Control/Status
			odie_reg_w(HOST_IF_CTRL_STAT, data);
			return;
		case 0x3:
			// HOST Interface Data
			odie_reg_w(HOST_IF_DATA, data);
			return;
		case 0x4:
			// ODIE internal address
			m_odie_page = data & 0xf;
			return;
		case 0x5:
			// ODIE internal data
			odie_reg_w(0x20 | m_odie_page, data);
			return;
	}

	// CD-ROM Interface Mode-0 - +6 to +7
	// CD-ROM Interface Mode-1 - +8 to +15
	// CD-ROM Interface Mode-2 - +16 to +31
	// CD-ROM Interface Mode-3 - +16 to +47
	if ((m_odie_cd_mode == 1) && (offset & 8)) {
		mcd_w(offset & 3, data);
		return;
	}

	logerror("formatc: ISA - unhandled ODIE write @ %02x, %02x\n", offset, data);
}

void formatc_device::sscape_map(address_map &map)
{
	map(0x000000, 0x3fffff).ram(); // RAM0
	map(0x400000, 0x7fffff).ram(); // BANK* - selectable
	map(0x800000, 0x9fffff).rw(m_es5506, FUNC(es5506_device::read), FUNC(es5506_device::write)); // OTTO
	map(0xa00000, 0xbfffff).rw(FUNC(formatc_device::sscape_odie_r), FUNC(formatc_device::sscape_odie_w)); // ODIE
	map(0xc00000, 0xffffff).ram(); // RAM* - selectable
}

uint8_t formatc_device::sscape_otto_r(offs_t offset)
{
	logerror("formatc: 68K - unhandled OTTO read @ %02x\n", offset);
	return 0xff;
}

void formatc_device::sscape_otto_w(offs_t offset, uint8_t data)
{
	logerror("formatc: 68K - unhandled OTTO write @ %02x, %02x\n", offset, data);
}

uint8_t formatc_device::sscape_odie_r(offs_t offset)
{
	return odie_reg_r(offset);
}

void formatc_device::sscape_odie_w(offs_t offset, uint8_t data)
{
	odie_reg_w(offset, data);
}

uint8_t formatc_device::odie_reg_r(offs_t offset)
{
	uint8_t result = m_odie_regs[offset];

	switch (offset) {
		case OBP_MISC_STAT:
			result = m_odie_regs[MEM_CONFIG_A] & 0x7f;
			break;
		//case 0x24: // 

		case 0x2c:
		case 0x2d:
		case 0x2e:
		case 0x2f:
			// page 2c-2f invalid
			result = offset & 0xf;
			break;
	}

	logerror("formatc: ODIE register read %02x = %02x\n", offset, result);
	return result;
}

void formatc_device::odie_reg_w(offs_t offset, uint8_t data)
{
	uint8_t changed = m_odie_regs[offset] ^ data;
	m_odie_regs[offset] = data;

	switch (offset) {
		case OBP_MISC_STAT:
		case HOST_INT_STAT:
			// read only register
			return;

		case HOST_DMAA_TRIG_STAT:
		case HOST_DMAB_TRIG_STAT:
			if ((changed == 0x01) && ((data & 0x01) == 0))
				trigger_odie_dma(offset & 1);
			break;

		case HOST_MASTER_CTRL:
			update_odie_mode();
			break;
	}

	logerror("formatc: ODIE register write %02x = %02x\n", offset, data);
}

void formatc_device::trigger_odie_dma(int which)
{
	m_odie_dma_channel[which] = (m_odie_regs[HOST_DMAA_TRIG_STAT + which] >> 4) & 0x7;
	switch (m_odie_dma_channel[which]) {
		case 0:
			m_isa->drq0_w(ASSERT_LINE);
			break;
		case 1:
			m_isa->drq1_w(ASSERT_LINE);
			break;
		case 3:
			m_isa->drq3_w(ASSERT_LINE);
			break;
		case 5:
			m_isa->drq5_w(ASSERT_LINE);
			break;
		case 6:
			m_isa->drq6_w(ASSERT_LINE);
			break;
		case 7:
			m_isa->drq7_w(ASSERT_LINE);
			break;
		case 2:
		case 4:
			// dma disabled
			logerror("formatc: ODIE disabled dma got triggered.\n");
	}
}

void formatc_device::update_odie_dma(int which)
{
	//m_odie_dma_address[which] = (m_odie_regs[DMAA_ADDR_2 + (which * 6)] << 16) | (m_odie_regs[DMAA_ADDR_1 + (which * 6)] << 8) | (m_odie_regs[DMAA_ADDR_0 + (which * 6)]);
}

void formatc_device::update_odie_mode()
{
	m_odie_cd_mode = (m_odie_regs[HOST_MASTER_CTRL] >> 4) & 0x3;

	uint8_t mode = (m_odie_regs[HOST_MASTER_CTRL] >> 6) & 0x3;
	if (m_odie_obp_mode != mode) {
		m_odie_obp_mode = mode;
		switch (m_odie_obp_mode) {
			case 0:
				// subsystem reset
				//m_m68000->set_input_line(INPUT_LINE_RESET, ASSERT_LINE);
				break;
			case 1:
				// normal operation
				//m_m68000->set_input_line(INPUT_LINE_RESET, CLEAR_LINE);
				break;
			case 2:
				// code download mode
				//m_m68000->set_input_line(INPUT_LINE_RESET, ASSERT_LINE);
				m_odie_regs[HOST_DMAA_TRIG_STAT] |= 0x04;
				m_odie_regs[HOST_DMAB_TRIG_STAT] |= 0x04;
				m_odie_dma_address[0] = 0;
				m_odie_dma_address[1] = 0;
				break;
			case 3:
				// obp startup mode
				//m_m68000->set_input_line(INPUT_LINE_RESET, CLEAR_LINE);
				break;
		}
	}
}

/*************************************************************
 *
 * Format C/D "Control"
 *
 *************************************************************/

uint8_t formatc_device::isa_ctrl_r(offs_t offset)
{
	logerror("formatc: ISA - unhandled CTRL read @ %02x\n", offset);
	return 0xff;
}

// 5 - volume register
void formatc_device::isa_ctrl_w(offs_t offset, uint8_t data)
{
	if (offset == 0x05) map_ram(); // dirty hack to keep memory mapping active
	logerror("formatc: ISA - unhandled CTRL write @ %02x, %02x\n", offset, data);
}

uint8_t formatc_device::isa_mem_r(offs_t offset)
{
	return m_ctrl_ram[offset];
}

void formatc_device::isa_mem_w(offs_t offset, uint8_t data)
{
	m_ctrl_ram[offset] = data;
}

void formatc_device::ctrl_map(address_map &map)
{
 // TODO: banking, ram etc.
	// 0000 - 00ff are internal
	// 0800 - 0803 82c54 #1
	map(0x2000, 0x7fff).rom().region("eprom", 0x2000);
	map(0x8000, 0x87ff).rw(FUNC(formatc_device::ctrl_mem_r), FUNC(formatc_device::ctrl_mem_w));
}

uint8_t formatc_device::ctrl_mem_r(offs_t offset)
{
	return m_ctrl_ram[offset];
}

void formatc_device::ctrl_mem_w(offs_t offset, uint8_t data)
{
	m_ctrl_ram[offset] = data;
}

/*************************************************************
 *
 * Mitsumi CD-ROM
 *
 *************************************************************/

uint8_t formatc_device::mcd_r(offs_t offset)
{
	uint8_t result = 0xff;
	switch (offset) {
		case 0:
			result = m_mcd->data_r();
			break;
		case 1:
			result = m_mcd->flag_r();
			break;
	}
	logerror("formatc: MITSUMI read %02x = %02x\n", offset, result);
	return result;
}

void formatc_device::mcd_w(offs_t offset, uint8_t data)
{
	logerror("formatc: MITSUMI write %02x = %02x\n", offset, data);
	switch (offset) {
		case 0:
			m_mcd->cmd_w(data);
			break;
		case 3:
			m_mcd->reset_w(data);
			break;
	}
}