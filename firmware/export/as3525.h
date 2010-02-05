/*
* (C) Copyright 2006
* Copyright (C) 2006 Austriamicrosystems, by thomas.luo
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston,
* MA 02111-1307 USA
*/
#ifndef __AS3525_H__
#define __AS3525_H__

#define UART_CHANNELS 1


#if MEMORYSIZE <= 2
/* we put the codec buffer in IRAM */
#define AMS_LOWMEM
#endif

/* Virtual addresses */
/* Do not apply to the bootloader, which uses physical addresses (no MMU) */
#define DRAM_ORIG 0x30000000
#define IRAM_ORIG (DRAM_ORIG + DRAM_SIZE) /* IRAM is mapped just next to DRAM */

#define DRAM_SIZE (MEMORYSIZE * 0x100000)
#define IRAM_SIZE 0x50000


/* AS352X only supports 512 Byte HW ECC */
#define ECCSIZE 512
#define ECCBYTES 3

/* AS352X MMU Page Table Entries */
#define TTB_SIZE                  0x4000
#define TTB_BASE_ADDR             (DRAM_ORIG + DRAM_SIZE - TTB_SIZE)


/* AS352X device base addresses */


/*
------------------------------------------------------------------------
*  AS352X Registers
*
------------------------------------------------------------------------
*
*/


/* AHB */
#define USB_BASE             0xC6000000
#define VIC_BASE             0xC6010000
#define DMAC_BASE            0xC6020000
#define MPMC_BASE            0xC6030000
#define MEMSTICK_BASE        0xC6040000
#define CF_IDE_BASE          0xC6050000

/* APB */
#define NAND_FLASH_BASE      0xC8000000
#define BIST_MANAGER_BASE    0xC8010000
#define SD_MCI_BASE          0xC8020000
#define TIMER_BASE           0xC8040000
#define WDT_BASE             0xC8050000
#define I2C_MS_BASE          0xC8060000
#define I2C_AUDIO_BASE       0xC8070000
#define SSP_BASE             0xC8080000
#define I2SIN_BASE           0xC8090000
#define I2SOUT_BASE          0xC80A0000
#define GPIOA_BASE           0xC80B0000
#define GPIOB_BASE           0xC80C0000
#define GPIOC_BASE           0xC80D0000
#define GPIOD_BASE           0xC80E0000
#define CGU_BASE             0xC80F0000
#define CCU_BASE             0xC8100000
#define UART0_BASE           0xC8110000
#define DBOP_BASE            0xC8120000








/*
------------------------------------------------------------------------
*  AS352X control registers
*
------------------------------------------------------------------------
*/

#define CCU_SRC           (*(volatile unsigned long *)(CCU_BASE + 0x00))
#define CCU_SRL           (*(volatile unsigned long *)(CCU_BASE + 0x04))
#define CCU_MEMMAP        (*(volatile unsigned long *)(CCU_BASE + 0x08))
#define CCU_IO            (*(volatile unsigned long *)(CCU_BASE + 0x0C))
#define CCU_SCON          (*(volatile unsigned long *)(CCU_BASE + 0x10))
#define CCU_VERS          (*(volatile unsigned long *)(CCU_BASE + 0x14))
#define CCU_SPARE1        (*(volatile unsigned long *)(CCU_BASE + 0x18))
#define CCU_SPARE2        (*(volatile unsigned long *)(CCU_BASE + 0x1C))

/* DBOP */
#define DBOP_TIMPOL_01    (*(volatile unsigned long *)(DBOP_BASE + 0x00))
#define DBOP_TIMPOL_23    (*(volatile unsigned long *)(DBOP_BASE + 0x04))
#define DBOP_CTRL         (*(volatile unsigned long *)(DBOP_BASE + 0x08))
#define DBOP_STAT         (*(volatile unsigned long *)(DBOP_BASE + 0x0C))
/* default is 16bit, but we switch to 32bit for some targets for better speed */
#define DBOP_DOUT         (*(volatile unsigned short*)(DBOP_BASE + 0x10))
#define DBOP_DOUT16       (*(volatile unsigned short*)(DBOP_BASE + 0x10))
#define DBOP_DOUT32       (*(volatile unsigned long *)(DBOP_BASE + 0x10))
#define DBOP_DIN          (*(volatile unsigned short*)(DBOP_BASE + 0x14))


/**
* Reset Control Lines in CCU_SRC register
**/
#define CCU_SRC_DBOP_EN        ( 1 << 24 )
#define CCU_SRC_SPDIF_EN       ( 1 << 22 )
#define CCU_SRC_TIMER_EN       ( 1 << 21 )
#define CCU_SRC_SSP_EN         ( 1 << 20 )
#define CCU_SRC_WDO_EN         ( 1 << 19 )
#define CCU_SRC_IDE_EN         ( 1 << 18 )
#define CCU_SRC_IDE_AHB_EN     ( 1 << 17 )
#define CCU_SRC_UART0          ( 1 << 16 )
#define CCU_SRC_NAF_EN         ( 1 << 15 )
#define CCU_SRC_SDMCI_EN       ( 1 << 14 )
#define CCU_SRC_GPIO_EN        ( 1 << 13 )
#define CCU_SRC_I2C_AUDIO_EN   ( 1 << 12 )
#define CCU_SRC_I2C_EN         ( 1 << 11 )
#define CCU_SRC_MST_EN         ( 1 << 10 )
#define CCU_SRC_I2SIN          ( 1 <<  9 )
#define CCU_SRC_I2SOUT         ( 1 <<  8 )
#define CCU_SRC_USB_AHB_EN     ( 1 <<  7 )
#define CCU_SRC_USB_PHY_EN     ( 1 <<  6 )
#define CCU_SRC_DMAC_EN        ( 1 <<  5 )
#define CCU_SRC_VIC_EN         ( 1 <<  4 )

/**
* Magic number for CCU_SRL for reset.
**/
#define CCU_SRL_MAGIC_NUMBER      0x1A720212

/**
* Chip select lines for NAF. Use these constants to select/deselct the
CE lines
* for NAND flashes in Register CCU_IO.
**/
#define CCU_IO_NAF_CE_LINE_0      ( 0 << 7 )
#define CCU_IO_NAF_CE_LINE_1      ( 1 << 7 )
#define CCU_IO_NAF_CE_LINE_2      ( 2 << 7 )
#define CCU_IO_NAF_CE_LINE_3      ( 3 << 7 )

/* CCU IO Select/Deselect IDE */
#define CCU_IO_IDE                ( 1 << 5 )

/* CCU IO Select/desect I2C */
#define CCU_IO_I2C_MASTER_SLAVE   ( 1 << 1 )

/* CCU IO Select/desect UART */
#define CCU_IO_UART0               ( 1 << 0 )


#define CCU_RESET_ALL_BUT_MEMORY \
 ( CCU_SRC_DBOP_EN        \
 | CCU_SRC_SPDIF_EN       \
 | CCU_SRC_TIMER_EN       \
 | CCU_SRC_SSP_EN         \
 | CCU_SRC_WDO_EN         \
 | CCU_SRC_IDE_EN         \
 | CCU_SRC_IDE_AHB_EN     \
 | CCU_SRC_UART0          \
 | CCU_SRC_NAF_EN         \
 | CCU_SRC_SDMCI_EN       \
 | CCU_SRC_GPIO_EN        \
 | CCU_SRC_I2C_AUDIO_EN   \
 | CCU_SRC_I2C_EN         \
 | CCU_SRC_MST_EN         \
 | CCU_SRC_I2SIN          \
 | CCU_SRC_I2SOUT         \
 | CCU_SRC_USB_AHB_EN     \
 | CCU_SRC_USB_PHY_EN     \
 | CCU_SRC_DMAC_EN        \
 | CCU_SRC_VIC_EN         \
 )

#define CCU_IO_UART               ( 1 << 0 )
/*
------------------------------------------------------------------------
*  AS352X clock control registers
*
------------------------------------------------------------------------
*/

#define CGU_PLLA         (*(volatile unsigned long *)(CGU_BASE + 0x00))
#define CGU_PLLB         (*(volatile unsigned long *)(CGU_BASE + 0x04))
#define CGU_PLLASUP      (*(volatile unsigned long *)(CGU_BASE + 0x08))
#define CGU_PLLBSUP      (*(volatile unsigned long *)(CGU_BASE + 0x0C))
#define CGU_PROC         (*(volatile unsigned long *)(CGU_BASE + 0x10))
#define CGU_PERI         (*(volatile unsigned long *)(CGU_BASE + 0x14))
#define CGU_AUDIO        (*(volatile unsigned long *)(CGU_BASE + 0x18))
#define CGU_USB          (*(volatile unsigned long *)(CGU_BASE + 0x1C))
#define CGU_INTCTRL      (*(volatile unsigned long *)(CGU_BASE + 0x20))
#define CGU_IRQ          (*(volatile unsigned long *)(CGU_BASE + 0x24))
#define CGU_COUNTA       (*(volatile unsigned long *)(CGU_BASE + 0x28))
#define CGU_COUNTB       (*(volatile unsigned long *)(CGU_BASE + 0x2C))
#define CGU_IDE          (*(volatile unsigned long *)(CGU_BASE + 0x30))
#define CGU_MEMSTICK     (*(volatile unsigned long *)(CGU_BASE + 0x34))
#define CGU_DBOP         (*(volatile unsigned long *)(CGU_BASE + 0x38))

#define CGU_VIC_CLOCK_ENABLE                 ( 1 << 23 ) /* vic */
/* --- are disabled after reset --- */
#define CGU_DMA_CLOCK_ENABLE                 ( 1 << 22 ) /* dma */
#define CGU_USB_CLOCK_ENABLE                 ( 1 << 21 ) /* usb */
#define CGU_I2SOUT_APB_CLOCK_ENABLE          ( 1 << 20 ) /* i2sout */
#define CGU_I2SIN_APB_CLOCK_ENABLE           ( 1 << 19 ) /* i2sin */
#define CGU_I2C_MASTER_SLAVE_CLOCK_ENABLE    ( 1 << 18 ) /* i2c master/slave */
#define CGU_I2C_AUDIO_MASTER_CLOCK_ENABLE    ( 1 << 17 ) /* i2c audio master */
#define CGU_GPIO_CLOCK_ENABLE                ( 1 << 16 ) /* gpio */
#define CGU_MCI_CLOCK_ENABLE                 ( 1 << 15 ) /* mmc + sd */
#define CGU_NAF_CLOCK_ENABLE                 ( 1 << 14 ) /* naf */
#define CGU_UART_APB_CLOCK_ENABLE            ( 1 << 13 ) /* uart */
#define CGU_WDOCNT_CLOCK_ENABLE              ( 1 << 12 ) /* watchdog counter */
#define CGU_WDOIF_CLOCK_ENABLE               ( 1 << 11 ) /* watchdog timer module */
#define CGU_SSP_CLOCK_ENABLE                 ( 1 << 10 ) /* ssp */
#define CGU_TIMER1_CLOCK_ENABLE              ( 1 <<  9 ) /* timer 1 */
#define CGU_TIMER2_CLOCK_ENABLE              ( 1 <<  8 ) /* timer 2 */
#define CGU_TIMERIF_CLOCK_ENABLE             ( 1 <<  7 ) /* timer
interface */

/**  ------------------------------------------------------------------
* Number of cycles to wait before cgu is safely locked.
**/
#define CGU_LOCK_CNT 0xFF

/* FIFO depth is 16 for tx and rx fifo */
#define UART_FIFO_DEPTH           16

/* ------------------- UART Line Control Register bit fields -------------------- */

#define UART_LNCTL_DLSEN          (1 << 7) /* Device latch select bit */


/* --------------   UART Interrupt Control Register bit fields --------------- */

#define UART_INTR_RXDRDY           0x1 /* Data ready interrupt */
#define UART_INTR_TXEMT            0x2 /* Transmit data empty interrupt */
#define UART_INTR_RXLINESTATUS     0x4 /* Receive line status interrupt */

/* ------------------- UART Line Status Register bit fields --------------------  */

#define UART_ERRORBITS             0x1E
#define UART_RX_DATA_READY        (1 << 0)
#define UART_TX_HOLD_EMPTY    (1 << 5)

/* -------------------        FIFO CNTL Register contants -------------------*/

#define UART_FIFO_EN              (1 << 0)  /* Enable the UART FIFO */
#define UART_TX_FIFO_RST          (1 << 1)  /* Enable the UART FIFO */
#define UART_RX_FIFO_RST          (1 << 2)
#define UART_RXFIFO_TRIGLVL_1     (0 << 4) /* RX FIFO TRIGGER_LEVEL 1 */
#define UART_RXFIFO_TRIGLVL_4      0x08    /* RX FIFO TRIGGER_LEVEL 4 */
#define UART_RXFIFO_TRIGLVL_8      0x10    /* RX FIFO TRIGGER_LEVEL 8 */
#define UART_RXFIFO_TRIGLVL_14     0x18    /* RX FIFO TRIGGER_LEVEL 14 */


/* -------------------        FIFO status Register contants ------------------*/
#define UART_TX_FIFO_FULL         (1 << 0)
#define UART_RX_FIFO_FULL         (1 << 1)
#define UART_TX_FIFO_EMPTY        (1 << 2)
#define UART_RX_FIFO_EMPTY        (1 << 3)


/* ----------------------- defines ---------------------------------------- */



#define UART_DATA_REG       (*(volatile unsigned long*)(UART0_BASE + 0x00)) /* Data register */
#define UART_DLO_REG        (*(volatile unsigned long*)(UART0_BASE + 0x00)) /* Clock divider(lower byte) register */
#define UART_DHI_REG        (*(volatile unsigned long*)(UART0_BASE + 0x04)) /* Clock divider(higher byte) register */
#define UART_INTEN_REG      (*(volatile unsigned long*)(UART0_BASE + 0x04)) /* Interrupt enable register */
#define UART_INTSTATUS_REG  (*(volatile unsigned long*)(UART0_BASE + 0x08)) /* Interrupt status register */
#define UART_FCTL_REG       (*(volatile unsigned long*)(UART0_BASE + 0x0C)) /* Fifo control  register */
#define UART_FSTATUS_REG    (*(volatile unsigned long*)(UART0_BASE + 0x0C)) /* Fifo status register */
#define UART_LNCTL_REG      (*(volatile unsigned long*)(UART0_BASE + 0x10)) /* Line control register */
#define UART_LNSTATUS_REG   (*(volatile unsigned long*)(UART0_BASE + 0x14)) /* Line status register */


#define SD_MCI_POWER        (*(volatile unsigned long*)(SD_MCI_BASE + 0x0))


#define TIMER1_LOAD      (*(volatile unsigned long*)(TIMER_BASE + 0x00)) /* 32-bit width */
#define TIMER1_VALUE     (*(volatile unsigned long*)(TIMER_BASE + 0x04)) /* 32 bit width */
#define TIMER1_CONTROL   (*(volatile unsigned long*)(TIMER_BASE + 0x08)) /*  8 bit width */
#define TIMER1_INTCLR    (*(volatile unsigned long*)(TIMER_BASE + 0x0C)) /* clears ir by write access */
#define TIMER1_RIS       (*(volatile unsigned long*)(TIMER_BASE + 0x10)) /*  1 bit width */
#define TIMER1_MIS       (*(volatile unsigned long*)(TIMER_BASE + 0x14)) /*  1 bit width */
#define TIMER1_BGLOAD    (*(volatile unsigned long*)(TIMER_BASE + 0x18)) /* 32-bit width */

#define TIMER2_LOAD      (*(volatile unsigned long*)(TIMER_BASE + 0x20)) /* 32-bit width */
#define TIMER2_VALUE     (*(volatile unsigned long*)(TIMER_BASE + 0x24)) /* 32 bit width */
#define TIMER2_CONTROL   (*(volatile unsigned long*)(TIMER_BASE + 0x28)) /*  8 bit width */
#define TIMER2_INTCLR    (*(volatile unsigned long*)(TIMER_BASE + 0x2C)) /* clears ir by write access */
#define TIMER2_RIS       (*(volatile unsigned long*)(TIMER_BASE + 0x30)) /*  1 bit width */
#define TIMER2_MIS       (*(volatile unsigned long*)(TIMER_BASE + 0x34)) /*  1 bit width */
#define TIMER2_BGLOAD    (*(volatile unsigned long*)(TIMER_BASE + 0x38)) /* 32-bit width */

/**
* Counter/Timer control register bits
**/
#define TIMER_ENABLE       0x80
#define TIMER_PERIODIC     0x40
#define TIMER_INT_ENABLE   0x20
#define TIMER_32_BIT   0x02
#define TIMER_ONE_SHOT   0x01
#define TIMER_PRESCALE_1   0x00
#define TIMER_PRESCALE_16   0x04
#define TIMER_PRESCALE_256 0x08


/* Watchdog registers */
#define WDT_LOAD        (*(volatile unsigned long*)(WDT_BASE))
#define WDT_CONTROL     (*(volatile unsigned long*)(WDT_BASE+8))


/* GPIO registers */

#define GPIOA_DIR       (*(volatile unsigned char*)(GPIOA_BASE+0x400))
#define GPIOA_IS        (*(volatile unsigned char*)(GPIOA_BASE+0x404))
#define GPIOA_IBE       (*(volatile unsigned char*)(GPIOA_BASE+0x408))
#define GPIOA_IEV       (*(volatile unsigned char*)(GPIOA_BASE+0x40C))
#define GPIOA_IE        (*(volatile unsigned char*)(GPIOA_BASE+0x410))
#define GPIOA_RIS       (*(volatile unsigned char*)(GPIOA_BASE+0x414))
#define GPIOA_MIS       (*(volatile unsigned char*)(GPIOA_BASE+0x418))
#define GPIOA_IC        (*(volatile unsigned char*)(GPIOA_BASE+0x41C))
#define GPIOA_AFSEL     (*(volatile unsigned char*)(GPIOA_BASE+0x420))
#define GPIOA_PIN(a)    (*(volatile unsigned char*)(GPIOA_BASE+(1<<((a)+2))))
#define GPIOA_DATA      (*(volatile unsigned char*)(GPIOA_BASE+(0xff<<2)))


#define GPIOB_DIR       (*(volatile unsigned char*)(GPIOB_BASE+0x400))
#define GPIOB_IS        (*(volatile unsigned char*)(GPIOB_BASE+0x404))
#define GPIOB_IBE       (*(volatile unsigned char*)(GPIOB_BASE+0x408))
#define GPIOB_IEV       (*(volatile unsigned char*)(GPIOB_BASE+0x40C))
#define GPIOB_IE        (*(volatile unsigned char*)(GPIOB_BASE+0x410))
#define GPIOB_RIS       (*(volatile unsigned char*)(GPIOB_BASE+0x414))
#define GPIOB_MIS       (*(volatile unsigned char*)(GPIOB_BASE+0x418))
#define GPIOB_IC        (*(volatile unsigned char*)(GPIOB_BASE+0x41C))
#define GPIOB_AFSEL     (*(volatile unsigned char*)(GPIOB_BASE+0x420))
#define GPIOB_PIN(a)    (*(volatile unsigned char*)(GPIOB_BASE+(1<<((a)+2))))
#define GPIOB_DATA      (*(volatile unsigned char*)(GPIOB_BASE+(0xff<<2)))

#define GPIOC_DIR       (*(volatile unsigned char*)(GPIOC_BASE+0x400))
#define GPIOC_IS        (*(volatile unsigned char*)(GPIOC_BASE+0x404))
#define GPIOC_IBE       (*(volatile unsigned char*)(GPIOC_BASE+0x408))
#define GPIOC_IEV       (*(volatile unsigned char*)(GPIOC_BASE+0x40C))
#define GPIOC_IE        (*(volatile unsigned char*)(GPIOC_BASE+0x410))
#define GPIOC_RIS       (*(volatile unsigned char*)(GPIOC_BASE+0x414))
#define GPIOC_MIS       (*(volatile unsigned char*)(GPIOC_BASE+0x418))
#define GPIOC_IC        (*(volatile unsigned char*)(GPIOC_BASE+0x41C))
#define GPIOC_AFSEL     (*(volatile unsigned char*)(GPIOC_BASE+0x420))
#define GPIOC_PIN(a)    (*(volatile unsigned char*)(GPIOC_BASE+(1<<((a)+2))))
#define GPIOC_DATA      (*(volatile unsigned char*)(GPIOC_BASE+(0xff<<2)))

#define GPIOD_DIR       (*(volatile unsigned char*)(GPIOD_BASE+0x400))
#define GPIOD_IS        (*(volatile unsigned char*)(GPIOD_BASE+0x404))
#define GPIOD_IBE       (*(volatile unsigned char*)(GPIOD_BASE+0x408))
#define GPIOD_IEV       (*(volatile unsigned char*)(GPIOD_BASE+0x40C))
#define GPIOD_IE        (*(volatile unsigned char*)(GPIOD_BASE+0x410))
#define GPIOD_RIS       (*(volatile unsigned char*)(GPIOD_BASE+0x414))
#define GPIOD_MIS       (*(volatile unsigned char*)(GPIOD_BASE+0x418))
#define GPIOD_IC        (*(volatile unsigned char*)(GPIOD_BASE+0x41C))
#define GPIOD_AFSEL     (*(volatile unsigned char*)(GPIOD_BASE+0x420))
#define GPIOD_PIN(a)    (*(volatile unsigned char*)(GPIOD_BASE+(1<<((a)+2))))
#define GPIOD_DATA      (*(volatile unsigned char*)(GPIOD_BASE+(0xff<<2)))

/* ARM PL172 Memory Controller registers */

#define MPMC_CONTROL              (*(volatile unsigned long*)(MPMC_BASE+0x000))
#define MPMC_STATUS               (*(volatile unsigned long*)(MPMC_BASE+0x004))
#define MPMC_CONFIG               (*(volatile unsigned long*)(MPMC_BASE+0x008))

#define MPMC_DYNAMIC_CONTROL      (*(volatile unsigned long*)(MPMC_BASE+0x020))
#define MPMC_DYNAMIC_REFRESH      (*(volatile unsigned long*)(MPMC_BASE+0x024))
#define MPMC_DYNAMIC_READ_CONFIG  (*(volatile unsigned long*)(MPMC_BASE+0x028))
#define MPMC_DYNAMIC_tRP          (*(volatile unsigned long*)(MPMC_BASE+0x030))
#define MPMC_DYNAMIC_tRAS         (*(volatile unsigned long*)(MPMC_BASE+0x034))
#define MPMC_DYNAMIC_tSREX        (*(volatile unsigned long*)(MPMC_BASE+0x038))
#define MPMC_DYNAMIC_tAPR         (*(volatile unsigned long*)(MPMC_BASE+0x03C))
#define MPMC_DYNAMIC_tDAL         (*(volatile unsigned long*)(MPMC_BASE+0x040))
#define MPMC_DYNAMIC_tWR          (*(volatile unsigned long*)(MPMC_BASE+0x044))
#define MPMC_DYNAMIC_tRC          (*(volatile unsigned long*)(MPMC_BASE+0x048))
#define MPMC_DYNAMIC_tRFC         (*(volatile unsigned long*)(MPMC_BASE+0x04C))
#define MPMC_DYNAMIC_tXSR         (*(volatile unsigned long*)(MPMC_BASE+0x050))
#define MPMC_DYNAMIC_tRRD         (*(volatile unsigned long*)(MPMC_BASE+0x054))
#define MPMC_DYNAMIC_tMRD         (*(volatile unsigned long*)(MPMC_BASE+0x058))

#define MPMC_STATIC_EXTENDED_WAIT (*(volatile unsigned long*)(MPMC_BASE+0x080))

#define MPMC_DYNAMIC_CONFIG_0     (*(volatile unsigned long*)(MPMC_BASE+0x100))
#define MPMC_DYNAMIC_CONFIG_1     (*(volatile unsigned long*)(MPMC_BASE+0x120))
#define MPMC_DYNAMIC_CONFIG_2     (*(volatile unsigned long*)(MPMC_BASE+0x140))
#define MPMC_DYNAMIC_CONFIG_3     (*(volatile unsigned long*)(MPMC_BASE+0x160))

#define MPMC_DYNAMIC_RASCAS_0     (*(volatile unsigned long*)(MPMC_BASE+0x104))
#define MPMC_DYNAMIC_RASCAS_1     (*(volatile unsigned long*)(MPMC_BASE+0x124))
#define MPMC_DYNAMIC_RASCAS_2     (*(volatile unsigned long*)(MPMC_BASE+0x144))
#define MPMC_DYNAMIC_RASCAS_3     (*(volatile unsigned long*)(MPMC_BASE+0x164))

#define MPMC_PERIPH_ID2           (*(volatile unsigned long*)(MPMC_BASE+0xFE8))

/* VIC controller (PL190) registers */

#define VIC_IRQ_STATUS      (*(volatile unsigned long*)(VIC_BASE+0x00))
#define VIC_FIQ_STATUS      (*(volatile unsigned long*)(VIC_BASE+0x04))
#define VIC_RAW_INTR        (*(volatile unsigned long*)(VIC_BASE+0x08))
#define VIC_INT_SELECT      (*(volatile unsigned long*)(VIC_BASE+0x0C))
#define VIC_INT_ENABLE      (*(volatile unsigned long*)(VIC_BASE+0x10))
#define VIC_INT_EN_CLEAR    (*(volatile unsigned long*)(VIC_BASE+0x14))
#define VIC_SOFT_INT        (*(volatile unsigned long*)(VIC_BASE+0x18))
#define VIC_SOFT_INT_CLEAR  (*(volatile unsigned long*)(VIC_BASE+0x1C))
#define VIC_PROTECTION      (*(volatile unsigned long*)(VIC_BASE+0x20))
#define VIC_VECT_ADDR       (*(volatile unsigned long*)(VIC_BASE+0x30))
#define VIC_DEF_VECT_ADDR   (*(volatile unsigned long*)(VIC_BASE+0x34))
#define VIC_VECT_ADDRS      ((volatile unsigned long*)(VIC_BASE+0x100))
#define VIC_VECT_CNTLS      ((volatile unsigned long*)(VIC_BASE+0x200))

/* Interrupt sources (for vectors setup) */
#define INT_SRC_WATCHDOG      0
#define INT_SRC_TIMER1        1
#define INT_SRC_TIMER2        2
#define INT_SRC_USB           3
#define INT_SRC_DMAC          4
#define INT_SRC_NAND          5
#define INT_SRC_IDE           6
#define INT_SRC_MCI0          7
#define INT_SRC_MCI1          8
#define INT_SRC_AUDIO         9
#define INT_SRC_SSP           10
#define INT_SRC_I2C_MS        11
#define INT_SRC_I2C_AUDIO     12
#define INT_SRC_I2SIN         13
#define INT_SRC_I2SOUT        14
#define INT_SRC_UART          15
#define INT_SRC_GPIOD         16
/* 17 reserved */
#define INT_SRC_CGU           18
#define INT_SRC_MEMORY_STICK  19
#define INT_SRC_DBOP          20
/* 21-28 reserved */
#define INT_SRC_GPIOA         29
#define INT_SRC_GPIOB         30
#define INT_SRC_GPIOC         31

/* Interrupt sources bitmask */
#define INTERRUPT_WATCHDOG      (1<<0)
#define INTERRUPT_TIMER1        (1<<1)
#define INTERRUPT_TIMER2        (1<<2)
#define INTERRUPT_USB           (1<<3)
#define INTERRUPT_DMAC          (1<<4)
#define INTERRUPT_NAND          (1<<5)
#define INTERRUPT_IDE           (1<<6)
#define INTERRUPT_MCI0          (1<<7)
#define INTERRUPT_MCI1          (1<<8)
#define INTERRUPT_AUDIO         (1<<9)
#define INTERRUPT_SSP           (1<<10)
#define INTERRUPT_I2C_MS        (1<<11)
#define INTERRUPT_I2C_AUDIO     (1<<12)
#define INTERRUPT_I2SIN         (1<<13)
#define INTERRUPT_I2SOUT        (1<<14)
#define INTERRUPT_UART          (1<<15)
#define INTERRUPT_GPIOD         (1<<16)
/* 17 reserved */
#define INTERRUPT_CGU           (1<<18)
#define INTERRUPT_MEMORY_STICK  (1<<19)
#define INTERRUPT_DBOP          (1<<20)
/* 21-28 reserved */
#define INTERRUPT_GPIOA         (1<<29)
#define INTERRUPT_GPIOB         (1<<30)
#define INTERRUPT_GPIOC         (1<<31)

/* I2SOUT registers */

#define I2SOUT_CONTROL      (*(volatile unsigned char*)(I2SOUT_BASE+0x00))
#define I2SOUT_MASK         (*(volatile unsigned char*)(I2SOUT_BASE+0x04))
#define I2SOUT_RAW_STATUS   (*(volatile unsigned char*)(I2SOUT_BASE+0x08))
#define I2SOUT_STATUS       (*(volatile unsigned char*)(I2SOUT_BASE+0x0C))
#define I2SOUT_CLEAR        (*(volatile unsigned char*)(I2SOUT_BASE+0x10))
#define I2SOUT_DATA         (volatile unsigned long*)(I2SOUT_BASE+0x14)


/* SSP registers (PrimeCell PL022) */

#define SSP_CR0             (*(volatile unsigned short*)(SSP_BASE+0x00))
#define SSP_CR1             (*(volatile unsigned char*)(SSP_BASE+0x04))
#define SSP_DATA            (*(volatile unsigned short*)(SSP_BASE+0x08))
#define SSP_SR              (*(volatile unsigned char*)(SSP_BASE+0x0C))
#define SSP_CPSR            (*(volatile unsigned char*)(SSP_BASE+0x10))
#define SSP_IMSC            (*(volatile unsigned char*)(SSP_BASE+0x14))
#define SSP_IRS             (*(volatile unsigned char*)(SSP_BASE+0x18))
#define SSP_MIS             (*(volatile unsigned char*)(SSP_BASE+0x1C))
#define SSP_ICR             (*(volatile unsigned char*)(SSP_BASE+0x20))
#define SSP_DMACR           (*(volatile unsigned char*)(SSP_BASE+0x24))

/* PCM addresses for obtaining buffers will be what DMA is using (physical) */
#define HAVE_PCM_DMA_ADDRESS

/* Timer frequency */
#define TIMER_FREQ (24000000 / 16)

/* USB : TODO */
#define USB_NUM_ENDPOINTS 4
#define USB_DEVBSS_ATTR IBSS_ATTR

/* I2SIN registers */

#define I2SIN_CONTROL      (*(volatile unsigned long*)(I2SIN_BASE+0x00))
#define I2SIN_MASK         (*(volatile unsigned char*)(I2SIN_BASE+0x04))
#define I2SIN_RAW_STATUS   (*(volatile unsigned char*)(I2SIN_BASE+0x08))
#define I2SIN_STATUS       (*(volatile unsigned char*)(I2SIN_BASE+0x0C))
#define I2SIN_CLEAR        (*(volatile unsigned char*)(I2SIN_BASE+0x10))
#define I2SIN_DATA         (volatile unsigned long*)(I2SIN_BASE+0x14)
#define I2SIN_SPDIF_STATUS (*(volatile unsigned long*)(I2SIN_BASE+0x18))

/* I2SIN_MASK */

#define I2SIN_MASK_PUER   ( 1<<6 ) /* push error */
#define I2SIN_MASK_POE    ( 1<<5 ) /* empty */
#define I2SIN_MASK_POAE   ( 1<<4 ) /* almost empty */
#define I2SIN_MASK_POHF   ( 1<<3 ) /* half full */
#define I2SIN_MASK_POAF   ( 1<<2 ) /* almost full */
#define I2SIN_MASK_POF    ( 1<<1 ) /* full */
#define I2SIN_MASK_POER   ( 1<<0 ) /* pop error */

#endif /*__AS3525_H__*/
