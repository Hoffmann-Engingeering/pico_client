/** Includes *************************************************************************************/
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "client.h"
/** Defines **************************************************************************************/
#define SSID "pico_test"
#define PASSWORD "password123"

/** Typedefs *************************************************************************************/
/** Variables ************************************************************************************/
/** Prototypes ***********************************************************************************/
static int _wifi_init();
static int _client_task();
/** Functions ************************************************************************************/

int main()
{
    stdio_init_all();

    /** Initialise and connect to the wifi network with 10sec timeout */
    _wifi_init(SSID, PASSWORD, 10000);

    while (true) {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}


static int _wifi_init(const char *ssid, const char *password, uint32_t timeout_ms) {
    // Initialise the Wi-Fi chip
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed\n");
        return -1;
    }

    // Enable wifi station
    cyw43_arch_enable_sta_mode();

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
