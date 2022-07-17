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


static INPUT_PORTS_START( formatc )
	PORT_START("FORMATC_IO_ODIE")
	PORT_CONFNAME(0x01, 0x00, "FORMAT-C ODIE I/O")
	PORT_CONFSETTING( 0x00, "330")
	PORT_CONFSETTING( 0x01, "350")

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
	M68000(config, m_m68000, SOUNDSCAPE_CLOCK1);
	m_m68000->set_addrmap(AS_PROGRAM, &formatc_device::sscape_map);

	SPEAKER(config, "lspeaker").front_left();
	SPEAKER(config, "rspeaker").front_right();

	ES5506(config, m_es5506, SOUNDSCAPE_CLOCK1);
	m_es5506->set_region0("ensoniq.0");
	m_es5506->set_region1("ensoniq.1");
	m_es5506->set_region2("ensoniq.2");
	m_es5506->set_region3("ensoniq.3");
	m_es5506->set_channels(1);
	m_es5506->add_route(0, "lspeaker", 0.1);
	m_es5506->add_route(1, "rspeaker", 0.1);

	P8098(config, m_i80198, CONTROL_CLOCK);
	m_i80198->set_addrmap(AS_PROGRAM, &formatc_device::ctrl_map);
}

formatc_device::formatc_device(const machine_config& mconfig, const char* tag, device_t* owner, uint32_t clock)
	: device_t(mconfig, ISA16_FORMATC, tag, owner, clock),
	device_isa16_card_interface(mconfig, *this),
	m_m68000(*this, "soundcpu"),
	m_es5506(*this, "ensoniq"),
	m_i80198(*this, "ctrlcpu")
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

	m_odie_page = 0;
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

/*************************************************************
 *
 * Ensoniq SoundScape S-1000
 *
 *************************************************************/
 
/*
ISA ODIE REGISTERS

+0 - MIDI Interface Emulation Control/Status
+1 - MIDI Interface Emulation Data
+2 - HOST Interface Control/Status
+3 - HOST Interface Data
+4 - ODIE Address
+5 - ODIE Data
*/
uint8_t formatc_device::isa_odie_r(offs_t offset)
{
	switch (offset) {
		case 4:
			// ODIE Address
			return m_odie_page;
		case 5:
			// ODIE Data
			return (m_odie_page < 0xc) ? 0 : m_odie_page; // page c-f invalid
	}
	logerror("formatc: ISA - unhandled ODIE read @ %02x\n", offset);
	return 0xff;
}

void formatc_device::isa_odie_w(offs_t offset, uint8_t data)
{
	switch (offset) {
		case 4:
			m_odie_page = data & 0xf;
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

/*
ODIE 68K REGISTERS
A00000 - MIDI Interface Emulation Control/Status
A00001 - MIDI Interface Emulation Data
A00002 - HOST Interface Control/Status
A00003 - HOST Interface Data
A00004 - MIDI Interface Control/Status
A00005 - MIDI Interface Data
A00006 - Interrupt Control
A00007 - Interrupt Status/YM-3812*
A00008 - DMA-A Control
A00009 - DMA-A Status
A0000A\
A0000B - DMA A Address 16-22
A0000C\
A0000D - DMA A Address 0-15
A0000E - DMA-B Control
A0000F - DMA-B Status
A00010\
A00011 - DMA B Address 16-22
A00012\
A00013 - DMA B Address 0-15
A00014\
A00015 - MIDI Clock Count
...
*/
uint8_t formatc_device::sscape_odie_r(offs_t offset)
{
	logerror("formatc: 68K - unhandled ODIE read @ %02x\n", offset);
	return 0xff;
}

void formatc_device::sscape_odie_w(offs_t offset, uint8_t data)
{
	logerror("formatc: 68K - unhandled ODIE write @ %02x, %02x\n", offset, data);
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