#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "sdkconfig.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_openthread.h"
#include "esp_openthread_cli.h"
#include "esp_openthread_lock.h"
#include "esp_openthread_netif_glue.h"
#include "esp_openthread_types.h"
#include <openthread/thread.h>
#include <openthread/ip6.h>

#include "nvs_flash.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/uart_types.h"
#include "openthread/error.h"
#include "openthread/logging.h"
#include "project_config.h"
#include "esp_vfs.h"
#include "esp_vfs_eventfd.h"

#include <openthread/config.h>
#include <openthread/cli.h>
#include <openthread/diag.h>
#include <openthread/tasklet.h>
#include <openthread/instance.h>
#include <openthread/message.h>
#include <openthread/udp.h>
#include <openthread/platform/logging.h>
#include <openthread/platform/misc.h>
#include "driver/temperature_sensor.h"
#include "esp_sleep.h"
#include <openthread/thread.h>

#include <openthread/dataset.h>
#include <openthread/joiner.h>
#include <openthread/link.h>
#include <openthread/netdata.h>
#include <openthread/tasklet.h>
#include <openthread/thread.h>

#define NETWORK_KEY "00112233445566778899aabbccddeeff"
#define LEADER_NAME "LeaderDevice"
#define MAX_JOIN_TIME_SECONDS (120)

static const char *TAG = "ot_end_device";
static otInstance *sInstance = NULL;

#define UDP_PORT 1212
#define UDP_DEST_ADDR "ff03::1"

static otUdpSocket sUdpSocket;

static int hex_digit_to_int(char hex)
{
    if ('A' <= hex && hex <= 'F')
    {
        return 10 + hex - 'A';
    }
    if ('a' <= hex && hex <= 'f')
    {
        return 10 + hex - 'a';
    }
    if ('0' <= hex && hex <= '9')
    {
        return hex - '0';
    }
    return -1;
}

static size_t hex_string_to_binary(const char *hex_string, uint8_t *buf, size_t buf_size)
{
    int num_char = strlen(hex_string);

    if (num_char != buf_size * 2)
    {
        return 0;
    }
    for (size_t i = 0; i < num_char; i += 2)
    {
        int digit0 = hex_digit_to_int(hex_string[i]);
        int digit1 = hex_digit_to_int(hex_string[i + 1]);

        if (digit0 < 0 || digit1 < 0)
        {
            return 0;
        }
        buf[i / 2] = (digit0 << 4) + digit1;
    }

    return buf_size;
}

void handleNetifStateChanged(uint32_t aFlags, void *aContext)
{
    if ((aFlags & OT_CHANGED_THREAD_ROLE) != 0)
    {
        otDeviceRole changedRole = otThreadGetDeviceRole(aContext);
        printf("Role changed to %d\n", changedRole);
    }
}

/**
 * Function to handle UDP datagrams received on the listening socket
 */
void handleUdpReceive(void *aContext, otMessage *aMessage,
                      const otMessageInfo *aMessageInfo)
{
    OT_UNUSED_VARIABLE(aContext);
    OT_UNUSED_VARIABLE(aMessage);
    OT_UNUSED_VARIABLE(aMessageInfo);
    ESP_LOGI(TAG, "Received UDP message");
}

/**
 * Initialize UDP socket
 */
void initUdp(otInstance *aInstance)
{
    otSockAddr listenSockAddr;

    memset(&sUdpSocket, 0, sizeof(sUdpSocket));
    memset(&listenSockAddr, 0, sizeof(listenSockAddr));

    listenSockAddr.mPort = UDP_PORT;

    otUdpOpen(aInstance, &sUdpSocket, handleUdpReceive, aInstance);
    otUdpBind(aInstance, &sUdpSocket, &listenSockAddr, OT_NETIF_THREAD);
}

otError sendUdp(const char *aMessage)
{
    otError error = OT_ERROR_NONE;
    otMessage *message = NULL;
    otMessageInfo messageInfo;
    otIp6Address destinationAddr;
    otInstance *aInstance = esp_openthread_get_instance();

    otDeviceRole deviceRole = otThreadGetDeviceRole(aInstance);
    if (deviceRole == OT_DEVICE_ROLE_DISABLED || deviceRole == OT_DEVICE_ROLE_DETACHED)
    {
        ESP_LOGE(TAG, "Device is not connected to the Thread network");
        return 0;
    }

    memset(&messageInfo, 0, sizeof(messageInfo));

    error = otIp6AddressFromString(UDP_DEST_ADDR, &destinationAddr);
    if (error != OT_ERROR_NONE)
    {
        ESP_LOGE(TAG, "Failed to parse destination address: %s", otThreadErrorToString(error));
    }
    messageInfo.mPeerAddr = destinationAddr;
    messageInfo.mPeerPort = 40001;

    message = otUdpNewMessage(aInstance, NULL);
    if (message == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate message");
    }

    error = otMessageAppend(message, aMessage, strlen(aMessage));
    if (error != OT_ERROR_NONE)
    {
        ESP_LOGE(TAG, "Failed to append message: %s", otThreadErrorToString(error));
    }

    printf("sending  message: ");

    error = otUdpSend(aInstance, &sUdpSocket, message, &messageInfo);
    if (error != OT_ERROR_NONE)
    {
        ESP_LOGE(TAG, "Failed to send message: %s", otThreadErrorToString(error));
    }

    ESP_LOGI(TAG, "Message sent successfully");
    message = NULL;
    if (message != NULL)
    {
        otMessageFree(message);
    }

    return ESP_OK;
}

/**
 * Override default network settings, such as panid, so the devices can join a
 network
 */
void joinExistingNetwork(otInstance *aInstance)
{
    size_t len = 0;
    otOperationalDataset aDataset;

    memset(&aDataset, 0, sizeof(otOperationalDataset));

    /* Set Channel to 15 */
    aDataset.mChannel = CONFIG_OPENTHREAD_NETWORK_CHANNEL;
    aDataset.mComponents.mIsChannelPresent = true;
    aDataset.mPanId = CONFIG_OPENTHREAD_NETWORK_PANID;
    aDataset.mComponents.mIsPanIdPresent = true;
    len = strlen(CONFIG_OPENTHREAD_NETWORK_NAME);
    assert(len <= OT_NETWORK_NAME_MAX_SIZE);
    memcpy(aDataset.mNetworkName.m8, CONFIG_OPENTHREAD_NETWORK_NAME, len + 1);
    aDataset.mComponents.mIsNetworkNamePresent = true;

    // Extended Pan ID
    len = hex_string_to_binary(CONFIG_OPENTHREAD_NETWORK_EXTPANID, aDataset.mExtendedPanId.m8,
                               sizeof(aDataset.mExtendedPanId.m8));
    aDataset.mComponents.mIsExtendedPanIdPresent = true;

    // Network Key
    len = hex_string_to_binary(CONFIG_OPENTHREAD_NETWORK_MASTERKEY, aDataset.mNetworkKey.m8,
                               sizeof(aDataset.mNetworkKey.m8));
    aDataset.mComponents.mIsNetworkKeyPresent = true;

    // /* Set Extended Pan ID to C0DE1AB5C0DE1AB5 */
    // uint8_t extPanId[OT_EXT_PAN_ID_SIZE] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
    // memcpy(aDataset.mExtendedPanId.m8, extPanId, sizeof(aDataset.mExtendedPanId));
    // aDataset.mComponents.mIsExtendedPanIdPresent = true;

    // /* Set network key to 1234C0DE1AB51234C0DE1AB51234C0DE */
    // uint8_t networkKey[OT_NETWORK_KEY_SIZE] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    //                                            0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    // memcpy(aDataset.mNetworkKey.m8, networkKey, sizeof(aDataset.mNetworkKey));
    // aDataset.mComponents.mIsNetworkKeyPresent = true;

    // Set Network Key
    // uint8_t networkKey[OT_NETWORK_KEY_SIZE];
    // size_t keyLength = strlen(NETWORK_KEY) / 2;
    // for (size_t i = 0; i < keyLength && i < OT_NETWORK_KEY_SIZE; i++) {
    //     sscanf(&NETWORK_KEY[i * 2], "%2hhx", &networkKey[i]);
    // }
    // memcpy(aDataset.mNetworkKey.m8, networkKey, sizeof(aDataset.mNetworkKey));
    // aDataset.mComponents.mIsNetworkKeyPresent = true;

    otDatasetSetActive(sInstance, &aDataset);
}

void cleanup(void)
{
    if (sInstance != NULL)
    {
        otInstanceFinalize(sInstance);
        esp_openthread_deinit();
    }
}

static void ot_task_worker(void *aContext)
{
    size_t len = 0;

    esp_openthread_platform_config_t config = {
        .radio_config = ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_OPENTHREAD_DEFAULT_HOST_CONFIG(),
        .port_config = ESP_OPENTHREAD_DEFAULT_PORT_CONFIG(),
    };

    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_OPENTHREAD();
    esp_netif_t *openthread_netif = esp_netif_new(&cfg);
    assert(openthread_netif != NULL);

    ESP_ERROR_CHECK(esp_openthread_init(&config));
    esp_openthread_lock_acquire(portMAX_DELAY);

    ESP_ERROR_CHECK(esp_netif_attach(openthread_netif, esp_openthread_netif_glue_init(&config)));
    (void)otLoggingSetLevel(CONFIG_LOG_DEFAULT_LEVEL);

    esp_openthread_lock_release();
    esp_openthread_launch_mainloop();

    esp_openthread_netif_glue_deinit();
    esp_netif_destroy(openthread_netif);
    esp_vfs_eventfd_unregister();
    vTaskDelete(NULL);
}

esp_err_t initThread()
{
    printf("Starting Thread network\n");

    esp_vfs_eventfd_config_t eventfd_config = {.max_fds = 4};
    ESP_ERROR_CHECK(esp_vfs_eventfd_register(&eventfd_config));
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    xTaskCreate(ot_task_worker, "ot_ts_wroker", 20480, xTaskGetCurrentTaskHandle(), 5, NULL);

    sInstance = esp_openthread_get_instance();

    joinExistingNetwork(sInstance);
    otSetStateChangedCallback(sInstance, handleNetifStateChanged, sInstance);
    initUdp(sInstance);

    ESP_ERROR_CHECK(otIp6SetEnabled(sInstance, true));
    ESP_ERROR_CHECK(otThreadSetEnabled(sInstance, true));
    return ESP_OK;
}

esp_err_t joinToThread(bool rx, bool (*rx_cb)(void))
{
    printf("Joining Thread network\n");

    otLinkModeConfig linkModeConfig = {0};
    linkModeConfig.mRxOnWhenIdle = 0;
    linkModeConfig.mDeviceType = rx;
    linkModeConfig.mNetworkData = false;
    otThreadSetLinkMode(sInstance, linkModeConfig);

    otLinkSetPollPeriod(sInstance, 5000);

    otLinkSetPcapCallback(sInstance, NULL, NULL);
    otPlatRadioSetPromiscuous(sInstance, false);

    int elapsed_time = 0;
    int delay_interval = 1000;
    while (otThreadGetDeviceRole(sInstance) != OT_DEVICE_ROLE_CHILD)
    {
        if (elapsed_time >= MAX_JOIN_TIME_SECONDS * 1000)
        {
            ESP_LOGE(TAG, "Failed to join Thread network within %d seconds", MAX_JOIN_TIME_SECONDS);
            return ESP_ERR_TIMEOUT;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
        vTaskDelay(pdMS_TO_TICKS(delay_interval));
        elapsed_time += delay_interval;
    }
    ESP_LOGI(TAG, "Successfully joined the Thread network");

    return ESP_OK;
}

void storeOperationalDataset()
{
    // otOperationalDataset dataset;
    // otDatasetGetActive(sInstance, &dataset);
    // esp_openthread_set_dataset_factory(&dataset);
}

void initialize_thread(bool rx, bool (*rx_cb)(void))
{

    initThread();
    joinToThread(rx, rx_cb);
}
