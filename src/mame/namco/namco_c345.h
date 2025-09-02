// license:BSD-3-Clause
// copyright-holders:Ariane Fugmann
/***************************************************************************

    Namco C345 - Serial I/F Controller

***************************************************************************/
#ifndef MAME_NAMCO_NAMCO_C345_H
#define MAME_NAMCO_NAMCO_C345_H

#pragma once

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************
class namco_c345_device : public device_t
{
public:
	// construction/destruction
	namco_c345_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	auto irq_cb() { return m_irq_cb.bind(); }
	void vblank_tick();

	// I/O operations
	void data_map(address_map &map) ATTR_COLD;
	void regs_map(address_map &map) ATTR_COLD;

	uint16_t ram_r(offs_t offset);
	void ram_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);

	uint16_t reg_r(offs_t offset);
	void reg_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);

protected:
	// device-level overrides
	virtual void device_start() override ATTR_COLD;
	virtual void device_stop() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;

	devcb_write_line m_irq_cb;

private:
	uint16_t m_ram[0x2000];
	uint16_t m_reg[0x0020];
	uint16_t m_id[0x0008];

	std::string m_localhost;
	std::string m_localport;
	std::string m_remotehost;
	std::string m_remoteport;
	
	class context;
	std::unique_ptr<context> m_context;

	uint8_t m_buffer[0x200];

	uint8_t m_framesync;
	uint8_t m_linkid;
	uint8_t m_linkstate;
	uint8_t m_txblock;

	void comm_tick();
	int comm_find_id(uint8_t id);
	unsigned read_frame(unsigned data_size);
	void send_frame(unsigned data_size);
};

// device type definition
DECLARE_DEVICE_TYPE(NAMCO_C345, namco_c345_device)

#endif // MAME_NAMCO_NAMCO_C345_H
