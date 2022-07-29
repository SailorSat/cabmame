// license:BSD-3-Clause
// copyright-holders:Carl

#include "emu.h"
#include "mcd2.h"
#include "coreutil.h"
#include "speaker.h"

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

DEFINE_DEVICE_TYPE(MCD, mcd2_device, "mcd_isa", "Mitsumi CD-ROM v2")

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

void mcd2_device::device_add_mconfig(machine_config &config)
{
	SPEAKER(config, "lheadphone").front_left();
	SPEAKER(config, "rheadphone").front_right();
	CDDA(config, m_cdda);
	m_cdda->add_route(0, "lheadphone", 1.0);
	m_cdda->add_route(1, "rheadphone", 1.0);
}

//-------------------------------------------------
//  mcd2_device - constructor
//-------------------------------------------------

mcd2_device::mcd2_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	cdrom_image_device(mconfig, MCD, tag, owner, clock),
	m_irq_cb(*this),
	m_drq_cb(*this),
	m_cdda(*this, "cdda")
{
	set_interface("cdrom");
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void mcd2_device::device_start()
{
	m_irq_cb.resolve_safe();
	m_drq_cb.resolve_safe();

	cdrom_image_device::device_start();
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void mcd2_device::device_reset()
{
	m_irq_cb(CLEAR_LINE);

	m_stat = m_cdrom_handle ? STAT_READY | STAT_CHANGE : 0;
	m_cmdrd_count = 0;
	m_cmdbuf_count = 0;
	m_buf_count = 0;
	m_curtoctrk = 0;
	m_dma = 0;
	m_irq = 0;
	m_conf = 0;
	m_dmalen = 2048;
	m_locked = false;
	m_change = true;
	m_data = false;
	m_delay = 0;
}

bool mcd2_device::read_sector(bool first)
{
	uint32_t lba = cdrom_file::msf_to_lba(m_readmsf);
	if(m_drvmode == DRV_MODE_CDDA)
	{
		if(m_cdrom_handle->get_track_type(m_cdrom_handle->get_track(lba)) == cdrom_file::CD_TRACK_AUDIO)
		{
			m_cdda->stop_audio();
			m_cdda->set_cdrom(m_cdrom_handle);
			m_cdda->start_audio(lba, m_readcount);
			return true;
		}
		else
			m_drvmode = DRV_MODE_READ;
	}
	if((m_irq & IRQ_DATACOMP) && !first)
		m_irq_cb(ASSERT_LINE);
	if(!m_readcount)
	{
		m_drq_cb(CLEAR_LINE);
		m_data = false;
		return false;
	}
	m_cdda->stop_audio();
	m_cdrom_handle->read_data(lba - 150, m_buf, m_mode & 0x40 ? cdrom_file::CD_TRACK_MODE1_RAW : cdrom_file::CD_TRACK_MODE1);
	if(m_mode & 0x40)
	{
		//correct the header
		m_buf[12] = dec_2_bcd((m_readmsf >> 16) & 0xff);
		m_buf[13] = dec_2_bcd((m_readmsf >> 8) & 0xff);
	}
	m_readmsf = cdrom_file::lba_to_msf_alt(lba + 1);
	m_buf_count = m_dmalen;// + 1
	if (m_mode & 0x80) // test mode
		m_buf_count += 2;
	m_buf_idx = 0;
	m_data = true;
	m_readcount--;
	if(m_dma)
		m_drq_cb(ASSERT_LINE);
	if((m_irq & IRQ_DATAREADY) && first)
		m_irq_cb(ASSERT_LINE);
	return true;
}

uint8_t mcd2_device::flag_r()
{
	if (m_delay)
	{
		m_delay--;
		return 0xef;
	}

	uint8_t ret = 0xe9;
	if (!machine().side_effects_disabled())
		m_irq_cb(CLEAR_LINE);
	if(!m_buf_count || !m_data || m_dma) // if dma enabled the cpu will never not see that flag as it will be halted
		ret |= FLAG_NO_DATA;
	if(!m_cmdbuf_count || m_buf_count)
		ret |= FLAG_NO_STAT; // all command results are status
	return ret;
}

uint8_t mcd2_device::data_r()
{
	if (m_delay)
	{
		m_delay--;
		return 0xff;
	}

	// data buffer has priority
	if(m_buf_count)
	{
		uint8_t ret = m_buf_idx < 2352 ? m_buf[m_buf_idx] : 0;
		if(!machine().side_effects_disabled())
		{
			m_buf_idx++;
			m_buf_count--;
			logerror("m_buf_count = %04x\n", m_buf_count);
			if(!m_buf_count)
				read_sector();
		}
		return ret;
	}
	else if(m_cmdbuf_count)
	{
		if(machine().side_effects_disabled())
			return m_cmdbuf[m_cmdbuf_idx];
		else
		{
			m_cmdbuf_count--;
			logerror("m_cmdbbuf_count = %04x\n", m_cmdbuf_count);
			return m_cmdbuf[m_cmdbuf_idx++];
		}
	}
	return 0xff;
}

uint16_t mcd2_device::dack16_r(int line)
{
	if(m_buf_count & ~1)
	{
		uint16_t ret = 0;
		if(m_buf_idx < 2351)
		{
			ret = m_buf[m_buf_idx++];
			ret |= (m_buf[m_buf_idx++] << 8);
		}
		m_buf_count -= 2;
		if(!m_buf_count)
			read_sector();
		return ret;
	}
	return 0;
}

void mcd2_device::reset_w(uint8_t data)
{
	reset();
}

void mcd2_device::ctrl_w(uint8_t data)
{
}

void mcd2_device::cfg_w(uint8_t data)
{
}

void mcd2_device::cmd_w(uint8_t data)
{
	if(m_cmdrd_count)
	{
		m_cmdrd_count--;
		switch(m_cmd)
		{
			case CMD_SET_MODE:
				m_mode = data;
				m_cmdbuf[1] = 0;
				m_cmdbuf_count = 2;
				break;
			case CMD_LOCK:
				m_locked = data & 1 ? true : false;
				m_cmdbuf[1] = 0;
				m_cmdbuf_count = 2;
				break;
			case CMD_CONFIG:
				switch(m_cmdrd_count)
				{
					case 1:
						if(m_conf == 1)
						{
							m_dmalen = data << 8;
							break;
						}
						m_conf = data;
						if(m_conf == 1)
							m_cmdrd_count++;
						break;
					case 0:
						switch(m_conf)
						{
							case 0x10:
								m_irq = data;
								break;
							case 0x01:
								//m_dmalen |= data; hm... 0801 seems to be a strange buffer size
								break;
							case 0x02:
								m_dma = data;
								break;
						}
						m_cmdbuf[1] = 0;
						m_cmdbuf_count = 2;
						m_conf = 0;
						break;
				}
				break;
			case CMD_READ1X:
			case CMD_READ2X:
				switch(m_cmdrd_count)
				{
					case 5:
						m_readmsf = 0;
						[[fallthrough]];
					case 4:
					case 3:
						m_readmsf |= bcd_2_dec(data) << ((m_cmdrd_count - 3) * 8);
						break;
					case 2:
						m_readcount = data << 16;
						break;
					case 1:
						m_readcount |= data << 8;
						break;
					case 0:
						m_readcount |= data;
						read_sector(true);
						m_cmdbuf_count = 1;
						m_cmdbuf[0] = STAT_SPIN | STAT_READY;
						break;
				}
				break;
		}
		if(!m_cmdrd_count)
			m_stat = m_cdrom_handle ? (STAT_READY | (m_change ? STAT_CHANGE : 0)) : 0;
		return;
	}
	m_cmd = data;
	m_cmdbuf_idx = 0;
	m_cmdrd_count = 0;
	m_cmdbuf_count = 1;
	m_cmdbuf[0] = m_cdrom_handle ? (STAT_READY | (m_change ? STAT_CHANGE : 0)) : 0;
	m_data = false;
	switch(data)
	{
		case CMD_GET_VOL_INFO:
			if(m_cdrom_handle)
			{
				uint32_t first = cdrom_file::lba_to_msf(150), last = cdrom_file::lba_to_msf(m_cdrom_handle->get_track_start(0xaa) + 150);
				m_cmdbuf[1] = 1;
				m_cmdbuf[2] = dec_2_bcd(m_cdrom_handle->get_last_track());
				m_cmdbuf[3] = (last >> 16) & 0xff;
				m_cmdbuf[4] = (last >> 8) & 0xff;
				m_cmdbuf[5] = last & 0xff;
				m_cmdbuf[6] = (first >> 16) & 0xff;
				m_cmdbuf[7] = (first >> 8) & 0xff;
				m_cmdbuf[8] = first & 0xff;
				m_cmdbuf_count = 9;
				m_readcount = 0;
			}
			else
			{
				m_cmdbuf_count = 1;
				m_cmdbuf[0] = STAT_CMD_CHECK;
			}
			break;
		case CMD_GET_DISK_INFO:
			if(m_cdrom_handle)
			{
				m_cmdbuf[1] = 0;
				m_cmdbuf[2] = 0;
				m_cmdbuf[3] = 0;
				m_cmdbuf[4] = 0;
				m_cmdbuf_count = 5;
				m_readcount = 0;
			}
			else
			{
				m_cmdbuf_count = 1;
				m_cmdbuf[0] = STAT_CMD_CHECK;
			}
			break;
		case CMD_GET_Q:
			if(m_cdrom_handle)
			{
				if (m_mode & MODE_GET_TOC)
				{
					// toc; note - we are offset by 150!
					int tracks = m_cdrom_handle->get_last_track();
					if(m_curtoctrk < 0)
					{
						// meta data tracks
						uint32_t start = cdrom_file::lba_to_msf(m_cdrom_handle->get_track_start(0xaa) + 150);
						uint8_t audio = m_cdrom_handle->get_track_type(tracks) == cdrom_file::CD_TRACK_AUDIO;
						uint32_t len = cdrom_file::lba_to_msf(0x20 + m_curtoctrk);
						m_cmdbuf[0] = STAT_SPIN | STAT_READY | (audio ? STAT_PLAY_CDDA | STAT_DISK_CDDA : 0);
						m_cmdbuf[1] = ((m_cdrom_handle->get_adr_control(tracks) << 4) & 0xf0) | 0x01;
						m_cmdbuf[2] = 0; // track num except when reading toc
						m_cmdbuf[3] = m_curtoctrk + 0xA3; // index
						m_cmdbuf[4] = (len >> 16) & 0xff;
						m_cmdbuf[5] = (len >> 8) & 0xff;
						m_cmdbuf[6] = len & 0xff;
						m_cmdbuf[7] = 0;
						switch (m_curtoctrk)
						{
							case -3: // first track
							case -2: // last track
								m_cmdbuf[8] = (m_curtoctrk == -3 ? 0x01 : dec_2_bcd(tracks));
								m_cmdbuf[9] = 0;
								m_cmdbuf[10] = 0;
								break;
							case -1:
								// lead out
								m_cmdbuf[8] = (start >> 16) & 0xff;
								m_cmdbuf[9] = (start >> 8) & 0xff;
								m_cmdbuf[10] = start & 0xff;
								break;
						}
					}
					else
					{
						// actual tracks
						const auto &toc = m_cdrom_handle->get_toc();
						uint32_t start = toc.tracks[m_curtoctrk].logframeofs;
						uint32_t len = toc.tracks[m_curtoctrk].logframes;
						logerror("tracks = %02x, m_curtoctrk = %02x, start = %04x, len = %04x\n", tracks, m_curtoctrk, start, len);
						len = cdrom_file::lba_to_msf(len);
						start = cdrom_file::lba_to_msf(start + 150);
						uint8_t audio = m_cdrom_handle->get_track_type(m_curtoctrk) == cdrom_file::CD_TRACK_AUDIO;
						m_cmdbuf[0] = STAT_SPIN | STAT_READY | (audio ? STAT_PLAY_CDDA | STAT_DISK_CDDA : 0);
						m_cmdbuf[1] = ((m_cdrom_handle->get_adr_control(m_curtoctrk) << 4) & 0xf0) | 0x01;
						m_cmdbuf[2] = 0; // track num except when reading toc
						m_cmdbuf[3] = dec_2_bcd(m_curtoctrk + 1); // index
						m_cmdbuf[4] = (len >> 16) & 0xff;
						m_cmdbuf[5] = (len >> 8) & 0xff;
						m_cmdbuf[6] = len & 0xff;
						m_cmdbuf[7] = 0;
						m_cmdbuf[8] = (start >> 16) & 0xff;
						m_cmdbuf[9] = (start >> 8) & 0xff;
						m_cmdbuf[10] = start & 0xff;
					}
					m_curtoctrk++;
					if(m_curtoctrk >= tracks)
						m_curtoctrk = -3; // add meta tracks
					m_cmdbuf_count = 11;
					m_readcount = 0;
				}
				else
				{
					// none toc; note - we are offset by 150
					uint32_t absolute = m_readmsf - 1;
					uint32_t lba = cdrom_file::msf_to_lba(absolute) - 150;
					int cur_track = m_cdrom_handle->get_track(lba);
					uint32_t start = m_cdrom_handle->get_track_start(cur_track);
					uint32_t relative = lba - start;
					logerror("lba = %04x, cur_track = %02x, start = %04x, relative = %04x\n", lba, cur_track, start, relative);
					relative = cdrom_file::lba_to_msf(relative);
					m_cmdbuf[0] = STAT_SPIN | STAT_READY;
					m_cmdbuf[1] = ((m_cdrom_handle->get_adr_control(cur_track) << 4) & 0xf0) | ((cur_track + 1) & 0x0f);
					m_cmdbuf[2] = cur_track + 1;            // track num
					m_cmdbuf[3] = dec_2_bcd(cur_track + 1); // index
					m_cmdbuf[4] = (relative >> 16) & 0xff;
					m_cmdbuf[5] = (relative >> 8) & 0xff;
					m_cmdbuf[6] = relative & 0xff;
					m_cmdbuf[7] = 0;
					m_cmdbuf[8] = (absolute >> 16) & 0xff;
					m_cmdbuf[9] = (absolute >> 8) & 0xff;
					m_cmdbuf[10] = absolute & 0xff;
					m_cmdbuf_count = 11;
					m_readcount = 0;
				}
			}
			else
			{
				m_cmdbuf_count = 1;
				m_cmdbuf[0] = STAT_CMD_CHECK;
			}
			break;
		case CMD_GET_STAT:
			m_change = false;
			m_cmdbuf[0] = m_stat | (m_cdda->audio_active() ? STAT_SPIN | STAT_DISK_CDDA | STAT_PLAY_CDDA : 0);
			break;
		case CMD_SET_MODE:
			m_cmdrd_count = 1;
			break;
		case CMD_SOFT_RESET:
			m_change = true;
			m_cmdbuf[0] = 0x60;
			m_delay = 0x8;
			break;
		case CMD_STOPCDDA:
		case CMD_STOP:
			m_cdda->stop_audio();
			m_drvmode = DRV_MODE_STOP;
			m_curtoctrk = 0;
			break;
		case CMD_CONFIG:
			m_cmdrd_count = 2;
			break;
		case CMD_READ1X:
		case CMD_READ2X:
			if(m_cdrom_handle)
			{
				m_readcount = 0;
				m_drvmode = data == CMD_READ1X ? DRV_MODE_CDDA : DRV_MODE_READ;
				m_cmdrd_count = 6;
			}
			else
			{
				m_cmdbuf_count = 1;
				m_cmdbuf[0] = STAT_CMD_CHECK;
			}
			break;
		case CMD_GET_VER:
			m_cmdbuf[0] = 0x40;
			m_cmdbuf[1] = 'D'; // double speed
			m_cmdbuf[2] = 2; // version 2
			m_cmdbuf_count = 3;
			break;
		case CMD_EJECT:
			m_readcount = 0;
			break;
		case CMD_LOCK:
			m_cmdrd_count = 1;
			break;
		default:
			m_cmdbuf[0] = m_stat | STAT_CMD_CHECK;
			break;
	}
}
