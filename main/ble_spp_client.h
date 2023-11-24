///////////////////////////////////////////////////////////////////////////////////
// File: ble_spp_client.h
//
// Subject: Standardy i Systemy Komunikacyjne, AGH, EiT
//
// Author: Paweł Majewski
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef BLE_SPP_CLIENT_H
#define BLE_SPP_CLIENT_H

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "driver/uart.h"

#include "esp_bt.h"
#include "nvs_flash.h"
#include "esp_bt_device.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_system.h"
#include "esp_gatt_common_api.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#define ESP_MAIN_TAG "ESP_START"

#define ESP_SPP_APP_ID 0

#define APP_ID 0
#define PROFILE_NUM 1

#define ESP_GATT_SPP_SERVICE_UUID   0xABF0
#define SCAN_ALL_THE_TIME 0

struct gattc_profile_inst {
    esp_gattc_cb_t gattc_cb;
    uint16_t gattc_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_start_handle;
    uint16_t service_end_handle;
    uint16_t char_handle;
    esp_bd_addr_t remote_bda;
};

enum{
    SPP_IDX_SVC,

    SPP_IDX_SPP_DATA_RECV_VAL,

    SPP_IDX_SPP_DATA_NTY_VAL,
    SPP_IDX_SPP_DATA_NTF_CFG,

    SPP_IDX_SPP_COMMAND_VAL,

    SPP_IDX_SPP_STATUS_VAL,
    SPP_IDX_SPP_STATUS_CFG,

    SPP_IDX_NB,
};

#endif /* BLE_SPP_CLIENT_H */