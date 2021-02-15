/* Copyright (C) 2008 Mokrushin I.V. aka McMCC <mcmcc@mail.ru>
 * A simple Flash USB Programmer FUProg v.1.0.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef __FUPROG_H
#define __FUPROG_H

#ifdef _WINDOWS
#include <windows.h>
#define sleep(x) Sleep(x * 1000)
#endif

#ifndef FUP_VERSION
#define FUP_VERSION		"1.0"
#endif
#define NAME_PRODUCT		"Flash USB Programmer"
#define VENDOR_PRODUCT		"McMCC"
#ifndef FUPROG_CFG
#define FUPROG_CFG		"fuprog.cfg"
#endif
#define USBDEV_SHARED_VENDOR	0x16C0
#define USBDEV_SHARED_PRODUCT	0x05DC
#define USB_MAXLEN_READ		254
#define USB_MAXLEN_WRITE	512
#define MAX_BUF_SIZE		1024
#define MAXLEN_STRBUF		USB_MAXLEN_READ

#define  size4K			0x1000
#define  size8K			0x2000
#define  size16K		0x4000
#define  size32K		0x8000
#define  size64K		0x10000
#define  size96K		0x18000
#define  size112K		0x1C000
#define  size128K		0x20000
#define  size256K		0x40000
#define  size384K		0x60000
#define  size512K		0x80000

#define  size1MB		0x100000
#define  size2MB		0x200000
#define  size4MB		0x400000
#define  size8MB		0x800000
#define  size16MB		0x1000000

#define  CMD_TYPE_FWI		0x01  /* AAMux CUI          */
#define  CMD_TYPE_SCS		0x02  /* CUI                */
#define  CMD_TYPE_AMD		0x03  /* JEDEC 0xAAA        */
#define  CMD_TYPE_SST		0x04  /* JEDEC 0x2AAA       */
#define  CMD_TYPE_FWH		0x05  /* AAMux JEDEC 0x2AAA */

#define  NUM_CMD_TYPE		6

typedef struct _flash_chip_type {
	unsigned char vendid;         /* Manufacturer Id        */
	unsigned char devid;          /* Device Id              */
	unsigned int  flash_size;     /* Total size in Bytes    */
	unsigned int  cmd_type;       /* Device CMD TYPE        */
	char*         flash_part;     /* Flash Chip Description */
	unsigned int  page_size;      /* Page size in Bytes     */
	unsigned int  region1_num;    /* Region 1 block count   */
	unsigned int  region1_size;   /* Region 1 block size    */
	unsigned int  region2_num;    /* Region 2 block count   */
	unsigned int  region2_size;   /* Region 2 block size    */
	unsigned int  region3_num;    /* Region 3 block count   */
	unsigned int  region3_size;   /* Region 3 block size    */
	unsigned int  region4_num;    /* Region 4 block count   */
	unsigned int  region4_size;   /* Region 4 block size    */
} flash_chip_type;

flash_chip_type Flash_Chip;
extern flash_chip_type flash_chip_list[];
extern flash_chip_type flash_chip_full_list[];

int FUProg_Init();
void FUProg_Close();
int FUProg_Read_Buf(int address, unsigned char *buffer, int bufflen);
int FUProg_Write_Buf(int address, unsigned char *buffer, int bufflen);
int FUProg_Flash_Cmd(int address, int byte);
int FUProg_Flash_CmdRun();
int FUProg_Flash_Init();
int FUProg_Flash_PageSize(int pagesize);
int FUProg_Test();
int FUProg_Flash_SetAAMux();

int identify_flash();
int erase_flash(int cmd_type, int address);
int read_flash(int cmd_type, int address, int len, unsigned char *buffer);
int write_flash(int cmd_type, int address, int pagesize, int len, unsigned char *buffer);
int verify_flash(int cmd_type, int address, int len, unsigned char *buffer);
int verify_erase_flash(int cmd_type, int address, int len);
void toggle_check(int cmd_type);
void abe_check(int cmd_type);
void read_config();

#endif /* __FUPROG_H */
