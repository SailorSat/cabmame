// license:BSD-3-Clause
// copyright-holders:Ariane Fugmann
#ifndef MAME_BUS_ISA_PIX1000_H
#define MAME_BUS_ISA_PIX1000_H

#pragma once

#include "isa.h"
//#include "cpu/m88000/m88000.h"

#define PIX_CLOCK       XTAL(40'000'000)
#define PIX_FIFOSIZE    2048
#define PIX_DRAMSIZE    8*1024*1024

class pix1000_device: public device_t,
						public device_isa16_card_interface
{
public:
	pix1000_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	uint8_t isa_proc_r(offs_t offset);
	void isa_proc_w(offs_t offset, uint8_t data);
	uint16_t isa_fifo_r(offs_t offset, uint16_t mem_mask);
	void isa_fifo_w(offs_t offset, uint16_t data, uint16_t mem_mask);
	uint8_t isa_mem_r(offs_t offset);
	void isa_mem_w(offs_t offset, uint8_t data);

	void m88110a_map(address_map &map);
	void m88110b_map(address_map &map);

protected:
	virtual void device_start() override;
	virtual void device_reset() override;
	virtual void device_reset_after_children() override;

	virtual void device_add_mconfig(machine_config &config) override;
	virtual ioport_constructor device_input_ports() const override;

private:
	void map_io();
	void map_ram();

	//required_device<mc88100_device> m_m88110a;
	//required_device<mc88100_device> m_m88110b;

	uint8_t m_pix_dram[PIX_DRAMSIZE];

	uint32_t m88110a_r(offs_t offset, uint32_t mem_mask);
	void m88110a_w(offs_t offset, uint32_t data, uint32_t mem_mask);
	uint32_t m88110b_r(offs_t offset, uint32_t mem_mask);
	void m88110b_w(offs_t offset, uint32_t data, uint32_t mem_mask);

	uint16_t m_fifo[PIX_FIFOSIZE];
	uint8_t m_fifo_pos;
	uint8_t m_fifo_end;

	uint8_t m_proc_reg0;
	uint8_t m_proc_reg1;
	uint8_t m_proc_reg2;
	uint8_t m_proc_reg3;
	uint8_t m_proc_reg4;

	void fifo_push(uint16_t data);
	uint16_t fifo_pop();
	uint16_t fifo_depth() const { return m_fifo_end - m_fifo_pos; }
};

DECLARE_DEVICE_TYPE(ISA16_PIX1000, pix1000_device)

#endif // MAME_BUS_ISA_PIX1000_H
