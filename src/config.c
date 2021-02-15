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
#include <ctype.h>

#include "fuprog.h"

#define is_blank(c) ((c) == ' ' || (c) == '\t')

flash_chip_type flash_chip_full_list[size1MB];

static char *strtostr_p(char *str)
{
	char new_str[MAXLEN_STRBUF];
	int i = 0, start = 0, len = strlen(str);

	memset(new_str, 0, MAXLEN_STRBUF);

	do {
		if(is_blank(*str) && !start)
			continue;

		if(isalnum(*str))
			start = 1;

		new_str[i++] = *str;
	} while (*str++);

	for(i = len; i > 0; i--)
	{
		if(is_blank(new_str[i]))
		{
			new_str[i] = '\0';
			continue;
		}
		if(isalnum(new_str[i]))
			break;
	}
	return (str = new_str);
}

static int strtoi_p(char *str)
{
	char new_str[MAXLEN_STRBUF];
	int i = 0;

	memset(new_str, 0, MAXLEN_STRBUF);

	do {
		if(is_blank(*str))
			continue;
		new_str[i++] = *str;
	} while (*str++);

	str = new_str;
	return (int)strtol(str, NULL, *str && *(str + 1) == 'x' ? 16 : 10);
}

void read_config()
{
	char *tmp, buffer[MAXLEN_STRBUF];
	FILE *cfg;
	flash_chip_type* flash_chip = flash_chip_list;
	int i = 0;

	while(flash_chip->vendid)
	{
		flash_chip_full_list[i].vendid = flash_chip->vendid;
		flash_chip_full_list[i].devid = flash_chip->devid;
		flash_chip_full_list[i].flash_size = flash_chip->flash_size;
		flash_chip_full_list[i].cmd_type = flash_chip->cmd_type;
		flash_chip_full_list[i].flash_part = flash_chip->flash_part;
		flash_chip_full_list[i].page_size = flash_chip->page_size;
		flash_chip_full_list[i].region1_num = flash_chip->region1_num;
		flash_chip_full_list[i].region1_size = flash_chip->region1_size;
		flash_chip_full_list[i].region2_num = flash_chip->region2_num;
		flash_chip_full_list[i].region2_size = flash_chip->region2_size;
		flash_chip_full_list[i].region3_num = flash_chip->region3_num;
		flash_chip_full_list[i].region3_size = flash_chip->region3_size;
		flash_chip_full_list[i].region4_num = flash_chip->region4_num;
		flash_chip_full_list[i].region4_size = flash_chip->region4_size;
		flash_chip++;
		i++;
	}

	if(!(cfg = fopen(FUPROG_CFG, "rb")))
		return;

	while(fgets(buffer, MAXLEN_STRBUF, cfg))
	{
		if (strchr(buffer, '\r')) *(strchr(buffer, '\r')) = '\0';
		if (strchr(buffer, '\n')) *(strchr(buffer, '\n')) = '\0';
		if (strchr(buffer, '#')) *(strchr(buffer, '#')) = '\0';
		if (strchr(buffer, ';')) *(strchr(buffer, ';')) = '\0';
		if (*buffer == '\0') continue;

		if((tmp = strtok(buffer, ",")))
			flash_chip_full_list[i].vendid = (unsigned char)strtoi_p(tmp);
		else
			continue;
		if((tmp = strtok(NULL, ",")))
			flash_chip_full_list[i].devid = (unsigned char)strtoi_p(tmp);
		else
			continue;
		if((tmp = strtok(NULL, ",")))
			flash_chip_full_list[i].flash_size = (unsigned int)strtoi_p(tmp);
		else
			continue;
		if((tmp = strtok(NULL, ",")))
			flash_chip_full_list[i].cmd_type = (unsigned int)strtoi_p(tmp);
		else
			continue;
		if((tmp = strtok(NULL, ",")))
			flash_chip_full_list[i].flash_part = strdup(strtostr_p(tmp));
		else
			continue;
		if((tmp = strtok(NULL, ",")))
			flash_chip_full_list[i].page_size = (unsigned int)strtoi_p(tmp);
		else
			continue;
		if((tmp = strtok(NULL, ",")))
			flash_chip_full_list[i].region1_num = (unsigned int)strtoi_p(tmp);
		else
			continue;
		if((tmp = strtok(NULL, ",")))
			flash_chip_full_list[i].region1_size = (unsigned int)strtoi_p(tmp);
		else
			continue;
		if((tmp = strtok(NULL, ",")))
			flash_chip_full_list[i].region2_num = (unsigned int)strtoi_p(tmp);
		else
			continue;
		if((tmp = strtok(NULL, ",")))
			flash_chip_full_list[i].region2_size = (unsigned int)strtoi_p(tmp);
		else
			continue;
		if((tmp = strtok(NULL, ",")))
			flash_chip_full_list[i].region3_num = (unsigned int)strtoi_p(tmp);
		else
			continue;
		if((tmp = strtok(NULL, ",")))
			flash_chip_full_list[i].region3_size = (unsigned int)strtoi_p(tmp);
		else
			continue;
		if((tmp = strtok(NULL, ",")))
			flash_chip_full_list[i].region4_num = (unsigned int)strtoi_p(tmp);
		else
			continue;
		if((tmp = strtok(NULL, ",")))
			flash_chip_full_list[i].region4_size = (unsigned int)strtoi_p(tmp);
		else
			continue;
		i++;
	}
	fclose(cfg);
}
