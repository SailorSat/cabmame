// license:BSD-3-Clause
// copyright-holders:Ariane Fugmann
#ifndef MAME_BUS_ISA_FORMATC_H
#define MAME_BUS_ISA_FORMATC_H

#pragma once

#include "isa.h"
#include "cpu/m68000/m68000.h"
#include "sound/es5506.h"
#include "machine/mcd2.h"
#include "cpu/mcs96/i8x9x.h"

#define FMTC_SSCAPE_CLOCK1   XTAL(32'000'000)
#define FMTC_SSCAPE_CLOCK2   XTAL(14'318'181)
#define FMTC_SSCAPE_CLOCK3   XTAL(10'000'000)
#define FMTC_CONTROL_CLOCK   XTAL(8'000'000)


class formatc_device: public device_t, public device_isa16_card_interface
{
public:
	formatc_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	uint8_t isa_odie_r(offs_t offset);
	void isa_odie_w(offs_t offset, uint8_t data);
	uint8_t isa_ctrl_r(offs_t offset);
	void isa_ctrl_w(offs_t offset, uint8_t data);
	uint8_t isa_mem_r(offs_t offset);
	void isa_mem_w(offs_t offset, uint8_t data);

	void sscape_map(address_map &map);
	void ctrl_map(address_map &map);

protected:
	virtual void device_start() override;
	virtual void device_reset() override;
	virtual void device_reset_after_children() override;

	virtual void device_add_mconfig(machine_config &config) override;
	virtual ioport_constructor device_input_ports() const override;
	virtual const tiny_rom_entry *device_rom_region() const override;

	uint8_t dack_r(int line) override;
	void dack_w(int line, uint8_t data) override;
	void eop_w(int state) override;

private:
	void map_io();
	void map_dma();
	void map_ram();

	uint8_t m_sscape_ram0[2*64*1024];
	uint8_t sscape_ram0_r(offs_t offset);
	void sscape_ram0_w(offs_t offset, uint8_t data);
	uint8_t sscape_ram1_r(offs_t offset);
	void sscape_ram1_w(offs_t offset, uint8_t data);
	uint8_t sscape_ram3_r(offs_t offset);
	void sscape_ram3_w(offs_t offset, uint8_t data);

	uint8_t sscape_otto_r(offs_t offset);
	void sscape_otto_w(offs_t offset, uint8_t data);
	uint8_t sscape_odie_r(offs_t offset);
	void sscape_odie_w(offs_t offset, uint8_t data);

	void trigger_odie_dma(int which);
	void update_odie_dma(int which);
	uint8_t m_odie_dma_channel[2];
	uint32_t m_odie_dma_address[2];

	void update_odie_mode();
	uint8_t m_odie_cd_mode;
	uint8_t m_odie_obp_mode;

	void drq_w(int state, int source);
	void irq_w(int state);
	uint8_t m_eop;

	uint8_t ctrl_mem_r(offs_t offset);
	void ctrl_mem_w(offs_t offset, uint8_t data);

	required_device<m68000_base_device> m_m68000;
	required_device<es5506_device> m_es5506;
	required_device<mcd2_device> m_mcd;
	required_device<i8x9x_device> m_i80198;

	uint8_t odie_reg_r(offs_t offset);
	void odie_reg_w(offs_t offset, uint8_t data);

	uint8_t m_odie_regs[0x30];
	uint8_t m_odie_page;

	uint8_t m_ctrl_ram[2*1024];

	uint8_t mcd_r(offs_t offset);
	void mcd_w(offs_t offset, uint8_t data);
};

DECLARE_DEVICE_TYPE(ISA16_FORMATC, formatc_device)

#endif // MAME_BUS_ISA_FORMATC_H
