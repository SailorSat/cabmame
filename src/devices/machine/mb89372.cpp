// license:BSD-3-Clause
// copyright-holders:Ariane Fugmann
/**
    MB89372

    Fujitsu
    Multi-Protocol Controller

 **/

#include "emu.h"
#include "mb89372.h"
#include "emuopts.h"

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

// device type definition
DEFINE_DEVICE_TYPE(MB89372, mb89372_device, "mb89372", "MB89372 Multi-Protocol Controller")

//-------------------------------------------------
//  mb89372_device - constructor
//-------------------------------------------------

mb89372_device::mb89372_device( const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock ) :
	device_t(mconfig, MB89372, tag, owner, clock),
	device_execute_interface(mconfig, *this),
	m_icount(0),
	m_out_hreq_cb(*this),
	m_out_irq_cb(*this),
	m_in_memr_cb(*this),
	m_out_memw_cb(*this)
{
	// prepare localhost "filename"
	m_localhost[0] = 0;
	strcat(m_localhost, "socket.");
	strcat(m_localhost, mconfig.options().comm_localhost());
	strcat(m_localhost, ":");
	strcat(m_localhost, mconfig.options().comm_localport());

	// prepare remotehost "filename"
	m_remotehost[0] = 0;
	strcat(m_remotehost, "socket.");
	strcat(m_remotehost, mconfig.options().comm_remotehost());
	strcat(m_remotehost, ":");
	strcat(m_remotehost, mconfig.options().comm_remoteport());
}


//-------------------------------------------------
//  device_start - device-specific startup
//------------------------------------------------

void mb89372_device::device_start()
{
	// set our instruction counter
	set_icountptr(m_icount);

	// resolve callbacks
	m_out_hreq_cb.resolve_safe();
	m_out_irq_cb.resolve_safe();
	m_in_memr_cb.resolve_safe(0);
	m_out_memw_cb.resolve_safe();
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void mb89372_device::device_reset()
{
	std::fill(std::begin(m_reg), std::end(m_reg), 0);

	set_hreq(0);
	set_irq(0);

	m_intr_delay = 0;
	m_sock_delay = 0x20;

	m_rx_length = 0x0000;
	m_rx_offset = 0x0000;

	m_tx_offset = 0x0000;
}


//-------------------------------------------------
//  execute_run -
//-------------------------------------------------

void mb89372_device::execute_run()
{
	while (m_icount > 0)
	{
		// TODO waste some cycles before triggering ints
		if (m_intr_delay > 0)
		{
			m_intr_delay--;
			if (m_intr_delay == 0)
			{
			}
		}
		if (m_sock_delay > 0)
		{
			m_sock_delay--;
			if (m_sock_delay == 0)
			{
				m_sock_delay = 0x20;
				checkSockets();
			}
		}
		m_icount--;
	}
}


//-------------------------------------------------
//  read - handler for register reading
//-------------------------------------------------

uint8_t mb89372_device::read(offs_t offset)
{
	uint8_t data = 0xff;
	switch (offset & 0x3f)
	{
		default:
			logerror("MB89372 unimplemented register read @%02X\n", offset);
	}
	return data;
}


//-------------------------------------------------
//  write - handler for register writing
//-------------------------------------------------

void mb89372_device::write(offs_t offset, uint8_t data)
{
	switch (offset & 0x3f)
	{
		default:
			logerror("MB89372 unimplemented register write @%02X = %02X\n", offset, data);
	}
}


//**************************************************************************
//  buffer logic
//**************************************************************************

void mb89372_device::rxReset()
{
	m_rx_length = 0;
	m_rx_offset = 0;
}

uint8_t mb89372_device::rxRead()
{
	uint8_t data = m_rx_buffer[m_rx_offset];
	m_rx_offset++;

	/*
	if (m_rx_offset == m_rx_length)
		m_rxsr0 |= 0x40; // EOF
	*/

	if (m_rx_offset >= m_rx_length)
		rxReset();
	return data;
}

void mb89372_device::txReset()
{
	m_tx_offset = 0;
	//m_txsr |= 0x05;
}

void mb89372_device::txWrite(uint8_t data)
{
	m_tx_buffer[m_tx_offset] = data;
	m_tx_offset++;
	//m_txsr = 0x6b;

	// prevent overflow
	if (m_tx_offset >= 0x0f00)
		m_tx_offset = 0x0eff;
}

void mb89372_device::txComplete()
{
	if (m_tx_offset > 0)
	{
		if (m_line_rx && m_line_tx)
		{
			m_socket_buffer[0x00] = m_tx_offset & 0xff;
			m_socket_buffer[0x01] = (m_tx_offset >> 8) & 0xff;
			for (int i = 0x00 ; i < m_tx_offset ; i++)
			{
				m_socket_buffer[i + 2] = m_tx_buffer[i];
			}

			std::uint32_t dataSize = m_tx_offset + 2;
			std::uint32_t written;

			m_line_tx->write(&m_socket_buffer, 0, dataSize, written);
		}
	}

	//m_txsr = 0x6f;

	txReset();
}

void mb89372_device::checkSockets()
{
	// check rx socket
	if (!m_line_rx)
	{
		osd_printf_verbose("MB89372 listen on %s\n", m_localhost);
		uint64_t filesize; // unused
		osd_file::open(m_localhost, OPEN_FLAG_CREATE, m_line_rx, filesize);
	}

	// check tx socket
	if (!m_line_tx)
	{
		osd_printf_verbose("MB89372 connect to %s\n", m_remotehost);
		uint64_t filesize; // unused
		osd_file::open(m_remotehost, 0, m_line_tx, filesize);
	}

	if (m_line_rx && m_line_tx)
	{
		// RXCR_RXE
		if (true)
		{
			if (m_rx_length == 0)
			{
				std::uint32_t recv = 0;
				m_line_rx->read(m_socket_buffer, 0, 2, recv);
				if (recv > 0)
				{
					if (recv == 2)
						m_rx_length = m_socket_buffer[0x01] << 8 | m_socket_buffer[0x00];
					else
					{
						m_rx_length = m_socket_buffer[0x00];
						m_line_rx->read(m_socket_buffer, 0, 1, recv);
						while (recv == 0) {}
						m_rx_length |= m_socket_buffer[0x00] << 8;
					}

					int offset = 0;
					int togo = m_rx_length;
					while (togo > 0)
					{
						m_line_rx->read(m_socket_buffer, 0, togo, recv);
						for (int i = 0x00 ; i < recv ; i++)
						{
							m_rx_buffer[offset] = m_socket_buffer[i];
							offset++;
						}
						togo -= recv;
					}

					m_rx_offset = 0;
					//m_rxsr0 = 0x01; // RXRDY

					/*
					if (m_rx_offset + 1 == m_rx_length)
						m_rxsr0 |= 0x40; // EOF
					*/

					//m_rxsr1 = 0xc8;

					//set_po3(!MASKR_MRXDRQ && RXIER_RXDI ? 1 : 0);
				}
			}
		}
	}
}

//**************************************************************************
//  INLINE HELPERS
//**************************************************************************

inline void mb89372_device::set_hreq(int state)
{
	if (m_hreq != state)
	{
		m_out_hreq_cb(state);
		m_hreq = state;
	}
}

inline void mb89372_device::set_irq(int state)
{
	if (m_irq != state)
	{
		m_out_irq_cb(state);
		m_irq = state;
	}
}

