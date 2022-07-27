// license:BSD-3-Clause
// copyright-holders:Carl
#ifndef MAME_BUS_ISA_MCD_H
#define MAME_BUS_ISA_MCD_H

#pragma once

#include "isa.h"
#include "imagedev/chd_cd.h"
#include "sound/cdda.h"
#include "machine/mcd.h"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> mcd_isa_device

class mcd_isa_device : public device_t, public device_isa16_card_interface
{
public:
	// construction/destruction
	mcd_isa_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

protected:
	virtual uint16_t dack16_r(int line) override;
	virtual void device_start() override;
	virtual void device_reset() override;
	virtual void device_add_mconfig(machine_config &config) override;

private:
	required_device<mcd_device> m_mcd;

	void map(address_map &map);

	void irq_w(int state);
	void drq_w(int state);
};


// device type definition
DECLARE_DEVICE_TYPE(ISA16_MCD, mcd_isa_device)

#endif // MAME_BUS_ISA_MCD_H
