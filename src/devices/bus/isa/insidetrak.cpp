// license:BSD-3-Clause
// copyright-holders:Ariane Fugmann

/*
Polhemus InsideTrak

Todo:
  add fifo logic
  add tms32031 dma, timers, serial etc.
  figure out tms32031 io 010000 area
  add irqs (not used by su2000/3000)
  hook to model1 for netmerc

Notes:
  1x TMS320C31PQL  - DSP
  4x MT5C2568DJ-25 - MICRON SRAM 32Kx8 / 25ns
  3x MS7201AL-80FC - MOSEL FIFO 512x9 / 80ns
  
  OSC1 33.3MHz
  OSC2 10.0MHz

  SU2000/SU3000: primary card jumpered to I/O 0x270
                 secondary card jumpered to I/O 0x278

  http://images.arianchen.de/virtuality/su2000_track1-01.jpg
  http://www.arianchen.de/su2000/details_tracker.html
*/
#include "emu.h"
#include "insidetrak.h"

ROM_START( insidetrak )
	ROM_REGION32_LE(0x040000, "tracker", 0)

	ROM_DEFAULT_BIOS("151.03")

	ROM_SYSTEM_BIOS( 0, "151.01", "Ver 151.01" )
	ROMX_LOAD( "ver_151_01_u16.u16", 0x00000, 0x20000, CRC(309180dd) SHA1(e32eafd8ad1b9e5d11027ab60b471bf97509f0e1), ROM_GROUPWORD | ROM_SKIP(2) | ROM_BIOS(0) )
	ROMX_LOAD( "ver_151_01_u17.u17", 0x00002, 0x20000, CRC(186bfd55) SHA1(900645c8bdfe7ece07ec8515863f23aba81a3187), ROM_GROUPWORD | ROM_SKIP(2) | ROM_BIOS(0) )

	ROM_SYSTEM_BIOS( 1, "151.02", "Ver 151.02" )
	ROMX_LOAD( "ver_151_02_u16.u16", 0x00000, 0x20000, CRC(869d4d41) SHA1(f66698ef872b3a4b903b2c8acbfec721b7191e94), ROM_GROUPWORD | ROM_SKIP(2) | ROM_BIOS(1) )
	ROMX_LOAD( "ver_151_02_u17.u17", 0x00002, 0x20000, CRC(053535b3) SHA1(7ca368854589fe75de00847f76c2468beee7e443), ROM_GROUPWORD | ROM_SKIP(2) | ROM_BIOS(1) )

	ROM_SYSTEM_BIOS( 2, "151.03", "Ver 151.03" )
	ROMX_LOAD( "ver_151_03_u16.u16", 0x00000, 0x20000, CRC(8354d059) SHA1(a88df7cc259c1c39316cc3bff9e08aa4e8d3d2c0), ROM_GROUPWORD | ROM_SKIP(2) | ROM_BIOS(2) )
	ROMX_LOAD( "ver_151_03_u17.u17", 0x00002, 0x20000, CRC(ace4081d) SHA1(f57287ded53f8d127bcdc9e34b8adb356fe55e5e), ROM_GROUPWORD | ROM_SKIP(2) | ROM_BIOS(2) )

	ROM_SYSTEM_BIOS( 3, "151.07", "Ver 151.07" )
	ROMX_LOAD( "ver_151_07_u16.u16", 0x00000, 0x20000, CRC(a4e96105) SHA1(955d22a9237eb7154c25f0969166d57ff1042be6), ROM_GROUPWORD | ROM_SKIP(2) | ROM_BIOS(3) )
	ROMX_LOAD( "ver_151_07_u17.u17", 0x00002, 0x20000, CRC(b7201df9) SHA1(8ec0f19de4d26bcd83bfd245387ade0471406472), ROM_GROUPWORD | ROM_SKIP(2) | ROM_BIOS(3) )
ROM_END

DEFINE_DEVICE_TYPE(ISA16_INSIDETRAK, insidetrak_device, "insidetrak", "Polhemus InsideTRAK")

void insidetrak_device::cpu_map(address_map &map)
{
	map(0x000000, 0x00ffff).rom().region("tracker", 0);
	map(0x010001, 0x010001).rw(FUNC(insidetrak_device::tms32031_010001_r), FUNC(insidetrak_device::tms32031_010001_w));
	map(0x020000, 0x027fff).ram();
	map(0x808000, 0x8080ff).rw(FUNC(insidetrak_device::tms32031_io_r), FUNC(insidetrak_device::tms32031_io_w));
}

void insidetrak_device::device_add_mconfig(machine_config &config)
{
	TMS32031(config, m_tms32031, INSIDETRAK_CLOCK1);
	m_tms32031->set_addrmap(AS_PROGRAM, &insidetrak_device::cpu_map);
}

insidetrak_device::insidetrak_device(const machine_config& mconfig, const char* tag, device_t* owner, uint32_t clock)
	: device_t(mconfig, ISA16_INSIDETRAK, tag, owner, clock),
	device_isa16_card_interface(mconfig, *this),
	m_tms32031(*this, "tms32031")
{
}

void insidetrak_device::device_start()
{
	set_isa_device();
}


void insidetrak_device::device_reset()
{
	map_io();

	m_010001 = 0 ;
}

void insidetrak_device::map_io()
{
	uint8_t io_base = ioport("INSIDETRAK_IO_BASE")->read() & 0x1;
	switch (io_base) {
		case 0:
			m_isa->install16_device(0x0270, 0x0277, read16s_delegate(*this, FUNC(insidetrak_device::isa_port_r)), write16s_delegate(*this, FUNC(insidetrak_device::isa_port_w)));
			break;
		case 1:
			m_isa->install16_device(0x0278, 0x027f, read16s_delegate(*this, FUNC(insidetrak_device::isa_port_r)), write16s_delegate(*this, FUNC(insidetrak_device::isa_port_w)));
			break;
	}
}

uint16_t insidetrak_device::isa_port_r(offs_t offset, uint16_t mem_mask) {
	if ((mem_mask == 0xffff) && (offset == 0)) {
		// 16bit read @ 0 = read fifo
		logerror("insidetrak: isa port word read %04X @ %04X\n", offset, mem_mask);
		return 0xffff;
	}
	if ((mem_mask == 0xff00) && (offset == 0)) {
		// 8bit read @ 1 = read status
		logerror("insidetrak: isa port byte read %04X @ %04X\n", offset, mem_mask);
		return 0xFE00;
	}
	logerror("insidetrak: unhandled isa port read %04X @ %04X\n", offset, mem_mask);
	return 0xffff;
}

void insidetrak_device::isa_port_w(offs_t offset, uint16_t data, uint16_t mem_mask) {
	if (mem_mask == 0xffff) {
		logerror("insidetrak: unhandled isa port word write %04X @ %04X, %04X\n", offset, mem_mask, data);
		return;
	}
	if ((mem_mask == 0x00ff) && (offset == 0)) {
		// 8bit write @ 0 = write fifo
		logerror("insidetrak: isa port byte write %04X @ %04X, %04X\n", offset, mem_mask, data);
		return;
	}
	if ((mem_mask == 0xff00) && (offset == 0)) {
		// 8bit write @ 1 = force rcvr1 update
		logerror("insidetrak: isa port byte write %04X @ %04X, %04X\n", offset, mem_mask, data);
		return;
	}
	if ((mem_mask == 0x00ff) && (offset == 1)) {
		// 8bit write @ 2 = force rcvr2 update
		logerror("insidetrak: isa port byte write %04X @ %04X, %04X\n", offset, mem_mask, data);
		return;
	}
	logerror("insidetrak: unhandled isa port byte write %04X @ %04X, %04X\n", offset, mem_mask, data);
	return;
}

uint32_t insidetrak_device::tms32031_010001_r(offs_t offset)
{
	logerror("insidetrak: unhandled tms32031 010001 read %04x\n", offset);
	return m_010001;
}

void insidetrak_device::tms32031_010001_w(offs_t offset, uint32_t data, uint32_t mem_mask)
{
	logerror("insidetrak: unhandled tms32031 010001 write %08X, %08X\n", offset, data);
	if (data == 0x31) m_010001 = 0x0020; // prevent tms32031 from eating up all cpu cycles (for now);
	return;
}

uint32_t insidetrak_device::tms32031_io_r(offs_t offset)
{
	logerror("insidetrak: unhandled tms32031 io read %04x\n", offset);
	if (offset == 0x30) return 0x0008; // prevent tms32031 from eating up all cpu cycles (for now);
	return 0;
}

void insidetrak_device::tms32031_io_w(offs_t offset, uint32_t data, uint32_t mem_mask)
{
	logerror("insidetrak: unhandled tms32031 io write %08X, %08X\n", offset, data);
	return;
}

static INPUT_PORTS_START( insidetrak )
	PORT_START("INSIDETRAK_IO_BASE")
	PORT_CONFNAME(0x01, 0x00, "InsideTRAK I/O address")
	PORT_CONFSETTING( 0x00, "270")
	PORT_CONFSETTING( 0x01, "278")
INPUT_PORTS_END

ioport_constructor insidetrak_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(insidetrak);
}

const tiny_rom_entry *insidetrak_device::device_rom_region() const
{
	return ROM_NAME( insidetrak );
}