// license:BSD-3-Clause
// copyright-holders:Philip Bennett
/***************************************************************************

    Virtuality SU2000 (Stand Up) and SD2000 (Sit Down) hardware

    preliminary driver by Phil Bennett

   Known SU2000 games:
        * Boxing
        * Dactyl Nightmare SP
        * Dactyl Nightmare 2 - Race For The Eggs!
        * PacManVR
        * Shoot For Loot
        * Sphere
        * Zone Hunter

    Known SD2000 games:
        * Buggy Ball
        * Ghost Train
        * Missile Command VR
        * X-Treme Strike

   SU2000 Hardware Info sources:
        An owner's breakdown of system: http://arianchen.de/su2000/status.html
        Technical Manual: http://arianchen.de/su2000/files/su2000tech.pdf

   SU2000 and SD2000 share the same base hardware:
        -Intel 486 DX-33 processor
        -8MB RAM
        -customised `Expality PIX 1000` card with 16MB Video RAM
         (has Dual Motorola MC88110 CPU, and each game comes with independent firmware for both CPUs)


    TODO:
        * Get system booting
        * Add game software
        * Fix i386 core issues (e.g. protected mode)
        * Write MC88110 CPU core
        * Emulate HDD and CD drive
        * Emulate PIX 1000 card
        * Emulate VID 1000 card
        * Emulate tracker card
        * Emulate format card

***************************************************************************/


#include "emu.h"

#include "bus/isa/isa_cards.h"
#include "bus/pc_kbd/keyboards.h"
#include "bus/pc_kbd/pc_kbdc.h"

#include "cpu/i386/i386.h"

#include "machine/at.h"
#include "machine/ds128x.h"
#include "machine/nvram.h"
#include "machine/ram.h"


namespace {

/*************************************
 *
 *  Defines
 *
 *************************************/

#define I486_CLOCK          33000000


/*************************************
 *
 *  State
 *
 *************************************/

class su2000_state : public driver_device
{
public:
	su2000_state(const machine_config &mconfig, device_type type, const char *tag) :
			driver_device(mconfig, type, tag),
			m_maincpu(*this, "maincpu"),
			m_mb(*this, "mb"),
			m_ram(*this, RAM_TAG)
		{ }

	void su2000(machine_config &config);
	void init_at();

protected:
	required_device<cpu_device> m_maincpu;
	required_device<at_mb_device> m_mb;
	required_device<ram_device> m_ram;

	void init_at_common(int xmsbase);

	void at32_io(address_map &map) ATTR_COLD;
	void at32_map(address_map &map) ATTR_COLD;
};


/*************************************
 *
 *  Main CPU Memory Map
 *
 *************************************/

void su2000_state::at32_map(address_map &map)
{
	map.unmap_value_low();
	map(0x00000000, 0x0009ffff).bankrw("bank10");
	map(0x000c0000, 0x000c7fff).rom().region("vgax", 0);
	map(0x000f0000, 0x000fffff).rom().region("bios", 0);
	map(0x00800000, 0x00800bff).ram().share("nvram");
	map(0xffff0000, 0xffffffff).rom().region("bios", 0);
}

void su2000_state::at32_io(address_map &map)
{
	map.unmap_value_high();
	map(0x0000, 0x00ff).m(m_mb, FUNC(at_mb_device::map));
}


/*************************************
 *
 *  Inputs
 *
 *************************************/


/*************************************************************
 *
 * IDE
 *
 *************************************************************/

#if 0
static void ide_interrupt(device_t *device, int state)
{
	su2000_state *drvstate = device->machine().driver_data<su2000_state>();
	pic8259_ir6_w(drvstate->m_pic8259_2, state);
}
#endif


/*************************************
 *
 *  Machine Configuration
 *
 *************************************/

void su2000_state::su2000(machine_config &config)
{
	i486_device &maincpu(I486(config, m_maincpu, I486_CLOCK));
	maincpu.set_addrmap(AS_PROGRAM, &su2000_state::at32_map);
	maincpu.set_addrmap(AS_IO, &su2000_state::at32_io);
	maincpu.set_irq_acknowledge_callback("mb:pic8259_master", FUNC(pic8259_device::inta_cb));

	config.set_maximum_quantum(attotime::from_hz(60));

	AT_MB(config, m_mb);
	m_mb->kbd_clk().set("kbd", FUNC(pc_kbdc_device::clock_write_from_mb));
	m_mb->kbd_data().set("kbd", FUNC(pc_kbdc_device::data_write_from_mb));

	NVRAM(config, "nvram", nvram_device::DEFAULT_ALL_0);

	// on-board devices on SBC
	ISA16_SLOT(config, "board1", 0, "mb:isabus", pc_isa16_cards, "fdcsmc", true); // FIXME: deteremine ISA bus clock
	ISA16_SLOT(config, "board2", 0, "mb:isabus", pc_isa16_cards, "comat", true);
	ISA16_SLOT(config, "board3", 0, "mb:isabus", pc_isa16_cards, "ide", true);
	ISA16_SLOT(config, "board4", 0, "mb:isabus", pc_isa16_cards, "lpt", true);
	// ISA slots
	ISA16_SLOT(config, "isa1", 0, "mb:isabus", pc_isa16_cards, "svga_et4k", false);
	ISA16_SLOT(config, "isa2", 0, "mb:isabus", pc_isa16_cards, "enet16", false);
	ISA16_SLOT(config, "isa3", 0, "mb:isabus", pc_isa16_cards, "formatc", false);
	ISA16_SLOT(config, "isa4", 0, "mb:isabus", pc_isa16_cards, "insidetrak", false);
	ISA16_SLOT(config, "isa5", 0, "mb:isabus", pc_isa16_cards, "pix1000", false);
	ISA16_SLOT(config, "isa6", 0, "mb:isabus", pc_isa16_cards, "vid1000", false);

	pc_kbdc_device &pc_kbdc(PC_KBDC(config, "kbd", pc_at_keyboards, STR_KBD_MICROSOFT_NATURAL));
	pc_kbdc.out_clock_cb().set(m_mb, FUNC(at_mb_device::kbd_clk_w));
	pc_kbdc.out_data_cb().set(m_mb, FUNC(at_mb_device::kbd_data_w));

	ds12885_device &rtc(DS12885(config.replace(), "mb:rtc"));
	rtc.irq().set("mb:pic8259_slave", FUNC(pic8259_device::ir0_w));
	rtc.set_century_index(0x32);

	/* internal ram */
	RAM(config, m_ram).set_default_size("4M");
}


/**********************************************************
 *
 * Init functions
 *
 **********************************************************/

void su2000_state::init_at_common(int xmsbase)
{
	address_space& space = m_maincpu->space(AS_PROGRAM);

	/* managed RAM */
	membank("bank10")->set_base(m_ram->pointer());

	if (m_ram->size() > xmsbase)
	{
		offs_t ram_limit = 0x100000 + m_ram->size() - xmsbase;
		space.install_ram(0x100000,  ram_limit - 1, m_ram->pointer() + xmsbase);
	}
}

void su2000_state::init_at()
{
	init_at_common(0xa0000);
}


/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( su2000 )
	ROM_REGION32_LE(0x010000, "bios", 0)
	//ROM_LOAD("amibios_486dx_isa_bios.u32",  0x00000, 0x10000, CRC(811d3639) SHA1(a64d6026c16ac8c79f22b2c241f149402449fafb) )
	ROM_LOAD("amibios_486dx_isa_bios_aa4025963.bin", 0x00000, 0x10000, CRC(65558d9e) SHA1(2e2840665d069112a2c7169afec687ad03449295) )
	//ROM_LOAD("amibios-486dx-1992-aa8707058-m27c512.bin", 0x00000, 0x10000, CRC(a2b3e326) SHA1(b29c5668fb3337893ef3a96f053f90b929bac0d6) )

	ROM_REGION32_LE(0x008000, "vgax", 0)
	ROM_LOAD("hmc86304_bios_v208.u7",       0x00000, 0x08000, CRC(c6c32f1a) SHA1(a07ade7f0567e1978cd8cee73c6a5d7b5e5f947f) )

	ROM_REGION(0x8000, "pals", 0)
	/**********/
	/* Format */
	/**********/

	/* GAL20V8A */
	ROM_LOAD("wfc065a.u65",                 0x00000, 0x00100, NO_DUMP )

	/* PALCE20V8H */
	ROM_LOAD("wfc066a.u66",                 0x00000, 0x00100, NO_DUMP )
	ROM_LOAD("wfc067b.u67",                 0x00000, 0x00100, NO_DUMP )

	/* GAL16V8B */
	ROM_LOAD("wfc071a.u71",                 0x00000, 0x00100, NO_DUMP )
	ROM_LOAD("wfc014a.u14",                 0x00000, 0x00100, NO_DUMP )
	ROM_LOAD("wfc072a.u72",                 0x00000, 0x00100, NO_DUMP )

	/***********/
	/* PIX1000 */
	/***********/

	/* PALCE16V8H */
	ROM_LOAD("wi01p006.u6",                 0x00000, 0x00892, CRC(528142eb) SHA1(2c152c6540f3d38cb881296decba957f8b5c7c58) )
	ROM_LOAD("wi01p026.u26",                0x00000, 0x00892, CRC(528142eb) SHA1(2c152c6540f3d38cb881296decba957f8b5c7c58) )
	ROM_LOAD("wi01p044.u44",                0x00000, 0x00892, CRC(a2ad9cf8) SHA1(67d44807f7346baf87c15d35c27458b0a7d5de89) )
	ROM_LOAD("wi01p045.u45",                0x00000, 0x00892, CRC(e2a13df7) SHA1(eaa5ee3a92d11f0722934cb6d3443ead903dc78f) )
	ROM_LOAD("wi01p046.u46",                0x00000, 0x00892, CRC(4c22fba5) SHA1(6fabc7a0790f8b883b83fa6fe5f1ee1302d284e2) )
	ROM_LOAD("wi01p050.u50",                0x00000, 0x00892, CRC(7cb0931f) SHA1(394c100b33e5b8d1b2f4979399ad0791af39da28) )
	ROM_LOAD("wi01p053.u53",                0x00000, 0x00892, CRC(d2eb2493) SHA1(a66b4733da0f4f0869839630063c52a733f5ea8c) )
	ROM_LOAD("wi01p054.u54",                0x00000, 0x00892, CRC(96d3506e) SHA1(682ba8a7a17176ff41047cd9a9cfefdc2dc0fd64) )
	ROM_LOAD("wi01p056.u56",                0x00000, 0x00892, CRC(8bf57db4) SHA1(1a5315397b1903c36d7450e873603584db8a00e8) )
	ROM_LOAD("wi01p058.u58",                0x00000, 0x00892, CRC(882f6405) SHA1(e3b7c278f4ee9e0e11741d9ed7c5b32978a3d185) )
	ROM_LOAD("wi01p067.u67",                0x00000, 0x00892, CRC(a97cff8e) SHA1(78451a4bffecd4e1655e7a89971e28b47f5c8829) )
	ROM_LOAD("wi01p075.u75",                0x00000, 0x00892, CRC(3d1adb17) SHA1(b1af8c069de3df3c757e211b372f535b513c6326) )
	ROM_LOAD("wi01p079.u79",                0x00000, 0x00892, CRC(3d3ad152) SHA1(a769bc80514873c2f238e6c8842c5b23596b4a42) )
	ROM_LOAD("wi01p080.u80",                0x00000, 0x00892, CRC(657a4495) SHA1(c5bbd5e5f065a99f9f3d7ce89ca833779c338da5) )

	/* PALCE20V8H */
	ROM_LOAD("wi01p048.u48",                0x00000, 0x00a92, CRC(b9958131) SHA1(058203eefb1645a446ff3ff4a170a673ad65076c) )
	ROM_LOAD("wi01p049.u49",                0x00000, 0x00a92, CRC(820d245a) SHA1(8690e6eb46617c3dc78e1b233970a9318101101c) )
	ROM_LOAD("wi01p070.u70",                0x00000, 0x00a92, CRC(4322291e) SHA1(e8751b585a4fb3f371e67616abf48c9d23847b77) )
	ROM_LOAD("wi01p072.u72",                0x00000, 0x00a92, CRC(fe77b66b) SHA1(aa13619346a566613f0fa9e3723419f65ffd3ea8) )
	ROM_LOAD("wi01p073.u73",                0x00000, 0x00a92, CRC(e02fe7c1) SHA1(ca22b30391453daae943007a0c04b5a28a958c18) )
	ROM_LOAD("wi01p074.u74",                0x00000, 0x00a92, CRC(fca2823c) SHA1(6628f1ba3827a3d333ed88c5f20ca94a1439c75e) )
	ROM_LOAD("wi01p083.u83",                0x00000, 0x00a92, CRC(902731aa) SHA1(862c2e6fb75fbd712aeaf9eb3b8a84210332fa66) )
	ROM_LOAD("wi01p092.u92",                0x00000, 0x00a92, CRC(f996657e) SHA1(01c2c774a9d5f4bcb78742cd606864eff512bfce) )

	/* PALCE22V10 */
	ROM_LOAD("wi01p047.u47",                0x00000, 0x00100, NO_DUMP )
	ROM_LOAD("wi01p077.u77",                0x00000, 0x00100, NO_DUMP )
	ROM_LOAD("wi01p087.u87",                0x00000, 0x00100, NO_DUMP )
	ROM_LOAD("wi01p025.u25",                0x00000, 0x00100, NO_DUMP )

	/***********/
	/* Tracker */
	/***********/

	/* TIBPAL16L8 */
	ROM_LOAD("u37 335d.u37",                0x00000, 0x00100, NO_DUMP )
	ROM_LOAD("u40 4116.u40",                0x00000, 0x00100, NO_DUMP )

	/***********/
	/* VID1000 */
	/***********/

	/* PALCE20V8H */
	ROM_LOAD("wi02p000.u23",                0x00000, 0x00a92, CRC(bab34c4b) SHA1(a3f4f17a122cc0d063cb700b3d183b894d857cb4) )
	ROM_LOAD("wi02p001.u32",                0x00000, 0x00a92, CRC(b438ce94) SHA1(f36c681b3d09835393a8be5079263915d3480b88) )
	ROM_LOAD("wi02p002.u22",                0x00000, 0x00a92, CRC(bab34c4b) SHA1(a3f4f17a122cc0d063cb700b3d183b894d857cb4) )
	ROM_LOAD("wi02p003.u31",                0x00000, 0x00a92, CRC(b438ce94) SHA1(f36c681b3d09835393a8be5079263915d3480b88) )
	ROM_LOAD("wi02p004.u6",                 0x00000, 0x00a92, CRC(bab34c4b) SHA1(a3f4f17a122cc0d063cb700b3d183b894d857cb4) )
	ROM_LOAD("wi02p005.u14",                0x00000, 0x00a92, CRC(b438ce94) SHA1(f36c681b3d09835393a8be5079263915d3480b88) )
	ROM_LOAD("wi02p006.u5",                 0x00000, 0x00a92, CRC(bab34c4b) SHA1(a3f4f17a122cc0d063cb700b3d183b894d857cb4) )
	ROM_LOAD("wi02p007.u13",                0x00000, 0x00a92, CRC(b438ce94) SHA1(f36c681b3d09835393a8be5079263915d3480b88) )

	/* PALCE16V8H */
	ROM_LOAD("wi02p008.u51",                0x00000, 0x00892, CRC(7c421890) SHA1(49d0ab674a8cb4e18ae5ba570132111ed3ccd546) )
	ROM_LOAD("wi02p009.u49",                0x00000, 0x00892, CRC(ff63d60c) SHA1(8ee8629cc24cd18b944ffab3830ed474a1189179) )
	ROM_LOAD("wi02p010.u72",                0x00000, 0x00892, CRC(68f48053) SHA1(17ae9f5caa4b2f4ff700ce44294d3d2881bd4e62) )
	ROM_LOAD("wi02p011.u88",                0x00000, 0x00892, CRC(78701d0f) SHA1(089d5df945638cb811caa963430f5d6658c68947) )
	ROM_LOAD("wi02p012.u85",                0x00000, 0x00892, CRC(1c489ff9) SHA1(a0e1c00c9a5dd870b84f869070961d25724cea92) )
	ROM_LOAD("wi02p013.u86",                0x00000, 0x00892, CRC(19de958f) SHA1(a160f6b0e2a7e42457b470dbcf9415feaad880aa) )
	ROM_LOAD("wi02p014.u82",                0x00000, 0x00892, CRC(845e4d48) SHA1(6aaeabad0e86fe480773b4b51ca4f7c1c8935993) )
	ROM_LOAD("wi02p015.u74",                0x00000, 0x00892, CRC(a10d1876) SHA1(4091fa0a3abb2baeabcc6b250342cf29829af21a) )
	ROM_LOAD("wi02p016.u52",                0x00000, 0x00892, CRC(d84b58c2) SHA1(087b5a56ddb6bef6b1bc93b9a8e0d23de27aa399) )
ROM_END

} // anonymous namespace


/*************************************
 *
 *  Game driver(s)
 *
 *************************************/

GAME( 1993, su2000, 0, su2000, 0, su2000_state, init_at, ROT0, "Virtuality", "SU2000", MACHINE_IS_BIOS_ROOT | MACHINE_NOT_WORKING | MACHINE_NO_SOUND )
