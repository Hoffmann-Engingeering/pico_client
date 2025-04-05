/** Includes *************************************************************************************/
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "client.h"
#include "wifi.h"
/** Defines **************************************************************************************/
#define SSID "pico_test"
#define PASSWORD "password123"
/** Typedefs *************************************************************************************/
/** Variables ************************************************************************************/
/** Prototypes ***********************************************************************************/
static int _client_task();
/** Functions ************************************************************************************/

int main()
{
    stdio_init_all();

    /** Initialise and connect to the wifi network with 10sec timeout */
    wifi_init(SSID, PASSWORD, 10000);

    while (true) {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}



