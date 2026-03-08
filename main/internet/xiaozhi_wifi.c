#include "xiaozhi_wifi.h"
bool provisioned = false;


/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

const int WIFI_CONNECTED_EVENT = BIT0;

static const char *TAG = "wifi station";
static int s_retry_num = 0;
#define PROV_QR_VERSION         "v1"
#define PROV_TRANSPORT_SOFTAP   "softap"
#define PROV_TRANSPORT_BLE      "ble"
#define QRCODE_BASE_URL         "https://espressif.github.io/esp-jumpstart/qrcode.html"
#define CONFIG_EXAMPLE_PROV_MGR_MAX_RETRY_CNT 5

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    static int retries;
    if (event_base == WIFI_PROV_EVENT) {// wifi 配网事件
        switch (event_id) {
            case WIFI_PROV_START:   // wifi 配网开始
                ESP_LOGI(TAG, "Provisioning started");
                break;
            case WIFI_PROV_CRED_RECV: {  // wifi 配网获取 wifi密码
                wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
                ESP_LOGI(TAG, "Received Wi-Fi credentials"
                         "\n\tSSID     : %s\n\tPassword : %s",
                         (const char *) wifi_sta_cfg->ssid,
                         (const char *) wifi_sta_cfg->password);
                break;
            }
            case WIFI_PROV_CRED_FAIL: { //wifi 配网失败
                wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
                ESP_LOGE(TAG, "Provisioning failed!\n\tReason : %s"
                         "\n\tPlease reset to factory and retry provisioning",
                         (*reason == WIFI_PROV_STA_AUTH_ERROR) ?
                         "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");
                retries++;
                if (retries >= CONFIG_EXAMPLE_PROV_MGR_MAX_RETRY_CNT) {
                    ESP_LOGI(TAG, "Failed to connect with provisioned AP, reseting provisioned credentials");
                    wifi_prov_mgr_reset_sm_state_on_failure();
                    retries = 0;
                }
                break;
            }
            case WIFI_PROV_CRED_SUCCESS:
                ESP_LOGI(TAG, "Provisioning successful");
                retries = 0;
                break;
            case WIFI_PROV_END:
                /* De-initialize manager once provisioning is finished */
                wifi_prov_mgr_deinit();
                break;
            default:
                break;
        }

    } else if (event_base == WIFI_EVENT)
    {
         switch (event_id) {
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(TAG, "Disconnected. Connecting to the AP again...");
                // esp_wifi_connect();
                break;
                default:
                break;
        }
    }else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
        /* Signal main application to continue execution */
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_EVENT);
    }else if (event_base == PROTOCOMM_TRANSPORT_BLE_EVENT)
    {
        switch (event_id) {
            case PROTOCOMM_TRANSPORT_BLE_CONNECTED:
                ESP_LOGI(TAG, "BLE transport: Connected!");
                break;
            case PROTOCOMM_TRANSPORT_BLE_DISCONNECTED:
                ESP_LOGI(TAG, "BLE transport: Disconnected!");
                break;
            default:
                break;
        }
    }else if (event_base == PROTOCOMM_SECURITY_SESSION_EVENT)
    {
        switch (event_id) {
            case PROTOCOMM_SECURITY_SESSION_SETUP_OK:
                ESP_LOGI(TAG, "Secured session established!");
                break;
            case PROTOCOMM_SECURITY_SESSION_INVALID_SECURITY_PARAMS:
                ESP_LOGE(TAG, "Received invalid security parameters for establishing secure session!");
                break;
            case PROTOCOMM_SECURITY_SESSION_CREDENTIALS_MISMATCH:
                ESP_LOGE(TAG, "Received incorrect username and/or PoP for establishing secure session!");
                break;
            default:
                break;
        }
    }
}

static void wifi_prov_print_qr(const char *name, const char *username, const char *pop, const char *transport)
{
    if (!name || !transport) {
        ESP_LOGW(TAG, "Cannot generate QR code payload. Data missing.");
        return;
    }
    char payload[150] = {0};
     if (pop) {
        snprintf(payload, sizeof(payload), "{\"ver\":\"%s\",\"name\":\"%s\"" \
                    ",\"pop\":\"%s\",\"transport\":\"%s\"}",
                    PROV_QR_VERSION, name, pop, transport);
    }else {
        snprintf(payload, sizeof(payload), "{\"ver\":\"%s\",\"name\":\"%s\"" \
                    ",\"transport\":\"%s\"}",
                    PROV_QR_VERSION, name, transport);
    }
    ESP_LOGI(TAG, "Scan this QR code from the provisioning application for Provisioning.");
    esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
    esp_qrcode_generate(&cfg, payload);
    ESP_LOGI(TAG, "If QR code is not visible, copy paste the below URL in a browser.\n%s?data=%s", QRCODE_BASE_URL, payload);
}
static void get_device_service_name(char *service_name, size_t max)
{
    uint8_t eth_mac[6];
    const char *ssid_prefix = "PROV_";
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    snprintf(service_name, max, "%s%02X%02X%02X",
             ssid_prefix, eth_mac[3], eth_mac[4], eth_mac[5]);
}
void xiaozhi_wifi_config(void)
{
    // 创建一个事件标志组
    s_wifi_event_group = xEventGroupCreate();
    // 初始化tcp/ip
    ESP_ERROR_CHECK(esp_netif_init());
    // 创建事件循环，后面添加进来的回调函数，会触发
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    // 创建wifi station模式
    esp_netif_create_default_wifi_sta();
    // wifi默认配置
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    // wifi 初始化
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    // 注册事件，注册的事件，会默认被添加上面创建的esp_event_loop_create_default中
    /* Register our event handler for Wi-Fi, IP and Provisioning related events */
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(PROTOCOMM_TRANSPORT_BLE_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(PROTOCOMM_SECURITY_SESSION_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    /* Configuration for the provisioning manager */
    wifi_prov_mgr_config_t config = {
        .scheme = wifi_prov_scheme_ble,//使用 ble + gatt 蓝牙配网模式
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM //配完网后，释放蓝牙资源
    };
    /* Initialize provisioning manager with the
     * configuration parameters set above */
    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));
    bool provisioned = false;
    /* Let's find out if the device is provisioned */
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));
    if (!provisioned) { // 如果没有配过网络，第一次配网
        ESP_LOGI(TAG, "Starting provisioning");

        /* - Wi-Fi SSID when scheme is wifi_prov_scheme_softap
            - device name when scheme is wifi_prov_scheme_ble */
        char service_name[12];
        // 生成一个唯一的设备服务名称。用于配网过程中的标识
        // 使用esp32本身的唯一硬件mac地址，生成一个 唯一的设备服务名称
        get_device_service_name(service_name, sizeof(service_name));

        //设置的安全等级为1  其中0是明文通讯，2是安全性更高的通讯
        wifi_prov_security_t security = WIFI_PROV_SECURITY_1;
        const char *pop = "abcd1234";
        wifi_prov_security1_params_t *sec_params = pop;
        const char *username  = NULL;
        const char *service_key = NULL;
        // ble配网 设置的一个128位的uuid
        uint8_t custom_service_uuid[] = {
            /* LSB <---------------------------------------
             * ---------------------------------------> MSB */
            0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
            0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02,
        };

        wifi_prov_scheme_ble_set_service_uuid(custom_service_uuid);

        /* Start provisioning service */
        ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, (const void *) sec_params, service_name, service_key));

        // 打印配网二维码
        wifi_prov_print_qr(service_name, username, pop, PROV_TRANSPORT_BLE);
        } else { // 下面是之前已经配过网了，可以直接进行连接
        ESP_LOGI(TAG, "Already provisioned, starting Wi-Fi STA");

        /* We don't need the manager as device is already provisioned,
         * so let's release it's resources */
        wifi_prov_mgr_deinit();

        /* Start Wi-Fi station */
        ESP_LOGI(TAG, "主程序 正在准备连接wifi");
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_start());
        ESP_LOGI(TAG, "wifi_init_sta finished.");

       EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_EVENT, true, true,3000 / portTICK_PERIOD_MS);
       if (bits & WIFI_CONNECTED_EVENT)
        {
            ESP_LOGI(TAG, "connected to ap SSID: password:");
        }else
        {
            ESP_LOGI(TAG, "disconnect");

        }
    }
}

void xiaozhi_wifi_init(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    xiaozhi_wifi_config();
}
void xiaozhi_wifi_reset_provisioning(void)
{
    // 停止当前的Wi-Fi连接
    esp_wifi_disconnect();
    esp_wifi_stop();
    
    // 重置配网管理器状态
    wifi_prov_mgr_reset_sm_state_on_failure();
    
    // 删除已保存的Wi-Fi配置
    wifi_config_t wifi_cfg = {0};
    esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);
    
    // 重新初始化配网管理器
    wifi_prov_mgr_deinit();
    
    wifi_prov_mgr_config_t config = {
        .scheme = wifi_prov_scheme_ble,
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM
    };
    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));
    
    // 重新启动配网流程
    bool provisioned = false;
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));
    if (!provisioned) {
        ESP_LOGI(TAG, "Starting provisioning after reset");

        char service_name[12];
        get_device_service_name(service_name, sizeof(service_name));

        wifi_prov_security_t security = WIFI_PROV_SECURITY_1;
        const char *pop = "abcd1234";
        wifi_prov_security1_params_t *sec_params = pop;
        const char *service_key = NULL;
        uint8_t custom_service_uuid[] = {
            0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
            0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02,
        };

        wifi_prov_scheme_ble_set_service_uuid(custom_service_uuid);

        ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, (const void *) sec_params, service_name, service_key));

        wifi_prov_print_qr(service_name, NULL, pop, PROV_TRANSPORT_BLE);
    }
}
