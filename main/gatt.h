///////////////////////////////////////////////////////////////////////////////////
// File: gatt.h
//
// Subject: Standardy i Systemy Komunikacyjne, AGH, EiT
//
// Author: Paweł Majewski
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef GATT_H
#define GATT_H

///////////////////////////////////////////////////////////////////////////////////
// HEDER FILES
///////////////////////////////////////////////////////////////////////////////////

#include "ble_spp_client.h"

///////////////////////////////////////////////////////////////////////////////////
// PRAMETERS
///////////////////////////////////////////////////////////////////////////////////

#define GATT_TAG "GATT"
///////////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Callback function for handling Bluetooth Low Energy (BLE) GATT client (GATTC) events.
 *
 * This function is invoked in response to various BLE GATTC events. It logs event information,
 * processes different GATTC events, and performs corresponding actions based on the event type.
 *
 * @param event     The type of GATTC event.
 * @param gattc_if  The GATTC interface associated with the event.
 * @param param     A pointer to the parameters associated with the event.
 */
void esp_gattc_handle(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);

/**
 * @brief Initialize the Serial Port Profile (SPP) UART communication.
 *
 * This function sets up the necessary configurations for UART communication,
 * creates a command registration queue, and spawns tasks for SPP client registration
 * and UART communication.
 */
void spp_uart_init(void);

#endif