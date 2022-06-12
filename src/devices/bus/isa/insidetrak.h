// license:BSD-3-Clause
// copyright-holders:Ariane Fugmann
#ifndef MAME_BUS_ISA_INSIDETRAK_H
#define MAME_BUS_ISA_INSIDETRAK_H

#pragma once

#include "isa.h"
#include "cpu/tms32031/tms32031.h"

#define INSIDETRAK_CLOCK1   XTAL(33'333'000)
#define INSIDETRAK_CLOCK2   XTAL(10'000'000)

class insidetrak_device: public device_t,
						public device_isa16_card_interface
{
public:
	insidetrak_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	uint16_t isa_port_r(offs_t offset, uint16_t mem_mask);
	void isa_port_w(offs_t offset, uint16_t data, uint16_t mem_mask);

	void cpu_map(address_map &map);

protected:
	virtual void device_start() override;
	virtual void device_reset() override;

	virtual void device_add_mconfig(machine_config &config) override;
	virtual ioport_constructor device_input_ports() const override;
	virtual const tiny_rom_entry *device_rom_region() const override;

private:
	void map_io();

	required_device<tms32031_device> m_tms32031;
	uint8_t m_ram[4*32*1024];
	uint8_t m_recv_fifo[2*512];
	uint8_t m_xmit_fifo[1*512];

	uint16_t m_010001;
	uint32_t tms32031_010001_r(offs_t offset);
	void tms32031_010001_w(offs_t offset, uint32_t data, uint32_t mem_mask);

	uint32_t tms32031_io_r(offs_t offset);
	void tms32031_io_w(offs_t offset, uint32_t data, uint32_t mem_mask);
};

DECLARE_DEVICE_TYPE(ISA16_INSIDETRAK, insidetrak_device)

#endif // MAME_BUS_ISA_INSIDETRAK_H
