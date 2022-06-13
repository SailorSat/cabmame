// license:BSD-3-Clause
// copyright-holders:Ariane Fugmann

/*
EXPALITY VID 1000
W.Industries 1992

ToDo:
  everything

Notes:
  8x AM7203A       - 2Kx9 FIFO
  5x IDT7132-SA25J - IDT 2Kx8 SRAM / 25ns
  2x Bt473KPJ35    - Brooktree Triple 8-bit True-Color RAMDAC (35 MHz)

  OSC 27.0MHz

  SU2000: jumpered to I/O 0x340

  http://images.arianchen.de/virtuality/su2000_video1-01.jpg
  http://www.arianchen.de/su2000/details_vid1000.html
*/
#include "emu.h"
#include "vid1000.h"
#include "screen.h"

DEFINE_DEVICE_TYPE(ISA16_VID1000, vid1000_device, "vid1000", "EXPALITY VID 1000")

void vid1000_device::device_add_mconfig(machine_config &config)
{
	// 27MHz OSC - use fake default resolution of 720x576 50Hz
	SCREEN(config, m_screen_a, SCREEN_TYPE_RASTER);
	m_screen_a->set_raw(27_MHz_XTAL, 864, 0, 720, 625, 0, 576);
	m_screen_a->set_screen_update(FUNC(vid1000_device::screen_update_a));


	SCREEN(config, m_screen_b, SCREEN_TYPE_RASTER);
	m_screen_b->set_raw(27_MHz_XTAL, 864, 0, 720, 625, 0, 576);
	m_screen_b->set_screen_update(FUNC(vid1000_device::screen_update_b));
}

vid1000_device::vid1000_device(const machine_config& mconfig, const char* tag, device_t* owner, uint32_t clock)
	: device_t(mconfig, ISA16_VID1000, tag, owner, clock),
	device_isa16_card_interface(mconfig, *this),
	m_screen_a(*this, "vid1000_a"),
	m_screen_b(*this, "vid1000_b")
{
}

void vid1000_device::device_start()
{
	set_isa_device();
}

void vid1000_device::device_reset()
{
	map_io();
}

void vid1000_device::map_io()
{
	m_isa->install_device(0x0340, 0x034f, read8sm_delegate(*this, FUNC(vid1000_device::isa_port_r)), write8sm_delegate(*this, FUNC(vid1000_device::isa_port_w)));
}

uint8_t vid1000_device::isa_port_r(offs_t offset)
{
	logerror("vid1000: unhandled port read @ %02x\n", offset);
	return 0xff;
}

void vid1000_device::isa_port_w(offs_t offset, uint8_t data)
{
	logerror("vid1000: unhandled port write @ %02x, %02x\n", offset, data);
}

// dummy
uint32_t vid1000_device::screen_update_a(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	return 0;
}

uint32_t vid1000_device::screen_update_b(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	return 0;
}