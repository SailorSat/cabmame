// license:BSD-3-Clause
// copyright-holders:Ariane Fugmann
#ifndef MAME_MACHINE_XBDCOMM_H
#define MAME_MACHINE_XBDCOMM_H

#pragma once

#define XBDCOMM_SIMULATION

#include "osdfile.h"
#include "cpu/z80/z80.h"
#include "machine/mb8421.h"


//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

class xbdcomm_device : public device_t
{
public:
	// construction/destruction
	xbdcomm_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// ex-bus connection to host
	uint8_t ex_r(offs_t offset);
	void ex_w(offs_t offset, uint8_t data);

	void xbdcomm_mem(address_map &map);
	void xbdcomm_io(address_map &map);

protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;
	virtual void device_reset_after_children() override;

	virtual const tiny_rom_entry *device_rom_region() const override;

	// optional information overrides
	virtual void device_add_mconfig(machine_config &config) override;

private:
	required_device<z80_device> m_cpu;
	required_device<mb8421_device> m_dpram;

	DECLARE_WRITE_LINE_MEMBER(dpram_int5_w);
	DECLARE_WRITE_LINE_MEMBER(dlc_int7_w);

	uint8_t m_dma_reg[0x8]{}; // probably more
	void dma_reg_w(offs_t offset, uint8_t data);
	void update_dma();
	uint8_t dma_mem_r(offs_t offset);
	void dma_mem_w(offs_t offset, uint8_t data);

	uint8_t m_ex_page = 0; // 74LS374 probably

	uint8_t m_xbd_stat = 0; // not sure about those yet - 7474 for top bit? and 74161 for lower 4 bits
	uint8_t m_z80_stat = 0; // not sure about those yet - 74LS374

	uint8_t z80_stat_r();
	void z80_stat_w(uint8_t data);

	osd_file::ptr m_line_rx; // rx line - can be either differential, simple serial or toslink
	osd_file::ptr m_line_tx; // tx line - is differential, simple serial and toslink
	char m_localhost[256]{};
	char m_remotehost[256]{};
	uint8_t m_buffer0[0x100]{};
	uint8_t m_buffer1[0x100]{};
};

// device type definition
DECLARE_DEVICE_TYPE(XBDCOMM, xbdcomm_device)

#endif // MAME_MACHINE_XBDCOMM_H
