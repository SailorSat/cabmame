// license:BSD-3-Clause
// copyright-holders:Ariane Fugmann
#ifndef MAME_BUS_ISA_VID1000_H
#define MAME_BUS_ISA_VID1000_H

#pragma once

#include "isa.h"

class vid1000_device: public device_t, public device_isa16_card_interface
{
public:
	vid1000_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// dummy
	virtual uint32_t screen_update_a(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	virtual uint32_t screen_update_b(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);

	uint8_t isa_port_r(offs_t offset);
	void isa_port_w(offs_t offset, uint8_t data);

protected:
	virtual void device_start() override;
	virtual void device_reset() override;

	required_device<screen_device> m_screen_a;
	required_device<screen_device> m_screen_b;

	virtual void device_add_mconfig(machine_config &config) override;

private:
	void map_io();
};

DECLARE_DEVICE_TYPE(ISA16_VID1000, vid1000_device)

#endif // MAME_BUS_ISA_VID1000_H
