// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/kernel.h>
#include "cam_cal_list.h"
#include "eeprom_i2c_common_driver.h"
#include "eeprom_i2c_custom_driver.h"
#include "kd_imgsensor.h"

#define MAX_EEPROM_SIZE_32K 0x8000
#define MAX_EEPROM_SIZE_16K 0x4000

/* gold's depth sensors keep their calibration in the sensor's OTP, read by its driver */
extern unsigned int sc202cs_aac_read_otp_info(struct i2c_client *client,
	unsigned int addr, unsigned char *data, unsigned int size);
extern unsigned int sc202cs_sunny_read_otp_info(struct i2c_client *client,
	unsigned int addr, unsigned char *data, unsigned int size);
extern unsigned int sc202cs_sunny2_read_otp_info(struct i2c_client *client,
	unsigned int addr, unsigned char *data, unsigned int size);

struct stCAM_CAL_LIST_STRUCT g_camCalList[] = {
	{OV16A1Q_AAC_FRONT_SENSOR_ID, 0xA2, Common_read_region, MAX_EEPROM_SIZE_16K},
	{OV16A1Q_SUNNY_FRONT_SENSOR_ID, 0xA2, Common_read_region, MAX_EEPROM_SIZE_16K},
	{OV64B40_OFILM_MAIN_SENSOR_ID, 0xA2, Common_read_region, MAX_EEPROM_SIZE_16K},
	{OV08D10_AAC_ULTRA_SENSOR_ID, 0xA0, Common_read_region, MAX_EEPROM_SIZE_16K},
	{OV08D10_SUNNY_ULTRA_SENSOR_ID, 0xA0, Common_read_region, MAX_EEPROM_SIZE_16K},
	{S5KHM6_SEMCO_MAIN_SENSOR_ID, 0xA2, Common_read_region, MAX_EEPROM_SIZE_16K},
	{OV50D40_SUNNY_MAIN_SENSOR_ID, 0xA2, Common_read_region, MAX_EEPROM_SIZE_16K},
	{SC202CS_AAC_DEPTH_SENSOR_ID, 0x6C, sc202cs_aac_read_otp_info},
	{SC202CS_SUNNY_DEPTH_SENSOR_ID, 0x6C, sc202cs_sunny_read_otp_info},
	{SC202CS_SUNNY2_DEPTH_SENSOR_ID, 0x6C, sc202cs_sunny2_read_otp_info},
	{SC202PCS_AAC_MACRO_SENSOR_ID, 0xA4, Common_read_region, MAX_EEPROM_SIZE_16K},
	{S5KHM6_AAC_MAIN_SENSOR_ID, 0xA2, Common_read_region, MAX_EEPROM_SIZE_16K},
	{SC202PCS_SUNNY_MACRO_SENSOR_ID, 0xA4, Common_read_region, MAX_EEPROM_SIZE_16K},
	{GC16B3_AAC_FRONT_SENSOR_ID, 0xA2, Common_read_region, MAX_EEPROM_SIZE_16K},
	{GC16B3_SUNNY_FRONT_SENSOR_ID, 0xA2, Common_read_region, MAX_EEPROM_SIZE_16K},
	{0, 0, 0}	/* end of list */
};

unsigned int cam_cal_get_sensor_list(
	struct stCAM_CAL_LIST_STRUCT **ppCamcalList)
{
	if (ppCamcalList == NULL)
		return 1;

	*ppCamcalList = &g_camCalList[0];
	return 0;
}


