// license:BSD-3-Clause
// copyright-holders:Ariane Fugmann
#ifndef MAME_BUS_ISA_ENET16_H
#define MAME_BUS_ISA_ENET16_H

#pragma once

#include "isa.h"
#include "machine/dp8390.h"

class enet16_device: public device_t, public device_isa16_card_interface
{
public:
	enet16_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	uint8_t isa_port_r(offs_t offset);
	void isa_port_w(offs_t offset, uint8_t data);
	uint8_t isa_mem_r(offs_t offset);
	void isa_mem_w(offs_t offset, uint8_t data);

protected:
	virtual void device_start() override;
	virtual void device_reset() override;

	virtual void device_add_mconfig(machine_config &config) override;
	virtual ioport_constructor device_input_ports() const override;

private:
	void map_io();
	void map_ram();
	void calc_checksum();

	void snic_irq_w(int state);
	uint8_t snic_mem_r(offs_t offset);
	void snic_mem_w(offs_t offset, uint8_t data);

	required_device<dp8390d_device> m_dp8390;
	uint8_t m_irq;
	uint8_t m_ram[16*1024];
	uint8_t m_prom[32];
};

DECLARE_DEVICE_TYPE(ISA16_ENET16, enet16_device)

#endif // MAME_BUS_ISA_ENET16_H
