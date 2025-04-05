/** Includes *************************************************************************************/
#include "wifi.h"
/** Defines **************************************************************************************/
#define WIFI_TASK_INTERVAL_MS 100
/** Typedefs *************************************************************************************/
typedef enum {
    WIFI_TASK_DISCONNECTED = 0,
    WIFI_TASK_CONNECTING,
    WIFI_TASK_CONNECTED,
} WifiTaskState_t;

typedef struct {
    WifiTaskState_t state;
    uint32_t last_run_ms;
    char ssid[50];
    char pw[50];
} WifiTask_t;

/** Variables ************************************************************************************/
WifiTask_t WifiTask = {
    .state = WIFI_TASK_DISCONNECTED,
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
    
    // Copy the ssid and password to the task struct
    memcpy(WifiTask.ssid, ssid, strlen(ssid) + 1);
    WifiTask.ssid[strlen(ssid)] = '\0';

    memcpy(WifiTask.pw, password, strlen(password) + 1);
    WifiTask.pw[strlen(password)] = '\0';
    
    // Initialise the Wi-Fi chip
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed\n");
        return -1;
    }

    // Enable wifi station
    cyw43_arch_enable_sta_mode();

    // Connect to the Wi-Fi network - Add it to the task
    // printf("Connecting to Wi-Fi...\n");
    // if (cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, timeout_ms)) {
    //     printf("failed to connect.\n");
    //     return -1;
    // } else {
    //     printf("Connected.\n");
    //     // Read the ip address in a human readable way
    //     uint8_t *ip_address = (uint8_t*)&(cyw43_state.netif[0].ip_addr.addr);
    //     printf("IP address %d.%d.%d.%d\n", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
    // }

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

    /** Run poll to check if there is new info on the lower driver */
    cyw43_arch_poll();

    /** Get the current wifi status */
    int currentWifiStatus = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);

    switch(WifiTask.state) {
        case WIFI_TASK_DISCONNECTED:

            char *ssid = WifiTask.ssid;
            char *pw = WifiTask.pw;

            if(currentWifiStatus == CYW43_LINK_NONET) {
                /** Try to connect */
                cyw43_arch_wifi_connect_bssid_async(ssid, NULL, pw, CYW43_AUTH_WPA2_AES_PSK);

                /** TODO: CH - Handle error */
            }

            /** Set the state to connecting */
            WifiTask.state = WIFI_TASK_CONNECTING;
            break;

        case WIFI_TASK_CONNECTING:
            /** Check if connected */
            if (currentWifiStatus == CYW43_LINK_UP) {
                // Connected
                printf("Connected to Wi-Fi\n");
                WifiTask.state = WIFI_TASK_CONNECTED;
            } else if (currentWifiStatus == CYW43_LINK_FAIL) {
                // Failed to connect
                printf("Failed to connect to Wi-Fi\n");
                WifiTask.state = WIFI_TASK_IDLE;
            } else if (currentWifiStatus == CYW43_LINK_BADAUTH) {
                // Bad auth
                printf("Bad auth\n");
                WifiTask.state = WIFI_TASK_IDLE;
            }
            break;

        case WIFI_TASK_CONNECTED:
            /** Check if we are still connected */
            if (currentWifiStatus != CYW43_LINK_UP) {
                // Disconnected
                printf("Disconnected from Wi-Fi\n");
                WifiTask.state = WIFI_TASK_DISCONNECTED;
            }
            break;

        default:
            /** Huston we have a problem */
            break;
    }

    return 0;
}


// if (new_status == CYW43_LINK_NONET) {
//     new_status = CYW43_LINK_JOIN;
//     err = cyw43_arch_wifi_connect_bssid_async(ssid, bssid, pw, auth);
//     if (err) return err;
// }
// if (new_status != status) {
//     status = new_status;
//     CYW43_ARCH_DEBUG("connect status: %s\n", cyw43_tcpip_link_status_name(status));
// }
// if (time_reached(until)) {
//     return PICO_ERROR_TIMEOUT;
// }
// // Do polling
// cyw43_arch_poll();