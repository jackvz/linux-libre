/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Afatech AF9013 demodulator driver
 *
 * Copyright (C) 2007 Antti Palosaari <crope@iki.fi>
 * Copyright (C) 2011 Antti Palosaari <crope@iki.fi>
 *
 * Thanks to Afatech who kindly provided information.
 */

#ifndef AF9013_PRIV_H
#define AF9013_PRIV_H

#include <media/dvb_frontend.h>
#include <media/dvb_math.h>
#include "af9013.h"
#include <linux/firmware.h>
#include <linux/i2c-mux.h>
#include <linux/math64.h>
#include <linux/regmap.h>

#define AF9013_FIRMWARE "/*(DEBLOBBED)*/"

struct af9013_reg_mask_val {
	u16 reg;
	u8  mask;
	u8  val;
};

struct af9013_coeff {
	u32 clock;
	u32 bandwidth_hz;
	u8 val[24];
};

/* pre-calculated coeff lookup table */
static const struct af9013_coeff coeff_lut[] = {
	/* 28.800 MHz */
	{ 28800000, 8000000, { 0x02, 0x8a, 0x28, 0xa3, 0x05, 0x14,
		0x51, 0x11, 0x00, 0xa2, 0x8f, 0x3d, 0x00, 0xa2, 0x8a,
		0x29, 0x00, 0xa2, 0x85, 0x14, 0x01, 0x45, 0x14, 0x14 } },
	{ 28800000, 7000000, { 0x02, 0x38, 0xe3, 0x8e, 0x04, 0x71,
		0xc7, 0x07, 0x00, 0x8e, 0x3d, 0x55, 0x00, 0x8e, 0x38,
		0xe4, 0x00, 0x8e, 0x34, 0x72, 0x01, 0x1c, 0x71, 0x32 } },
	{ 28800000, 6000000, { 0x01, 0xe7, 0x9e, 0x7a, 0x03, 0xcf,
		0x3c, 0x3d, 0x00, 0x79, 0xeb, 0x6e, 0x00, 0x79, 0xe7,
		0x9e, 0x00, 0x79, 0xe3, 0xcf, 0x00, 0xf3, 0xcf, 0x0f } },
	/* 20.480 MHz */
	{ 20480000, 8000000, { 0x03, 0x92, 0x49, 0x26, 0x07, 0x24,
		0x92, 0x13, 0x00, 0xe4, 0x99, 0x6e, 0x00, 0xe4, 0x92,
		0x49, 0x00, 0xe4, 0x8b, 0x25, 0x01, 0xc9, 0x24, 0x25 } },
	{ 20480000, 7000000, { 0x03, 0x20, 0x00, 0x01, 0x06, 0x40,
		0x00, 0x00, 0x00, 0xc8, 0x06, 0x40, 0x00, 0xc8, 0x00,
		0x00, 0x00, 0xc7, 0xf9, 0xc0, 0x01, 0x90, 0x00, 0x00 } },
	{ 20480000, 6000000, { 0x02, 0xad, 0xb6, 0xdc, 0x05, 0x5b,
		0x6d, 0x2e, 0x00, 0xab, 0x73, 0x13, 0x00, 0xab, 0x6d,
		0xb7, 0x00, 0xab, 0x68, 0x5c, 0x01, 0x56, 0xdb, 0x1c } },
	/* 28.000 MHz */
	{ 28000000, 8000000, { 0x02, 0x9c, 0xbc, 0x15, 0x05, 0x39,
		0x78, 0x0a, 0x00, 0xa7, 0x34, 0x3f, 0x00, 0xa7, 0x2f,
		0x05, 0x00, 0xa7, 0x29, 0xcc, 0x01, 0x4e, 0x5e, 0x03 } },
	{ 28000000, 7000000, { 0x02, 0x49, 0x24, 0x92, 0x04, 0x92,
		0x49, 0x09, 0x00, 0x92, 0x4d, 0xb7, 0x00, 0x92, 0x49,
		0x25, 0x00, 0x92, 0x44, 0x92, 0x01, 0x24, 0x92, 0x12 } },
	{ 28000000, 6000000, { 0x01, 0xf5, 0x8d, 0x10, 0x03, 0xeb,
		0x1a, 0x08, 0x00, 0x7d, 0x67, 0x2f, 0x00, 0x7d, 0x63,
		0x44, 0x00, 0x7d, 0x5f, 0x59, 0x00, 0xfa, 0xc6, 0x22 } },
	/* 25.000 MHz */
	{ 25000000, 8000000, { 0x02, 0xec, 0xfb, 0x9d, 0x05, 0xd9,
		0xf7, 0x0e, 0x00, 0xbb, 0x44, 0xc1, 0x00, 0xbb, 0x3e,
		0xe7, 0x00, 0xbb, 0x39, 0x0d, 0x01, 0x76, 0x7d, 0x34 } },
	{ 25000000, 7000000, { 0x02, 0x8f, 0x5c, 0x29, 0x05, 0x1e,
		0xb8, 0x14, 0x00, 0xa3, 0xdc, 0x29, 0x00, 0xa3, 0xd7,
		0x0a, 0x00, 0xa3, 0xd1, 0xec, 0x01, 0x47, 0xae, 0x05 } },
	{ 25000000, 6000000, { 0x02, 0x31, 0xbc, 0xb5, 0x04, 0x63,
		0x79, 0x1b, 0x00, 0x8c, 0x73, 0x91, 0x00, 0x8c, 0x6f,
		0x2d, 0x00, 0x8c, 0x6a, 0xca, 0x01, 0x18, 0xde, 0x17 } },
};

/*
 * Afatech AF9013 demod init
 */
static const struct af9013_reg_mask_val demod_init_tab[] = {
	{0xd73a, 0xff, 0xa1},
	{0xd73b, 0xff, 0x1f},
	{0xd73c, 0xf0, 0xa0},
	{0xd732, 0x08, 0x00},
	{0xd731, 0x30, 0x30},
	{0xd73d, 0x80, 0x80},
	{0xd740, 0x01, 0x00},
	{0xd740, 0x02, 0x00},
	{0xd740, 0x04, 0x00},
	{0xd740, 0x08, 0x08},
	{0xd3c1, 0x10, 0x10},
	{0x9124, 0xff, 0x58},
	{0x9125, 0x03, 0x02},
	{0xd3a2, 0xff, 0x00},
	{0xd3a3, 0xff, 0x04},
	{0xd305, 0xff, 0x32},
	{0xd306, 0xff, 0x10},
	{0xd304, 0xff, 0x04},
	{0x9112, 0x01, 0x01},
	{0x911d, 0x01, 0x01},
	{0x911a, 0x01, 0x01},
	{0x911b, 0x01, 0x01},
	{0x9bce, 0x0f, 0x02},
	{0x9116, 0x01, 0x01},
	{0x9122, 0xff, 0xd0},
	{0xd2e0, 0xff, 0xd0},
	{0xd2e9, 0x0f, 0x0d},
	{0xd38c, 0xff, 0xfc},
	{0xd38d, 0xff, 0x00},
	{0xd38e, 0xff, 0x7e},
	{0xd38f, 0xff, 0x00},
	{0xd390, 0xff, 0x2f},
	{0xd145, 0x10, 0x10},
	{0xd1a9, 0x10, 0x10},
	{0xd158, 0xe0, 0x20},
	{0xd159, 0x3f, 0x06},
	{0xd167, 0xff, 0x00},
	{0xd168, 0x0f, 0x07},
	{0xd1c3, 0xe0, 0x00},
	{0xd1c4, 0x3f, 0x00},
	{0xd1c5, 0x7f, 0x10},
	{0xd1c6, 0x07, 0x02},
	{0xd080, 0x7c, 0x0c},
	{0xd081, 0xf0, 0x90},
	{0xd098, 0xf0, 0xf0},
	{0xd098, 0x0f, 0x03},
	{0xdbc0, 0x10, 0x10},
	{0xdbc7, 0xff, 0x08},
	{0xdbc8, 0xf0, 0x00},
	{0xdbc9, 0x1f, 0x01},
	{0xd280, 0xff, 0xe0},
	{0xd281, 0xff, 0xff},
	{0xd282, 0xff, 0xff},
	{0xd283, 0xff, 0xc3},
	{0xd284, 0xff, 0xff},
	{0xd285, 0x0f, 0x01},
	{0xd0f0, 0x7f, 0x1a},
	{0xd0f1, 0x10, 0x10},
	{0xd0f2, 0xff, 0x0c},
	{0xd101, 0xe0, 0xc0},
	{0xd103, 0x0f, 0x08},
	{0xd0f8, 0x7f, 0x20},
	{0xd111, 0x20, 0x00},
	{0xd111, 0x40, 0x00},
	{0x910b, 0xff, 0x0a},
	{0x9115, 0xff, 0x02},
	{0x910c, 0xff, 0x02},
	{0x910d, 0xff, 0x08},
	{0x910e, 0xff, 0x0a},
	{0x9bf6, 0xff, 0x06},
	{0x9bf8, 0xff, 0x02},
	{0x9bf7, 0xff, 0x05},
	{0x9bf9, 0xff, 0x0f},
	{0x9bfc, 0xff, 0x13},
	{0x9bd3, 0xff, 0xff},
	{0x9bbe, 0x01, 0x01},
	{0x9bcc, 0x01, 0x01},
};

/*
 * Panasonic ENV77H11D5 tuner init
 * AF9013_TUNER_ENV77H11D5    0x81
 */
static const struct af9013_reg_mask_val tuner_init_tab_env77h11d5[] = {
	{0x9bd5, 0xff, 0x01},
	{0x9bd6, 0xff, 0x03},
	{0x9bbe, 0xff, 0x01},
	{0xd1a0, 0x02, 0x02},
	{0xd000, 0x01, 0x01},
	{0xd000, 0x02, 0x00},
	{0xd001, 0x02, 0x02},
	{0xd001, 0x01, 0x00},
	{0xd001, 0x20, 0x00},
	{0xd002, 0x1f, 0x19},
	{0xd003, 0x1f, 0x1a},
	{0xd004, 0x1f, 0x19},
	{0xd005, 0x1f, 0x1a},
	{0xd00e, 0x1f, 0x10},
	{0xd00f, 0x07, 0x04},
	{0xd00f, 0x38, 0x28},
	{0xd010, 0x07, 0x04},
	{0xd010, 0x38, 0x28},
	{0xd016, 0xf0, 0x30},
	{0xd01f, 0x3f, 0x0a},
	{0xd020, 0x3f, 0x0a},
	{0x9bda, 0xff, 0x00},
	{0x9be3, 0xff, 0x00},
	{0xd015, 0xff, 0x50},
	{0xd016, 0x01, 0x00},
	{0xd044, 0xff, 0x46},
	{0xd045, 0x01, 0x00},
	{0xd008, 0xff, 0xdf},
	{0xd009, 0x03, 0x02},
	{0xd006, 0xff, 0x44},
	{0xd007, 0x03, 0x01},
	{0xd00c, 0xff, 0xeb},
	{0xd00d, 0x03, 0x02},
	{0xd00a, 0xff, 0xf4},
	{0xd00b, 0x03, 0x01},
	{0x9bba, 0xff, 0xf9},
	{0x9bc3, 0xff, 0xdf},
	{0x9bc4, 0xff, 0x02},
	{0x9bc5, 0xff, 0xeb},
	{0x9bc6, 0xff, 0x02},
	{0x9bc9, 0xff, 0x52},
	{0xd011, 0xff, 0x3c},
	{0xd012, 0x03, 0x01},
	{0xd013, 0xff, 0xf7},
	{0xd014, 0x03, 0x02},
	{0xd040, 0xff, 0x0b},
	{0xd041, 0x03, 0x02},
	{0xd042, 0xff, 0x4d},
	{0xd043, 0x03, 0x00},
	{0xd045, 0x02, 0x00},
	{0x9bcf, 0x01, 0x01},
	{0xd045, 0x04, 0x04},
	{0xd04f, 0xff, 0x9a},
	{0xd050, 0x01, 0x01},
	{0xd051, 0xff, 0x5a},
	{0xd052, 0x01, 0x01},
	{0xd053, 0xff, 0x50},
	{0xd054, 0xff, 0x46},
	{0x9bd7, 0xff, 0x0a},
	{0x9bd8, 0xff, 0x14},
	{0x9bd9, 0xff, 0x08},
};

/*
 * Microtune MT2060 tuner init
 * AF9013_TUNER_MT2060        0x82
 */
static const struct af9013_reg_mask_val tuner_init_tab_mt2060[] = {
	{0x9bd5, 0xff, 0x01},
	{0x9bd6, 0xff, 0x07},
	{0xd1a0, 0x02, 0x02},
	{0xd000, 0x01, 0x01},
	{0xd000, 0x02, 0x00},
	{0xd001, 0x02, 0x02},
	{0xd001, 0x01, 0x00},
	{0xd001, 0x20, 0x00},
	{0xd002, 0x1f, 0x19},
	{0xd003, 0x1f, 0x1a},
	{0xd004, 0x1f, 0x19},
	{0xd005, 0x1f, 0x1a},
	{0xd00e, 0x1f, 0x10},
	{0xd00f, 0x07, 0x04},
	{0xd00f, 0x38, 0x28},
	{0xd010, 0x07, 0x04},
	{0xd010, 0x38, 0x28},
	{0xd016, 0xf0, 0x30},
	{0xd01f, 0x3f, 0x0a},
	{0xd020, 0x3f, 0x0a},
	{0x9bda, 0xff, 0x00},
	{0x9be3, 0xff, 0x00},
	{0x9bbe, 0x01, 0x00},
	{0x9bcc, 0x01, 0x00},
	{0x9bb9, 0xff, 0x75},
	{0x9bcd, 0xff, 0x24},
	{0x9bff, 0xff, 0x30},
	{0xd015, 0xff, 0x46},
	{0xd016, 0x01, 0x00},
	{0xd044, 0xff, 0x46},
	{0xd045, 0x01, 0x00},
	{0xd008, 0xff, 0x0f},
	{0xd009, 0x03, 0x02},
	{0xd006, 0xff, 0x32},
	{0xd007, 0x03, 0x01},
	{0xd00c, 0xff, 0x36},
	{0xd00d, 0x03, 0x03},
	{0xd00a, 0xff, 0x35},
	{0xd00b, 0x03, 0x01},
	{0x9bc7, 0xff, 0x07},
	{0x9bc8, 0xff, 0x90},
	{0x9bc3, 0xff, 0x0f},
	{0x9bc4, 0xff, 0x02},
	{0x9bc5, 0xff, 0x36},
	{0x9bc6, 0xff, 0x03},
	{0x9bba, 0xff, 0xc9},
	{0x9bc9, 0xff, 0x79},
	{0xd011, 0xff, 0x10},
	{0xd012, 0x03, 0x01},
	{0xd013, 0xff, 0x45},
	{0xd014, 0x03, 0x03},
	{0xd040, 0xff, 0x98},
	{0xd041, 0x03, 0x00},
	{0xd042, 0xff, 0xcf},
	{0xd043, 0x03, 0x03},
	{0xd045, 0x02, 0x00},
	{0x9bcf, 0x01, 0x01},
	{0xd045, 0x04, 0x04},
	{0xd04f, 0xff, 0x9a},
	{0xd050, 0x01, 0x01},
	{0xd051, 0xff, 0x5a},
	{0xd052, 0x01, 0x01},
	{0xd053, 0xff, 0x50},
	{0xd054, 0xff, 0x46},
	{0x9bd7, 0xff, 0x0a},
	{0x9bd8, 0xff, 0x14},
	{0x9bd9, 0xff, 0x08},
	{0x9bd0, 0xff, 0xcc},
	{0x9be4, 0xff, 0xa0},
	{0x9bbd, 0xff, 0x8e},
	{0x9be2, 0xff, 0x4d},
	{0x9bee, 0x01, 0x01},
};

/*
 * Microtune MT2060 tuner init
 * AF9013_TUNER_MT2060_2      0x93
 */
static const struct af9013_reg_mask_val tuner_init_tab_mt2060_2[] = {
	{0x9bd5, 0xff, 0x01},
	{0x9bd6, 0xff, 0x06},
	{0x9bbe, 0xff, 0x01},
	{0xd1a0, 0x02, 0x02},
	{0xd000, 0x01, 0x01},
	{0xd000, 0x02, 0x00},
	{0xd001, 0x02, 0x02},
	{0xd001, 0x01, 0x00},
	{0xd001, 0x20, 0x00},
	{0xd002, 0x1f, 0x19},
	{0xd003, 0x1f, 0x1a},
	{0xd004, 0x1f, 0x19},
	{0xd005, 0x1f, 0x1a},
	{0xd00e, 0x1f, 0x10},
	{0xd00f, 0x07, 0x04},
	{0xd00f, 0x38, 0x28},
	{0xd010, 0x07, 0x04},
	{0xd010, 0x38, 0x28},
	{0xd016, 0xf0, 0x30},
	{0xd01f, 0x3f, 0x0a},
	{0xd020, 0x3f, 0x0a},
	{0xd015, 0xff, 0x46},
	{0xd016, 0x01, 0x00},
	{0xd044, 0xff, 0x46},
	{0xd045, 0x01, 0x00},
	{0xd008, 0xff, 0x0f},
	{0xd009, 0x03, 0x02},
	{0xd006, 0xff, 0x32},
	{0xd007, 0x03, 0x01},
	{0xd00c, 0xff, 0x36},
	{0xd00d, 0x03, 0x03},
	{0xd00a, 0xff, 0x35},
	{0xd00b, 0x03, 0x01},
	{0x9bc7, 0xff, 0x07},
	{0x9bc8, 0xff, 0x90},
	{0x9bc3, 0xff, 0x0f},
	{0x9bc4, 0xff, 0x02},
	{0x9bc5, 0xff, 0x36},
	{0x9bc6, 0xff, 0x03},
	{0x9bba, 0xff, 0xc9},
	{0x9bc9, 0xff, 0x79},
	{0xd011, 0xff, 0x10},
	{0xd012, 0x03, 0x01},
	{0xd013, 0xff, 0x45},
	{0xd014, 0x03, 0x03},
	{0xd040, 0xff, 0x98},
	{0xd041, 0x03, 0x00},
	{0xd042, 0xff, 0xcf},
	{0xd043, 0x03, 0x03},
	{0xd045, 0x02, 0x00},
	{0x9bcf, 0xff, 0x01},
	{0xd045, 0x04, 0x04},
	{0xd04f, 0xff, 0x9a},
	{0xd050, 0x01, 0x01},
	{0xd051, 0xff, 0x5a},
	{0xd052, 0x01, 0x01},
	{0xd053, 0xff, 0x96},
	{0xd054, 0xff, 0x46},
	{0xd045, 0x80, 0x00},
	{0x9bd7, 0xff, 0x0a},
	{0x9bd8, 0xff, 0x14},
	{0x9bd9, 0xff, 0x08},
};

/*
 * MaxLinear MXL5003 tuner init
 * AF9013_TUNER_MXL5003D      0x03
 */
static const struct af9013_reg_mask_val tuner_init_tab_mxl5003d[] = {
	{0x9bd5, 0xff, 0x01},
	{0x9bd6, 0xff, 0x09},
	{0xd1a0, 0x02, 0x02},
	{0xd000, 0x01, 0x01},
	{0xd000, 0x02, 0x00},
	{0xd001, 0x02, 0x02},
	{0xd001, 0x01, 0x00},
	{0xd001, 0x20, 0x00},
	{0xd002, 0x1f, 0x19},
	{0xd003, 0x1f, 0x1a},
	{0xd004, 0x1f, 0x19},
	{0xd005, 0x1f, 0x1a},
	{0xd00e, 0x1f, 0x10},
	{0xd00f, 0x07, 0x04},
	{0xd00f, 0x38, 0x28},
	{0xd010, 0x07, 0x04},
	{0xd010, 0x38, 0x28},
	{0xd016, 0xf0, 0x30},
	{0xd01f, 0x3f, 0x0a},
	{0xd020, 0x3f, 0x0a},
	{0x9bda, 0xff, 0x00},
	{0x9be3, 0xff, 0x00},
	{0x9bfc, 0xff, 0x0f},
	{0x9bf6, 0xff, 0x01},
	{0x9bbe, 0x01, 0x01},
	{0xd015, 0xff, 0x33},
	{0xd016, 0x01, 0x00},
	{0xd044, 0xff, 0x40},
	{0xd045, 0x01, 0x00},
	{0xd008, 0xff, 0x0f},
	{0xd009, 0x03, 0x02},
	{0xd006, 0xff, 0x6c},
	{0xd007, 0x03, 0x00},
	{0xd00c, 0xff, 0x3d},
	{0xd00d, 0x03, 0x00},
	{0xd00a, 0xff, 0x45},
	{0xd00b, 0x03, 0x01},
	{0x9bc7, 0xff, 0x07},
	{0x9bc8, 0xff, 0x52},
	{0x9bc3, 0xff, 0x0f},
	{0x9bc4, 0xff, 0x02},
	{0x9bc5, 0xff, 0x3d},
	{0x9bc6, 0xff, 0x00},
	{0x9bba, 0xff, 0xa2},
	{0x9bc9, 0xff, 0xa0},
	{0xd011, 0xff, 0x56},
	{0xd012, 0x03, 0x00},
	{0xd013, 0xff, 0x50},
	{0xd014, 0x03, 0x00},
	{0xd040, 0xff, 0x56},
	{0xd041, 0x03, 0x00},
	{0xd042, 0xff, 0x50},
	{0xd043, 0x03, 0x00},
	{0xd045, 0x02, 0x00},
	{0x9bcf, 0xff, 0x01},
	{0xd045, 0x04, 0x04},
	{0xd04f, 0xff, 0x9a},
	{0xd050, 0x01, 0x01},
	{0xd051, 0xff, 0x5a},
	{0xd052, 0x01, 0x01},
	{0xd053, 0xff, 0x50},
	{0xd054, 0xff, 0x46},
	{0x9bd7, 0xff, 0x0a},
	{0x9bd8, 0xff, 0x14},
	{0x9bd9, 0xff, 0x08},
};

/*
 * MaxLinear MXL5005S & MXL5007T tuner init
 * AF9013_TUNER_MXL5005D      0x0d
 * AF9013_TUNER_MXL5005R      0x1e
 * AF9013_TUNER_MXL5007T      0xb1
 */
static const struct af9013_reg_mask_val tuner_init_tab_mxl5005[] = {
	{0x9bd5, 0xff, 0x01},
	{0x9bd6, 0xff, 0x07},
	{0xd1a0, 0x02, 0x02},
	{0xd000, 0x01, 0x01},
	{0xd000, 0x02, 0x00},
	{0xd001, 0x02, 0x02},
	{0xd001, 0x01, 0x00},
	{0xd001, 0x20, 0x00},
	{0xd002, 0x1f, 0x19},
	{0xd003, 0x1f, 0x1a},
	{0xd004, 0x1f, 0x19},
	{0xd005, 0x1f, 0x1a},
	{0xd00e, 0x1f, 0x10},
	{0xd00f, 0x07, 0x04},
	{0xd00f, 0x38, 0x28},
	{0xd010, 0x07, 0x04},
	{0xd010, 0x38, 0x28},
	{0xd016, 0xf0, 0x30},
	{0xd01f, 0x3f, 0x0a},
	{0xd020, 0x3f, 0x0a},
	{0x9bda, 0xff, 0x01},
	{0x9be3, 0xff, 0x01},
	{0x9bbe, 0x01, 0x01},
	{0x9bcc, 0x01, 0x01},
	{0x9bb9, 0xff, 0x00},
	{0x9bcd, 0xff, 0x28},
	{0x9bff, 0xff, 0x24},
	{0xd015, 0xff, 0x40},
	{0xd016, 0x01, 0x00},
	{0xd044, 0xff, 0x40},
	{0xd045, 0x01, 0x00},
	{0xd008, 0xff, 0x0f},
	{0xd009, 0x03, 0x02},
	{0xd006, 0xff, 0x73},
	{0xd007, 0x03, 0x01},
	{0xd00c, 0xff, 0xfa},
	{0xd00d, 0x03, 0x01},
	{0xd00a, 0xff, 0xff},
	{0xd00b, 0x03, 0x01},
	{0x9bc7, 0xff, 0x23},
	{0x9bc8, 0xff, 0x55},
	{0x9bc3, 0xff, 0x01},
	{0x9bc4, 0xff, 0x02},
	{0x9bc5, 0xff, 0xfa},
	{0x9bc6, 0xff, 0x01},
	{0x9bba, 0xff, 0xff},
	{0x9bc9, 0xff, 0xff},
	{0x9bd3, 0xff, 0x95},
	{0xd011, 0xff, 0x70},
	{0xd012, 0x03, 0x01},
	{0xd013, 0xff, 0xfb},
	{0xd014, 0x03, 0x01},
	{0xd040, 0xff, 0x70},
	{0xd041, 0x03, 0x01},
	{0xd042, 0xff, 0xfb},
	{0xd043, 0x03, 0x01},
	{0xd045, 0x02, 0x00},
	{0x9bcf, 0x01, 0x01},
	{0xd045, 0x04, 0x04},
	{0xd04f, 0xff, 0x9a},
	{0xd050, 0x01, 0x01},
	{0xd051, 0xff, 0x5a},
	{0xd052, 0x01, 0x01},
	{0xd053, 0xff, 0x50},
	{0xd054, 0xff, 0x46},
	{0x9bd7, 0xff, 0x0a},
	{0x9bd8, 0xff, 0x14},
	{0x9bd9, 0xff, 0x08},
	{0x9bd0, 0xff, 0x93},
	{0x9be4, 0xff, 0xfe},
	{0x9bbd, 0xff, 0x63},
	{0x9be2, 0xff, 0xfe},
	{0x9bee, 0x01, 0x01},
};

/*
 * Quantek QT1010 tuner init
 * AF9013_TUNER_QT1010        0x86
 * AF9013_TUNER_QT1010A       0xa2
 */
static const struct af9013_reg_mask_val tuner_init_tab_qt1010[] = {
	{0x9bd5, 0xff, 0x01},
	{0x9bd6, 0xff, 0x09},
	{0xd1a0, 0x02, 0x02},
	{0xd000, 0x01, 0x01},
	{0xd000, 0x02, 0x00},
	{0xd001, 0x02, 0x02},
	{0xd001, 0x01, 0x00},
	{0xd001, 0x20, 0x00},
	{0xd002, 0x1f, 0x19},
	{0xd003, 0x1f, 0x1a},
	{0xd004, 0x1f, 0x19},
	{0xd005, 0x1f, 0x1a},
	{0xd00e, 0x1f, 0x10},
	{0xd00f, 0x07, 0x04},
	{0xd00f, 0x38, 0x28},
	{0xd010, 0x07, 0x04},
	{0xd010, 0x38, 0x28},
	{0xd016, 0xf0, 0x30},
	{0xd01f, 0x3f, 0x0a},
	{0xd020, 0x3f, 0x0a},
	{0x9bda, 0xff, 0x01},
	{0x9be3, 0xff, 0x01},
	{0xd015, 0xff, 0x46},
	{0xd016, 0x01, 0x00},
	{0xd044, 0xff, 0x46},
	{0xd045, 0x01, 0x00},
	{0x9bbe, 0x01, 0x01},
	{0x9bcc, 0x01, 0x01},
	{0x9bb9, 0xff, 0x00},
	{0x9bcd, 0xff, 0x28},
	{0x9bff, 0xff, 0x20},
	{0xd008, 0xff, 0x0f},
	{0xd009, 0x03, 0x02},
	{0xd006, 0xff, 0x99},
	{0xd007, 0x03, 0x01},
	{0xd00c, 0xff, 0x0f},
	{0xd00d, 0x03, 0x02},
	{0xd00a, 0xff, 0x50},
	{0xd00b, 0x03, 0x01},
	{0x9bc7, 0xff, 0x00},
	{0x9bc8, 0xff, 0x00},
	{0x9bc3, 0xff, 0x0f},
	{0x9bc4, 0xff, 0x02},
	{0x9bc5, 0xff, 0x0f},
	{0x9bc6, 0xff, 0x02},
	{0x9bba, 0xff, 0xc5},
	{0x9bc9, 0xff, 0xff},
	{0xd011, 0xff, 0x58},
	{0xd012, 0x03, 0x02},
	{0xd013, 0xff, 0x89},
	{0xd014, 0x03, 0x01},
	{0xd040, 0xff, 0x58},
	{0xd041, 0x03, 0x02},
	{0xd042, 0xff, 0x89},
	{0xd043, 0x03, 0x01},
	{0xd045, 0x02, 0x00},
	{0x9bcf, 0x01, 0x01},
	{0xd045, 0x04, 0x04},
	{0xd04f, 0xff, 0x9a},
	{0xd050, 0x01, 0x01},
	{0xd051, 0xff, 0x5a},
	{0xd052, 0x01, 0x01},
	{0xd053, 0xff, 0x50},
	{0xd054, 0xff, 0x46},
	{0x9bd7, 0xff, 0x0a},
	{0x9bd8, 0xff, 0x14},
	{0x9bd9, 0xff, 0x08},
	{0x9bd0, 0xff, 0xcd},
	{0x9be4, 0xff, 0xbb},
	{0x9bbd, 0xff, 0x93},
	{0x9be2, 0xff, 0x80},
	{0x9bee, 0x01, 0x01},
};

/*
 * Freescale MC44S803 tuner init
 * AF9013_TUNER_MC44S803      0x85
 */
static const struct af9013_reg_mask_val tuner_init_tab_mc44s803[] = {
	{0x9bd5, 0xff, 0x01},
	{0x9bd6, 0xff, 0x06},
	{0xd1a0, 0x02, 0x02},
	{0xd000, 0x01, 0x01},
	{0xd000, 0x02, 0x00},
	{0xd001, 0x02, 0x02},
	{0xd001, 0x01, 0x00},
	{0xd001, 0x20, 0x00},
	{0xd002, 0x1f, 0x19},
	{0xd003, 0x1f, 0x1a},
	{0xd004, 0x1f, 0x19},
	{0xd005, 0x1f, 0x1a},
	{0xd00e, 0x1f, 0x10},
	{0xd00f, 0x07, 0x04},
	{0xd00f, 0x38, 0x28},
	{0xd010, 0x07, 0x04},
	{0xd010, 0x38, 0x28},
	{0xd016, 0xf0, 0x30},
	{0xd01f, 0x3f, 0x0a},
	{0xd020, 0x3f, 0x0a},
	{0x9bda, 0xff, 0x00},
	{0x9be3, 0xff, 0x00},
	{0x9bf6, 0xff, 0x01},
	{0x9bf8, 0xff, 0x02},
	{0x9bf9, 0xff, 0x02},
	{0x9bfc, 0xff, 0x1f},
	{0x9bbe, 0x01, 0x01},
	{0x9bcc, 0x01, 0x01},
	{0x9bb9, 0xff, 0x00},
	{0x9bcd, 0xff, 0x24},
	{0x9bff, 0xff, 0x24},
	{0xd015, 0xff, 0x46},
	{0xd016, 0x01, 0x00},
	{0xd044, 0xff, 0x46},
	{0xd045, 0x01, 0x00},
	{0xd008, 0xff, 0x01},
	{0xd009, 0x03, 0x02},
	{0xd006, 0xff, 0x7b},
	{0xd007, 0x03, 0x00},
	{0xd00c, 0xff, 0x7c},
	{0xd00d, 0x03, 0x02},
	{0xd00a, 0xff, 0xfe},
	{0xd00b, 0x03, 0x01},
	{0x9bc7, 0xff, 0x08},
	{0x9bc8, 0xff, 0x9a},
	{0x9bc3, 0xff, 0x01},
	{0x9bc4, 0xff, 0x02},
	{0x9bc5, 0xff, 0x7c},
	{0x9bc6, 0xff, 0x02},
	{0x9bba, 0xff, 0xfc},
	{0x9bc9, 0xff, 0xaa},
	{0xd011, 0xff, 0x6b},
	{0xd012, 0x03, 0x00},
	{0xd013, 0xff, 0x88},
	{0xd014, 0x03, 0x02},
	{0xd040, 0xff, 0x6b},
	{0xd041, 0x03, 0x00},
	{0xd042, 0xff, 0x7c},
	{0xd043, 0x03, 0x02},
	{0xd045, 0x02, 0x00},
	{0x9bcf, 0x01, 0x01},
	{0xd045, 0x04, 0x04},
	{0xd04f, 0xff, 0x9a},
	{0xd050, 0x01, 0x01},
	{0xd051, 0xff, 0x5a},
	{0xd052, 0x01, 0x01},
	{0xd053, 0xff, 0x50},
	{0xd054, 0xff, 0x46},
	{0x9bd7, 0xff, 0x0a},
	{0x9bd8, 0xff, 0x14},
	{0x9bd9, 0xff, 0x08},
	{0x9bd0, 0xff, 0x9e},
	{0x9be4, 0xff, 0xff},
	{0x9bbd, 0xff, 0x9e},
	{0x9be2, 0xff, 0x25},
	{0x9bee, 0x01, 0x01},
	{0xd73b, 0x08, 0x00},
};

/*
 * Unknown, probably for tin can tuner, tuner init
 * AF9013_TUNER_UNKNOWN       0x8c
 */
static const struct af9013_reg_mask_val tuner_init_tab_unknown[] = {
	{0x9bd5, 0xff, 0x01},
	{0x9bd6, 0xff, 0x02},
	{0xd1a0, 0x02, 0x02},
	{0xd000, 0x01, 0x01},
	{0xd000, 0x02, 0x00},
	{0xd001, 0x02, 0x02},
	{0xd001, 0x01, 0x00},
	{0xd001, 0x20, 0x00},
	{0xd002, 0x1f, 0x19},
	{0xd003, 0x1f, 0x1a},
	{0xd004, 0x1f, 0x19},
	{0xd005, 0x1f, 0x1a},
	{0xd00e, 0x1f, 0x10},
	{0xd00f, 0x07, 0x04},
	{0xd00f, 0x38, 0x28},
	{0xd010, 0x07, 0x04},
	{0xd010, 0x38, 0x28},
	{0xd016, 0xf0, 0x30},
	{0xd01f, 0x3f, 0x0a},
	{0xd020, 0x3f, 0x0a},
	{0x9bda, 0xff, 0x01},
	{0x9be3, 0xff, 0x01},
	{0xd1a0, 0x02, 0x00},
	{0x9bbe, 0x01, 0x01},
	{0x9bcc, 0x01, 0x01},
	{0x9bb9, 0xff, 0x00},
	{0x9bcd, 0xff, 0x18},
	{0x9bff, 0xff, 0x2c},
	{0xd015, 0xff, 0x46},
	{0xd016, 0x01, 0x00},
	{0xd044, 0xff, 0x46},
	{0xd045, 0x01, 0x00},
	{0xd008, 0xff, 0xdf},
	{0xd009, 0x03, 0x02},
	{0xd006, 0xff, 0x44},
	{0xd007, 0x03, 0x01},
	{0xd00c, 0xff, 0x00},
	{0xd00d, 0x03, 0x02},
	{0xd00a, 0xff, 0xf6},
	{0xd00b, 0x03, 0x01},
	{0x9bba, 0xff, 0xf9},
	{0x9bc8, 0xff, 0xaa},
	{0x9bc3, 0xff, 0xdf},
	{0x9bc4, 0xff, 0x02},
	{0x9bc5, 0xff, 0x00},
	{0x9bc6, 0xff, 0x02},
	{0x9bc9, 0xff, 0xf0},
	{0xd011, 0xff, 0x3c},
	{0xd012, 0x03, 0x01},
	{0xd013, 0xff, 0xf7},
	{0xd014, 0x03, 0x02},
	{0xd040, 0xff, 0x0b},
	{0xd041, 0x03, 0x02},
	{0xd042, 0xff, 0x4d},
	{0xd043, 0x03, 0x00},
	{0xd045, 0x02, 0x00},
	{0x9bcf, 0x01, 0x01},
	{0xd045, 0x04, 0x04},
	{0xd04f, 0xff, 0x9a},
	{0xd050, 0x01, 0x01},
	{0xd051, 0xff, 0x5a},
	{0xd052, 0x01, 0x01},
	{0xd053, 0xff, 0x50},
	{0xd054, 0xff, 0x46},
	{0x9bd7, 0xff, 0x0a},
	{0x9bd8, 0xff, 0x14},
	{0x9bd9, 0xff, 0x08},
};

/*
 * NXP TDA18271 & TDA18218 tuner init
 * AF9013_TUNER_TDA18271      0x9c
 * AF9013_TUNER_TDA18218      0xb3
 */
static const struct af9013_reg_mask_val tuner_init_tab_tda18271[] = {
	{0x9bd5, 0xff, 0x01},
	{0x9bd6, 0xff, 0x04},
	{0xd1a0, 0x02, 0x02},
	{0xd000, 0x01, 0x01},
	{0xd000, 0x02, 0x00},
	{0xd001, 0x02, 0x02},
	{0xd001, 0x01, 0x00},
	{0xd001, 0x20, 0x00},
	{0xd002, 0x1f, 0x19},
	{0xd003, 0x1f, 0x1a},
	{0xd004, 0x1f, 0x19},
	{0xd005, 0x1f, 0x1a},
	{0xd00e, 0x1f, 0x10},
	{0xd00f, 0x07, 0x04},
	{0xd00f, 0x38, 0x28},
	{0xd010, 0x07, 0x04},
	{0xd010, 0x38, 0x28},
	{0xd016, 0xf0, 0x30},
	{0xd01f, 0x3f, 0x0a},
	{0xd020, 0x3f, 0x0a},
	{0x9bda, 0xff, 0x01},
	{0x9be3, 0xff, 0x01},
	{0xd1a0, 0x02, 0x00},
	{0x9bbe, 0x01, 0x01},
	{0x9bcc, 0x01, 0x01},
	{0x9bb9, 0xff, 0x00},
	{0x9bcd, 0xff, 0x18},
	{0x9bff, 0xff, 0x2c},
	{0xd015, 0xff, 0x46},
	{0xd016, 0x01, 0x00},
	{0xd044, 0xff, 0x46},
	{0xd045, 0x01, 0x00},
	{0xd008, 0xff, 0xdf},
	{0xd009, 0x03, 0x02},
	{0xd006, 0xff, 0x44},
	{0xd007, 0x03, 0x01},
	{0xd00c, 0xff, 0x00},
	{0xd00d, 0x03, 0x02},
	{0xd00a, 0xff, 0xf6},
	{0xd00b, 0x03, 0x01},
	{0x9bba, 0xff, 0xf9},
	{0x9bc8, 0xff, 0xaa},
	{0x9bc3, 0xff, 0xdf},
	{0x9bc4, 0xff, 0x02},
	{0x9bc5, 0xff, 0x00},
	{0x9bc6, 0xff, 0x02},
	{0x9bc9, 0xff, 0xf0},
	{0xd011, 0xff, 0x3c},
	{0xd012, 0x03, 0x01},
	{0xd013, 0xff, 0xf7},
	{0xd014, 0x03, 0x02},
	{0xd040, 0xff, 0x0b},
	{0xd041, 0x03, 0x02},
	{0xd042, 0xff, 0x4d},
	{0xd043, 0x03, 0x00},
	{0xd045, 0x02, 0x00},
	{0x9bcf, 0x01, 0x01},
	{0xd045, 0x04, 0x04},
	{0xd04f, 0xff, 0x9a},
	{0xd050, 0x01, 0x01},
	{0xd051, 0xff, 0x5a},
	{0xd052, 0x01, 0x01},
	{0xd053, 0xff, 0x50},
	{0xd054, 0xff, 0x46},
	{0x9bd7, 0xff, 0x0a},
	{0x9bd8, 0xff, 0x14},
	{0x9bd9, 0xff, 0x08},
	{0x9bd0, 0xff, 0xa8},
	{0x9be4, 0xff, 0x7f},
	{0x9bbd, 0xff, 0xa8},
	{0x9be2, 0xff, 0x20},
	{0x9bee, 0x01, 0x01},
};

#endif /* AF9013_PRIV_H */