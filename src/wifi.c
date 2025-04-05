/** Includes *************************************************************************************/
#include "wifi.h"
/** Defines **************************************************************************************/
#define WIFI_TASK_INTERVAL_MS 100
/** Typedefs *************************************************************************************/
typedef enum {
    WIFI_TASK_IDLE = 0,
    WIFI_TASK_CONNECTING,
    WIFI_TASK_CONNECTED,
    WIFI_TASK_DISCONNECTED,
} WifiTaskState_t;

typedef struct {
    WifiTaskState_t state;
    uint32_t last_run_ms;
} WifiTask_t;

/** Variables ************************************************************************************/
WifiTask_t WifiTask = {
    .state = WIFI_TASK_IDLE,
    .last_run_ms = 0,
};
/** Prototypes ***********************************************************************************/
/** Functions ************************************************************************************/

/**
 * @brief Initialise the Wi-Fi chip and connect to the network
 * @param ssid The SSID of the Wi-Fi network to connect to
 * @param password The password of the Wi-Fi network to connect to
 * @param timeout_ms The timeout in milliseconds to wait for the connection
 * @return 0 on success, -1 on failure
 */
int wifi_init(const char *ssid, const char *password, uint32_t timeout_ms) {
    // Initialise the Wi-Fi chip
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed\n");
        return -1;
    }

    // Enable wifi station
    cyw43_arch_enable_sta_mode();

    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, timeout_ms)) {
        printf("failed to connect.\n");
        return -1;
    } else {
        printf("Connected.\n");
        // Read the ip address in a human readable way
        uint8_t *ip_address = (uint8_t*)&(cyw43_state.netif[0].ip_addr.addr);
        printf("IP address %d.%d.%d.%d\n", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
    }

    return 0;
}

/**
 * @brief The wifi task is used to handle the wifi connection. Connecting and reconnecting
 * 
 * The wifi task is used to handle the wifi connection. Connecting and reconnecting
 * as needed. It is called from the main loop and should be run in a separate thread.
 * 
 * @return int 0 on success, -1 on failure
 * 
 */
int wifi_task() {

    /** Check if the task should run */
    uint32_t timeLastRunMs = 0;
    uint32_t currentTimeMs = to_ms_since_boot(get_absolute_time());

    if (currentTimeMs - timeLastRunMs < WIFI_TASK_INTERVAL_MS) {
        return 0;
    }

    /** Time to run. Update last run */
    timeLastRunMs = currentTimeMs;

    switch(WifiTask.state) {
        case WIFI_TASK_IDLE:
            // Do nothing
            break;
        case WIFI_TASK_CONNECTING:
            // Check if connected
            break;
        case WIFI_TASK_CONNECTED:
            // Check if disconnected
            break;
        case WIFI_TASK_DISCONNECTED:
            // Try to reconnect
            break;
    }


    
    return 0;
}