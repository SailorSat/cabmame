// license:BSD-3-Clause
// copyright-holders:Ariane Fugmann

/*
COMPEX ReadyLINK ENET16/V (REV B)

ToDo:
  make actual 16bit device
  implement jumpers
  check various drivers for different modes of operation

Notes:
  1x EN90295-179  - custom compex asic
  1x DP83901AV    - Serial Network Interface Controller (SNIC)
  2x HY6264ALJ-70 - HYUNDAI SRAM 8Kx8 / 70ns
  1x DM74S288AN   - 256-Bit TTL PROM (32 x 8)

  SU2000: jumpered to IRQ 5, I/O 0x280, MEM 0xC8000, "WDPLUS" mode
  SU3000: jumpered to IRQ 3, I/O 0x280, MEM 0xCC000, "WDPLUS" mode
  third machine has SMC UltraChip 83C790QF in place

  MAC 00:80:48:86:f6:21 (03:97) - last 2 bytes are type and checksum

  http://images.arianchen.de/virtuality/su2000_net1-01.jpg
  http://www.arianchen.de/su2000/details_network.html

  https://elixir.bootlin.com/linux/1.3.100/source/drivers/net/wd.c
  https://unix.superglobalmegacorp.com/NetBSD-0.9/newsrc/arch/i386/netboot/wd80x3.c.html
*/
#include "emu.h"
#include "enet16.h"


DEFINE_DEVICE_TYPE(ISA16_ENET16, enet16_device, "enet16", "COMPEX ReadyLINK ENET16 Network Adapter")

void enet16_device::device_add_mconfig(machine_config &config)
{
	DP8390D(config, m_dp8390, 0);
	m_dp8390->irq_callback().set(FUNC(enet16_device::snic_irq_w));
	m_dp8390->mem_read_callback().set(FUNC(enet16_device::snic_mem_r));
	m_dp8390->mem_write_callback().set(FUNC(enet16_device::snic_mem_w));
}

enet16_device::enet16_device(const machine_config& mconfig, const char* tag, device_t* owner, uint32_t clock)
	: device_t(mconfig, ISA16_ENET16, tag, owner, clock),
	device_isa16_card_interface(mconfig, *this),
	m_dp8390(*this, "dp8390d"),
	m_irq(0)
{
}

void enet16_device::device_start()
{
	char mac[7];
	uint32_t num = machine().rand();
	memset(m_prom, 0, 32);
	sprintf(mac+1, "\x80\x48%c%c%c", (num >> 16) & 0xff, (num >> 8) & 0xff, num & 0xff);
	mac[0] = 0;  // avoid gcc warning
	memcpy(m_prom, mac, 6);
	m_dp8390->set_mac(mac);

	set_isa_device();
}

void enet16_device::device_reset()
{
	calc_checksum();

	m_irq = ioport("ENET16_IRQ")->read() & 0xff;
	map_io();
	map_ram();
}

void enet16_device::calc_checksum()
{
	memcpy(m_prom, m_dp8390->get_mac(), 6);
	m_prom[6] = 0x03;
	uint16_t sum = 0;
	for (int i = 0; i < 7; i++)
		sum += m_prom[i];
	m_prom[7] = 0xff - (sum & 0xff);
}

void enet16_device::map_io()
{
	m_isa->install_device(0x0280, 0x029f, read8sm_delegate(*this, FUNC(enet16_device::isa_port_r)), write8sm_delegate(*this, FUNC(enet16_device::isa_port_w)));
}

void enet16_device::map_ram()
{
	uint8_t mem_base = ioport("ENET16_MEM_BASE")->read() & 0x1;
	switch (mem_base) {
		case 0:
			m_isa->install_memory(0xc8000, 0xcbfff, read8sm_delegate(*this, FUNC(enet16_device::isa_mem_r)), write8sm_delegate(*this, FUNC(enet16_device::isa_mem_w)));
			break;
		case 1:
			m_isa->install_memory(0xcc000, 0xcffff, read8sm_delegate(*this, FUNC(enet16_device::isa_mem_r)), write8sm_delegate(*this, FUNC(enet16_device::isa_mem_w)));
			break;
	}
}

uint8_t enet16_device::isa_port_r(offs_t offset)
{
	switch ((offset >> 3) & 3) { 
		case 0:
			// offset 0x00 to 0x07 read FF
			return 0xff;
		case 1:
			// offset 0x07 to 0x0F read PROM
			return m_prom[offset & 7];
		case 2:
		case 3:
			// offset 0x10 to 0x1F read 8390
			return m_dp8390->cs_read(offset & 15);
	}
	return 0xff;
}

void enet16_device::isa_port_w(offs_t offset, uint8_t data)
{
	switch ((offset >> 3) & 3) { 
		case 0:
			// 0x00 = WD_CMDREG0 ; ETS writes 0xFF, then 0x7F
			//  0x80 WD_RESET  - board reset
			//  0x40 WD_MEMENB - enable shared memory
			// 0x01 = ?
			// 0x05 = WD_CMDREG5 ; ETS writes 0x41
			//  0x40 NIC16     - 16bit access from 8390
			// 0x07 = ?
			logerror("enet16: write to register. %02x, %02x\n", offset, data);
			if ((offset == 0) && (data & 0x40))
				map_ram();
			return;
		case 1:
			// offset 0x07 to 0x0F write PROM
			logerror("enet16: invalid attempt to write to prom. %02x, %02x\n", offset, data);
			return;
		case 2:
		case 3:
			// offset 0x10 to 0x1F read 8390
			return m_dp8390->cs_write(offset & 15, data);
	}
}

uint8_t enet16_device::isa_mem_r(offs_t offset)
{
	return m_ram[offset & 0x3fff];
}

void enet16_device::isa_mem_w(offs_t offset, uint8_t data)
{
	m_ram[offset & 0x3fff] = data;
}

WRITE_LINE_MEMBER(enet16_device::snic_irq_w)
{
	switch (m_irq) {
		case 0:
			m_isa->irq7_w(state);
			break;
		case 1:
			m_isa->irq5_w(state);
			break;
		case 2:
			m_isa->irq4_w(state);
			break;
		case 3:
			m_isa->irq3_w(state);
			break;
		case 4:
			m_isa->irq2_w(state);
			break;
		case 5:
			m_isa->irq10_w(state);
			break;
		case 6:
			m_isa->irq11_w(state);
			break;
		case 7:
			m_isa->irq15_w(state);
			break;
	}
}

uint8_t enet16_device::snic_mem_r(offs_t offset)
{
	if((offset < 0x8000) || (offset >= 0xC000)) return 0xff;
	return m_ram[offset & 0x3fff];
}

void enet16_device::snic_mem_w(offs_t offset, uint8_t data)
{
	if((offset < 0x8000) || (offset >= 0xC000)) return;
	m_ram[offset & 0x3fff] = data;
}

static INPUT_PORTS_START( enet16 )
	PORT_START("ENET16_IRQ")
	PORT_CONFNAME(0x07, 0x01, "ENET16 IRQ jumper (J6A-H)")
	PORT_CONFSETTING( 0x00, "IRQ7")
	PORT_CONFSETTING( 0x01, "IRQ5")
	PORT_CONFSETTING( 0x02, "IRQ4")
	PORT_CONFSETTING( 0x03, "IRQ3")
	PORT_CONFSETTING( 0x04, "IRQ2/9")
	PORT_CONFSETTING( 0x05, "IRQ10")
	PORT_CONFSETTING( 0x06, "IRQ11")
	PORT_CONFSETTING( 0x07, "IRQ15")

	PORT_START("ENET16_MEM_BASE")
	PORT_CONFNAME(0x01, 0x00, "ENET16 BASE MEMORY jumper (J10A-F)")
	PORT_CONFSETTING( 0x00, "C8000h")
	PORT_CONFSETTING( 0x01, "CC000h")
INPUT_PORTS_END

ioport_constructor enet16_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(enet16);
}
