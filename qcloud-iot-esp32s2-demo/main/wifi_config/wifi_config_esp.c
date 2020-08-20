/*
 * Copyright (c) 2020 Tencent Cloud. All rights reserved.

 * Licensed under the MIT License (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT

 * Unless required by applicable law or agreed to in writing, software distributed under the License is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */



#include <string.h>
#include <time.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_smartconfig.h"
#include "esp_event_loop.h"
#include "tcpip_adapter.h"

#include "qcloud_iot_export.h"
#include "qcloud_iot_import.h"
#include "utils_hmac.h"
#include "utils_base64.h"

#include "wifi_config_internal.h"
#include "board_ops.h"

#include "nvs.h"
#include "nvs_flash.h"


/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t sg_wifi_event_group;
static nvs_handle handle;
static const char *NVS_CUSTOMER = "customer data";
static const char *DATA1 = "param 1";
static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;
static const int APSTA_DISCONNECTED_BIT = BIT2;
static const int          STA_DISCONNECTED_BIT = BIT2;

static bool sg_wifi_init_done = false;
static bool sg_wifi_sta_connected = false;

static system_event_cb_t g_cb_bck = NULL;
//============================ ESP wifi functions begin ===========================//

bool is_wifi_sta_connected(void)
{
    return sg_wifi_sta_connected;
}


static void _wifi_event_handler(void* arg, esp_event_base_t event_base, 
                                int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        Log_i("WIFI_EVENT_AP_STACONNECTED, STA:" MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
        
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        xEventGroupSetBits(sg_wifi_event_group, APSTA_DISCONNECTED_BIT);
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        
        Log_i("WIFI_EVENT_AP_STADISCONNECTED, STA:" MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);
        
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        Log_i("SYSTEM_EVENT_STA_START");
        
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        
        wifi_event_sta_connected_t* event = (wifi_event_sta_connected_t *)event_data;
        char ssid[33] = {0};
        memcpy(ssid, event->ssid, 32);
        Log_e("WIFI_EVENT_STA_CONNECTED with AP: %s in channel %u", ssid, event->channel);
        xEventGroupClearBits(sg_wifi_event_group, CONNECTED_BIT);
        sg_wifi_sta_connected = false;
        
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        
        wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t *)event_data;
        char ssid[33] = {0};
        memcpy(ssid, event->ssid, 32);
        Log_e("WIFI_EVENT_STA_DISCONNECTED with AP: %s", ssid);
        xEventGroupClearBits(sg_wifi_event_group, CONNECTED_BIT);
        sg_wifi_sta_connected = false;
        
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;        
        Log_i("STA Got IPv4[%s]", ip4addr_ntoa(&event->ip_info.ip));

        xEventGroupSetBits(sg_wifi_event_group, CONNECTED_BIT);
        sg_wifi_sta_connected = true;
        Log_i("sg_wifi_sta_connected[%d]", sg_wifi_sta_connected);
        
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
        Log_i("Scan done");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
        Log_i("Found channel");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {

        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
        wifi_config_t wifi_config;
        uint8_t ssid[33] = { 0 };
        uint8_t password[65] = { 0 };
        memcpy(ssid, evt->ssid, sizeof(evt->ssid));
        memcpy(password, evt->password, sizeof(evt->password));
        Log_i( "SC_EVENT_GOT_SSID_PSWD SSID:%s PSD: %s", ssid, password);
        
        bzero(&wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
        wifi_config.sta.bssid_set = evt->bssid_set;
        if (wifi_config.sta.bssid_set == true) {
            memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
        }

        int ret = esp_wifi_disconnect();
        if ( ESP_OK != ret ) {
            Log_e("esp_wifi_disconnect failed: %d", ret);
            //push_error_log(ERR_WIFI_DISCONNECT, ret);
        }

        ret = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
        if ( ESP_OK != ret ) {
            Log_e("esp_wifi_set_config failed: %d", ret);
            //push_error_log(ERR_WIFI_CONFIG, ret);
        }

         ESP_ERROR_CHECK( nvs_open( NVS_CUSTOMER, NVS_READWRITE, &handle) ); 
         ESP_ERROR_CHECK( nvs_set_blob( handle, DATA1, &wifi_config, sizeof(wifi_config)) );
         ESP_ERROR_CHECK( nvs_commit(handle) );

        ret = esp_wifi_connect();
        if ( ESP_OK != ret ) {
            Log_e("esp_wifi_connect failed: %d", ret);
            push_error_log(ERR_WIFI_CONNECT, ret);
        }
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
        Log_w( "SC_EVENT_SEND_ACK_DONE");
        xEventGroupSetBits(sg_wifi_event_group, ESPTOUCH_DONE_BIT);
    }
}


int wifi_ap_init(const char *ssid, const char *psw, uint8_t ch)
{
    esp_err_t rc;
    if (!sg_wifi_init_done) {
        tcpip_adapter_init();
        sg_wifi_event_group = xEventGroupCreate();
        if (sg_wifi_event_group == NULL) {
            Log_e("xEventGroupCreate failed!");
            return ESP_ERR_NO_MEM;
        }

        rc = esp_event_loop_create_default();
        if (rc != ESP_OK) {
            Log_e("esp_event_loop_create_default failed: %d", rc);
            return rc;
        }
        
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        rc = esp_wifi_init(&cfg);
        if (rc != ESP_OK) {
            Log_e("esp_wifi_init failed: %d", rc);
            return rc;
        }
        sg_wifi_init_done = true;
    }

    sg_wifi_sta_connected = false;

    xEventGroupClearBits(sg_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT | STA_DISCONNECTED_BIT);

    // should disconnect first, could be failed if not connected
    rc = esp_wifi_disconnect();
    if (ESP_OK != rc) {
        Log_w("esp_wifi_disconnect failed: %d", rc);
    }

    rc = esp_wifi_stop();
    if (rc != ESP_OK) {
        Log_w("esp_wifi_stop failed: %d", rc);
    }

    if (esp_event_loop_init(_wifi_event_handler, NULL) && g_cb_bck == NULL) {
        Log_w("replace esp wifi event handler");
        g_cb_bck = esp_event_loop_set_cb(_wifi_event_handler, NULL);
    }

    rc = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (rc != ESP_OK) {
        Log_e("esp_wifi_set_storage failed: %d", rc);
        return rc;
    }

    wifi_config_t wifi_config = {0};
    strcpy((char *)wifi_config.ap.ssid, ssid);
    wifi_config.ap.ssid_len       = strlen(ssid);
    wifi_config.ap.max_connection = 3;
    wifi_config.ap.channel        = (uint8_t)ch;
    if (psw) {
        strcpy((char *)wifi_config.ap.password, psw);
        wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    } else
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;

    rc = esp_wifi_set_mode(WIFI_MODE_AP);
    if (rc != ESP_OK) {
        Log_e("esp_wifi_set_mode failed: %d", rc);
        return rc;
    }

    rc = esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config);
    if (rc != ESP_OK) {
        Log_e("esp_wifi_set_config failed: %d", rc);
        return rc;
    }

    return ESP_OK;

}


// connect in APSTA mode
int wifi_ap_sta_connect(const char *ssid, const char *psw)
{
    static wifi_config_t router_wifi_config = {0};
    memset(&router_wifi_config, 0, sizeof(router_wifi_config));
    strncpy((char *)router_wifi_config.sta.ssid, ssid, 32);
    strncpy((char *)router_wifi_config.sta.password, psw, 64);

    esp_err_t rc = ESP_OK;

    rc = esp_wifi_set_storage(WIFI_STORAGE_FLASH);
    if (rc != ESP_OK) {
        Log_e("esp_wifi_set_storage failed: %d", rc);
        return rc;
    }

    rc = esp_wifi_set_mode(WIFI_MODE_APSTA);
    if (rc != ESP_OK) {
        Log_e("esp_wifi_set_mode failed: %d", rc);
        return rc;
    }

    rc = esp_wifi_set_config(ESP_IF_WIFI_STA, &router_wifi_config);
    if (rc != ESP_OK) {
        Log_e("esp_wifi_set_config failed: %d", rc);
        return rc;
    }

    rc = esp_wifi_connect();
    if ( ESP_OK != rc ) {
        Log_e("esp_wifi_connect failed: %d", rc);
        return rc;
    }

    return 0;
}

// connect in STA mode
int wifi_sta_connect(const char *ssid, const char *psw)
{
    static wifi_config_t router_wifi_config = {0};
    memset(&router_wifi_config, 0, sizeof(router_wifi_config));
    strncpy((char *)router_wifi_config.sta.ssid, ssid, 32);
    strncpy((char *)router_wifi_config.sta.password, psw, 64);

    esp_err_t rc = ESP_OK;

    rc = esp_wifi_set_storage(WIFI_STORAGE_FLASH);
    if (rc != ESP_OK) {
        Log_e("esp_wifi_set_storage failed: %d", rc);
        return rc;
    }

    rc = esp_wifi_set_mode(WIFI_MODE_STA);
    if (rc != ESP_OK) {
        Log_e("esp_wifi_set_mode failed: %d", rc);
        return rc;
    }

    rc = esp_wifi_set_config(ESP_IF_WIFI_STA, &router_wifi_config);
    if (rc != ESP_OK) {
        Log_e("esp_wifi_set_config failed: %d", rc);
        return rc;
    }

    rc = esp_wifi_connect();
    if ( ESP_OK != rc ) {
        Log_e("esp_wifi_connect failed: %d", rc);
        return rc;
    }

    return 0;
}

int wifi_stop_softap(void)
{
    Log_i("Switch to STA mode");

    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK) {
        Log_e("esp_wifi_set_mode STA failed");
    }

    if (g_cb_bck) {
        esp_event_loop_set_cb(g_cb_bck, NULL);
        g_cb_bck = NULL;
    }

    return 0;
}


int wifi_sta_init(void)
{
    esp_err_t rc;
    if (!sg_wifi_init_done) {
        tcpip_adapter_init();
        sg_wifi_event_group = xEventGroupCreate();
        if (sg_wifi_event_group == NULL) {
            Log_e("xEventGroupCreate failed!");
            return ESP_ERR_NO_MEM;
        }

        rc = esp_event_loop_create_default();
        if (rc != ESP_OK) {
            Log_e("esp_event_loop_create_default failed: %d", rc);
            return rc;
        }
        
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        rc = esp_wifi_init(&cfg);
        if (rc != ESP_OK) {
            Log_e("esp_wifi_init failed: %d", rc);
            return rc;
        }
        sg_wifi_init_done = true;
    }

    sg_wifi_sta_connected = false;

    xEventGroupClearBits(sg_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT | APSTA_DISCONNECTED_BIT);

    rc = esp_wifi_stop();
    if (rc != ESP_OK) {
        Log_w("esp_wifi_stop failed: %d", rc);
    }

    rc = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &_wifi_event_handler, NULL);
    if (rc != ESP_OK) {
        Log_e("esp_event_handler_register WIFI_EVENT failed: %d", rc);
        return rc;
    }
    
    rc = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &_wifi_event_handler, NULL);
    if (rc != ESP_OK) {
        Log_e("esp_event_handler_register IP_EVENT failed: %d", rc);
        return rc;
    }    

    rc = esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &_wifi_event_handler, NULL);
    if (rc != ESP_OK) {
        Log_e("esp_event_handler_register SC_EVENT failed: %d", rc);
        return rc;
    }
    
    rc = esp_wifi_set_storage(WIFI_STORAGE_FLASH);
    if (rc != ESP_OK) {
        Log_e("esp_wifi_set_storage failed: %d", rc);
        return rc;
    }

    rc = esp_wifi_set_mode(WIFI_MODE_STA);
    if (rc != ESP_OK) {
        Log_e("esp_wifi_set_mode failed: %d", rc);
        return rc;
    }

    return ESP_OK;
}

int wifi_start_smartconfig(void)
{
    int ret = esp_smartconfig_set_type(SC_TYPE_ESPTOUCH);
    if ( ESP_OK != ret ) {
        Log_e("esp_smartconfig_set_type failed: %d", ret);
        return ret;
    }

    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    ret = esp_smartconfig_start(&cfg);
    if ( ESP_OK != ret ) {
        Log_e("esp_smartconfig_start failed: %d", ret);
        return ret;
    }
    xEventGroupWaitBits(sg_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);

    return ESP_OK;
}

int wifi_stop_smartconfig(void)
{
    int ret = esp_smartconfig_stop();
    xEventGroupClearBits(sg_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT);

    if (g_cb_bck) {
        esp_event_loop_set_cb(g_cb_bck, NULL);
        g_cb_bck = NULL;
    }

    return ret;
}

int wifi_wait_event(unsigned int timeout_ms)
{
    EventBits_t uxBits = xEventGroupWaitBits(sg_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT,
                         true, false, timeout_ms / portTICK_RATE_MS);
    if (uxBits & CONNECTED_BIT) {
        return EVENT_WIFI_CONNECTED;
    }

    if (uxBits & ESPTOUCH_DONE_BIT) {
        return EVENT_SMARTCONFIG_STOP;
    }

    return EVENT_WAIT_TIMEOUT;
}

int wifi_start_running(void)
{
    return esp_wifi_start();
}
//============================ ESP wifi functions end ===========================//

