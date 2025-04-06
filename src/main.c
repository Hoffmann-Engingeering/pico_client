/** Includes *************************************************************************************/
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "client.h"
#include "wifi.h"

/** Defines **************************************************************************************/
/** Typedefs *************************************************************************************/
/** Variables ************************************************************************************/
/** Prototypes ***********************************************************************************/
/** Functions ************************************************************************************/

int main()
{
    /** Initialise the stdio library */
    stdio_init_all();

    sleep_ms(5000);

    /** Initialise and connect to the wifi network with 10sec timeout */
    if(wifi_init(SSID, PASSWORD) != 0)
    {
        printf("Failed to initialise Wi-Fi\n");
        return -1;
    }

    printf("Wi-Fi initialised\n");

    while (true)
    {
        /** Run the wifi task to check if we are connected */
        wifi_task();

        /** Sleep for 10ms */
        sleep_ms(10);
    }
}
