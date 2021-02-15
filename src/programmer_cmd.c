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
#include <usb.h>

#include "fuprog.h"

#define USB_FUNC_READ		1
#define USB_FUNC_WRITE		2
#define USB_FUNC_HOLD_CMD	3
#define USB_FUNC_RUN_CMD	4
#define USB_FUNC_INIT_CMD	5
#define USB_FUNC_BSIZE		6
#define USB_FUNC_TEST		7
#define USB_FUNC_AAMUX		100

#define USB_TIMEOUT		5000

#define USB_ERROR_NOTFOUND	1
#define USB_ERROR_ACCESS	2
#define USB_ERROR_IO		3

usb_dev_handle *handle_dev = NULL;

static int usbGetStringAscii(usb_dev_handle *dev, int index, int langid, char *buf, int buflen)
{
	char buffer[256];
	int  rval, i;

	if((rval = usb_control_msg(dev, USB_ENDPOINT_IN, USB_REQ_GET_DESCRIPTOR, (USB_DT_STRING << 8) + index, langid, buffer, sizeof(buffer), 1000)) < 0)
		return rval;
	if(buffer[1] != USB_DT_STRING)
		return 0;
	if((unsigned char)buffer[0] < rval)
		rval = (unsigned char)buffer[0];
	rval /= 2;
	for(i = 1; i < rval; i++) {
		if(i > buflen)
			break;
		buf[i-1] = buffer[2 * i];
		if(buffer[2 * i + 1] != 0)
			buf[i-1] = '?';
	}
	buf[i - 1] = 0;
	return i - 1;
}

static int usbOpenDevice(usb_dev_handle **device, int vendor, char *vendorName, int product, char *productName)
{
	struct usb_bus *bus;
	struct usb_device *dev;
	usb_dev_handle *handle = NULL;
	int errorCode = USB_ERROR_NOTFOUND;
	static int didUsbInit = 0;

	if(!didUsbInit) {
		didUsbInit = 1;
		usb_init();
	}

	usb_find_busses();
	usb_find_devices();

	for(bus=usb_get_busses(); bus; bus=bus->next) {
		for(dev=bus->devices; dev; dev=dev->next) {
			if(dev->descriptor.idVendor == vendor && dev->descriptor.idProduct == product) {
				char string[256];
				int len;
				handle = usb_open(dev);
				if(!handle) {
					errorCode = USB_ERROR_ACCESS;
					printf("Warning: cannot open USB device: %s\n", usb_strerror());
					continue;
				}
				if(vendorName == NULL && productName == NULL)
					break;
				len = usbGetStringAscii(handle, dev->descriptor.iManufacturer, 0x0409, string, sizeof(string));
				if(len < 0) {
					errorCode = USB_ERROR_IO;
					printf("Warning: cannot query manufacturer for device: %s\n", usb_strerror());
				} else {
					errorCode = USB_ERROR_NOTFOUND;
					if(strcmp(string, vendorName) == 0) {
						len = usbGetStringAscii(handle, dev->descriptor.iProduct, 0x0409, string, sizeof(string));
						if(len < 0) {
							errorCode = USB_ERROR_IO;
							printf("Warning: cannot query product for device: %s\n", usb_strerror());
						} else {
							errorCode = USB_ERROR_NOTFOUND;
							if(strcmp(string, productName) == 0)
								break;
						}
					}
				}
				usb_close(handle);
				handle = NULL;
			}
		}
		if(handle)
			break;
	}
	if(handle != NULL) {
		errorCode = 0;
		*device = handle;
	}
	return errorCode;
}

int FUProg_Init()
{
	usb_init();

	if(usbOpenDevice(&handle_dev, USBDEV_SHARED_VENDOR, VENDOR_PRODUCT, USBDEV_SHARED_PRODUCT, NAME_PRODUCT) != 0)
		return 0;
	return 1;
}

void FUProg_Close()
{
	usb_close(handle_dev);
}

int FUProg_Read_Buf(int address, unsigned char *buffer, int bufflen)
{
	return usb_control_msg(handle_dev, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, USB_FUNC_READ,
				address & 0xFFFF, (address >> 16) & 0xFFFF, (char *)buffer, bufflen, USB_TIMEOUT);
}

int FUProg_Write_Buf(int address, unsigned char *buffer, int bufflen)
{
	return usb_control_msg(handle_dev, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT, USB_FUNC_WRITE,
				address & 0xFFFF, (address >> 16) & 0xFFFF, (char *)buffer, bufflen, USB_TIMEOUT);
}

int FUProg_Flash_Cmd(int address, int byte)
{
	char buffer[1];
	return usb_control_msg(handle_dev, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT, USB_FUNC_HOLD_CMD,
				address & 0xFFFF, ((address >> 16) & 0xFF) | ((byte << 8) & 0xFF00), buffer, 0, USB_TIMEOUT);
}

int FUProg_Flash_CmdRun()
{
	char buffer[1];
	return usb_control_msg(handle_dev, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
				USB_FUNC_RUN_CMD, 0, 0, buffer, 0, USB_TIMEOUT);
}

int FUProg_Flash_Init()
{
	char buffer[1];
	return usb_control_msg(handle_dev, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
				USB_FUNC_INIT_CMD, 0, 0, buffer, 0, USB_TIMEOUT);
}

int FUProg_Flash_PageSize(int pagesize)
{
	char buffer[1];
	if(pagesize > USB_MAXLEN_WRITE)
		return -1;
	return usb_control_msg(handle_dev, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
				USB_FUNC_BSIZE, pagesize, 0, buffer, 0, USB_TIMEOUT);
}

int FUProg_Test()
{
	char buffer[1];
	return usb_control_msg(handle_dev, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
				USB_FUNC_TEST, 0, 0, buffer, 0, USB_TIMEOUT);
}

int FUProg_Flash_SetAAMux()
{
	char buffer[1];
	return usb_control_msg(handle_dev, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
				USB_FUNC_AAMUX, 1, 0, buffer, 0, USB_TIMEOUT);
}
