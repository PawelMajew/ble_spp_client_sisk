///////////////////////////////////////////////////////////////////////////////////
// File: ble_spp_client.c
//
// Subject: Standardy i Systemy Komunikacyjne, AGH, EiT
//
// Author: Paweł Majewski
//
///////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////
// HEDER FILES
///////////////////////////////////////////////////////////////////////////////////

#include "ble_spp_client.h"
#include "gap.h"

void app_main(void)
{
    esp_err_t error;

    ESP_LOGE(ESP_MAIN_TAG, "NVS init");
    // Initialize NVS.
    error = nvs_flash_init();
    if (error == ESP_OK)
    {
        ESP_LOGE(ESP_MAIN_TAG, "NVS init successful");
    }
    else
    {
        ESP_LOGE(ESP_MAIN_TAG, "NVS init unsuccessful");
    }

    // Bluetooth controller initialization with default config.
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    error = esp_bt_controller_init(&bt_cfg);
    if (error == ESP_OK)
    {
        ESP_LOGE(ESP_MAIN_TAG, "Controller init successful");
    }
    else
    {
        ESP_LOGE(ESP_MAIN_TAG, "Controller init unsuccessful");
    }

    // Turning on the controller in ble mode.
    error = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (error == ESP_OK)
    {
        ESP_LOGE(ESP_MAIN_TAG, "Controller turn on in ble mode successful");
    }
    else
    {
        ESP_LOGE(ESP_MAIN_TAG, "Controller turn on in ble mode unsuccessful");
    }

    // Init and alloc the resource for bluetooth.
    error = esp_bluedroid_init();
    if (error == ESP_OK)
    {
        ESP_LOGE(ESP_MAIN_TAG, "Bluedroid init successful");
    }
    else
    {
        ESP_LOGE(ESP_MAIN_TAG, "Bluedroid init unsuccessful");
    }

    // Enables the BlueDroid Bluetooth stack after it initializes.
    error = esp_bluedroid_enable();
    if (error == ESP_OK)
    {
        ESP_LOGE(ESP_MAIN_TAG, "Bluedroid enable stack successful");
    }
    else
    {
        ESP_LOGE(ESP_MAIN_TAG, "Bluedroid enable stack unsuccessful");
    }

    // Registering a function that supports gatt.
    error = esp_ble_gattc_register_callback(esp_gattc_cb);
    if (error == ESP_OK)
    {
        ESP_LOGE(ESP_MAIN_TAG, "Gatt handle registration successful");
    }
    else
    {
        ESP_LOGE(ESP_MAIN_TAG, "Gatt handle registration unsuccessful");
    }

    // Registers a gap handling function.
    error = esp_ble_gap_register_callback(esp_gap_cb);
    if (error == ESP_OK)
    {
        ESP_LOGE(ESP_MAIN_TAG, "Gap handle registration successful");
    }
    else
    {
        ESP_LOGE(ESP_MAIN_TAG, "Gap handle registration unsuccessful");
    }

    // Gatt application registration.
    error = esp_ble_gattc_app_register(ESP_SPP_APP_ID);
    if (error == ESP_OK)
    {
        ESP_LOGE(ESP_MAIN_TAG, "Gatt app registration successful");
    }
    else
    {
        ESP_LOGE(ESP_MAIN_TAG, "Gatt app registration unsuccessful");
    }

    // Set local MTU.
    error = esp_ble_gatt_set_local_mtu(200);
    if (error == ESP_OK){
        ESP_LOGE(ESP_MAIN_TAG, "set local  MTU successful.");
    }
    else
    {
        ESP_LOGE(ESP_MAIN_TAG, "set local  MTU failed.");
    }

    // Task initialization SPP.
    spp_uart_init();

    return;
}
