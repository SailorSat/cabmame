// license:BSD-3-Clause
// copyright-holders:Ariane Fugmann
#ifndef MAME_JALECO_CISCHEAT_COMLINK_H
#define MAME_JALECO_CISCHEAT_COMLINK_H

#pragma once

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

class jaleco_cischeat_comlink_device : public device_t
{
public:
	jaleco_cischeat_comlink_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// shared memory 2k
	uint8_t share_r(offs_t offset);
	void share_w(offs_t offset, uint8_t data);

	void handle_vint_irq();

	void setup_comlink(uint16_t frame_size, uint8_t link_max);

protected:
	virtual void device_start() override ATTR_COLD;
	virtual void device_stop() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;

	uint8_t m_shared[0x0800]; // 2k shared memory

private:
	class context;
	std::unique_ptr<context> m_context;

	uint8_t m_buffer[0x0201];
	uint16_t m_framesize;
	uint8_t m_framesync;
	uint8_t m_linkid;
	uint8_t m_linkmax;
	uint8_t m_linkwait;

	void comm_tick();
	void read_fg();
	unsigned read_frame(unsigned data_size);
	void send_data(uint8_t frame_type, unsigned frame_start, unsigned frame_size, unsigned data_size);
	void send_frame(unsigned data_size);
};

// device type definition
DECLARE_DEVICE_TYPE(JALECO_CISCOHEAT_COMLINK, jaleco_cischeat_comlink_device)

#endif  // MAME_JALECO_CISCHEAT_COMLINK_H
