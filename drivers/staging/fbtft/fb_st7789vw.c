// SPDX-License-Identifier: GPL-2.0
/*
 * FB driver for the ST7789VW LCD Controller (IPS 1.14" 240x135)
 *
 * Ported from linux_card / kernel 4.14:
 *   drivers/staging/fbtft/fb_st7789vw.c
 * Used by Quark-N H3/H5 boards (same panel as fbtft_device ips_114inch_240_135).
 */

#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <video/mipi_display.h>

#include "fbtft.h"

#define DRVNAME "fb_st7789vw"

#define DEFAULT_GAMMA \
	"D0 04 0D 11 13 2B 3F 54 4C 18 0D 0B 1F 23\n" \
	"D0 04 0C 11 13 2C 3F 44 51 2F 1F 1F 20 23"

/*
 * Match linux_card fb_st7789vw: ROTATION 90 -> MADCTL 0x70 (no BGR).
 * Keep fbtft "rotate" property at 0 so framebuffer stays 240x135.
 * Pixel byte-order is handled by fbtft write_vmem16_bus8 (cpu_to_be16).
 */
#define ROTATION 90

static void LCD_Write_Command(struct fbtft_par *par, u8 data)
{
	fbtft_write_buf_dc(par, &data, 1, 0);
}

static void LCD_WriteData_Byte(struct fbtft_par *par, u8 data)
{
	fbtft_write_buf_dc(par, &data, 1, 1);
}

static void reset(struct fbtft_par *par)
{
	if (!par->gpio.reset)
		return;

	gpiod_set_value_cansleep(par->gpio.reset, 1);
	mdelay(200);
	gpiod_set_value_cansleep(par->gpio.reset, 0);
	mdelay(200);

	pr_info("%s: Reset screen done (reset-gpio ok)\n", DRVNAME);
}

static int init_display(struct fbtft_par *par)
{
	u8 madctl_par = 0;

	par->fbtftops.reset(par);

	switch (ROTATION) {
	case 0:
		madctl_par = 0x00;
		break;
	case 90:
		madctl_par = 0x70;
		break;
	case 180:
		madctl_par = 0xC0;
		break;
	case 270:
		madctl_par = 0xA0;
		break;
	}

	pr_info("%s: init 240x135 rot=%d madctl=0x%02X dc=%s reset=%s\n",
		DRVNAME, ROTATION, madctl_par,
		par->gpio.dc ? "ok" : "MISSING",
		par->gpio.reset ? "ok" : "MISSING");

	LCD_Write_Command(par, 0x36);
	LCD_WriteData_Byte(par, madctl_par);

	LCD_Write_Command(par, 0x3A);
	LCD_WriteData_Byte(par, 0x05);

	LCD_Write_Command(par, 0xB2);
	LCD_WriteData_Byte(par, 0x0C);
	LCD_WriteData_Byte(par, 0x0C);
	LCD_WriteData_Byte(par, 0x00);
	LCD_WriteData_Byte(par, 0x33);
	LCD_WriteData_Byte(par, 0x33);

	LCD_Write_Command(par, 0xB7);
	LCD_WriteData_Byte(par, 0x35);

	LCD_Write_Command(par, 0xBB);
	LCD_WriteData_Byte(par, 0x19);

	LCD_Write_Command(par, 0xC0);
	LCD_WriteData_Byte(par, 0x2C);

	LCD_Write_Command(par, 0xC2);
	LCD_WriteData_Byte(par, 0x01);

	LCD_Write_Command(par, 0xC3);
	LCD_WriteData_Byte(par, 0x12);

	LCD_Write_Command(par, 0xC4);
	LCD_WriteData_Byte(par, 0x20);

	LCD_Write_Command(par, 0xC6);
	LCD_WriteData_Byte(par, 0x0F);

	LCD_Write_Command(par, 0xD0);
	LCD_WriteData_Byte(par, 0xA4);
	LCD_WriteData_Byte(par, 0xA1);

	LCD_Write_Command(par, 0xE0);
	LCD_WriteData_Byte(par, 0xD0);
	LCD_WriteData_Byte(par, 0x04);
	LCD_WriteData_Byte(par, 0x0D);
	LCD_WriteData_Byte(par, 0x11);
	LCD_WriteData_Byte(par, 0x13);
	LCD_WriteData_Byte(par, 0x2B);
	LCD_WriteData_Byte(par, 0x3F);
	LCD_WriteData_Byte(par, 0x54);
	LCD_WriteData_Byte(par, 0x4C);
	LCD_WriteData_Byte(par, 0x18);
	LCD_WriteData_Byte(par, 0x0D);
	LCD_WriteData_Byte(par, 0x0B);
	LCD_WriteData_Byte(par, 0x1F);
	LCD_WriteData_Byte(par, 0x23);

	LCD_Write_Command(par, 0xE1);
	LCD_WriteData_Byte(par, 0xD0);
	LCD_WriteData_Byte(par, 0x04);
	LCD_WriteData_Byte(par, 0x0C);
	LCD_WriteData_Byte(par, 0x11);
	LCD_WriteData_Byte(par, 0x13);
	LCD_WriteData_Byte(par, 0x2C);
	LCD_WriteData_Byte(par, 0x3F);
	LCD_WriteData_Byte(par, 0x44);
	LCD_WriteData_Byte(par, 0x51);
	LCD_WriteData_Byte(par, 0x2F);
	LCD_WriteData_Byte(par, 0x1F);
	LCD_WriteData_Byte(par, 0x1F);
	LCD_WriteData_Byte(par, 0x20);
	LCD_WriteData_Byte(par, 0x23);

	LCD_Write_Command(par, 0x21);
	LCD_Write_Command(par, 0x11);
	LCD_Write_Command(par, 0x29);

	mdelay(200);

	pr_info("%s: Init screen done.\n", DRVNAME);
	return 0;
}

static void set_addr_win(struct fbtft_par *par, int xs, int ys, int xe, int ye)
{
	/* Same offsets as linux_card fb_st7789vw */
	xs += 40;
	xe += 40;
	ys += 53;
	ye += 53;

	write_reg(par, MIPI_DCS_SET_COLUMN_ADDRESS,
		  xs >> 8, xs & 0xFF, xe >> 8, xe & 0xFF);

	write_reg(par, MIPI_DCS_SET_PAGE_ADDRESS,
		  ys >> 8, ys & 0xFF, ye >> 8, ye & 0xFF);

	write_reg(par, MIPI_DCS_WRITE_MEMORY_START);
}

static int blank(struct fbtft_par *par, bool on)
{
	if (on)
		write_reg(par, MIPI_DCS_SET_DISPLAY_OFF);
	else
		write_reg(par, MIPI_DCS_SET_DISPLAY_ON);
	return 0;
}

static struct fbtft_display display = {
	.regwidth = 8,
	.width = 240,
	.height = 135,
	.gamma_num = 2,
	.gamma_len = 14,
	.gamma = DEFAULT_GAMMA,
	.fbtftops = {
		.reset = reset,
		.init_display = init_display,
		.set_addr_win = set_addr_win,
		.blank = blank,
	},
};

FBTFT_REGISTER_SPI_DRIVER(DRVNAME, "sitronix", "st7789vw", &display);

MODULE_DESCRIPTION("FB driver for the ST7789VW LCD Controller (240x135)");
MODULE_AUTHOR("FriendlyElec / ported for Quark-N");
MODULE_LICENSE("GPL");
