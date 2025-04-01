/** Includes *************************************************************************************/
#include "client.h"
/** Defines **************************************************************************************/
#define SERVER_PORT 4242
#define CLINET_POLL_TIME_S 10
/** Typedefs *************************************************************************************/
/** Variables ************************************************************************************/
/** Prototypes ***********************************************************************************/
/** Functions ************************************************************************************/
static int _client_open(client_t *client);
static err_t _client_poll(void *arg, struct tcp_pcb *tpcb);
static err_t _client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
static err_t _client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static void _client_err(void *arg, err_t err);
static err_t _client_connected(void *arg, struct tcp_pcb *tpcb, err_t err);

int client_init(client_t *client, const char *ip_address) {
    // Perform initialisation
    if(client == NULL) {
        return -1;
    }
    /** Initialise client with the server ip address */
    return ip4addr_aton(ip_address, &client->remote_addr);
}


static int _client_open(client_t *client) {
    client->tcp_pcb = tcp_new_ip_type(IP_GET_TYPE(&client->remote_addr));

    if(!client->tcp_pcb) {
        printf("Failed to setup the pcb");
        return -1;
    }

    /** Setup the pcb argument as client, and then set the other tcp/ip callbacks */
    tcp_arg(client->tcp_pcb, client);
    tcp_poll(client->tcp_pcb, _client_poll, CLINET_POLL_TIME_S);
    tcp_sent(client->tcp_pcb, _client_sent);
    tcp_recv(client->tcp_pcb, _client_recv);
    tcp_err(client->tcp_pcb, _client_err);

    // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure correct locking.
    // You can omit them if you are in a callback from lwIP. Note that when using pico_cyw_arch_poll
    // these calls are a no-op and can be omitted, but it is a good practice to use them in
    // case you switch the cyw43_arch type later.
    cyw43_arch_lwip_begin();
    /** The function will trigger the _client_connected callback once the client has connected */
    err_t err = tcp_connect(client->tcp_pcb, &client->remote_addr, SERVER_PORT, _client_connected);
    cyw43_arch_lwip_end();

    return 0;
}


static err_t _client_poll(void *arg, struct tcp_pcb *tpcb) { return 0;}
static err_t _client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) { return 0;}
static err_t _client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) { return 0;}
static void _client_err(void *arg, err_t err) { /** Nothing atm*/ }
static err_t _client_connected(void *arg, struct tcp_pcb *tpcb, err_t err) { return 0;}
