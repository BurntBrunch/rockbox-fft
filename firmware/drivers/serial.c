/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr & Nick Robinson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "button.h"
#include "config.h"
#include "cpu.h"
#include "system.h"
#include "kernel.h"
#include "lcd.h"
#include "serial.h"
#include "iap.h"

#if CONFIG_CPU == IMX31L
#include "serial-imx31.h"
#endif

#if CONFIG_CPU == SH7034

/* FIX: this doesn't work on iRiver or iPod yet */
/* iFP7xx has no remote */

/* Received byte identifiers */
#define PLAY  0xC1
#define STOP  0xC2
#define PREV  0xC4
#define NEXT  0xC8
#define VOLUP 0xD0
#define VOLDN 0xE0

void serial_setup (void)
{
    /* Set PB10 function to serial Rx */
    PBCR1 = (PBCR1 & 0xffcf) | 0x0020;

    SMR1 = 0x00;
    SCR1 = 0;
    BRR1 = (FREQ/(32*9600))-1;
    and_b(0, &SSR1); /* The status bits must be read before they are cleared,
                        so we do an AND operation */

    /* Let the hardware settle. The serial port needs to wait "at least
       the interval required to transmit or receive one bit" before it
       can be used. */
    sleep(1);

    SCR1 = 0x10; /* Enable the receiver, no interrupt */
}

int tx_rdy(void)
{
    /* a dummy */
    return 1;
}

int rx_rdy(void)
{
    if(SSR1 & SCI_RDRF)
        return 1;
    else
        return 0;
}

void tx_writec(unsigned char c)
{
    /* a dummy */
    (void)c;
}

unsigned char rx_readc(void)
{
    char tmp;
    /* Read byte and clear the Rx Full bit */
    tmp = RDR1;
    and_b(~SCI_RDRF, &SSR1);
    return tmp;
}


/* This function returns the received remote control code only if it is
   received without errors before or after the reception.
   It therefore returns the received code on the second call after the
   code has been received. */
int remote_control_rx(void)
{
    static int last_valid_button = BUTTON_NONE;
    static int last_was_error = false;
    int btn;
    int ret = BUTTON_NONE;

    /* Errors? Just clear'em. The receiver stops if we don't */
    if(SSR1 & (SCI_ORER | SCI_FER | SCI_PER)) {
        and_b(~(SCI_ORER | SCI_FER | SCI_PER), &SSR1);
        last_valid_button = BUTTON_NONE;
        last_was_error = true;
        return BUTTON_NONE;
    }

    if(rx_rdy()) {
        btn = rx_readc();

        if(last_was_error)
        {
            last_valid_button = BUTTON_NONE;
            ret = BUTTON_NONE;
        }
        else
        {
            switch (btn)
            {
                case STOP:
                    last_valid_button = BUTTON_RC_STOP;
                    break;

                case PLAY:
                    last_valid_button = BUTTON_RC_PLAY;
                    break;

                case VOLUP:
                    last_valid_button = BUTTON_RC_VOL_UP;
                    break;

                case VOLDN:
                    last_valid_button = BUTTON_RC_VOL_DOWN;
                    break;

                case PREV:
                    last_valid_button = BUTTON_RC_LEFT;
                    break;

                case NEXT:
                    last_valid_button = BUTTON_RC_RIGHT;
                    break;

                default:
                    last_valid_button = BUTTON_NONE;
                    break;
            }
        }
    }
    else
    {
        /* This means that a valid remote control character was received
           the last time we were called, with no receiver errors either before
           or after. Then we can assume that there really is a remote control
           attached, and return the button code. */
        ret = last_valid_button;
        last_valid_button = BUTTON_NONE;
    }

    last_was_error = false;

    return ret;
}

#elif defined(CPU_COLDFIRE)

void serial_setup (void)
{
    UCR0 = 0x30; /* Reset transmitter */
    UCSR0 = 0xdd; /* Timer mode */

    UCR0 = 0x10;  /* Reset pointer */
    UMR0 = 0x13; /* No parity, 8 bits */
    UMR0 = 0x07; /* 1 stop bit */

    UCR0 = 0x04; /* Tx enable */
}

int tx_rdy(void)
{
    if(USR0 & 0x04)
        return 1;
    else
        return 0;
}

int rx_rdy(void)
{
    /* a dummy */
    return 0;
}

void tx_writec(unsigned char c)
{
    UTB0 = c;
}

#elif (CONFIG_CPU == IMX31L)

void serial_setup(void)
{
#ifdef UART_INT /*enable UART Interrupts */
    UCR1_1 |= (EUARTUCR1_TRDYEN | EUARTUCR1_RRDYEN | EUARTUCR1_TXMPTYEN);
    UCR4_1 |= (EUARTUCR4_TCEN);
#else /*disable UART Interrupts*/
    UCR1_1 &= ~(EUARTUCR1_TRDYEN | EUARTUCR1_RRDYEN | EUARTUCR1_TXMPTYEN);
    UCR4_1 &= ~(EUARTUCR4_TCEN);
#endif
    UCR1_1 |= EUARTUCR1_UARTEN;
    UCR2_1 |= (EUARTUCR2_TXEN  | EUARTUCR2_RXEN | EUARTUCR2_IRTS);

    /* Tx,Rx Interrupt Trigger levels, Disable for now*/
    /*UFCR1 |= (UFCR1_TXTL_32 | UFCR1_RXTL_32);*/
}

int tx_rdy(void)
{
    if((UTS1 & EUARTUTS_TXEMPTY))
        return 1;
    else
        return 0;
}

/*Not ready...After first Rx, UTS1 & UTS1_RXEMPTY
  keeps returning true*/
int rx_rdy(void)
{
    if(!(UTS1 & EUARTUTS_RXEMPTY))
        return 1;
    else
        return 0;
}

void tx_writec(unsigned char c)
{
    UTXD1=(int) c;
}

#elif defined(IPOD_ACCESSORY_PROTOCOL)
static int autobaud = 0;
void serial_setup (void)
{
    int tmp;

#if (MODEL_NUMBER == 3) || (MODEL_NUMBER == 8)

    /* Route the Tx/Rx pins.  4G Ipod??? */
    outl(0x70000018, inl(0x70000018) & ~0xc00);
#elif (MODEL_NUMBER == 4) || (MODEL_NUMBER == 5)

    /* Route the Tx/Rx pins.  5G Ipod */
    (*(volatile unsigned long *)(0x7000008C)) &= ~0x0C;
    GPO32_ENABLE &= ~0x0C;
#endif

    DEV_EN = DEV_EN | DEV_SER0;
    CPU_HI_INT_DIS = SER0_MASK;

    DEV_RS |= DEV_SER0;
    sleep(1);
    DEV_RS &= ~DEV_SER0;

    SER0_LCR = 0x80; /* Divisor latch enable */
    SER0_DLM = 0x00;
    SER0_LCR = 0x03; /* Divisor latch disable, 8-N-1 */
    SER0_IER = 0x01;

    SER0_FCR = 0x07; /* Tx+Rx FIFO reset and FIFO enable */

    CPU_INT_EN |= HI_MASK;
    CPU_HI_INT_EN |= SER0_MASK;
    tmp = SER0_RBR;

    serial_bitrate(0);
}

void serial_bitrate(int rate)
{
    if(rate == 0)
    {
        autobaud = 2;
        SER0_LCR = 0x80; /* Divisor latch enable */
        SER0_DLL = 0x0D; /* 24000000/13/16 = 115384 baud */
        SER0_LCR = 0x03; /* Divisor latch disable, 8-N-1 */
        return;
    }

    autobaud = 0;
    SER0_LCR = 0x80; /* Divisor latch enable */
    SER0_DLL = 24000000L / rate / 16;
    SER0_LCR = 0x03; /* Divisor latch disable, 8-N-1 */
}

int tx_rdy(void)
{
    if((SER0_LSR & 0x20))
        return 1;
    else
        return 0;
}

int rx_rdy(void)
{
    if((SER0_LSR & 0x1))
        return 1;
    else
        return 0;
}

void tx_writec(unsigned char c)
{
    SER0_THR =(int) c;
}

unsigned char rx_readc(void)
{
    return (SER0_RBR & 0xFF);
}

void SERIAL0(void)
{
    static int badbaud = 0;
    static bool newpkt = true;
    char temp;

    while(rx_rdy())
    {
        temp = rx_readc();
        if (newpkt && autobaud > 0)
        {
            if (autobaud == 1)
            {
                switch (temp)
                {
                    case 0xFF:
                    case 0x55:
                        break;
                    case 0xFC:
                        SER0_LCR = 0x80; /* Divisor latch enable */
                        SER0_DLL = 0x4E; /* 24000000/78/16 = 19230 baud */
                        SER0_LCR = 0x03; /* Divisor latch disable, 8-N-1 */
                        temp = 0xFF;
                        break;
                    case 0xE0:
                        SER0_LCR = 0x80; /* Divisor latch enable */
                        SER0_DLL = 0x9C; /* 24000000/156/16 = 9615 baud */
                        SER0_LCR = 0x03; /* Divisor latch disable, 8-N-1 */
                        temp = 0xFF;
                        break;
                    default:
                        badbaud++;
                        if (badbaud >= 6) /* Switch baud detection mode */
                        {
                            autobaud = 2;
                            SER0_LCR = 0x80; /* Divisor latch enable */
                            SER0_DLL = 0x0D; /* 24000000/13/16 = 115384 baud */
                            SER0_LCR = 0x03; /* Divisor latch disable, 8-N-1 */
                            badbaud = 0;
                        } else {
                            SER0_LCR = 0x80; /* Divisor latch enable */
                            SER0_DLL = 0x1A; /* 24000000/26/16 = 57692 baud */
                            SER0_LCR = 0x03; /* Divisor latch disable, 8-N-1 */
                        }
                        continue;
                }
            } else {
                switch (temp)
                {
                    case 0xFF:
                    case 0x55:
                        break;
                    case 0xFE:
                        SER0_LCR = 0x80; /* Divisor latch enable */
                        SER0_DLL = 0x1A; /* 24000000/26/16 = 57692 baud */
                        SER0_LCR = 0x03; /* Divisor latch disable, 8-N-1 */
                        temp = 0xFF;
                        break;
                    case 0xFC:
                            SER0_LCR = 0x80; /* Divisor latch enable */
                        SER0_DLL = 0x27; /* 24000000/39/16 = 38461 baud */
                        SER0_LCR = 0x03; /* Divisor latch disable, 8-N-1 */
                        temp = 0xFF;
                        break;
                    case 0xE0:
                        SER0_LCR = 0x80; /* Divisor latch enable */
                        SER0_DLL = 0x4E; /* 24000000/78/16 = 19230 baud */
                        SER0_LCR = 0x03; /* Divisor latch disable, 8-N-1 */
                        temp = 0xFF;
                        break;
                    default:
                        badbaud++;
                        if (badbaud >= 6) /* Switch baud detection */
                        {
                            autobaud = 1;
                            SER0_LCR = 0x80; /* Divisor latch enable */
                            SER0_DLL = 0x1A; /* 24000000/26/16 = 57692 baud */
                            SER0_LCR = 0x03; /* Divisor latch disable, 8-N-1 */
                            badbaud = 0;
                        } else {
                            SER0_LCR = 0x80; /* Divisor latch enable */
                            SER0_DLL = 0x0D; /* 24000000/13/16 = 115384 baud */
                            SER0_LCR = 0x03; /* Divisor latch disable, 8-N-1 */
                        }
                        continue;
                }
            }
        }
        bool pkt = iap_getc(temp);
        if(newpkt && !pkt)
            autobaud = 0; /* Found good baud */
        newpkt = pkt;
    }
}

#endif

void dprintf(const char * str, ... )
{
    char dprintfbuff[256];
    char * ptr;

    va_list ap;
    va_start(ap, str);

    ptr = dprintfbuff;
    vsnprintf(ptr,sizeof(dprintfbuff),str,ap);
    va_end(ap);

    serial_tx((unsigned char *)ptr);
}

void serial_tx(const unsigned char * buf)
{
    /*Tx*/
    for(;;) {
        if(tx_rdy()) {
            if(*buf == '\0')
                return;
            if(*buf == '\n')
                tx_writec('\r');
            tx_writec(*buf);
            buf++;
        }
    }
}
