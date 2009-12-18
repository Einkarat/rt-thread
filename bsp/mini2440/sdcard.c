/*
 * File      : sd.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006, 2007, RT-Thread Develop Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://openlab.rt-thread.com/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2007-12-02     Yi.Qiu      the first version
 */

#include "sdcard.h"

extern rt_uint32_t PCLK;
volatile rt_uint32_t rd_cnt;
volatile rt_uint32_t wt_cnt;
volatile rt_int32_t RCA;

/**
 * This function will set a hook function, which will be invoked when a memory
 * block is allocated from heap memory.
 *
 * @param hook the hook function
 */
static void sd_delay(rt_uint32_t ms)
{
	ms *= 7326;
	while(--ms);
}

/**
 * This function will set a hook function, which will be invoked when a memory
 * block is allocated from heap memory.
 *
 * @param hook the hook function
 */
static int sd_cmd_end(int cmd, int be_resp)
{
	int finish0;

	if(!be_resp)
	{
		finish0=SDICSTA;

		while((finish0&0x800)!=0x800)
	    		finish0=SDICSTA;

		SDICSTA=finish0;

		return RT_EOK;
	}
	else
	{
		finish0=SDICSTA;

		while( !( ((finish0&0x200)==0x200) | ((finish0&0x400)==0x400) ))
		        finish0=SDICSTA;

		if(cmd==1 || cmd==41)
		{
		    	if( (finish0&0xf00) != 0xa00 )
	    		{
	        		SDICSTA=finish0;
	    			if(((finish0&0x400)==0x400))
	        			return RT_ERROR;
	    		}
	    		SDICSTA=finish0;
		}
		else
		{
		    	if( (finish0&0x1f00) != 0xa00 )
		    	{
				rt_kprintf("CMD%d:SDICSTA=0x%x, SDIRSP0=0x%x\n",
					cmd, SDICSTA, SDIRSP0);
		    		SDICSTA=finish0;
		    		if(((finish0&0x400)==0x400))
		        		return RT_ERROR;
		    	}
	    		SDICSTA=finish0;
		}
		return RT_EOK;
	}
}

/**
 * This function will set a hook function, which will be invoked when a memory
 * block is allocated from heap memory.
 *
 * @param hook the hook function
 */
static int sd_data_end(void)
{
	int finish;

	finish=SDIDSTA;

	while( !( ((finish&0x10)==0x10) | ((finish&0x20)==0x20) ))
		finish=SDIDSTA;

	if( (finish&0xfc) != 0x10 )
	{
		SDIDSTA=0xec;
		return RT_ERROR;
	}
	return RT_EOK;
}

/**
 * This function will set a hook function, which will be invoked when a memory
 * block is allocated from heap memory.
 *
 * @param hook the hook function
 */
static void sd_cmd0(void)
{
	SDICARG=0x0;
	SDICCON=(1<<8)|0x40;

	sd_cmd_end(0, 0);
}

/**
 * This function will set a hook function, which will be invoked when a memory
 * block is allocated from heap memory.
 *
 * @param hook the hook function
 */
static int sd_cmd55(void)
{
	SDICARG = RCA << 16;
	SDICCON = (0x1 << 9) | (0x1 << 8) | 0x77;

	if(sd_cmd_end(55, 1) == RT_ERROR)
		return RT_ERROR;

	return RT_EOK;
}

/**
 * This function will set a hook function, which will be invoked when a memory
 * block is allocated from heap memory.
 *
 * @param hook the hook function
 */
static void sd_sel_desel(char sel_desel)
{
	if(sel_desel)
	{
RECMDS7:
		SDICARG =RCA << 16;
		SDICCON = (0x1 << 9) | (0x1 << 8) | 0x47;

		if(sd_cmd_end(7, 1) == RT_ERROR)
	    		goto RECMDS7;

		if( SDIRSP0 & (0x1e00 != 0x800))
	    		goto RECMDS7;
	}
	else
	{
RECMDD7:
		SDICARG=0<<16;
		SDICCON=(0x1<<8)|0x47;

		if(sd_cmd_end(7, 0) == RT_ERROR)
	    		goto RECMDD7;
	}
}

/**
 * This function will set a hook function, which will be invoked when a memory
 * block is allocated from heap memory.
 *
 * @param hook the hook function
 */
static void sd_setbus(void)
{
SET_BUS:
	sd_cmd55();

	SDICARG=1<<1;
	SDICCON=(0x1<<9)|(0x1<<8)|0x46;

	if(sd_cmd_end(6, 1) == RT_ERROR)
		goto SET_BUS;
}

/**
 * This function will set a hook function, which will be invoked when a memory
 * block is allocated from heap memory.
 *
 * @param hook the hook function
 */
int sd_mmc_ocr(void)
{
	int i;

	/* Negotiate operating condition for MMC, it makes card ready state */
	for(i=0; i<100; i++)
	{
		SDICARG = 0xff8000;
		SDICCON = (0x1<<9)|(0x1<<8)|0x41;

		/* Check end of CMD1 */
		if((sd_cmd_end(1, 1) == RT_EOK) && (SDIRSP0>>16)==0x80ff)
		{
			SDICSTA=0xa00;
			return RT_EOK;
		}
	}
	SDICSTA=0xa00;

	return RT_ERROR;
}

/**
 * This function will set a hook function, which will be invoked when a memory
 * block is allocated from heap memory.
 *
 * @param hook the hook function
 */
int sd_ocr(void)
{
	int i;

	/* Negotiate operating condition for SD, it makes card ready state */
	for(i=0;i<50;i++)
	{
		sd_cmd55();

		SDICARG=0xff8000;
		SDICCON=(0x1<<9)|(0x1<<8)|0x69;

		/* if using real board, should replace code here. need to modify qemu in near future*/
		/* Check end of ACMD41 */
		//if( rt_hw_sd_cmd_end(41, 1) && SDIRSP0==0x80ff8000 )
		if( sd_cmd_end(41, 1) == RT_EOK)
		{
			SDICSTA=0xa00;
	    		return RT_EOK;
		}

		sd_delay(200);
	}
	SDICSTA=0xa00;

	return RT_ERROR;
}

/**
 * This function will set a hook function, which will be invoked when a memory
 * block is allocated from heap memory.
 *
 * @param hook the hook function
 */
rt_uint8_t sd_init(void)
{
	//-- SD controller & card initialize
	int i;
	/* Important notice for MMC test condition */
	/* Cmd & Data lines must be enabled by pull up resister */
	SDIPRE=PCLK/(INICLK)-1;
	SDICON=1;
	SDIFSTA=SDIFSTA|(1<<16);
	SDIBSIZE = 0x200;
	SDIDTIMER=0x7fffff;

	/* Wait 74SDCLK for MMC card */
	for(i=0; i<0x1000; i++);

	sd_cmd0();
	/* Check MMC card OCR */
	if(sd_mmc_ocr() == RT_EOK)
	{
		rt_kprintf("In MMC ready\n");
		goto RECMD2;
	}
	rt_kprintf("MMC check end!!\n");
	/* Check SD card OCR */
	if(sd_ocr() == RT_EOK)
	{
		rt_kprintf("In SD ready\n");
	}
	else
	{
		rt_kprintf("Initialize fail\nNo Card assertion\n");
		return RT_ERROR;
	}

RECMD2:
	SDICARG=0x0;
	SDICCON=(0x1<<10)|(0x1<<9)|(0x1<<8)|0x42;
	if(sd_cmd_end(2, 1) == RT_ERROR)
		goto RECMD2;

RECMD3:
	SDICARG=0<<16;
	SDICCON=(0x1<<9)|(0x1<<8)|0x43;
	if(sd_cmd_end(3, 1) == RT_ERROR)
		goto RECMD3;

	RCA=(SDIRSP0 & 0xffff0000 )>>16;
	SDIPRE=(PCLK/(SDCLK*2))-1;
	if( SDIRSP0 & (0x1e00!=0x600) )
		goto RECMD3;

	sd_sel_desel(1);
	sd_delay(200);
	sd_setbus();

	return RT_EOK;
}

/**
 * This function will set a hook function, which will be invoked when a memory
 * block is allocated from heap memory.
 *
 * @param hook the hook function
 */
rt_uint8_t sd_readblock(rt_uint32_t address, rt_uint8_t* buf)
{
	int status;

	rd_cnt=0;
	// SDICON |= (1<<1);
	SDIDCON = (2 << 22) | (1 << 19) | (1 << 17) | (1 << 16) | (1 << 14) | (2 << 12) | (1 << 0);
	SDICARG = address;

RERDCMD:
	SDICCON = (0x1 << 9 ) | (0x1 << 8) | 0x51;
	if(sd_cmd_end(17, 1) == RT_ERROR)
		goto RERDCMD;

	SDICSTA = 0xa00;

	while(rd_cnt < 128)
	{
		if((SDIDSTA & 0x20) == 0x20)
		{
			SDIDSTA = 0x1 << 0x5;
			break;
		}
		status = SDIFSTA;
		if((status & 0x1000) == 0x1000)
		{
#if 0
			register rt_uint32_t value;

			value = SDIDAT;

			/* swap 4 bytes */
			buf[0] = (value >> 24) & 0xff;
			buf[1] = (value >> 16) & 0xff;
			buf[2] = (value >> 8) & 0xff;
			buf[3] = value & 0xff;
#endif
			*(rt_uint32_t *)buf = SDIDAT;
			rd_cnt++;
			buf += 4;
		}
    	}
	if(sd_data_end() == RT_ERROR)
	{
		rt_kprintf("Dat error\n");
		return RT_ERROR;
	}
	SDIDSTA = 0x10;
	SDIDCON = SDIDCON &~ (7<<12);
	SDIFSTA = SDIFSTA & 0x200;
	SDIDSTA = 0x10;

	return RT_EOK;
}

/**
 * This function will set a hook function, which will be invoked when a memory
 * block is allocated from heap memory.
 *
 * @param hook the hook function
 */
rt_uint8_t sd_writeblock(rt_uint32_t address, rt_uint8_t* buf)
{
	int status;

	wt_cnt=0;
	SDIFSTA = SDIFSTA | (1 << 16);
	SDIDCON = (2 << 22) | (1 << 20) | (1 << 17) | (1 << 16) | (1 << 14) | (3 << 12) | (1 << 0);
	SDICARG = address;

REWTCMD:
	SDICCON = (0x1 << 9) | (0x1 << 8) |0x58;

	if(sd_cmd_end(24, 1) == RT_ERROR)
		goto REWTCMD;

	SDICSTA=0xa00;

    	while(wt_cnt < 128*1)
    	{
    		status = SDIFSTA;
    		if((status & 0x2000) == 0x2000)
    		{
        		SDIDAT=*(rt_uint32_t*)buf;
        		wt_cnt++;
	       	buf += 4;
    		}
    	}
	if(sd_data_end() == RT_ERROR)
	{
		rt_kprintf("Data Error\n");
		return RT_ERROR;
	}
	SDIDCON = SDIDCON &~ (7<<12);
	SDIDSTA = 0x10;

	return RT_EOK;
}

#ifdef RT_USING_DFS
/* RT-Thread Device Driver Interface */
#include <rtthread.h>

#include <dfs_fs.h>

struct rt_device sdcard_device[4];
struct dfs_partition part[4];
/**
 * This function will set a hook function, which will be invoked when a memory
 * block is allocated from heap memory.
 *
 * @param hook the hook function
 */
static rt_err_t rt_sdcard_init(rt_device_t dev)
{
	return 0;
}

/**
 * This function will set a hook function, which will be invoked when a memory
 * block is allocated from heap memory.
 *
 * @param hook the hook function
 */
static rt_err_t rt_sdcard_open(rt_device_t dev, rt_uint16_t oflag)
{
	return 0;
}

/**
 * This function will set a hook function, which will be invoked when a memory
 * block is allocated from heap memory.
 *
 * @param hook the hook function
 */
static rt_err_t rt_sdcard_close(rt_device_t dev)
{
	return 0;
}

/**
 * This function will set a hook function, which will be invoked when a memory
 * block is allocated from heap memory.
 *
 * @param hook the hook function
 */
static rt_err_t rt_sdcard_control(rt_device_t dev, rt_uint8_t cmd, void *args)
{
	return 0;
}

/**
 * This function will set a hook function, which will be invoked when a memory
 * block is allocated from heap memory.
 *
 * @param hook the hook function
 */
static rt_size_t rt_sdcard_read(rt_device_t dev, rt_off_t pos, void* buffer, rt_size_t size)
{
	int i;
	struct dfs_partition *part = (struct dfs_partition *)dev->private;

	if ( dev == RT_NULL )
	{
		rt_set_errno(-DFS_STATUS_EINVAL);
		return 0;
	}

	/* read all sectors */
	for (i = 0; i < size / SECTOR_SIZE; i ++)
	{
		rt_sem_take(part->lock, RT_WAITING_FOREVER);
		sd_readblock((part->offset + i)*SECTOR_SIZE + pos,
			(rt_uint8_t*)((rt_uint8_t*)buffer + i * SECTOR_SIZE));
		rt_sem_release(part->lock);
	}

	/* the length of reading must align to SECTOR SIZE */
	return size;
}

/**
 * This function will set a hook function, which will be invoked when a memory
 * block is allocated from heap memory.
 *
 * @param hook the hook function
 */
static rt_size_t rt_sdcard_write (rt_device_t dev, rt_off_t pos, const void* buffer, rt_size_t size)
{
	int i;
	struct dfs_partition *part = (struct dfs_partition *)dev->private;

	if ( dev == RT_NULL )
	{
		rt_set_errno(-DFS_STATUS_EINVAL);
		return 0;
	}

	/* read all sectors */
	for (i = 0; i < size / SECTOR_SIZE; i++)
	{
		rt_sem_take(part->lock, RT_WAITING_FOREVER);
		sd_writeblock((part->offset + i)*SECTOR_SIZE + pos,
			(rt_uint8_t*)((rt_uint8_t*)buffer + i * SECTOR_SIZE));
		rt_sem_release(part->lock);
	}

	/* the length of reading must align to SECTOR SIZE */
	return size;
}

/**
 * This function will register sd card to device system
 *
 * @param hook the hook function
 */
void rt_hw_sdcard_init()
{
	rt_uint8_t i, status;
	rt_uint8_t *sector;
	char dname[4];
	char sname[8];

	if (sd_init() == RT_EOK)
	{
		/* get the first sector to read partition table */
		sector = (rt_uint8_t*) rt_malloc (512);
		if (sector == RT_NULL)
		{
			rt_kprintf("allocate partition sector buffer failed\n");
			return;
		}
		status = sd_readblock(0, sector);
		if (status == RT_EOK)
		{
			for(i=0; i<4; i++)
			{
				/* get the first partition */
				status = dfs_filesystem_get_partition(&part[i], sector, i);
				if (status == RT_EOK)
				{
					rt_snprintf(dname, 4, "sd%d",  i);
					rt_snprintf(sname, 8, "sem_sd%d",  i);
					part[i].lock = rt_sem_create(sname, 1, RT_IPC_FLAG_FIFO);

					/* register sdcard device */
					sdcard_device[i].init = rt_sdcard_init;
					sdcard_device[i].open = rt_sdcard_open;
					sdcard_device[i].close = rt_sdcard_close;
					sdcard_device[i].read = rt_sdcard_read;
					sdcard_device[i].write = rt_sdcard_write;
					sdcard_device[i].control = rt_sdcard_control;
					sdcard_device[i].private= &part[i];

					rt_device_register(&sdcard_device[i], dname,
						RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_REMOVABLE | RT_DEVICE_FLAG_STANDALONE);
				}
				else
				{
					if(i == 0)
					{
						/* there is no partition table */
						part[0].offset = 0;
						part[0].size   = 0;
						part[0].lock = rt_sem_create("sem_sd0", 1, RT_IPC_FLAG_FIFO);

						/* register sdcard device */
						sdcard_device[0].init = rt_sdcard_init;
						sdcard_device[0].open = rt_sdcard_open;
						sdcard_device[0].close = rt_sdcard_close;
						sdcard_device[0].read = rt_sdcard_read;
						sdcard_device[0].write = rt_sdcard_write;
						sdcard_device[0].control = rt_sdcard_control;
						sdcard_device[0].private= &part[0];

						rt_device_register(&sdcard_device[0], "sd0",
							RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_REMOVABLE | RT_DEVICE_FLAG_STANDALONE);

						break;
					}
				}
			}
		}
		else
		{
			rt_kprintf("read sdcard first sector failed\n");
		}

		/* release sector buffer */
		rt_free(sector);

		return;
	}
	else
	{
		rt_kprintf("sdcard init failed\n");
	}
}

#endif
