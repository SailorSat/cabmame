// license:BSD-3-Clause
// copyright-holders:Carl

#include "emu.h"
#include "mcd.h"
#include "coreutil.h"
#include "speaker.h"

void mcd_isa_device::map(address_map &map)
{
	map(0x0, 0x0).rw(m_mcd, FUNC(mcd_device::data_r), FUNC(mcd_device::cmd_w));
	map(0x1, 0x1).rw(m_mcd, FUNC(mcd_device::flag_r), FUNC(mcd_device::reset_w));
}

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

DEFINE_DEVICE_TYPE(ISA16_MCD, mcd_isa_device, "mcd_isa", "Mitsumi ISA CD-ROM Adapter")

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

void mcd_isa_device::device_add_mconfig(machine_config &config)
{
	MCD(config, m_mcd, 0);
	m_mcd->irq_callback().set(FUNC(mcd_isa_device::irq_w));
	m_mcd->drq_callback().set(FUNC(mcd_isa_device::drq_w));
}

//-------------------------------------------------
//  mcd_isa_device - constructor
//-------------------------------------------------

mcd_isa_device::mcd_isa_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, ISA16_MCD, tag, owner, clock),
	device_isa16_card_interface( mconfig, *this ),
	m_mcd(*this, "mcd")
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void mcd_isa_device::device_start()
{
	set_isa_device();
	m_isa->set_dma_channel(5, this, false);
	m_isa->install_device(0x0310, 0x0313, *this, &mcd_isa_device::map);
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void mcd_isa_device::device_reset()
{
}

uint16_t mcd_isa_device::dack16_r(int line)
{
	return m_mcd->dack16_r(line);
}

WRITE_LINE_MEMBER(mcd_isa_device::irq_w)
{
	m_isa->irq5_w(state);
}

WRITE_LINE_MEMBER(mcd_isa_device::drq_w)
{
	m_isa->drq5_w(state);
}
