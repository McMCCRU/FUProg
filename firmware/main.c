/* Copyright (C) 2008 Mokrushin I.V. aka McMCC <mcmcc@mail.ru>
 * A simple Flash USB Programmer FUProg v.1.0.
 *
 * Firmware for AVR Atmega8 12MHz(Please edit for 16MHz).
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

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>

#include "usbdrv.h"

#define F_CPU			12000000UL
#include <util/delay.h>

#define USB_FUNC_READ		1
#define USB_FUNC_WRITE		2
#define USB_FUNC_HOLD_CMD	3
#define USB_FUNC_RUN_CMD	4
#define USB_FUNC_INIT_CMD	5
#define USB_FUNC_BSIZE		6
#define USB_FUNC_TEST		7
#define USB_FUNC_AAMUX		100
#define PROG_STATE_READ		9
#define PROG_STATE_WRITE	10

#define A1_Up()		PORTC |= _BV(PC2)  /* "1" - A0 for 82c55 */
#define A1_Down()	PORTC &= ~_BV(PC2) /* "0" - A0 for 82c55 */ 
#define A0_Up()		PORTC |= _BV(PC3)  /* "1" - A1 for 82c55 */
#define A0_Down()	PORTC &= ~_BV(PC3) /* "0" - A1 for 82c55 */
#define CS_Up()		PORTC |= _BV(PC4)  /* "1" - CS for 82c55 */
#define CS_Down()	PORTC &= ~_BV(PC4) /* "0" - CS for 82c55 */
#define WR_Up()		PORTD |= _BV(PD1)  /* "1" - WR for 82c55 */
#define WR_Down()	PORTD &= ~_BV(PD1) /* "0" - WR for 82c55 */
#define CE_Up()		PORTC |= _BV(PC5)
#define CE_Down()	PORTC &= ~_BV(PC5)
#define WE_Up()		PORTD |= _BV(PD3)
#define WE_Down()	PORTD &= ~_BV(PD3)
#define OE_Up()		PORTD |= _BV(PD4)
#define OE_Down()	PORTD &= ~_BV(PD4)
#define LED1_Up()	PORTD |= _BV(PD5)  /* Green Led */
#define LED1_Down()	PORTD &= ~_BV(PD5)
#define LED2_Up()	PORTD |= _BV(PD6)  /* Red Led */
#define LED2_Down()	PORTD &= ~_BV(PD6)
#define Test_Dev(m)	uchar n; \
	LED1_Down();             \
	LED2_Down();             \
	for(n = 0; n < 6; n++)   \
	{                        \
		LED1_Up();       \
		_delay_ms(m);    \
		wdt_reset();     \
		LED1_Down();     \
		LED2_Up();       \
		_delay_ms(m);    \
		wdt_reset();     \
		LED2_Down();     \
	}

static uint32_t f_address = 0;
static uchar prog_state = 0;
static uint32_t blkSize = 0;
static uint32_t blkPos = 0;
static uchar Index = 0, Mode_AAMux = 0;
static uchar CmdBuf[16][4];
static uchar uBuf[512];
static uint16_t uBufSize = 0, uBufPos = 0;

static uchar Read_Data()
{
	uchar byte;

	DDRB = 0; /* Set PORTB as all inputs */
	DDRC &= ~_BV(PC0); 
	DDRC &= ~_BV(PC1);
	byte = (PINB & 0x3F) | ((PINC & 3) << 6); 
	return byte;
}

static void Write_Data(uchar byte)
{
	DDRB = 0x3F; /* Set PORTB as all outputs */
	DDRC |= _BV(PC0);
	DDRC |= _BV(PC1);
	PORTB = byte & 0x3F;
	if(byte & 0x40) PORTC |= _BV(PC0);
	else PORTC &= ~_BV(PC0);
	if(byte & 0x80) PORTC |= _BV(PC1);
	else PORTC &= ~_BV(PC1);
}

static void Set_Addr(uint32_t address)
{
	wdt_reset();

	CS_Down();

	/* Port A 82c55 */
	A1_Down();
	A0_Down();
	WR_Down();
	Write_Data(address & 0xFF);
	WR_Up();

	/* Port B 82c55 */
	A1_Down();
	A0_Up();
	WR_Down();
	Write_Data((address >> 8) & 0xFF);
	WR_Up();

	/* Port C 82c55 */
	A1_Up();
	A0_Down();
	WR_Down();
	Write_Data((address >> 16) & 0xFF);
	WR_Up();

	CS_Up();
}

static void Set_Addr_AAMux(uint32_t address)
{
	wdt_reset();

	CS_Down();

	/* Port A 82c55 */
	A1_Down();
	A0_Down();
	WR_Down();
	Write_Data(address & 0xFF);
	WR_Up();

	/* Port B 82c55 */
	A1_Down();
	A0_Up();
	WR_Down();
	Write_Data((address >> 8) & 0xFF);
	WR_Up();

	CE_Down();

	/* Port A 82c55 */
	A1_Down();
	A0_Down();
	WR_Down();
	Write_Data((address >> 11) & 0xFF);
	WR_Up();

	/* Port B 82c55 */
	A1_Down();
	A0_Up();
	WR_Down();
	Write_Data((address >> 19) & 0xFF);
	WR_Up();

	CE_Up();

	CS_Up();
}

static void Put_Byte(uint32_t address, uchar byte)
{
	if(!Mode_AAMux)
	{
		Set_Addr(address);
		CE_Down();
		WE_Down();
		Write_Data(byte);
		WE_Up();
		CE_Up();
	} else {
		Set_Addr_AAMux(address);
		WE_Down();
		Write_Data(byte);
		WE_Up();
	}
}

static void Hold_Cmd(uchar byte_a, uchar byte_b, uchar byte_c, uchar byte_d)
{
	if(Index > 16) Index = 0;

	CmdBuf[Index][0] = byte_a;
	CmdBuf[Index][1] = byte_b;
	CmdBuf[Index][2] = byte_c;
	CmdBuf[Index][3] = byte_d;
	Index++;
}

static void Run_Cmd()
{
	uchar i;
	uint32_t ChkAddr;

	for(i = 0; i < Index; i++)
	{
		ChkAddr = *(uint32_t*)&CmdBuf[i][0] & 0xFFFFFF;
		if(ChkAddr == 0xFFFFFF) /* Hack for i28xxx */
			Put_Byte(f_address, CmdBuf[i][3]);
		else
			Put_Byte(ChkAddr, CmdBuf[i][3]);
	}
}

uchar usbFunctionSetup(uchar data[8])
{
	usbRequest_t *rq = (void *)data;
	prog_state = 0;

	switch(rq->bRequest) {
		case USB_FUNC_HOLD_CMD:
			Hold_Cmd(rq->wValue.bytes[0], rq->wValue.bytes[1], rq->wIndex.bytes[0], rq->wIndex.bytes[1]);
			break;
		case USB_FUNC_RUN_CMD:
			WE_Up();
			OE_Up();
			CE_Up();
			Run_Cmd();
			break;
		case USB_FUNC_BSIZE:
			blkSize = (((unsigned int)rq->wValue.word) & 0xFFFF) | ((((uint32_t)rq->wIndex.word) << 16) & 0xFFFF0000);
			break;
		case USB_FUNC_AAMUX:
			Mode_AAMux = rq->wValue.bytes[0];
			break;
		case USB_FUNC_INIT_CMD:
			Mode_AAMux = 0;
			blkSize = 0;
			blkPos = 0;
			Index = 0;
			break;
		case USB_FUNC_READ:
			prog_state = PROG_STATE_READ;
			f_address = (((unsigned int)rq->wValue.word) & 0xFFFF) | ((((uint32_t)rq->wIndex.word) << 16) & 0xFFFF0000);
			return 0xFF;
		case USB_FUNC_WRITE:
			prog_state = PROG_STATE_WRITE;
			uBufPos = 0;
			uBufSize = (uint16_t)(rq->wLength.word & 0xFFFF);
			f_address = (((unsigned int)rq->wValue.word) & 0xFFFF) | ((((uint32_t)rq->wIndex.word) << 16) & 0xFFFF0000);
			return 0xFF;
		case USB_FUNC_TEST:
		{
			Test_Dev(50);
			break;
		}
		default:
			break;
	}
	return 0;
}

uchar usbFunctionRead(uchar *data, uchar len)
{
	uchar i;

	if( prog_state == PROG_STATE_READ)
	{
		WE_Up();
		CE_Up();
		LED1_Up();
		if(!Mode_AAMux)
		OE_Down();
		for(i = 0; i < len; i++)
		{
			if(!Mode_AAMux) {
				Set_Addr(f_address);
				CE_Down();
				data[i] = Read_Data();
				CE_Up();
			} else {
				Set_Addr_AAMux(f_address);
				OE_Down();
				data[i] = Read_Data();
				OE_Up();
			}
			f_address++;
		}
		if(!Mode_AAMux)
			OE_Up();
		LED1_Down();
		return len;
	}
	return 0xff;
}

uchar usbFunctionWrite(uchar *data, uchar len)
{
	uint16_t i;

	if(prog_state == PROG_STATE_WRITE) {
		while(len-- > 0 && uBufPos < uBufSize)
		{ 
			wdt_reset();
			uBuf[uBufPos++] = *data++;
		}
		if(uBufPos < uBufSize) return 0;
		else {
			WE_Up();
			OE_Up();
			CE_Up();
			LED2_Up();
			for(i = 0; i < uBufSize; i++)
			{
				if(!blkPos && Index)
				{
					blkPos = blkSize;
					Run_Cmd();
				}
				Put_Byte(f_address, uBuf[i]);
				f_address++;
				if(blkPos)
					blkPos--;
			}
			LED2_Down();
			uBufPos = uBufSize = 0;
			return 1;
		}
	}
	return 0xff;
}

int main(void)
{
	uchar i;

	DDRB = 0;
	DDRC = 0x3F;
	DDRC &= ~_BV(PC0); 
	DDRC &= ~_BV(PC1);
	DDRD &= ~_BV(PD2); 
	DDRD |= _BV(PD1);
	DDRD |= _BV(PD3);
	DDRD |= _BV(PD4);
	DDRD |= _BV(PD5);
	DDRD |= _BV(PD6);
	DDRD |= _BV(PD7);
	PORTD = 0;
	PORTC = 0x3;
	PORTB = 0x3F;
	WE_Up();
	OE_Up();
	CE_Up();
	WR_Up();

	wdt_enable(WDTO_2S);
	Test_Dev(250);

	/* Init port 82c55 */
	CS_Down();
	A1_Up();
	A0_Up();
	WR_Down();
	Write_Data(0x80);
	WR_Up();
	CS_Up();

	usbDeviceDisconnect();
	i = 0;
	while(--i) {         /* fake USB disconnect for > 500 ms */
		wdt_reset();
		_delay_ms(2);
	}
	usbDeviceConnect();

	usbInit();
	sei();
	for(;;) {
		wdt_reset();
		usbPoll();
	}
	return 0;
}
