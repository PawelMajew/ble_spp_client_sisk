///////////////////////////////////////////////////////////////////////////////////
// File: gap.h
//
// Subject: Standardy i Systemy Komunikacyjne, AGH, EiT
//
// Author: Paweł Majewski
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef GAP_H
#define GAP_H

///////////////////////////////////////////////////////////////////////////////////
// HEDER FILES
///////////////////////////////////////////////////////////////////////////////////

#include "ble_spp_client.h"

///////////////////////////////////////////////////////////////////////////////////
// PRAMETERS
///////////////////////////////////////////////////////////////////////////////////

#define GAP_TAG  "GAP"
#define GATTC_TAG "GATT"

extern esp_bd_addr_t address_pm;
extern uint16_t esp_if_pm;
///////////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Callback function for handling Bluetooth Low Energy (BLE) GAP events.
 *
 * This function is invoked in response to various BLE GAP events. It switches
 * on the event type and performs corresponding actions or processing.
 *
 * @param event   The type of BLE GAP event.
 * @param param   A pointer to the parameters associated with the event.
 */
void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

/**
 * @brief Callback function for handling Bluetooth Low Energy (BLE) GATT client (GATTC) events.
 *
 * This function is invoked in response to various BLE GATTC events. It logs event information,
 * handles registration events to associate the GATTC interface with each profile, and calls
 * the appropriate profile's callback function based on the GATTC interface and event type.
 *
 * @param event     The type of GATTC event.
 * @param gattc_if  The GATTC interface associated with the event.
 * @param param     A pointer to the parameters associated with the event.
 */
void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);

/**
 * @brief Initialize the Serial Port Profile (SPP) UART communication.
 *
 * This function sets up the necessary configurations for UART communication,
 * creates a command registration queue, and spawns tasks for SPP client registration
 * and UART communication.
 */
void spp_uart_init(void);

#endif /* GAP_H */