/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *  Misc system stuff needed by Doom, implemented for Linux.
 *  Mainly timer handling, and ENDOOM/ENDBOOM.
 *
 *-----------------------------------------------------------------------------
 */

#define WAD_SDCARD

#include <stdio.h>

#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#ifdef _MSC_VER
#define    F_OK    0    /* Check for file existence */
#define    W_OK    2    /* Check for write permission */
#define    R_OK    4    /* Check for read permission */
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#endif
#include <sys/stat.h>



#include "config.h"
#include <unistd.h>
#include <sched.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#include "m_argv.h"
#include "lprintf.h"
#include "doomtype.h"
#include "doomdef.h"
#include "lprintf.h"
#include "m_fixed.h"
#include "r_fps.h"
#include "i_system.h"
#include "i_joy.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_partition.h"
#include "esp_spi_flash.h"

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "esp_timer.h"

#ifdef __GNUG__
#pragma implementation "i_system.h"
#endif
#include "i_system.h"

#include <sys/time.h>

#define PIN_NUM_MISO 2
#define PIN_NUM_MOSI 15
#define PIN_NUM_CLK  14
#define PIN_NUM_CS   13

enum wad_source_e {
    WAD_SOURCE_INTERNALFLASH,
    WAD_SOURCE_SDCARD
};

static uint8_t WAD_source = WAD_SOURCE_INTERNALFLASH;

void I_InitStorage() {
    if(I_InitSD()) {
        WAD_source = WAD_SOURCE_SDCARD;
    } else {
        /* ToDo: implement dynamic fallback if the SD fails */
        lprintf(LO_WARN, "Falling back to internal flash as WAD source\n");
    }
}

bool I_InitSD() {
	lprintf(LO_INFO, "Init_SD: Initializing SD card...\n");
	esp_err_t ret;
	esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t* card;
	sdmmc_host_t host = SDSPI_HOST_DEFAULT();
	host.max_freq_khz = 20000;
	host.slot = HSPI_HOST;
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    ret = spi_bus_initialize(host.slot, &bus_cfg, 2);
    if (ret != ESP_OK) {
        lprintf(LO_INFO, "Failed to initialize bus.");
        return false;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            lprintf(LO_INFO, "Init_SD: Failed to mount filesystem.\n");
        } else {
           lprintf(LO_INFO, "Init_SD: Failed to initialize the card. %d\n", ret);
        }
        return false;
    }
	lprintf(LO_INFO, "Init_SD: init successful. Card info:\n");
	sdmmc_card_print_info(stdout, card);
	return true;
}

int realtime=0;


void I_uSleep(unsigned long usecs)
{
	vTaskDelay(usecs/1000);
}

static unsigned long getMsTicks() {
  struct timeval tv;
  struct timezone tz;
  unsigned long thistimereply;

  gettimeofday(&tv, &tz);

  //convert to ms
  unsigned long now = tv.tv_usec/1000+tv.tv_sec*1000;
  return now;
}

int I_GetTime_RealTime (void)
{
  struct timeval tv;
  struct timezone tz;
  unsigned long thistimereply;

  gettimeofday(&tv, &tz);

  thistimereply = (tv.tv_sec * TICRATE + (tv.tv_usec * TICRATE) / 1000000);

  return thistimereply;

}

const int displaytime=0;

fixed_t I_GetTimeFrac (void)
{
  unsigned long now;
  fixed_t frac;


  now = getMsTicks();

  if (tic_vars.step == 0)
    return FRACUNIT;
  else
  {
    frac = (fixed_t)((now - tic_vars.start + displaytime) * FRACUNIT / tic_vars.step);
    if (frac < 0)
      frac = 0;
    if (frac > FRACUNIT)
      frac = FRACUNIT;
    return frac;
  }
}


void I_GetTime_SaveMS(void)
{
  if (!movement_smooth)
    return;

  tic_vars.start = getMsTicks();
  tic_vars.next = (unsigned int) ((tic_vars.start * tic_vars.msec + 1.0f) / tic_vars.msec);
  tic_vars.step = tic_vars.next - tic_vars.start;
}

unsigned long I_GetRandomTimeSeed(void)
{
	return 4; //per https://xkcd.com/221/
}

const char* I_GetVersionString(char* buf, size_t sz)
{
  sprintf(buf,"%s v%s (http://prboom.sourceforge.net/)",PACKAGE,VERSION);
  return buf;
}

const char* I_SigString(char* buf, size_t sz, int signum)
{
  return buf;
}

extern unsigned char *doom1waddata;

#ifdef WAD_SDCARD
typedef struct {
	FILE* file;
	int offset;
	int size;
	char name[12];
} FileDesc;
#else
typedef struct {
	const esp_partition_t* part;
	int offset;
	int size;
} FileDesc;
#endif

static FileDesc fds[32];
static const char fileName[] = "DOOM.WAD";

int I_Open(const char *wad, int flags) {

	lprintf(LO_INFO, "I_Open: open %s\n", wad);

#ifdef WAD_SDCARD
	char fname[12];
	strcpy(fname, wad);
	memcpy(fname+(strlen(fname)-4), ".WAD", 4);
	lprintf(LO_INFO, "I_Open: Opening File: %s (as %s)\n", wad, fname);

	int x=0;
	while (fds[x].file!=NULL && strcmp(fds[x].name ,fname)!=0 && x < 32)
		x++;
	lprintf(LO_INFO, "I_Open: Got handle: %d\n", x);

	if(x == 31 && fds[x].file!=NULL)
	{
		lprintf(LO_INFO, "I_Open: Too many handles open\n");
		return -1;
	}

	if(strcmp(fds[x].name, fname) == 0)
	{
		lprintf(LO_INFO, "I_Open: File already open\n");
		rewind(fds[x].file);
		return x;
	}

	if (strcmp(fname, fileName)==0) {
		fds[x].file=fopen("/sdcard/doom.wad", "rb");
	} else if(strcmp("prboom.WAD", fname)==0) {
		fds[x].file=fopen("/sdcard/prboom.wad", "rb");
	}
	if(fds[x].file)
	{
		fds[x].offset=0;
		strcpy(fds[x].name, fname);
		fseek(fds[x].file, 0L, SEEK_END);
		fds[x].size=ftell(fds[x].file);
		rewind(fds[x].file);
		lprintf(LO_INFO, "File size: %d bytes (%dK)\n", fds[x].size, fds[x].size/1000);
	} else {
		lprintf(LO_INFO, "I_Open: open %s failed\n", fname);
		return -1;
	}
	return x;
#else
	int x=3;
	while (fds[x].part!=NULL) x++;
	if (strcmp(wad, "DOOM1.WAD")==0) {
		fds[x].part=esp_partition_find_first(66, 6, NULL);
		fds[x].offset=0;
		fds[x].size=fds[x].part->size;
	} else {
		lprintf(LO_INFO, "I_Open: open %s failed\n", wad);
		return -1;
	}
	return x;
#endif
}

int I_Lseek(int ifd, off_t offset, int whence) {
#ifdef WAD_SDCARD
	if (whence==SEEK_SET) {
		fds[ifd].offset=offset;
		fseek(fds[ifd].file, offset, SEEK_SET);
	} else if (whence==SEEK_CUR) {
		fds[ifd].offset+=offset;
		fseek(fds[ifd].file, offset, SEEK_CUR);
	} else if (whence==SEEK_END) {
		lprintf(LO_INFO, "I_Lseek: SEEK_END unimplemented\n");
	}
	return fds[ifd].offset;
#else
	if (whence==SEEK_SET) {
		fds[ifd].offset=offset;
	} else if (whence==SEEK_CUR) {
		fds[ifd].offset+=offset;
	} else if (whence==SEEK_END) {
		lprintf(LO_INFO, "I_Lseek: SEEK_END unimplemented\n");
	}
	return fds[ifd].offset;
#endif
}

int I_Filelength(int ifd)
{
	return fds[ifd].size;
}

void I_Close(int fd) {
#ifdef WAD_SDCARD
	lprintf(LO_INFO, "I_Open: Closing File: %s\n", fds[fd].name);
	sprintf(fds[fd].name, " ");
	fclose(fds[fd].file);
	fds[fd].file=NULL;
#else
	fds[fd].part=NULL;
#endif
}


#ifdef WAD_SDCARD
	typedef struct {
		int ifd;
		void *addr;
		int offset;
		size_t len;
		int used;
	} MmapHandle;
#else
typedef struct {
	spi_flash_mmap_handle_t handle;
	void *addr;
	int offset;
	size_t len;
	int used;
} MmapHandle;
#endif

#define NO_MMAP_HANDLES 128
static MmapHandle mmapHandle[NO_MMAP_HANDLES];

static int nextHandle=0;
static int getFreeHandle() {
	int n=NO_MMAP_HANDLES;
	while (mmapHandle[nextHandle].used!=0 && n!=0) {
		nextHandle++;
		if (nextHandle==NO_MMAP_HANDLES) nextHandle=0;
		n--;
	}
	if (n==0) {
		lprintf(LO_ERROR, "I_Mmap: More mmaps than NO_MMAP_HANDLES!");
		exit(0);
	}
	
	if (mmapHandle[nextHandle].addr) {
		#ifdef WAD_SDCARD
			free(mmapHandle[nextHandle].addr);
		#else
			spi_flash_munmap(mmapHandle[nextHandle].handle);
		#endif
		mmapHandle[nextHandle].addr=NULL;
		//printf("mmap: freeing handle %d\n", nextHandle);
	}
	int r=nextHandle;
	nextHandle++;
	if (nextHandle==NO_MMAP_HANDLES) nextHandle=0;

	return r;
}

static void freeUnusedMmaps() {
	for (int i=0; i<NO_MMAP_HANDLES; i++) {
		//Check if handle is not in use but is mapped.
		if (mmapHandle[i].used==0 && mmapHandle[i].addr!=NULL) {
			#ifdef WAD_SDCARD
				free(mmapHandle[i].addr);
				mmapHandle[i].ifd=NULL;
			#else
				spi_flash_munmap(mmapHandle[i].handle);
			#endif
			mmapHandle[i].addr=NULL;
			printf("Freeing handle %d\n", i);
		}
	}
}

void *I_Mmap(void *addr, size_t length, int prot, int flags, int ifd, off_t offset) {
#ifdef WAD_SDCARD
	//lprintf(LO_INFO, "I_Mmap: ifd %d, length: %d, offset: %d\n", ifd, (int)length, (int)offset);

	int i;
	esp_err_t err;
	void *retaddr=NULL;

	for (i=0; i<NO_MMAP_HANDLES; i++) {
		if (mmapHandle[i].offset==offset && mmapHandle[i].len==length && mmapHandle[i].ifd==ifd) {
			mmapHandle[i].used++;
			return mmapHandle[i].addr;
		}
	}

	i=getFreeHandle();

	retaddr = malloc(length);
	if(!retaddr)
	{
		lprintf(LO_ERROR, "I_Mmap: No free address space. Cleaning up unused cached mmaps...\n");
		freeUnusedMmaps();
		retaddr = malloc(length);
	}

	if(retaddr)
	{
		I_Lseek(ifd, offset, SEEK_SET);
		I_Read(ifd, retaddr, length);
		mmapHandle[i].addr=retaddr;
		mmapHandle[i].len=length;
		mmapHandle[i].used=1;
		mmapHandle[i].offset=offset;
		mmapHandle[i].ifd=ifd;
	} else {
		lprintf(LO_ERROR, "I_Mmap: Can't mmap offset: %d (len=%d)!\n", (int)offset, length);
		return NULL;
	}

	return retaddr;
#else
	int i;
	esp_err_t err;
	void *retaddr=NULL;

	for (i=0; i<NO_MMAP_HANDLES; i++) {
		if (mmapHandle[i].offset==offset && mmapHandle[i].len==length) {
			mmapHandle[i].used++;
			return mmapHandle[i].addr;
		}
	}

	i=getFreeHandle();

	//lprintf(LO_INFO, "I_Mmap: mmaping offset %d size %d handle %d\n", (int)offset, (int)length, i);
	err=esp_partition_mmap(fds[ifd].part, offset, length, SPI_FLASH_MMAP_DATA, (const void**)&retaddr, &mmapHandle[i].handle);
	if (err==ESP_ERR_NO_MEM) {
		lprintf(LO_ERROR, "I_Mmap: No free address space. Cleaning up unused cached mmaps...\n");
		freeUnusedMmaps();
		err=esp_partition_mmap(fds[ifd].part, offset, length, SPI_FLASH_MMAP_DATA, (const void**)&retaddr, &mmapHandle[i].handle);
	}
	mmapHandle[i].addr=retaddr;
	mmapHandle[i].len=length;
	mmapHandle[i].used=1;
	mmapHandle[i].offset=offset;

	if (err!=ESP_OK) {
		lprintf(LO_ERROR, "I_Mmap: Can't mmap: %x (len=%d)!", err, length);
		return NULL;
	}

	return retaddr;
#endif
}


int I_Munmap(void *addr, size_t length) {
	int i;
	for (i=0; i<NO_MMAP_HANDLES; i++) {
		if (mmapHandle[i].addr==addr && mmapHandle[i].len==length) break;
	}
	if (i==NO_MMAP_HANDLES) {
		lprintf(LO_ERROR, "I_Mmap: Freeing non-mmapped address/len combo!");
		exit(0);
	}
	//lprintf(LO_INFO, "I_Mmap: freeing handle %d\n", i);
	mmapHandle[i].used--;
	return 0;
}

void I_Read(int ifd, void* vbuf, size_t sz)
{
	//lprintf(LO_INFO, "I_Read: Reading %d bytes...\n", (int)sz);

#ifdef WAD_SDCARD
	int readBytes = 0;
    for(int i = 0; i < 20; i++)
	{
		readBytes = fread(vbuf, sz, 1, fds[ifd].file);
		if( readBytes == 1)//(int)sz)
		{
			return;
		}
		lprintf(LO_INFO, "Error Reading %d bytes\n", (int)sz);
		//vTaskDelay(300 / portTICK_RATE_MS);
	}

	I_Error("I_Read: Error Reading %d bytes after 20 tries\n", (int)sz);
#else
	uint8_t *d=I_Mmap(NULL, sz, 0, 0, ifd, fds[ifd].offset);
	memcpy(vbuf, d, sz);
	I_Munmap(d, sz);
#endif
}

const char *I_DoomExeDir(void)
{
  return "";
}



char* I_FindFile(const char* wfname, const char* ext)
{
  char *p;
  p = malloc(strlen(wfname)+4);
  sprintf(p, "%s.%s", wfname, ext);
  return NULL;
}

void I_SetAffinityMask(void)
{
}
