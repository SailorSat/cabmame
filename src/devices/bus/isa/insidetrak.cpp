// license:BSD-3-Clause
// copyright-holders:Ariane Fugmann

/*
Polhemus InsideTrak

Todo:
  check/add timings/delays etc.
  add actual inputs

Notes:
  1x TMS320C31PQL  - DSP
  4x MT5C2568DJ-25 - MICRON SRAM 32Kx8 / 25ns
  3x MS7201AL-80FC - MOSEL FIFO 512x9 / 80ns

  1x DSP56ADC16    - Motorola 16-bit ADC
  1x AD7840        - Analog Devices 14-bit DAC
  
  OSC1 33.3MHz
  OSC2 10.0MHz

  SU2000/SU3000: primary card jumpered to I/O 0x270
                 secondary card jumpered to I/O 0x278

  DSP56ADC hooked to serial rx
    sample clock = 10MHz / 32
    serial clock = 10MHz / 2
    serial size  = 16bit (msb first)

  http://images.arianchen.de/virtuality/su2000_track1-01.jpg
  http://www.arianchen.de/su2000/details_tracker.html
*/
#include "emu.h"
#include "insidetrak.h"

/*************************************
 *
 *  internal I/O port definitions
 *
 *************************************/

#define DMA_GLOBAL_CTL          0x00
#define DMA_SOURCE_ADDR         0x04
#define DMA_DEST_ADDR           0x06
#define DMA_TRANSFER_COUNT      0x08

#define TIMER0_GLOBAL_CTL       0x20
#define TIMER0_COUNTER          0x24
#define TIMER0_PERIOD           0x28

#define TIMER1_GLOBAL_CTL       0x30
#define TIMER1_COUNTER          0x34
#define TIMER1_PERIOD           0x38

#define SPORT_GLOBAL_CTL        0x40
#define SPORT_TX_CTL            0x42
#define SPORT_RX_CTL            0x43
#define SPORT_TIMER_CTL         0x44
#define SPORT_TIMER_COUNTER     0x45
#define SPORT_TIMER_PERIOD      0x46
#define SPORT_DATA_TX           0x48
#define SPORT_DATA_RX           0x4c

static INPUT_PORTS_START( insidetrak )
	PORT_START("INSIDETRAK_IO_BASE")
	PORT_CONFNAME(0x01, 0x00, "InsideTRAK I/O address")
	PORT_CONFSETTING( 0x00, "270")
	PORT_CONFSETTING( 0x01, "278")

	PORT_START("INSIDETRAK_IRQ")
	PORT_CONFNAME(0x0b, 0x00, "InsideTRAK IRQ")
	PORT_CONFSETTING( 0x00, "NONE")
	PORT_CONFSETTING( 0x01, "IRQ3")
	PORT_CONFSETTING( 0x02, "IRQ4")
	PORT_CONFSETTING( 0x03, "IRQ5")
	PORT_CONFSETTING( 0x04, "IRQ6")
	PORT_CONFSETTING( 0x05, "IRQ7")
	PORT_CONFSETTING( 0x06, "IRQ9")
	PORT_CONFSETTING( 0x07, "IRQ10")
	PORT_CONFSETTING( 0x08, "IRQ11")
	PORT_CONFSETTING( 0x09, "IRQ12")
	PORT_CONFSETTING( 0x0a, "IRQ14")
	PORT_CONFSETTING( 0x0b, "IRQ15")
INPUT_PORTS_END

ROM_START( insidetrak )
	ROM_REGION32_LE(0x040000, "tracker", 0)

	ROM_DEFAULT_BIOS("151.03")

	ROM_SYSTEM_BIOS( 0, "151.01", "Ver 151.01" )
	ROMX_LOAD( "ver_151_01_u16.u16", 0x00000, 0x20000, CRC(309180dd) SHA1(e32eafd8ad1b9e5d11027ab60b471bf97509f0e1), ROM_GROUPWORD | ROM_SKIP(2) | ROM_BIOS(0) )
	ROMX_LOAD( "ver_151_01_u17.u17", 0x00002, 0x20000, CRC(186bfd55) SHA1(900645c8bdfe7ece07ec8515863f23aba81a3187), ROM_GROUPWORD | ROM_SKIP(2) | ROM_BIOS(0) )

	ROM_SYSTEM_BIOS( 1, "151.02", "Ver 151.02" )
	ROMX_LOAD( "ver_151_02_u16.u16", 0x00000, 0x20000, CRC(869d4d41) SHA1(f66698ef872b3a4b903b2c8acbfec721b7191e94), ROM_GROUPWORD | ROM_SKIP(2) | ROM_BIOS(1) )
	ROMX_LOAD( "ver_151_02_u17.u17", 0x00002, 0x20000, CRC(053535b3) SHA1(7ca368854589fe75de00847f76c2468beee7e443), ROM_GROUPWORD | ROM_SKIP(2) | ROM_BIOS(1) )

	ROM_SYSTEM_BIOS( 2, "151.03", "Ver 151.03" )
	ROMX_LOAD( "ver_151_03_u16.u16", 0x00000, 0x20000, CRC(8354d059) SHA1(a88df7cc259c1c39316cc3bff9e08aa4e8d3d2c0), ROM_GROUPWORD | ROM_SKIP(2) | ROM_BIOS(2) )
	ROMX_LOAD( "ver_151_03_u17.u17", 0x00002, 0x20000, CRC(ace4081d) SHA1(f57287ded53f8d127bcdc9e34b8adb356fe55e5e), ROM_GROUPWORD | ROM_SKIP(2) | ROM_BIOS(2) )

	ROM_SYSTEM_BIOS( 3, "151.07", "Ver 151.07" )
	ROMX_LOAD( "ver_151_07_u16.u16", 0x00000, 0x20000, BAD_DUMP CRC(a4e96105) SHA1(955d22a9237eb7154c25f0969166d57ff1042be6), ROM_GROUPWORD | ROM_SKIP(2) | ROM_BIOS(3) )
	ROMX_LOAD( "ver_151_07_u17.u17", 0x00002, 0x20000, CRC(b7201df9) SHA1(8ec0f19de4d26bcd83bfd245387ade0471406472), ROM_GROUPWORD | ROM_SKIP(2) | ROM_BIOS(3) )
ROM_END

ioport_constructor insidetrak_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(insidetrak);
}

const tiny_rom_entry *insidetrak_device::device_rom_region() const
{
	return ROM_NAME( insidetrak );
}

DEFINE_DEVICE_TYPE(ISA16_INSIDETRAK, insidetrak_device, "insidetrak", "Polhemus InsideTRAK")

void insidetrak_device::device_add_mconfig(machine_config &config)
{
	TMS32031(config, m_tms32031, INSIDETRAK_CLOCK_33);
	m_tms32031->set_addrmap(AS_PROGRAM, &insidetrak_device::tms32031_map);

	TIMER(config, m_tms32031_timer[0]).configure_generic(DEVICE_SELF, FUNC(insidetrak_device::tms32031_timer_callback));
	TIMER(config, m_tms32031_timer[1]).configure_generic(DEVICE_SELF, FUNC(insidetrak_device::tms32031_timer_callback));

	TIMER(config, m_tms32031_dma_timer).configure_generic(DEVICE_SELF, FUNC(insidetrak_device::tms32031_dma_timer_callback));

	TIMER(config, m_xf0_timer).configure_generic(DEVICE_SELF, FUNC(insidetrak_device::xf0_timer_callback));
	TIMER(config, m_tclk1_timer).configure_generic(DEVICE_SELF, FUNC(insidetrak_device::tclk1_timer_callback));
}

insidetrak_device::insidetrak_device(const machine_config& mconfig, const char* tag, device_t* owner, uint32_t clock)
	: device_t(mconfig, ISA16_INSIDETRAK, tag, owner, clock),
	device_isa16_card_interface(mconfig, *this),
	m_tms32031(*this, "tms32031"),
	m_tms32031_timer(*this, "tms32031_timer%u", 0U),
	m_tms32031_dma_timer(*this, "tms32031_dma_timer"),
	m_xf0_timer(*this, "insidetrak_xf0_timer"),
	m_tclk1_timer(*this, "insidetrak_tclk1_timer")
{
}

void insidetrak_device::device_start()
{
	set_isa_device();

	m_tms32031_h1_clock_period = attotime::from_hz(m_tms32031->clock()) * 2;
}


void insidetrak_device::device_reset()
{
	map_io();

	memset(m_tms32031_int_regs, 0, 0x100 * 4);

	m_tms32031_timer[0]->reset();
	m_tms32031_timer[1]->reset();
	m_tms32031_timer_enabled[0] = 0;
	m_tms32031_timer_enabled[1] = 0;

	m_xf0_timer->reset();
	m_xf0_timer->adjust(attotime::from_hz(INSIDETRAK_CLOCK_10 / 24));

	m_tclk1_timer->reset();
	m_tclk1_timer->adjust(attotime::from_hz(INSIDETRAK_CLOCK_10 / 48));

	m_recv_fifo_pos = 0;
	m_recv_fifo_end = 0;
	m_xmit_fifo_pos = 0;
	m_xmit_fifo_end = 0;
}

/*************************************************************
 *
 * ISA BUS
 *
 *************************************************************/
 
void insidetrak_device::map_io()
{
	uint8_t io_base = ioport("INSIDETRAK_IO_BASE")->read() & 0x1;
	switch (io_base) {
		case 0:
			m_isa->install16_device(0x0270, 0x0277, read16s_delegate(*this, FUNC(insidetrak_device::isa_port_r)), write16s_delegate(*this, FUNC(insidetrak_device::isa_port_w)));
			break;
		case 1:
			m_isa->install16_device(0x0278, 0x027f, read16s_delegate(*this, FUNC(insidetrak_device::isa_port_r)), write16s_delegate(*this, FUNC(insidetrak_device::isa_port_w)));
			break;
	}
}

uint16_t insidetrak_device::isa_port_r(offs_t offset, uint16_t mem_mask) {
	if ((mem_mask == 0xffff) && (offset == 0)) {
		// 16bit read @ 0 = read fifo
		// clear_irq();
		return recv_fifo_pop();
	}
	if ((mem_mask == 0xff00) && (offset == 0)) {
		// 8bit read @ 1 = read fifo status
		// b0 = recv fifo; 0 empty, 1 data
		// b1 = xmit fifo; 0 full, 1 free
		uint8_t ret = 0xFC;
		if (recv_fifo_depth() > 0) ret |= 1;
		if (m_xmit_fifo_end < INSIDETRAK_FIFOSIZE) ret |= 2;
		return ret << 8;
	}
	logerror("insidetrak: unhandled isa port read %04X @ %04X\n", offset, mem_mask);
	return 0xffff;
}

void insidetrak_device::isa_port_w(offs_t offset, uint16_t data, uint16_t mem_mask) {
	if ((mem_mask == 0x00ff) && (offset == 0)) {
		// 8bit write @ 0 = write fifo
		xmit_fifo_push(data);
		return;
	}
	if ((mem_mask == 0xff00) && (offset == 0)) {
		// 8bit write @ 1 = force rcvr1 update
		m_tms32031->set_input_line(TMS3203X_IRQ0, ASSERT_LINE);
		m_tms32031->set_input_line(TMS3203X_IRQ0, CLEAR_LINE);
		return;
	}
	if ((mem_mask == 0x00ff) && (offset == 1)) {
		// 8bit write @ 2 = force rcvr2 update
		m_tms32031->set_input_line(TMS3203X_IRQ1, ASSERT_LINE);
		m_tms32031->set_input_line(TMS3203X_IRQ1, CLEAR_LINE);
		return;
	}
	if ((mem_mask == 0xff00) && (offset == 1)) {
		// 8bit write @ 3 = software sync pulse
		m_tms32031->set_input_line(TMS3203X_IRQ2, ASSERT_LINE);
		m_tms32031->set_input_line(TMS3203X_IRQ2, CLEAR_LINE);
		logerror("insidetrak: undocumented INT2 raised\n");
		return;
	}
	logerror("insidetrak: unhandled isa port write %04X @ %04X, %04X\n", offset, mem_mask, data);
	return;
}

void insidetrak_device::isa_irq_w(int state)
{
	if (m_isa_irq != state) {
		m_isa_irq = state;
		uint8_t irq = ioport("INSIDETRAK_IRQ")->read() & 0xb;

		switch(irq)
		{
			case 0x1:
				m_isa->irq3_w(state);
				break;
			case 0x2:
				m_isa->irq4_w(state);
				break;
			case 0x3:
				m_isa->irq5_w(state);
				break;
			case 0x4:
				m_isa->irq6_w(state);
				break;
			case 0x5:
				m_isa->irq7_w(state);
				break;
			case 0x6:
				m_isa->irq2_w(state);
				break;
			case 0x7:
				m_isa->irq10_w(state);
				break;
			case 0x8:
				m_isa->irq11_w(state);
				break;
			case 0x9:
				m_isa->irq12_w(state);
				break;
			case 0xa:
				m_isa->irq14_w(state);
				break;
			case 0xb:
				m_isa->irq15_w(state);
				break;
		}
	}
}

/*************************************************************
 *
 * Internal BUS
 *
 *************************************************************/

void insidetrak_device::tms32031_map(address_map &map)
{
	map(0x000000, 0x00ffff).rom().region("tracker", 0);
	map(0x010000, 0x010003).rw(FUNC(insidetrak_device::tms32031_ext_r), FUNC(insidetrak_device::tms32031_ext_w));
	map(0x020000, 0x027fff).ram();
	map(0x808000, 0x8080ff).rw(FUNC(insidetrak_device::tms32031_int_r), FUNC(insidetrak_device::tms32031_int_w));
}

uint32_t insidetrak_device::tms32031_ext_r(offs_t offset, uint32_t mem_mask)
{
	switch (offset) {
		case 0:
			// 8-bit FIFO
			return xmit_fifo_pop();
		case 1:
			// U30 status register
			return status_r();
		default:
			logerror("insidetrak: unhandled tms32031 ext read %08X:%08X\n", offset, mem_mask);
			return 0;
	}
}

void insidetrak_device::tms32031_ext_w(offs_t offset, uint32_t data, uint32_t mem_mask)
{
	switch (offset) {
		case 0:
			// 16-bit FIFO
			recv_fifo_push(data & 0xffff);
			// TODO irq stuff -- if (data & 0x10000) assert_irq();
			break;
		case 1:
			// U31 control register
			control_w(data & 0xff);
			break;
		case 2:
			// AD7840 14-bit, d10-d23
			// 0x1fff = +3v
			// 0x2000 = -3v
			m_ad7840 = (data >> 10) & 0x3fff;
			break;
		default:
			logerror("insidetrak: unhandled tms32031 ext write %08X:%08X, %08X\n", offset, mem_mask, data);
	}
}

uint32_t insidetrak_device::tms32031_int_r(offs_t offset, uint32_t mem_mask)
{
	uint32_t result = m_tms32031_int_regs[offset];

	switch (offset) {
		case DMA_GLOBAL_CTL:
			result = (result & ~0xc) | (m_tms32031_dma_enabled ? 0xc : 0x0);
			break;
		case TIMER0_GLOBAL_CTL:
			result = (result & ~0x8) | (recv_fifo_depth() > 0 ? 0x8 : 0x0);
			break;
		case SPORT_GLOBAL_CTL:
			result |= 0x0001; // TODO - force RRDY for now.
			break;
		case SPORT_DATA_RX:
			result = machine().rand() & 0xffff; // TODO - no real data for now
			break;
		case DMA_TRANSFER_COUNT:
		case TIMER1_COUNTER:
			 // TODO - well... we don't actually count this up
			 break;
		case TIMER1_GLOBAL_CTL:
			break;
		default:
			logerror("insidetrak: unhandled tms32031 int read %08X:%08X\n", offset, mem_mask);
	}
	return result;
}

void insidetrak_device::tms32031_int_w(offs_t offset, uint32_t data, uint32_t mem_mask)
{
	COMBINE_DATA(&m_tms32031_int_regs[offset]);

	switch (offset) {
		case DMA_GLOBAL_CTL:
		case DMA_SOURCE_ADDR:
		case DMA_DEST_ADDR:
		case DMA_TRANSFER_COUNT:
			update_tms32031_dma();
			break;
		case TIMER0_GLOBAL_CTL:
		case TIMER0_COUNTER:
		case TIMER0_PERIOD:
			update_tms32031_timer(0);
			break;
		case TIMER1_GLOBAL_CTL:
		case TIMER1_COUNTER:
		case TIMER1_PERIOD:
			update_tms32031_timer(1);
			break;
		case SPORT_TX_CTL:
			update_xf1();
			break;
		case SPORT_GLOBAL_CTL:
		case SPORT_RX_CTL:
			break;
		default:
			logerror("insidetrak: unhandled tms32031 int write %08X:%08X %08X\n", offset, mem_mask, data);
	}
}

uint8_t insidetrak_device::status_r()
{
	// U30 status register
	// bit 0 - JP7 P4 - EXT p5
	// bit 1 - JP7 P5 - EXT p9 - U37 P18
	// bit 2 - JP7 P6 - EXT p4
	// bit 3 - JP7 P7 - EXT p7
	// bit 4 - 8-bit FIFO EF-
	// bit 5 - U43 P11 - U42 P11 - JP7 P21 (pull-up)
	// bit 6 - N/C
	// bit 7 - N/C
	uint8_t ret = 0xe0;
	if (xmit_fifo_depth() > 0) ret |= 0x10;
	ret |= 0x20; // pull-up
	return ret;
}

void insidetrak_device::control_w(uint8_t data)
{
	// U31 control register
	// bit 0 - U1 P9     ; analog receiver sensor select
	// bit 1 - U9/U11 P10; analog receiver channel select bit 0
	// bit 2 - U9/U11 P9 ; analog receiver channel select bit 1
	// bit 3 - U29 P1    ; analog transmitter 1 select ; JP7 P8
	// bit 4 - U29 P16   ; analog transmitter 2 select ; JP7 P9
	// bit 5 - U29 P9    ; analog transmitter 3 select
	// bit 6 - U23 P1/P2 ; enable FSI-timer U18/U19
	// bit 7 - Q11 pn2222 npn transistor base (C=GND) - U43 P12
	m_control = data;

	m_adc_sens = m_control & 0x1;
	m_adc_recv = (m_control >> 1) & 0x3;
	m_adc_sync = (m_control >> 6) & 0x3;

	switch (m_control & 0x38) {
		case 0x38:
			m_adc_xmit = 0;
			break;
		case 0x30:
			m_adc_xmit = 1;
			break;
		case 0x28:
			m_adc_xmit = 2;
			break;
		case 0x18:
			m_adc_xmit = 3;
			break;
		default:
			m_adc_xmit = 0;
			logerror("insidetrak: unhandled transmitter selection = %02X\n", m_control & 0x38);
	}

	//logerror("insidetrak: m_control = %02X, sens = %u, recv = %u, xmit = %u, sync = %u\n", m_control, m_adc_sens, m_adc_recv, m_adc_xmit, m_adc_sync);
}

void insidetrak_device::update_xf1()
{
	int iof = m_tms32031->state_int(TMS3203X_IOF);
	int xmit_ctl = m_tms32031_int_regs[SPORT_TX_CTL];

	int val = 0;
	if (xmit_ctl & 0x0400) val |= 4; // FSX set
	if (xmit_ctl & 0x0040) val |= 2; // DX set
	if (0) val |= 1; // U32 P3 - TODO

	switch (val) {
		case 0:
		case 1:
		case 3:
		case 4:
			// XF1 = 0
			iof &= ~0x80;
			break;

		case 2:
		case 5:
		case 6:
		case 7:
			// XF1 = 1
			iof |= 0x80;
			break;
	}

	m_tms32031->set_state_int(TMS3203X_IOF, iof);
}

void insidetrak_device::recv_fifo_push(uint16_t data)
{
	if (m_recv_fifo_end >= INSIDETRAK_FIFOSIZE) return;
	m_recv_fifo[m_recv_fifo_end++] = data;
}

uint16_t insidetrak_device::recv_fifo_pop()
{
	uint16_t ret = 0xffff;
	if (m_recv_fifo_pos < m_recv_fifo_end) ret = m_recv_fifo[m_recv_fifo_pos++];
	if (m_recv_fifo_pos == m_recv_fifo_end) {
		m_recv_fifo_pos = 0;
		m_recv_fifo_end = 0;
	}
	return ret;
}

void insidetrak_device::xmit_fifo_push(uint8_t data)
{
	if (m_xmit_fifo_end >= INSIDETRAK_FIFOSIZE) return;
	m_xmit_fifo[m_xmit_fifo_end++] = data;
}

uint8_t insidetrak_device::xmit_fifo_pop()
{
	uint8_t ret = 0xff;
	if (m_xmit_fifo_pos < m_xmit_fifo_end) ret = m_xmit_fifo[m_xmit_fifo_pos++];
	if (m_xmit_fifo_pos == m_xmit_fifo_end) {
		m_xmit_fifo_pos = 0;
		m_xmit_fifo_end = 0;
	}
	return ret;
}

/*************************************************************
 *
 * internal timers
 *
 *************************************************************/
TIMER_DEVICE_CALLBACK_MEMBER( insidetrak_device::tms32031_timer_callback )
{
	int which = param;
	/* set the interrupt */
	m_tms32031->set_input_line(TMS3203X_TINT0 + which, ASSERT_LINE);
	m_tms32031_timer_enabled[which] = 0;
	update_tms32031_timer(which);
}

void insidetrak_device::update_tms32031_timer(int which)
{
	/* determine the new enabled state */
	int base = 0x10 * which;
	int enabled = ((m_tms32031_int_regs[base + TIMER0_GLOBAL_CTL] & 0xc0) == 0xc0);

	/* see if we turned on */
	if (enabled && !m_tms32031_timer_enabled[which])
	{
		int ticks = m_tms32031_int_regs[base + TIMER0_PERIOD] - m_tms32031_int_regs[base + TIMER0_COUNTER];
		attotime period = m_tms32031_h1_clock_period * (2 * ticks);
		m_tms32031_timer[which]->adjust(period, which);
	}

	/* see if we turned off */
	else if (!enabled && m_tms32031_timer_enabled[which])
	{
		m_tms32031_timer[which]->adjust(attotime::never, which);
	}

	/* set the new state */
	m_tms32031_timer_enabled[which] = enabled;
}

/*************************************************************
 *
 * internal dma
 *
 *************************************************************/
TIMER_DEVICE_CALLBACK_MEMBER( insidetrak_device::tms32031_dma_timer_callback )
{
	/* if we weren't enabled, don't do anything, just shut ourself off */
	if (!m_tms32031_dma_enabled)
	{
		if (m_tms32031_dma_timer_enabled)
		{
			m_tms32031_dma_timer->adjust(attotime::never);
			m_tms32031_dma_timer_enabled = 0;
		}
		return;
	}

	/* set the final count to 0 and the source address to the final address */
	m_tms32031_int_regs[DMA_TRANSFER_COUNT] = 0;
	m_tms32031_int_regs[DMA_SOURCE_ADDR] = param;

	/* set the interrupt */
	m_tms32031->set_input_line(TMS3203X_DINT0, ASSERT_LINE);
	m_tms32031_dma_enabled = 0;
}

void insidetrak_device::update_tms32031_dma()
{
	/* determine the new enabled state */
	int enabled = ((m_tms32031_int_regs[DMA_GLOBAL_CTL] & 3) == 3) && (m_tms32031_int_regs[DMA_TRANSFER_COUNT] != 0);

	/* see if we turned on */
	if (enabled && !m_tms32031_dma_enabled)
	{
		uint32_t dma_dat, dma_src, dma_dst, dma_inc;
		int i;

		/* make sure our assumptions are correct */
		if ((m_tms32031_int_regs[DMA_GLOBAL_CTL] & 0xffc) != 0x410)
			logerror("insidetrak: unexpected DMA transfer params %08X!\n", m_tms32031_int_regs[DMA_GLOBAL_CTL]);

		/* do the DMA up front */
		dma_src = m_tms32031_int_regs[DMA_SOURCE_ADDR];
		dma_dst = m_tms32031_int_regs[DMA_DEST_ADDR];
		dma_inc = (m_tms32031_int_regs[DMA_GLOBAL_CTL] >> 4) & 1;
		for (i = 0; i < m_tms32031_int_regs[DMA_TRANSFER_COUNT]; i++)
		{
			dma_dat = m_tms32031->space(AS_PROGRAM).read_dword(dma_src);
			dma_src += dma_inc;
			m_tms32031->space(AS_PROGRAM).write_dword(dma_dst, dma_dat);
		}

		/* compute the time of the interrupt and set the timer */
		if (!m_tms32031_dma_timer_enabled)
		{
			attotime period = m_tms32031_h1_clock_period * m_tms32031_int_regs[DMA_TRANSFER_COUNT];
			m_tms32031_dma_timer->adjust(period, dma_src, period);
			m_tms32031_dma_timer_enabled = 1;
		}
	}

	/* see if we turned off */
	else if (!enabled && m_tms32031_dma_enabled)
	{
		m_tms32031_dma_timer->reset();
		m_tms32031_dma_timer_enabled = 0;
	}

	/* set the new state */
	m_tms32031_dma_enabled = enabled;
}

/*************************************************************
 *
 * external timers
 *
 *************************************************************/
TIMER_DEVICE_CALLBACK_MEMBER( insidetrak_device::xf0_timer_callback )
{
	m_xf0_timer->adjust(attotime::from_hz(INSIDETRAK_CLOCK_10 / 24));

	int iof = m_tms32031->state_int(TMS3203X_IOF);
	iof ^= 0x08;
	m_tms32031->set_state_int(TMS3203X_IOF, iof);
}

TIMER_DEVICE_CALLBACK_MEMBER( insidetrak_device::tclk1_timer_callback )
{
	m_tclk1_timer->adjust(attotime::from_hz(INSIDETRAK_CLOCK_10 / 48));

	int t1ctl = m_tms32031_int_regs[TIMER1_GLOBAL_CTL];
	t1ctl ^= 0x08;
	m_tms32031_int_regs[TIMER1_GLOBAL_CTL] = t1ctl;
}