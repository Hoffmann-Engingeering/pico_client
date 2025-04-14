
#ifndef _CLIENT_H_
#define _CLIENT_H_
/** Includes *************************************************************************************/
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
/** Defines **************************************************************************************/
#define BUF_SIZE 1024

/** Typedefs *************************************************************************************/
typedef enum {
    CLIENT_DISCONNECTED = 0,
    CLIENT_CONNECTED = 1,
} client_state_t;

typedef struct {
    struct tcp_pcb *tcp_pcb;
    ip_addr_t remote_addr;
    uint8_t buffer[BUF_SIZE];
    uint16_t buffer_len;
    client_state_t state;
} client_t;

/** Variables ************************************************************************************/
/** Prototypes ***********************************************************************************/
/** Functions ************************************************************************************/
int client_init(client_t *client, const char *ip_address);
int client_task(client_t *client);


#endif /* _CLIENT_H_ */