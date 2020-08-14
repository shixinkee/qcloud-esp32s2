idf_component_register(SRCS "qcloud_iot_c_sdk/sdk_src/data_template_aciton.c"          "qcloud_iot_c_sdk/sdk_src/gateway_common.c"       "qcloud_iot_c_sdk/sdk_src/mqtt_client_subscribe.c"    "qcloud_iot_c_sdk/sdk_src/ota_lib.c"            "qcloud_iot_c_sdk/sdk_src/utils_getopt.c"
                        "qcloud_iot_c_sdk/sdk_src/data_template_client.c"          "qcloud_iot_c_sdk/sdk_src/json_parser.c"          "qcloud_iot_c_sdk/sdk_src/mqtt_client_unsubscribe.c"  "qcloud_iot_c_sdk/sdk_src/ota_mqtt.c"           "qcloud_iot_c_sdk/sdk_src/utils_hmac.c"
                        "qcloud_iot_c_sdk/sdk_src/data_template_client_common.c"   "qcloud_iot_c_sdk/sdk_src/json_token.c"           "qcloud_iot_c_sdk/sdk_src/mqtt_client_yield.c"        "qcloud_iot_c_sdk/sdk_src/qcloud_iot_ca.c"      "qcloud_iot_c_sdk/sdk_src/utils_httpc.c"
                        "qcloud_iot_c_sdk/sdk_src/data_template_client_json.c"     "qcloud_iot_c_sdk/sdk_src/mqtt_client.c"          "qcloud_iot_c_sdk/sdk_src/network_interface.c"        "qcloud_iot_c_sdk/sdk_src/qcloud_iot_device.c"  "qcloud_iot_c_sdk/sdk_src/utils_list.c"
                        "qcloud_iot_c_sdk/sdk_src/data_template_client_manager.c"  "qcloud_iot_c_sdk/sdk_src/mqtt_client_common.c"   "qcloud_iot_c_sdk/sdk_src/network_socket.c"           "qcloud_iot_c_sdk/sdk_src/qcloud_iot_log.c"     "qcloud_iot_c_sdk/sdk_src/utils_md5.c"
                        "qcloud_iot_c_sdk/sdk_src/data_template_event.c"           "qcloud_iot_c_sdk/sdk_src/mqtt_client_connect.c"  "qcloud_iot_c_sdk/sdk_src/network_tls.c"              "qcloud_iot_c_sdk/sdk_src/string_utils.c"       "qcloud_iot_c_sdk/sdk_src/utils_ringbuff.c"
                        "qcloud_iot_c_sdk/sdk_src/dynreg.c"                        "qcloud_iot_c_sdk/sdk_src/mqtt_client_net.c"      "qcloud_iot_c_sdk/sdk_src/ota_client.c"               "qcloud_iot_c_sdk/sdk_src/utils_aes.c"          "qcloud_iot_c_sdk/sdk_src/utils_sha1.c"
                        "qcloud_iot_c_sdk/sdk_src/gateway_api.c"                   "qcloud_iot_c_sdk/sdk_src/mqtt_client_publish.c"  "qcloud_iot_c_sdk/sdk_src/ota_fetch.c"                "qcloud_iot_c_sdk/sdk_src/utils_base64.c"       "qcloud_iot_c_sdk/sdk_src/utils_timer.c"
                        "qcloud_iot_c_sdk/platform/HAL_Device_freertos.c"          "qcloud_iot_c_sdk/platform/HAL_OS_freertos.c"     "qcloud_iot_c_sdk/platform/HAL_Timer_freertos.c"      "qcloud_iot_c_sdk/platform/HAL_UDP_lwip.c"
                        "qcloud_iot_c_sdk/platform/HAL_DTLS_mbedtls.c"             "qcloud_iot_c_sdk/platform/HAL_TCP_lwip.c"        "qcloud_iot_c_sdk/platform/HAL_TLS_mbedtls.c"
                        INCLUDE_DIRS "qcloud_iot_c_sdk/include" "qcloud_iot_c_sdk/include/exports" "qcloud_iot_c_sdk/sdk_src/internal_inc"
                        REQUIRES mbedtls
                        )
                        
#                         # "qcloud_iot_c_sdk/sdk_src/*.c" 
#                         # "qcloud_iot_c_sdk/platform/*.c"


# idf_component_register(SRCS "qcloud_iot_c_sdk/sdk_src/*.c" "qcloud_iot_c_sdk/platform/*.c"
#                         INCLUDE_DIRS "qcloud_iot_c_sdk/include" "qcloud_iot_c_sdk/include/exports" "qcloud_iot_c_sdk/sdk_src/internal_inc")