// license:BSD-3-Clause
// copyright-holders:Ariane Fugmann

/*
EXPALITY PIX 1000
Virtuality Entertainment 1993

ToDo:
  88110 emulation :(
  everything

Notes:
  2x  MC88110       - Motorola 88110 CPU
  16x TC528257SZ    - Toshiba 256Kx8 VRAM
  16x TC514400AZ    - Toshiba 1Mx4 DRAM
  2x AM7203A        - 2Kx9 FIFO

  OSC 40.0MHz

  SU2000: primary card jumpered to I/O 0x300, I/O 0x320, MEM D0000
          secondary card jumpered to I/O 0x3600, I/O 0x320, MEM D0000

  http://images.arianchen.de/virtuality/su2000_pix1-01.jpg
  http://www.arianchen.de/su2000/details_pix1000.html
*/
#include "emu.h"
#include "pix1000.h"


static INPUT_PORTS_START( pix1000 )
	PORT_START("PIX1000_IO_BASE")
	PORT_CONFNAME(0x01, 0x00, "PIX1000 I/O address")
	PORT_CONFSETTING( 0x00, "300")
	PORT_CONFSETTING( 0x01, "360")
INPUT_PORTS_END

ioport_constructor pix1000_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(pix1000);
}

DEFINE_DEVICE_TYPE(ISA16_PIX1000, pix1000_device, "pix1000", "EXPALITY PIX 1000")

void pix1000_device::device_add_mconfig(machine_config &config)
{
	//MC88110(config, m_m88110a, PIX_CLOCK);
	//m_m88110a->set_addrmap(AS_PROGRAM, &pix1000_device::m88110a_map);

	//MC88110(config, m_m88110b, PIX_CLOCK);
	//m_m88110b->set_addrmap(AS_PROGRAM, &pix1000_device::m88110b_map);
}

pix1000_device::pix1000_device(const machine_config& mconfig, const char* tag, device_t* owner, uint32_t clock)
	: device_t(mconfig, ISA16_PIX1000, tag, owner, clock),
	device_isa16_card_interface(mconfig, *this)
	//m_m88110a(*this, "m88110a"),
	//m_m88110a(*this, "m88110b")
{
}

void pix1000_device::device_start()
{
	set_isa_device();
}

void pix1000_device::device_reset()
{
	map_io();
	map_ram();
}

void pix1000_device::device_reset_after_children()
{
	//m_m88110a->set_input_line(INPUT_LINE_RESET, ASSERT_LINE);
	//m_m88110b->set_input_line(INPUT_LINE_RESET, ASSERT_LINE); // keep in reset for now...
}

/*************************************************************
 *
 * ISA BUS
 *
 *************************************************************/
 
void pix1000_device::map_io()
{
	uint8_t io_ctrl = ioport("PIX1000_IO_BASE")->read() & 0x1;
	switch (io_ctrl) {
		case 0:
			m_isa->install_device(0x0300, 0x0307, read8sm_delegate(*this, FUNC(pix1000_device::isa_proc_r)), write8sm_delegate(*this, FUNC(pix1000_device::isa_proc_w)));
			break;
		case 1:
			m_isa->install_device(0x0360, 0x0367, read8sm_delegate(*this, FUNC(pix1000_device::isa_proc_r)), write8sm_delegate(*this, FUNC(pix1000_device::isa_proc_w)));
			break;
	}

	m_isa->install16_device(0x0320, 0x0327, read16s_delegate(*this, FUNC(pix1000_device::isa_fifo_r)), write16s_delegate(*this, FUNC(pix1000_device::isa_fifo_w)));
}

void pix1000_device::map_ram()
{
	m_isa->install_memory(0xd0000, 0xdffff, read8sm_delegate(*this, FUNC(pix1000_device::isa_mem_r)), write8sm_delegate(*this, FUNC(pix1000_device::isa_mem_w)));
}

uint8_t pix1000_device::isa_proc_r(offs_t offset)
{
	logerror("pix1000: unhandled proc read @ %02x\n", offset);
	return 0xff;
}

void pix1000_device::isa_proc_w(offs_t offset, uint8_t data)
{
	if (offset == 0) map_ram(); // dirty hack to keep memory mapping active
	logerror("pix1000: unhandled proc write @ %02x, %02x\n", offset, data);
}

uint16_t pix1000_device::isa_fifo_r(offs_t offset, uint16_t mem_mask) {
	logerror("insidetrak: unhandled fifo read %04X:%04X\n", offset, mem_mask);
	return 0xffff;
}

void pix1000_device::isa_fifo_w(offs_t offset, uint16_t data, uint16_t mem_mask) {
	logerror("pix1000: unhandled fifo write %04X;%04X, %04X\n", offset, mem_mask, data);
}

uint8_t pix1000_device::isa_mem_r(offs_t offset)
{
	logerror("pix1000: unhandled mem read @ %02x\n", offset);
	return 0xff;
}

void pix1000_device::isa_mem_w(offs_t offset, uint8_t data)
{
	if (offset == 0) map_ram(); // dirty hack to keep memory mapping active
	logerror("pix1000: unhandled mem write @ %02x, %02x\n", offset, data);
}

void pix1000_device::m88110a_map(address_map &map)
{
	map(0x00000000, 0xffffffff).rw(FUNC(pix1000_device::m88110a_r), FUNC(pix1000_device::m88110a_w));
}

void pix1000_device::m88110b_map(address_map &map)
{
	map(0x00000000, 0xffffffff).rw(FUNC(pix1000_device::m88110b_r), FUNC(pix1000_device::m88110b_w));
}

uint32_t pix1000_device::m88110a_r(offs_t offset, uint32_t mem_mask)
{
	logerror("pix1000: unhandled mc88110a read %08x:%08x\n", offset, mem_mask);
	return 0;
}

void pix1000_device::m88110a_w(offs_t offset, uint32_t data, uint32_t mem_mask)
{
	logerror("pix1000: unhandled mc88110a write %08x:%08x, %08x\n", offset, mem_mask, data);
	return;
}

uint32_t pix1000_device::m88110b_r(offs_t offset, uint32_t mem_mask)
{
	logerror("pix1000: unhandled mc88110b read %08x:%08x\n", offset, mem_mask);
	return 0;
}

void pix1000_device::m88110b_w(offs_t offset, uint32_t data, uint32_t mem_mask)
{
	logerror("pix1000: unhandled mc88110b write %08x:%08x, %08x\n", offset, mem_mask, data);
	return;
}