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

///////////////////////////////////////////////////////////////////////////////////
// HEDER FILES
///////////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////////
// PRAMETERS
///////////////////////////////////////////////////////////////////////////////////

#define ESP_MAIN_TAG "ESP_START"

#define ESP_SPP_APP_ID 0

#define APP_ID 0
#define PROFILE_NUM 1

#define ESP_GATT_SPP_SERVICE_UUID   0xABF0
#define SCAN_ALL_THE_TIME 0

///////////////////////////////////////////////////////////////////////////////////
// GLOBAL DATA
///////////////////////////////////////////////////////////////////////////////////

extern esp_bd_addr_t address_pm;
extern uint16_t esp_if_pm;
extern bool is_con;

///////////////////////////////////////////////////////////////////////////////////
// DATA
///////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Structure representing a GATT client profile instance.
 */
struct gattc_profile_inst {
    esp_gattc_cb_t gattc_cb;         /**< GATT client callback. */
    uint16_t gattc_if;               /**< GATT client interface. */
    uint16_t app_id;                 /**< Application ID. */
    uint16_t conn_id;                /**< Connection ID. */
    uint16_t service_start_handle;   /**< Service start handle. */
    uint16_t service_end_handle;     /**< Service end handle. */
    uint16_t char_handle;            /**< Characteristic handle. */
    esp_bd_addr_t remote_bda;        /**< Remote Bluetooth device address. */
};

/**
 * @brief Enumeration representing the indices of attributes in the GATT server profile.
 */
enum {
    SPP_IDX_SVC,               /**< Service index. */

    SPP_IDX_SPP_DATA_RECV_VAL, /**< Data receive characteristic value index. */

    SPP_IDX_SPP_DATA_NTY_VAL,  /**< Data notify characteristic value index. */
    SPP_IDX_SPP_DATA_NTF_CFG,  /**< Data notify characteristic configuration index. */

    SPP_IDX_SPP_COMMAND_VAL,   /**< Command characteristic value index. */

    SPP_IDX_SPP_STATUS_VAL,    /**< Status characteristic value index. */
    SPP_IDX_SPP_STATUS_CFG,    /**< Status characteristic configuration index. */

    SPP_IDX_NB,                /**< Total number of indices. */
};

#endif /* BLE_SPP_CLIENT_H */