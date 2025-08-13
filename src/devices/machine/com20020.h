// license:BSD-3-Clause
// copyright-holders:Ariane Fugmann
/**************************************************************************************************

Standard Microsystems Corp. COM20020 Universal Local Area Netowrk Controller (ULANC)

**************************************************************************************************/

#ifndef MAME_MACHINE_COM20020_H
#define MAME_MACHINE_COM20020_H

#pragma once

class com20020_device : public device_t
{
public:
	// construction/destruction
	com20020_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock = 0U);

	auto irq_cb() { return m_irq_cb.bind(); }

	virtual void regs_map(address_map &map) ATTR_COLD;


protected:
	com20020_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, u32 clock);

	// device-level overrides
	virtual void device_start() override ATTR_COLD;
	virtual void device_stop() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;

	memory_share_creator<uint8_t> m_ram;

	devcb_write_line m_irq_cb;
	int m_irq_state;

private:
	uint8_t m_reg[0x10];
	uint8_t m_cmd_tx;
	uint8_t m_cmd_rx;
	uint8_t m_cfg;
	uint8_t m_txd;

	emu_timer *m_tick_timer;

	class context;
	std::unique_ptr<context> m_context;

	uint8_t m_buffer[0x200];

	TIMER_CALLBACK_MEMBER(tick_timer_callback);

	void update_irq();

	void comm_tick();
	unsigned read_frame(unsigned data_size);
	void send_frame(unsigned data_size);
};

// device type definition
DECLARE_DEVICE_TYPE(COM20020, com20020_device)
#endif // MAME_MACHINE_COM20020_H
