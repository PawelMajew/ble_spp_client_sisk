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
void esp_gap_handle(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

#endif /* GAP_H */