// license:BSD-3-Clause
// copyright-holders:Ariane Fugmann
#ifndef MAME_MACHINE_YBDCOMM_H
#define MAME_MACHINE_YBDCOMM_H

#pragma once

#define YBDCOMM_SIMULATION_OFF

#include "osdfile.h"
#include "cpu/z80/z80.h"
#include "machine/mb8421.h"
#include "machine/mb89372.h"


//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

class ybdcomm_device : public device_t
{
public:
	// construction/destruction
	ybdcomm_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// ex-bus connection to host
	uint8_t ex_r(offs_t offset);
	void ex_w(offs_t offset, uint8_t data);

	void ybdcomm_mem(address_map &map);
	void ybdcomm_io(address_map &map);

protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;
	virtual void device_reset_after_children() override;

	virtual ioport_constructor device_input_ports() const override;
	virtual const tiny_rom_entry *device_rom_region() const override;

	// optional information overrides
	virtual void device_add_mconfig(machine_config &config) override;

private:
	required_device<z80_device> m_cpu;
	required_device<mb8421_device> m_dpram;
	required_device<mb89372_device> m_mpc;

	// MB8421
	DECLARE_WRITE_LINE_MEMBER(dpram_int5_w);

	uint8_t m_dma_reg[0x8]{}; // probably more
	void dma_reg_w(offs_t offset, uint8_t data);
	void update_dma();

	// MB89372
	DECLARE_WRITE_LINE_MEMBER(mpc_hreq_w);
	DECLARE_WRITE_LINE_MEMBER(mpc_int7_w);
	uint8_t mpc_mem_r(offs_t offset);
	void mpc_mem_w(offs_t offset, uint8_t data);

	uint8_t m_ybd_stat = 0; // not sure about those yet - 7474 for top bit? and 74161 for lower 4 bits
	uint8_t m_z80_stat = 0; // not sure about those yet - 74LS374

	uint8_t z80_stat_r();
	void z80_stat_w(uint8_t data);

#ifdef YBDCOMM_SIMULATION
	TIMER_CALLBACK_MEMBER(tick_timer);

	osd_file::ptr m_line_rx; // rx line - can be either differential, simple serial or toslink
	osd_file::ptr m_line_tx; // tx line - is differential, simple serial and toslink
	char m_localhost[256]{};
	char m_remotehost[256]{};
	uint8_t m_buffer0[0x200]{};
	uint8_t m_buffer1[0x200]{};

	uint8_t m_linkenable = 0;
	uint16_t m_linktimer = 0;
	uint8_t m_linkalive = 0;
	uint8_t m_linkid = 0;
	uint8_t m_linkcount = 0;

	int comm_frameOffset(uint8_t cabIdx);
	int comm_frameSize(uint8_t cabIdx);
	void comm_tick();
	int read_frame(int dataSize);
	void send_data(uint8_t frameType, int frameOffset, int frameSize, int dataSize);
	void send_frame(int dataSize);

	emu_timer *m_tick_timer;
#endif
};

// device type definition
DECLARE_DEVICE_TYPE(YBDCOMM, ybdcomm_device)

#endif // MAME_MACHINE_YBDCOMM_H
