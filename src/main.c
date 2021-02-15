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
#include <getopt.h>

#include "fuprog.h"

unsigned int blocks[MAX_BUF_SIZE];
int block_total = 0;
unsigned int block_addr = 0;

static void define_block(unsigned int block_count, unsigned int block_size)
{
	unsigned int i;

	for (i = 1; i <= block_count; i++)
	{
		block_total++;
		blocks[block_total] = block_addr;
		block_addr = block_addr + block_size;
	}
}

static int flash_erase_area(int verify, unsigned int start, unsigned int length)
{
	int cur_block, tot_blocks = 0;
	unsigned int reg_start, reg_end, block_len = 0;

	if((!block_total) || ((Flash_Chip.cmd_type != CMD_TYPE_SCS) && !start && !length))
	{
		printf("Full Erase Flash Chip: "); fflush(stdout);
		if(!erase_flash(Flash_Chip.cmd_type, 0xFFFFFF))
		{
			printf("Error.\n"); fflush(stdout);
			return 0;
		}
		printf("Done.\n"); fflush(stdout);
		if(verify && !verify_erase_flash(Flash_Chip.cmd_type, 0, Flash_Chip.flash_size))
			return 0;
		sleep(1);
		return 1;
	}

	reg_start = start;

	if(!length)
		reg_end = Flash_Chip.flash_size;
	else
		reg_end = reg_start + length;

	for (cur_block = 1; cur_block <= block_total; cur_block++)
	{
		block_addr = blocks[cur_block];
		if ((block_addr >= reg_start) && (block_addr < reg_end))  tot_blocks++;
	}

	printf("Total Blocks to Erase: %d\n\n", tot_blocks);

	for (cur_block = 1; cur_block <= block_total; cur_block++)
	{
		block_addr = blocks[cur_block];
		if(cur_block == block_total)
			block_len = Flash_Chip.flash_size - block_addr;
		else
			block_len = blocks[cur_block + 1] - block_addr;

		if ((block_addr >= reg_start) && (block_addr < reg_end))
		{
			printf("Erasing block: %d (addr = 0x%08X len = %d)...", cur_block, block_addr, block_len);
			fflush(stdout);
			if(!erase_flash(Flash_Chip.cmd_type, block_addr))
			{
				printf("Error!\n");  fflush(stdout);
				return 0;
			}
			printf("Done!\n");  fflush(stdout);
			if(verify && !verify_erase_flash(Flash_Chip.cmd_type, block_addr, block_addr+block_len))
				return 0;
			fflush(stdout);
		}
	}
	sleep(1);
	return 1;
}

static void print_supported_chips()
{
	flash_chip_type* flash_chip = flash_chip_full_list;
	int i = 0;

	printf("Supported ROM chips:\n"
		"--------------------------------------\n\n");
	while(flash_chip->vendid)
	{
		i++;
		printf("%04d: %s\n", i, flash_chip->flash_part);
		flash_chip++;
	}
	printf("\n--------------------------------------\n\n");
	exit(1);
}

static void title()
{
	printf("\nFlash USB Programmer - FUProg v.%s\n", FUP_VERSION);
	printf("Copyright (C) 2008 Mokrushin I.V. (McMCC) <mcmcc@mail.ru>\n\n");
}

static void usage(char *name)
{
	printf("Usage: %s [-dehrwvtL] [-c num] [-a address] [-l length] [filename]\n", name);
	printf ("      -r         read flash and save into file\n"
		"      -w         write file into flash\n"
		"      -e         erase flash device\n"
		"      -v         verify flash against file or after erasing\n"
		"      -a <addr>  address position\n"
		"      -l <len>   length dump\n"
		"      -c <num>   flash chip number(see -L)\n"
		"      -d         flash chip detect only\n"
		"      -t         testing Flash USB Programmer\n"
		"      -L         print supported devices\n"
		"      -h         print this help text.\n\n"
		);
	exit(1);
}

int flash_chip_num(int num_chip)
{
	flash_chip_type* flash_chip = flash_chip_full_list;
	int list_num = 0, num = num_chip - 1;

	while(flash_chip->vendid)
	{
		if(list_num == num)
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
		list_num++;
	}
	return 0;
}

int main(int argc, char **argv)
{
	unsigned char *buffer = NULL;
	long int addr_l = 0, len_l = 0;
	int c, read_it = 0, write_it = 0, erase_it = 0, detect_it = 0, verify_it = 0, test_it = 0, chip_num = 0;
	int list_it = 0;
	char *filename = NULL, *addr_str = NULL, *len_str = NULL, *chip_str = NULL;
	FILE *fp;

	title();

	while ((c = getopt(argc, argv, "edhrwvtLa:c:l:")) != -1)
	{
		switch (c) {
			case 'e':
				erase_it = 1;
				break;
			case 'r':
				read_it = 1;
				break;
			case 'w':
				write_it = 1;
				break;
			case 'v':
				verify_it = 1;
				break;
			case 't':
				test_it = 1;
				break;
			case 'd':
				detect_it = 1;
				break;
			case 'L':
				list_it = 1;
				break;
			case 'c':
				chip_str = strdup(optarg);
				chip_num = atoi(chip_str);
				break;
			case 'a':
				addr_str = strdup(optarg);
				addr_l = strtol(addr_str, NULL, *addr_str && *(addr_str + 1) == 'x' ? 16 : 10);
				break;
			case 'l':
				len_str = strdup(optarg);
				len_l = strtol(len_str, NULL, *len_str && *(len_str + 1) == 'x' ? 16 : 10);
				break;
			case 'h':
			default:
				usage(argv[0]);
		}
	}

	if(argc < 2) usage(argv[0]);
	if(optind < argc) filename = argv[optind++];

	read_config();

	if(list_it)
		print_supported_chips();

	if((read_it && write_it) || ((read_it || write_it) && !filename))
		usage(argv[0]);

	if(read_it)
	{
		erase_it = 0;
		write_it = 0;
	}

	if(!FUProg_Init())
	{
		printf("\nCould not find USB device \"%s\" with vid=0x%x pid=0x%x\n\n",
			NAME_PRODUCT, USBDEV_SHARED_VENDOR, USBDEV_SHARED_PRODUCT);
		return 0;
	}

	if(test_it)
	{
		if(FUProg_Test() < 0)
		{
			printf("\n%s not found!!!\n", NAME_PRODUCT);
			goto error_prog;
		}
		printf("\n%s found OK!\n", NAME_PRODUCT);
		sleep(1);
		if(!read_it && !write_it && !verify_it && !erase_it && !detect_it) goto exit_prog;
	}
	if(!read_it && !write_it && !verify_it && !erase_it && !detect_it) usage(argv[0]);

	if(chip_num && flash_chip_num(chip_num))
	{
		printf("\nManual Flash Chip Select: %s\n", Flash_Chip.flash_part);
	}
	else if(!identify_flash())
	{
		printf("\n*** Unknown or NO Flash Chip Detected ***\n\n");
		goto error_prog;
	}

	if(Flash_Chip.region1_num)
		define_block(Flash_Chip.region1_num, Flash_Chip.region1_size);
	if(Flash_Chip.region2_num)
		define_block(Flash_Chip.region2_num, Flash_Chip.region2_size);
	if(Flash_Chip.region3_num)
		define_block(Flash_Chip.region3_num, Flash_Chip.region3_size);
	if(Flash_Chip.region4_num)
		define_block(Flash_Chip.region4_num, Flash_Chip.region4_size);
	printf("\n------------------------------------------------------------\n");
	printf("Flash Chip Detected: %s\nTotal Flash Memory Size: %d bytes\n",
		Flash_Chip.flash_part,
		Flash_Chip.flash_size);

	if(detect_it)
	{
		if(Flash_Chip.region1_num || Flash_Chip.region2_num || Flash_Chip.region3_num || Flash_Chip.region4_num)
			printf("Blocks Memory Map:       ");
		if(Flash_Chip.region1_num)
			printf("%d blocks, %d bytes\n", Flash_Chip.region1_num, Flash_Chip.region1_size);
		if(Flash_Chip.region2_num)
			printf("                         %d blocks, %d bytes\n", Flash_Chip.region2_num, Flash_Chip.region2_size);
		if(Flash_Chip.region3_num)
			printf("                         %d blocks, %d bytes\n", Flash_Chip.region3_num, Flash_Chip.region3_size);
		if(Flash_Chip.region4_num)
			printf("                         %d blocks, %d bytes\n", Flash_Chip.region4_num, Flash_Chip.region4_size);
		if(Flash_Chip.page_size)
			printf("Memory Page Size:        %d bytes\n", Flash_Chip.page_size);
	}
	printf("------------------------------------------------------------\n\n");
	if(detect_it) goto exit_prog;

	if(verify_it && !read_it && !write_it && !filename && !erase_it) 
	{
		if(!len_l) len_l = (long int)Flash_Chip.flash_size - addr_l;
		verify_erase_flash(Flash_Chip.cmd_type, (int)addr_l, (int)addr_l+(int)len_l);
		goto exit_prog;
	}

	if(erase_it)
		flash_erase_area(verify_it, (int)addr_l, (int)len_l);
	if(!read_it && !write_it && !filename) goto exit_prog;

	if(!len_l) len_l = (long int)Flash_Chip.flash_size - addr_l;

	buffer = (unsigned char*)malloc(Flash_Chip.flash_size);
	memset(buffer, 0xFF, Flash_Chip.flash_size);

	if(verify_it && !read_it && !write_it && filename)
	{
		if(!(fp = fopen(filename, "rb")))
			goto error_free_prog;

		len_l = fread(buffer, 1, Flash_Chip.flash_size, fp);
		fclose(fp);

		if(len_l <= 0)
			goto error_free_prog;

		if(!verify_flash(Flash_Chip.cmd_type, (int)addr_l, (int)addr_l+(int)len_l, buffer))
			goto error_free_prog;
		sleep(1);
		goto exit_free_prog;
	}

	if(read_it)
	{
		if(!read_flash(Flash_Chip.cmd_type, (int)addr_l, (int)addr_l+(int)len_l, buffer))
			goto error_free_prog;

		if(!(fp = fopen(filename, "wb")))
			goto error_free_prog;

		fwrite(buffer, 1, (int)len_l, fp);
		fclose(fp);
		sleep(1);
		goto exit_free_prog;
	}

	if(!(fp = fopen(filename, "rb")))
		goto error_free_prog;

	len_l = fread(buffer, 1, Flash_Chip.flash_size, fp);

	if(len_l <= 0)
		goto error_free_prog;

	fclose(fp);
	if(!write_flash(Flash_Chip.cmd_type, (int)addr_l, Flash_Chip.page_size, (int)addr_l+(int)len_l, buffer))
		goto error_free_prog;
	sleep(1);
	if(verify_it && !verify_flash(Flash_Chip.cmd_type, (int)addr_l, (int)addr_l+(int)len_l, buffer))
		goto error_free_prog;

exit_free_prog:
	if(buffer) free(buffer);
exit_prog:
	FUProg_Close();
	printf("\n");
	return 0;

error_free_prog:
	if(buffer) free(buffer);
error_prog:
	FUProg_Close();
	printf("\n");
	return 1;
}
