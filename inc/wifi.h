#ifndef _WIFI_H_
#define _WIFI_H_
/** Includes *************************************************************************************/
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
/** Defines **************************************************************************************/
/** Typedefs *************************************************************************************/
/** Variables ************************************************************************************/
/** Prototypes ***********************************************************************************/
/** Functions ************************************************************************************/

/**
 * @brief Initialise the Wi-Fi chip and connect to the network
 * @param ssid The SSID of the Wi-Fi network to connect to
 * @param password The password of the Wi-Fi network to connect to
 * @return 0 on success, -1 on failure
 * 
 */
int wifi_init(const char *ssid, const char *password);

/**
 * @brief The wifi task is used to handle the wifi connection.
 *
 * The wifi task is used to handle the wifi connection. Connecting and reconnecting
 * as needed. It should be called in a loop to ensure that the wifi connection is maintained.
 *
 * @return int 0 on success, -1 on failure
 *
 */
int wifi_task(void);

#endif /* _WIFI_H_ */