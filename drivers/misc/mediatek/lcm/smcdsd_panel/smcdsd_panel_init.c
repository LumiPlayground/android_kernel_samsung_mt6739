// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/fb.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "lcm_drv.h"
#include "smcdsd_board.h"
#include "smcdsd_panel.h"

#define dbg_info(fmt, ...)		pr_info(pr_fmt("smcdsd: "fmt), ##__VA_ARGS__)
#define dbg_none(fmt, ...)		pr_debug(pr_fmt("smcdsd: "fmt), ##__VA_ARGS__)

#define STREQ(a, b)			(a && b && (*(a) == *(b)) && (strcmp((a), (b)) == 0))
#define STRNEQ(a, b)			(a && b && (strncmp((a), (b), (strlen(a))) == 0))

#define PANEL_DTS_NAME			"smcdsd_panel"

/* -------------------------------------------------------------------------- */
/* boot param */
/* -------------------------------------------------------------------------- */
unsigned int lcdtype = 1;
EXPORT_SYMBOL(lcdtype);

unsigned int blictype = 1;
EXPORT_SYMBOL(blictype);

static int __init get_lcd_type(char *arg)
{
	get_option(&arg, &lcdtype);

	dbg_info("%s: lcdtype: %6X\n", __func__, lcdtype);

	return 0;
}
early_param("lcdtype", get_lcd_type);

static int __init get_blic_type(char *arg)
{
	get_option(&arg, &blictype);

	dbg_info("%s: blictype: %6X\n", __func__, blictype);

	return 0;
}
early_param("blictype", get_blic_type);

/* -------------------------------------------------------------------------- */
/* helper to parse dt */
/* -------------------------------------------------------------------------- */
static void smcdsd_of_property_read_u32(const struct device_node *np, const char *propname, u32 *out_value)
{
	int ret = 0, count = 0, i;
	char print_buf[32] = {0, };
	struct seq_file m = {
		.buf = print_buf,
		.size = sizeof(print_buf) - 1,
	};

	if (!np || !propname || !out_value)
		dbg_none("of_property_read_u32 null\n");

	count = of_property_count_u32_elems(np, propname);
	if (count < 1) {
		dbg_none("of_property_count_u32_elems fail for %s(%d)\n", propname, count);
		return;
	}

	ret = of_property_read_u32_array(np, propname, out_value, count);
	if (ret < 0)
		dbg_none("of_property_read_u32_array fail for %s(%d)\n", propname, ret);
	else {
		for (i = 0; i < count; i++)
			seq_printf(&m, "%d, ", *(out_value + i));

		dbg_info("of_property_read_u32_array %s(%s)\n", propname, m.buf);
	}
}

int __smcdsd_panel_get_params_from_dt(struct LCM_PARAMS *lcm_params)
{
	struct device_node *np = NULL;
	struct mipi_dsi_lcd_common *plcd = get_lcd_common(0);
	int ret = 0;

	np = of_find_recommend_lcd_info(NULL);
	if (!np) {
		dbg_info("of_find_recommend_lcd_info fail\n");
		return -EINVAL;
	}

	smcdsd_of_property_read_u32(np, "lcm_params-types",
		&lcm_params->type);
	smcdsd_of_property_read_u32(np, "lcm_params-resolution",
		&lcm_params->width);
	smcdsd_of_property_read_u32(np, "lcm_params-io_select_mode",
		&lcm_params->io_select_mode);
	smcdsd_of_property_read_u32(np, "lcm_params-dsi-mode",
		&lcm_params->dsi.mode);
	smcdsd_of_property_read_u32(np, "lcm_params-dsi-lane_num",
		&lcm_params->dsi.LANE_NUM);
	smcdsd_of_property_read_u32(np, "lcm_params-dsi-packet_size",
		&lcm_params->dsi.packet_size);
	smcdsd_of_property_read_u32(np, "lcm_params-dsi-ps",
		&lcm_params->dsi.PS);
	smcdsd_of_property_read_u32(np, "lcm_params-dsi-ssc_disable",
		&lcm_params->dsi.ssc_disable);
	smcdsd_of_property_read_u32(np, "lcm_params-dsi-vertical_sync_active",
		&lcm_params->dsi.vertical_sync_active);
	smcdsd_of_property_read_u32(np, "lcm_params-dsi-vertical_backporch",
		&lcm_params->dsi.vertical_backporch);
	smcdsd_of_property_read_u32(np, "lcm_params-dsi-vertical_frontporch",
		&lcm_params->dsi.vertical_frontporch);
	smcdsd_of_property_read_u32(np,
		"lcm_params-dsi-vertical_frontporch_for_low_power",
		&lcm_params->dsi.vertical_frontporch_for_low_power);
	smcdsd_of_property_read_u32(np, "lcm_params-dsi-vertical_active_line",
		&lcm_params->dsi.vertical_active_line);
	smcdsd_of_property_read_u32(np, "lcm_params-dsi-horizontal_sync_active",
		&lcm_params->dsi.horizontal_sync_active);
	smcdsd_of_property_read_u32(np, "lcm_params-dsi-horizontal_backporch",
		&lcm_params->dsi.horizontal_backporch);
	smcdsd_of_property_read_u32(np, "lcm_params-dsi-horizontal_frontporch",
		&lcm_params->dsi.horizontal_frontporch);
	smcdsd_of_property_read_u32(np, "lcm_params-dsi-horizontal_blanking_pixel",
		&lcm_params->dsi.horizontal_blanking_pixel);
	smcdsd_of_property_read_u32(np, "lcm_params-dsi-horizontal_active_pixel",
		&lcm_params->dsi.horizontal_active_pixel);
	smcdsd_of_property_read_u32(np, "lcm_params-dsi-cont_clock",
		&lcm_params->dsi.cont_clock);
	smcdsd_of_property_read_u32(np, "lcm_params-physical_width",
		&lcm_params->physical_width);
	smcdsd_of_property_read_u32(np, "lcm_params-physical_height",
		&lcm_params->physical_height);
	smcdsd_of_property_read_u32(np, "lcm_params-physical_width_um",
		&lcm_params->physical_width_um);
	smcdsd_of_property_read_u32(np, "lcm_params-physical_height_um",
		&lcm_params->physical_height_um);
	smcdsd_of_property_read_u32(np, "lcm_params-od_table_size",
		&lcm_params->od_table_size);
	smcdsd_of_property_read_u32(np, "lcm_params-od_table",
		(u32 *) (&lcm_params->od_table));

	/* Get MIPI CLK table from DT array */
	smcdsd_of_property_read_u32(np, "lcm_params-dsi-data_rate", plcd->data_rate);
	/* First CLK is default CLK */
	lcm_params->dsi.data_rate = plcd->data_rate[0];

	smcdsd_of_property_read_u32(np, "lcm_params-hbm_enable_wait_frame", &lcm_params->hbm_enable_wait_frame);
	smcdsd_of_property_read_u32(np, "lcm_params-hbm_disable_wait_frame", &lcm_params->hbm_disable_wait_frame);
	smcdsd_of_property_read_u32(np, "lcd_params-gpara-len", &gpara_len);

	return ret;
}

int smcdsd_panel_get_params_from_dt(struct LCM_PARAMS *params)
{
	memset(params, 0, sizeof(struct LCM_PARAMS));

	params->type = LCM_TYPE_DSI;
	params->lcm_if = LCM_INTERFACE_DSI0;

	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	params->dsi.packet_size = 256;
	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

	__smcdsd_panel_get_params_from_dt(params);

	params->dsi.vertical_active_line = params->dsi.vertical_active_line ? params->dsi.vertical_active_line : params->height;
	params->dsi.horizontal_active_pixel = params->dsi.horizontal_active_pixel ? params->dsi.horizontal_active_pixel : params->width;

	params->dsi.PLL_CLOCK = params->dsi.PLL_CLOCK ? params->dsi.PLL_CLOCK : params->dsi.data_rate >> 1;

	return 0;
}

/* -------------------------------------------------------------------------- */
/* helper to find lcd driver */
/* -------------------------------------------------------------------------- */
struct mipi_dsi_lcd_driver *mipi_lcd_driver;

static int __lcd_driver_update_by_lcd_info_name(struct mipi_dsi_lcd_driver *drv)
{
	struct device_node *node;
	int count = 0, ret = 0;

	node = of_find_node_with_property(NULL, PANEL_DTS_NAME);
	if (!node) {
		dbg_info("%s: %s of_find_node_with_property fail\n", __func__, PANEL_DTS_NAME);
		ret = -EINVAL;
		goto exit;
	}

	count = of_count_phandle_with_args(node, PANEL_DTS_NAME, NULL);
	if (count < 0) {
		dbg_info("%s: %s of_count_phandle_with_args fail: %d\n", __func__, PANEL_DTS_NAME, count);
		ret = -EINVAL;
		goto exit;
	}

	node = of_parse_phandle(node, PANEL_DTS_NAME, 0);
	if (!node) {
		dbg_info("%s: %s of_parse_phandle fail\n", __func__, PANEL_DTS_NAME);
		ret = -EINVAL;
		goto exit;
	}

	if (IS_ERR_OR_NULL(drv) || IS_ERR_OR_NULL(drv->driver.name)) {
		dbg_info("%s: we need lcd_driver name to compare with dts node name(%s)\n", __func__, node->name);
		ret = -EINVAL;
		goto exit;
	}

	if (!strcmp(node->name, drv->driver.name) && mipi_lcd_driver != drv) {
		mipi_lcd_driver = drv;
		dbg_info("%s: driver(%s) is updated\n", __func__, mipi_lcd_driver->driver.name);
	} else {
		dbg_none("%s: driver(%s) is diffferent with %s node(%s)\n", __func__, PANEL_DTS_NAME, node->name, drv->driver.name);
	}

exit:
	return ret;
}

static void __lcd_driver_dts_update(void)
{
	struct device_node *nplcd = NULL, *np = NULL;
	int i = 0, pcount, count = 0, ret = -EINVAL;
	unsigned int id_index, mask, expect;
	u32 id_match_info[10] = {0, };

	nplcd = of_find_node_with_property(NULL, PANEL_DTS_NAME);
	if (!nplcd) {
		dbg_info("%s: %s property does not exist\n", __func__, PANEL_DTS_NAME);
		return;
	}

	pcount = of_count_phandle_with_args(nplcd, PANEL_DTS_NAME, NULL);
	if (pcount < 2) {
		/* dbg_info("%s: %s property phandle count is %d. so no need to update check\n", __func__, PANEL_DTS_NAME, count); */
		return;
	}

	for (i = 0; i < pcount; i++) {
		np = of_parse_phandle(nplcd, PANEL_DTS_NAME, i);
		dbg_info("%s: %dth dts is %s\n", __func__, i, (np && np->name) ? np->name : "null");
		if (!np || !of_get_property(np, "id_match", NULL))
			continue;

		count = of_property_count_u32_elems(np, "id_match");
		if (count < 0 || count > ARRAY_SIZE(id_match_info) || count % 2) {
			dbg_info("%s: %dth dts(%s) has invalid id_match count(%d)\n", __func__, i, np->name, count);
			continue;
		}

		memset(id_match_info, 0, sizeof(id_match_info));
		ret = of_property_read_u32_array(np, "id_match", id_match_info, count);
		if (ret < 0) {
			dbg_info("%s: of_property_read_u32_array fail. ret(%d)\n", __func__, ret);
			continue;
		}

		for (id_index = 0; id_index < count; id_index += 2) {
			mask = id_match_info[id_index];
			expect = id_match_info[id_index + 1];

			if ((lcdtype & mask) == (expect & mask)) {
				dbg_info("%s: %dth dts(%s) id_match. lcdtype(%06X) mask(%06X) expect(%06X)\n",
					__func__, i, np->name, lcdtype, mask, expect);
				if (i > 0)
					of_update_phandle_property(NULL, PANEL_DTS_NAME, np->name);
				return;
			}
		}
	}
}

static int lcd_driver_is_valid(struct mipi_dsi_lcd_driver *lcd_driver)
{
	if (!lcd_driver)
		return 0;

	if (!lcd_driver->driver.name)
		return 0;

	return 1;
}

static int lcd_driver_need_update_dts(const char *driver_name)
{
	struct device_node *nplcd = NULL, *np = NULL;
	int need_update = 0;

	nplcd = of_find_node_with_property(NULL, PANEL_DTS_NAME);
	if (!nplcd) {
		dbg_info("%s: %s property does not exist\n", __func__, PANEL_DTS_NAME);
		return need_update;
	}

	np = of_parse_phandle(nplcd, PANEL_DTS_NAME, 0);
	if (!np) {
		dbg_info("%s: %s of_parse_phandle fail\n", __func__, PANEL_DTS_NAME);
		return need_update;
	}

	if (!STREQ(np->name, driver_name))
		need_update = 1;

	return need_update;
}

static void __lcd_driver_update_by_id_match(void)
{
	struct mipi_dsi_lcd_driver **p, **first;
	struct mipi_dsi_lcd_driver *lcd_driver = NULL;
	struct device_node *np = NULL;
	int i = 0, count = 0, ret = -EINVAL, do_match = 0;
	unsigned int id_index, mask, expect;
	u32 id_match_info[10] = {0, };

	p = first = __start___lcd_driver;

	for (i = 0, p = __start___lcd_driver; p < __stop___lcd_driver; p++, i++) {
		lcd_driver = *p;

		if (lcd_driver && lcd_driver->driver.name) {
			np = of_find_node_by_name(NULL, lcd_driver->driver.name);
			if (np && of_get_property(np, "id_match", NULL)) {
				dbg_info("%s: %dth lcd_driver(%s) has dts id_match property\n", __func__, i, lcd_driver->driver.name);
				do_match++;
			}
		}
	}

	if (!do_match)
		return;

	if (i != do_match) {
		lcd_driver = *first;
		of_update_phandle_property(NULL, PANEL_DTS_NAME, lcd_driver->driver.name);
	}

	for (i = 0, p = __start___lcd_driver; p < __stop___lcd_driver; p++, i++) {
		lcd_driver = *p;

		if (!lcd_driver_is_valid(lcd_driver)) {
			dbg_info("%dth lcd_driver is invalid\n", i);
			continue;
		}

		np = of_find_node_by_name(NULL, lcd_driver->driver.name);
		if (!np || !of_get_property(np, "id_match", NULL)) {
			/* dbg_info("%dth lcd_driver(%s) has no id_match property\n", lcd_driver->driver.name); */
			continue;
		}

		count = of_property_count_u32_elems(np, "id_match");
		if (count < 0 || count > ARRAY_SIZE(id_match_info) || count % 2) {
			dbg_info("%s: %dth lcd_driver(%s) has invalid id_match count(%d)\n", __func__, i, lcd_driver->driver.name, count);
			continue;
		}

		memset(id_match_info, 0, sizeof(id_match_info));
		ret = of_property_read_u32_array(np, "id_match", id_match_info, count);
		if (ret < 0) {
			dbg_info("%s: of_property_read_u32_array fail. ret(%d)\n", __func__, ret);
			continue;
		}

		for (id_index = 0; id_index < count; id_index += 2) {
			mask = id_match_info[id_index];
			expect = id_match_info[id_index + 1];

			if ((lcdtype & mask) == (expect & mask)) {
				dbg_info("%s: %dth lcd_driver(%s) id_match. lcdtype(%06X) mask(%06X) expect(%06X)\n",
					__func__, i, lcd_driver->driver.name, lcdtype, mask, expect);

				mipi_lcd_driver = lcd_driver;

				if (lcd_driver_need_update_dts(lcd_driver->driver.name))
					of_update_phandle_property(NULL, PANEL_DTS_NAME, lcd_driver->driver.name);

				return;
			}
		}
	}
}

static void __lcd_driver_update_by_function(void)
{
	struct mipi_dsi_lcd_driver **p, **first;
	struct mipi_dsi_lcd_driver *lcd_driver = NULL;
	int i = 0, ret = -EINVAL, do_match = 0;

	p = first = __start___lcd_driver;

	for (i = 0, p = __start___lcd_driver; p < __stop___lcd_driver; p++, i++) {
		lcd_driver = *p;

		if (lcd_driver && lcd_driver->match) {
			dbg_info("%s: %dth lcd_driver %s and has driver match function\n", __func__, i, lcd_driver->driver.name);
			do_match++;
		}
	}

	if (!do_match)
		return;

	if (i != do_match) {
		lcd_driver = *first;
		of_update_phandle_property(NULL, PANEL_DTS_NAME, lcd_driver->driver.name);
	}

	for (i = 0, p = __start___lcd_driver; p < __stop___lcd_driver; p++, i++) {
		lcd_driver = *p;

		if (!lcd_driver_is_valid(lcd_driver)) {
			dbg_info("%dth lcd_driver is invalid\n", i);
			continue;
		}

		ret = lcd_driver->match(NULL);
		if (ret & NOTIFY_OK) {
			dbg_info("%s: %dth lcd_driver(%s) NOTIFY_OK(%04X)\n", __func__, i, lcd_driver->driver.name, ret);

			mipi_lcd_driver = lcd_driver;

			if (lcd_driver_need_update_dts(lcd_driver->driver.name))
				of_update_phandle_property(NULL, PANEL_DTS_NAME, lcd_driver->driver.name);

			return;
		}

		if (ret & NOTIFY_STOP_MASK) {
			dbg_info("%s: %dth lcd_driver(%s) NOTIFY_STOP_MASK(%04X)\n", __func__, i, lcd_driver->driver.name, ret);
			return;
		}
	}
}

int lcd_driver_init(void)
{
	struct mipi_dsi_lcd_driver **p, **first;
	struct mipi_dsi_lcd_driver *lcd_driver = NULL;
	int i = 0;
	unsigned long count;

	p = first = __start___lcd_driver;

	__lcd_driver_dts_update();

	if (mipi_lcd_driver && mipi_lcd_driver->driver.name) {
		dbg_info("%s: %s driver is already registered\n", __func__, mipi_lcd_driver->driver.name);
		return 0;
	}

	count = __stop___lcd_driver - __start___lcd_driver;
	if (count == 1) {
		mipi_lcd_driver = *first;
		return 0;
	}

	__lcd_driver_update_by_id_match();
	__lcd_driver_update_by_function();

	for (i = 0, p = __start___lcd_driver; p < __stop___lcd_driver; p++, i++) {
		lcd_driver = *p;

		if (!lcd_driver_is_valid(lcd_driver))
			continue;

		dbg_info("%s: %dth lcd_driver is %s\n", __func__, i, lcd_driver->driver.name);

		__lcd_driver_update_by_lcd_info_name(lcd_driver);
	}

	WARN_ON(!mipi_lcd_driver);
	mipi_lcd_driver = mipi_lcd_driver ? mipi_lcd_driver : *first;

	dbg_info("%s: %s driver is registered\n", __func__, mipi_lcd_driver->driver.name ? mipi_lcd_driver->driver.name : "null");

	return 0;
}
