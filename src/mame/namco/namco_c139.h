// license:BSD-3-Clause
// copyright-holders:Angelo Salese
/***************************************************************************

    Namco C139 - Serial I/F Controller

***************************************************************************/
#ifndef MAME_MACHINE_NAMCO_C139_H
#define MAME_MACHINE_NAMCO_C139_H

#pragma once

#define C139_SIMULATION

#include "osdfile.h"


//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> namco_c139_device

class namco_c139_device : public device_t
{
public:
	// construction/destruction
	namco_c139_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	auto irq_cb() { return m_irq_cb.bind(); }

	// I/O operations
	void data_map(address_map& map);
	void regs_map(address_map &map);

	uint16_t ram_r(offs_t offset);
	void ram_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);

	uint16_t reg_r(offs_t offset);
	void reg_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);

protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;

	devcb_write_line m_irq_cb;

private:
	uint16_t m_ram[0x2000]{};
	uint16_t m_reg[0x0010]{};

#ifdef C139_SIMULATION
	osd_file::ptr m_line_rx;
	osd_file::ptr m_line_tx;
	char m_localhost[256]{};
	char m_remotehost[256]{};
	uint8_t m_buffer0[0x200]{};
	uint8_t m_buffer1[0x200]{};
	uint8_t m_framesync;

	uint8_t m_linkenable = 0;
	uint16_t m_linktimer = 0;
	uint8_t m_linkalive = 0;
	uint8_t m_linkid = 0;
	uint8_t m_linkcount = 0;

	emu_timer *m_tick_timer = nullptr;
	TIMER_CALLBACK_MEMBER(tick_timer_callback);

	void comm_tick();
	int find_sync_bit();
	int read_frame(int dataSize);
	void send_data(int dataSize);
	void send_frame(int dataSize);
#endif
};


// device type definition
DECLARE_DEVICE_TYPE(NAMCO_C139, namco_c139_device)



//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************



#endif // MAME_MACHINE_NAMCO_C139_H
