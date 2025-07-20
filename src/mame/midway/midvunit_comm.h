// license:BSD-3-Clause
// copyright-holders:Ariane Fugmann
#ifndef MAME_MIDWAY_VUNIT_COMM_H
#define MAME_MIDWAY_VUNIT_COMM_H

#pragma once

#include "cpu/tms32031/tms32031.h"

#include "asio.h"


//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

class midway_vunit_comm_device : public device_t
{
public:
	midway_vunit_comm_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	void set_linktype(uint8_t linktype);

	// reads/writes at I/O 0x997000
	uint32_t data_r();
	void data_w(uint32_t data);

	// reads/writes at I/O 0x997001
	uint32_t flags_r();
	void flags_w(uint32_t data);
protected:
	virtual void device_start() override ATTR_COLD;
	virtual void device_stop() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;

	required_device<tms32031_device> m_maincpu;


private:
	asio::io_context m_ioctx;
	std::optional<asio::ip::tcp::endpoint> m_localaddr;
	std::optional<asio::ip::tcp::endpoint> m_remoteaddr;
	asio::ip::tcp::acceptor m_acceptor;
	asio::ip::tcp::socket m_sock_rx;
	asio::ip::tcp::socket m_sock_tx;
	asio::steady_timer m_tx_timeout;
	uint8_t m_rx_state;
	uint8_t m_tx_state;

	uint8_t m_buffer0[0x200]{};

	uint8_t m_linktype = 0;
	uint8_t m_linkid = 0;
	uint8_t m_linkstate = 0;

	uint8_t m_link_buffer[0x04][0x0200]{};
	uint8_t m_link_length[0x04]{};
	uint8_t m_link_offset[0x04]{};

	uint16_t m_data = 0;
	uint8_t m_flags = 0;
	uint8_t m_readcount = 0;

	uint8_t m_intcount = 0;

	uint32_t data_r_crusnusa();
	void data_w_crusnusa(uint32_t data);
	void flags_w_crusnusa(uint32_t data);

	uint32_t data_r_crusnwld();
	void data_w_crusnwld(uint32_t data);
	void flags_w_crusnwld(uint32_t data);

	void send_vsync(uint8_t state);

	void check_sockets();
	void comm_start();
	void comm_stop();
	void comm_tick();
	unsigned read_frame(unsigned data_size);
	void send_data(uint8_t frame_type, unsigned frame_start, unsigned frame_size, unsigned data_size);
	void send_frame(unsigned data_size);
};

// device type definition
DECLARE_DEVICE_TYPE(MIDWAY_VUNIT_COMM, midway_vunit_comm_device)

#endif  // MAME_MIDWAY_VUNIT_COMM_H
