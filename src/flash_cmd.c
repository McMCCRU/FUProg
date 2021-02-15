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
  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "fuprog.h"
#include "flash_table.h"

static void reset_flash(int cmd_type)
{
	switch(cmd_type)
	{
		case CMD_TYPE_FWH:
			if(FUProg_Flash_SetAAMux() < 0)
				return;
		case CMD_TYPE_SST:
			if(FUProg_Flash_Cmd(0x5555, 0xAA) < 0)
				return;
			if(FUProg_Flash_Cmd(0x2AAA, 0x55) < 0)
				return;
			if(FUProg_Flash_Cmd(0x5555, 0xF0) < 0)
				return;
			break;
		case CMD_TYPE_AMD:
			if(FUProg_Flash_Cmd(0, 0xF0) < 0)
				return;
			break;
		case CMD_TYPE_FWI:
			if(FUProg_Flash_SetAAMux() < 0)
				return;
		case CMD_TYPE_SCS:
			if(FUProg_Flash_Cmd(0, 0x50) < 0)
				return;
			if(FUProg_Flash_Cmd(0, 0xFF) < 0)
				return;
			break;
		default:
			return;
	}
	if(FUProg_Flash_CmdRun() < 0)
		return;
}

int erase_flash(int cmd_type, int address)
{
	if(FUProg_Flash_Init() < 0)
		return 0;

	reset_flash(cmd_type);

	switch(cmd_type)
	{
		case CMD_TYPE_FWH:
			if(FUProg_Flash_SetAAMux() < 0)
				return 0;
		case CMD_TYPE_SST:
			if(FUProg_Flash_Cmd(0x5555, 0xAA) < 0)
				return 0;
			if(FUProg_Flash_Cmd(0x2AAA, 0x55) < 0)
				return 0;
			if(FUProg_Flash_Cmd(0x5555, 0x80) < 0)
				return 0;
			if(FUProg_Flash_Cmd(0x5555, 0xAA) < 0)
				return 0;
			if(FUProg_Flash_Cmd(0x2AAA, 0x55) < 0)
				return 0;
			if(address == 0xFFFFFF)
			{
				if(FUProg_Flash_Cmd(0x5555, 0x10) < 0)
					return 0;
			} else {
				if(FUProg_Flash_Cmd(address, 0x30) < 0)
					return 0;
			}
			break;
		case CMD_TYPE_AMD:
			if(FUProg_Flash_Cmd(0xAAA, 0xAA) < 0)
				return 0;
			if(FUProg_Flash_Cmd(0x555, 0x55) < 0)
				return 0;
			if(FUProg_Flash_Cmd(0xAAA, 0x80) < 0)
				return 0;
			if(FUProg_Flash_Cmd(0xAAA, 0xAA) < 0)
				return 0;
			if(FUProg_Flash_Cmd(0x555, 0x55) < 0)
				return 0;
			if(address == 0xFFFFFF)
			{
				if(FUProg_Flash_Cmd(0xAAA, 0x10) < 0)
					return 0;
			} else {
				if(FUProg_Flash_Cmd(address, 0x30) < 0)
					return 0;
			}
			break;
		case CMD_TYPE_FWI:
			if(FUProg_Flash_SetAAMux() < 0)
				return 0;
		case CMD_TYPE_SCS:
			if(FUProg_Flash_Cmd(address, 0x50) < 0)
				return 0;
			if(FUProg_Flash_Cmd(address, 0x60) < 0) /* Unlock */
				return 0;
			if(FUProg_Flash_Cmd(address, 0xD0) < 0)
				return 0;
			if(FUProg_Flash_CmdRun() < 0)
				return 0;
			if(FUProg_Flash_Cmd(address, 0x50) < 0)
				return 0;
			if(FUProg_Flash_Cmd(address, 0x20) < 0) /* Erase */
				return 0;
			if(FUProg_Flash_Cmd(address, 0xD0) < 0)
				return 0;
			break;
		default:
			return 0;
	} 
	if(FUProg_Flash_CmdRun() < 0)
		return 0;

	if(cmd_type == CMD_TYPE_SCS || cmd_type == CMD_TYPE_FWI)
		abe_check(cmd_type);
	else
		toggle_check(cmd_type);

	return 1;
}

int read_flash(int cmd_type, int address, int len, unsigned char *buffer)
{
	unsigned char buf[USB_MAXLEN_READ];
	int i, j = 0;
	time_t end_time, elapsed_seconds;
	int buflen = sizeof(buf);
	time_t start_time = time(0);

	if(FUProg_Flash_Init() < 0)
		return 0;

	if((cmd_type == CMD_TYPE_FWH) || (cmd_type == CMD_TYPE_FWI))
	{
		if(FUProg_Flash_SetAAMux() < 0)
			return 0;
	}

	for(i = address; i < len; )
	{
		memset(buf, 0xFF, USB_MAXLEN_READ);
		if(FUProg_Read_Buf(i, buf, buflen) < 0)
			return 0;
		if((i + buflen) > len)
			buflen = buflen - ((i + buflen) - len);
		memcpy(buffer + i - address, buf, buflen);
		i = i + buflen;
		printf("\bREAD Percent: %3d%% Address: 0x%08X ", j, i);
		printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
		j = ((i - address) * 100 / (len - address));
		fflush(stdout);
	}
	printf("READ Percent: %3d%% Address: 0x%08X Done.\n", j, i);
	time(&end_time);
	elapsed_seconds = difftime(end_time, start_time);
	printf("Elapsed time: %d seconds\n\n", (int)elapsed_seconds);
	fflush(stdout);

	if(FUProg_Flash_Init() < 0)
		return 0;

	return 1;
}

int write_flash(int cmd_type, int address, int pagesize, int len, unsigned char *buffer)
{
	unsigned char buf[USB_MAXLEN_WRITE];
	int i, j = 0;
	time_t end_time, elapsed_seconds;
	int buflen = sizeof(buf);
	time_t start_time = time(0);

	if(FUProg_Flash_Init() < 0)
		return 0;

	if(FUProg_Flash_PageSize(pagesize) < 0)
		return 0;

	if(pagesize) buflen = pagesize;

	reset_flash(cmd_type);

	switch(cmd_type)
	{
		case CMD_TYPE_FWH:
			if(FUProg_Flash_SetAAMux() < 0)
				return 0;
		case CMD_TYPE_SST:
			if(FUProg_Flash_Cmd(0x5555, 0xAA) < 0)
				return 0;
			if(FUProg_Flash_Cmd(0x2AAA, 0x55) < 0)
				return 0;
			if(FUProg_Flash_Cmd(0x5555, 0xA0) < 0)
				return 0;
			break;
		case CMD_TYPE_AMD:
			if(FUProg_Flash_Cmd(0xAAA, 0xAA) < 0)
				return 0;
			if(FUProg_Flash_Cmd(0x555, 0x55) < 0)
				return 0;
			if(FUProg_Flash_Cmd(0xAAA, 0xA0) < 0)
				return 0;
			break;
		case CMD_TYPE_FWI:
			if(FUProg_Flash_SetAAMux() < 0)
				return 0;
		case CMD_TYPE_SCS:
			if(FUProg_Flash_Cmd(0xFFFFFF, 0x50) < 0)
				return 0;
			if(FUProg_Flash_Cmd(0xFFFFFF, 0x40) < 0)
				return 0;
			break;
		default:
			return 0;
	}

	for(i = address; i < len; )
	{
		memset(buf, 0xFF, buflen);
		memcpy(buf, buffer + i - address, buflen);
		if(FUProg_Write_Buf(i, buf, buflen) < 0)
			return 0;
		i = i + buflen;
		printf("\bWRITE Percent: %3d%% Address: 0x%08X ", j, i);
		printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
		j = ((i - address) * 100 / (len - address));
		fflush(stdout);
	}
	printf("WRITE Percent: %3d%% Address: 0x%08X Done.\n", j, i);
	time(&end_time);
	elapsed_seconds = difftime(end_time, start_time);
	printf("Elapsed time: %d seconds\n\n", (int)elapsed_seconds);
	fflush(stdout);

	sleep(1); /* fix for sst49*** */

	if(FUProg_Flash_Init() < 0)
		return 0;

	return 1;
}

static int probe_flash(int cmd_type, unsigned char *buffer, int bufflen)
{
	if(FUProg_Flash_Init() < 0)
		return 0;

	reset_flash(cmd_type);

	switch(cmd_type)
	{
		case CMD_TYPE_FWH:
			if(FUProg_Flash_SetAAMux() < 0)
				return 0;
		case CMD_TYPE_SST:
			if(FUProg_Flash_Cmd(0x5555, 0xAA) < 0)
				return 0;
			if(FUProg_Flash_Cmd(0x2AAA, 0x55) < 0)
				return 0;
			if(FUProg_Flash_Cmd(0x5555, 0x90) < 0)
				return 0;
			break;
		case CMD_TYPE_AMD:
			if(FUProg_Flash_Cmd(0xAAA, 0xAA) < 0)
				return 0;
			if(FUProg_Flash_Cmd(0x555, 0x55) < 0)
				return 0;
			if(FUProg_Flash_Cmd(0xAAA, 0x90) < 0)
				return 0;
			break;
		case CMD_TYPE_FWI:
			if(FUProg_Flash_SetAAMux() < 0)
				return 0;
		case CMD_TYPE_SCS:
			if(FUProg_Flash_Cmd(0, 0x90) < 0)
				return 0;
			break;
		default:
			return 0;
	}

	if(FUProg_Flash_CmdRun() < 0)
		return 0;

	if(FUProg_Read_Buf(0, buffer, bufflen) < 0)
		return 0;

	if(FUProg_Flash_Init() < 0)
		return 0;

	reset_flash(cmd_type);

	return 1;
}

int identify_flash()
{
	int i;
	unsigned char buf[4];
	flash_chip_type* flash_chip;

	for(i = CMD_TYPE_SCS; i < NUM_CMD_TYPE; i++)
	{
		if(!probe_flash(i, buf, sizeof(buf)))
			return 0;

		flash_chip = flash_chip_full_list;

		while(flash_chip->vendid)
		{
			if (((flash_chip->vendid == buf[0]) && (flash_chip->devid == buf[1])) ||
				((flash_chip->vendid == buf[1]) && (flash_chip->devid == buf[2])))
			{
				Flash_Chip.vendid = flash_chip->vendid;
				Flash_Chip.devid = flash_chip->devid;
				Flash_Chip.flash_size = flash_chip->flash_size;
				Flash_Chip.cmd_type = flash_chip->cmd_type;
				Flash_Chip.flash_part = flash_chip->flash_part;
				Flash_Chip.page_size = flash_chip->page_size;
				Flash_Chip.region1_num = flash_chip->region1_num;
				Flash_Chip.region1_size = flash_chip->region1_size;
				Flash_Chip.region2_num = flash_chip->region2_num;
				Flash_Chip.region2_size = flash_chip->region2_size;
				Flash_Chip.region3_num = flash_chip->region3_num;
				Flash_Chip.region3_size = flash_chip->region3_size;
				Flash_Chip.region4_num = flash_chip->region4_num;
				Flash_Chip.region4_size = flash_chip->region4_size;
				return 1;
			}
			flash_chip++;
		}
	}
	return 0;
}

void abe_check(int cmd_type)
{
	unsigned char buf[1];
	unsigned long timeout = 0;

	if(FUProg_Flash_Init() < 0)
		return;

	if(cmd_type == CMD_TYPE_FWI)
	{
		if(FUProg_Flash_SetAAMux() < 0)
			return;
	}

	while(timeout < 0x07FFFFFF)
	{

		if(FUProg_Read_Buf(1, buf, 1) < 0)
			return;

		if(buf[0] & 0x80)
			break;

		timeout++;
	}

	reset_flash(cmd_type);
	reset_flash(cmd_type);
	sleep(2);

	if(FUProg_Flash_Init() < 0)
		return;
}

void toggle_check(int cmd_type)
{
	unsigned char byte, check, buf[1];
	unsigned long timeout = 0;

	if(FUProg_Flash_Init() < 0)
		return;

	if(cmd_type == CMD_TYPE_FWH)
	{
		if(FUProg_Flash_SetAAMux() < 0)
			return;
	}

	if(FUProg_Read_Buf(1, buf, 1) < 0)
		return;

	byte = buf[0] & 0x40;

	while(timeout < 0x07FFFFFF)
	{
		if(FUProg_Read_Buf(1, buf, 1) < 0)
			return;
		check = buf[0] & 0x40;
		if(byte == check) break;
		byte = check;
		timeout++;
	}
	if(FUProg_Flash_Init() < 0)
		return;
}

int verify_flash(int cmd_type, int address, int len, unsigned char *buffer)
{
	unsigned char buf[USB_MAXLEN_READ], *tmp;
	int i, j = 0, l, res = 1;
	time_t end_time, elapsed_seconds;
	int buflen = sizeof(buf);
	time_t start_time = time(0);

	if(FUProg_Flash_Init() < 0)
		return 0;

	reset_flash(cmd_type);

	if((cmd_type == CMD_TYPE_FWH) || (cmd_type == CMD_TYPE_FWI))
	{
		if(FUProg_Flash_SetAAMux() < 0)
			return 0;
	}

	for(i = address; i < len;)
	{
		memset(buf, 0xFF, USB_MAXLEN_READ);
		if(FUProg_Read_Buf(i, buf, buflen) < 0)
			return 0;

		if((i + buflen) > len)
			buflen = buflen - ((i + buflen) - len);

		tmp = buffer + i - address;
		l = buflen;
		for(l = 0; l < buflen; l++)
		{
			printf("\bVERIFY Percent: %3d%% Address: 0x%08X ", j, i + l);
			printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
			fflush(stdout);
			if(tmp[l] != buf[l])
			{
				res = 0;
				printf("VERIFY Percent: %3d%% Address: 0x%08X Error: orig = %02X read = %02X.\n",
						j, i + l, tmp[l], buf[l]);
				printf("*** VERIFY: ERROR! ***\n\7");
				fflush(stdout);
				goto error_verify;
			}
		}
		i = i + buflen;
		j = ((i - address) * 100 / (len - address));
		fflush(stdout);
	}
	printf("VERIFY Percent: %3d%% Address: 0x%08X Done.\n", j, i);
	printf("*** VERIFY: OK! ***\n");

error_verify:
	time(&end_time);
	elapsed_seconds = difftime(end_time, start_time);
	printf("Elapsed time: %d seconds\n\n", (int)elapsed_seconds);
	fflush(stdout);

	if(FUProg_Flash_Init() < 0)
		return 0;

	return res;
}

int verify_erase_flash(int cmd_type, int address, int len)
{
	unsigned char buf[USB_MAXLEN_READ];
	int i, j = 0, l, res = 1;
	time_t end_time, elapsed_seconds;
	int buflen = sizeof(buf);
	time_t start_time = time(0);

	if(FUProg_Flash_Init() < 0)
		return 0;

	reset_flash(cmd_type);

	if((cmd_type == CMD_TYPE_FWH) || (cmd_type == CMD_TYPE_FWI))
	{
		if(FUProg_Flash_SetAAMux() < 0)
			return 0;
	}

	for(i = address; i < len;)
	{
		memset(buf, 0xFF, USB_MAXLEN_READ);
		if(FUProg_Read_Buf(i, buf, buflen) < 0)
			return 0;

		if((i + buflen) > len)
			buflen = buflen - ((i + buflen) - len);

		l = buflen;
		for(l = 0; l < buflen; l++)
		{
			printf("\bVERIFY ERASE Percent: %3d%% Address: 0x%08X ", j, i + l);
			printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
			fflush(stdout);
			if(buf[l] != 0xFF)
			{
				res = 0;
				printf("VERIFY ERASE Percent: %3d%% Address: 0x%08X Error: orig = FF read = %02X.\n",
						j, i + l, buf[l]);
				printf("*** VERIFY ERASE: ERROR OR CHIP WRITE PROTECTION! ***\n\7");
				fflush(stdout);
				goto error_verify_erase;
			}
		}
		i = i + buflen;
		j = ((i - address) * 100 / (len - address));
		fflush(stdout);
	}
	printf("VERIFY ERASE Percent: %3d%% Address: 0x%08X Done.\n", j, i);
	printf("*** VERIFY ERASE: OK! ***\n");

error_verify_erase:
	time(&end_time);
	elapsed_seconds = difftime(end_time, start_time);
	printf("Elapsed time: %d seconds\n\n", (int)elapsed_seconds);
	fflush(stdout);

	if(FUProg_Flash_Init() < 0)
		return 0;

	return res;
}
