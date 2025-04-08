/** Includes *************************************************************************************/
#include "client.h"
/** Defines **************************************************************************************/
#define SERVER_PORT 4242
#define CLIENT_POLL_TIME_S 10
#define CLIENT_TASK_TIMEOUT_MS 100

/** Typedefs *************************************************************************************/
/** Variables ************************************************************************************/
/** Prototypes ***********************************************************************************/
int _client_ip_string_to_ip_addr(const char *ip_str, ip_addr_t *ip_addr);
/** Functions ************************************************************************************/
static int _client_open(client_t *client);
static err_t _client_poll(void *arg, struct tcp_pcb *tpcb);
static err_t _client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
static err_t _client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static void _client_err(void *arg, err_t err);
static err_t _client_connected(void *arg, struct tcp_pcb *tpcb, err_t err);

int client_init(client_t *client, const char *ip_address)
{
    // Perform initialisation
    if (client == NULL)
    {
        return -1;
    }

    /** Initialise client with the server ip address */
    _client_ip_string_to_ip_addr(ip_address, &client->remote_addr);

    /** Create a new TCP PCB (Protocol Control Block) for the client */
    client->tcp_pcb = tcp_new_ip_type(IP_GET_TYPE(&client->remote_addr));
    if (client->tcp_pcb == NULL)
    {
        printf("Failed to create new TCP PCB\n");
        return -1;
    }

    /** Initialise the client state */
    client->state = CLIENT_DISCONNECTED;

    return 0;
}

/**
 * @brief Run the client task
 * This function is called in the main loop to run the client task. It checks if the client is connected and if not, it opens the client connection.
 * It also runs the lwIP stack to process any incoming data and send any outgoing data.
 * @param client Pointer to the client structure.
 * @return int 0 on success, -1 on failure.
 */
int client_task(client_t *client)
{
    if (client == NULL)
    {
        return -1;
    }
    /** Check if it is time to run the client task */
    static uint32_t timeLastRunMs = 0;
    uint32_t currentTimeMs = to_ms_since_boot(get_absolute_time());
    if (currentTimeMs - timeLastRunMs < CLIENT_TASK_TIMEOUT_MS)
    {
        return 0;
    }
    /** Update the last run time */
    timeLastRunMs = currentTimeMs;

    /** poll the cwy43 arch to process any incoming data */
    cyw43_arch_poll();

    /** Run the state machine */
    switch (client->state)
    {
    case CLIENT_DISCONNECTED:
        static int timeoutMs = 0;

        if (timeoutMs > 0)
        {
            timeoutMs -= CLIENT_TASK_TIMEOUT_MS;
            return 0;
        }
        else if (timeoutMs <= 0)
        {   
            _client_open(client) != ERR_OK;
            timeoutMs = 2000;
        }
    
        break;
    case CLIENT_CONNECTED:
        /** If the client is connected, do nothing */
        break;
    default:
        /** Huston we have a problem */
        break;
    }

    return 0;
}

static int _client_open(client_t *client)
{
    
    if (!client->tcp_pcb)
    {
        printf("Failed to setup the pcb");
        return -1;
    }

    /** Setup the pcb argument as client, and then set the other tcp/ip callbacks */
    tcp_arg(client->tcp_pcb, client);
    tcp_poll(client->tcp_pcb, _client_poll, CLIENT_POLL_TIME_S);
    tcp_sent(client->tcp_pcb, _client_sent);
    tcp_recv(client->tcp_pcb, _client_recv);
    tcp_err(client->tcp_pcb, _client_err);

    printf("Connecting to %s:%d\n", ipaddr_ntoa(&client->remote_addr), SERVER_PORT);

    // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure correct locking.
    // You can omit them if you are in a callback from lwIP. Note that when using pico_cyw_arch_poll
    // these calls are a no-op and can be omitted, but it is a good practice to use them in
    // case you switch the cyw43_arch type later.
    cyw43_arch_lwip_begin();
    /** The function will trigger the _client_connected callback once the client has connected */
    err_t err = tcp_connect(client->tcp_pcb, &client->remote_addr, SERVER_PORT, _client_connected);
    cyw43_arch_lwip_end();

    return err;
}

static err_t _client_poll(void *arg, struct tcp_pcb *tpcb) { return 0; }
static err_t _client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) { return 0; }
static err_t _client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) { return 0; }
static void _client_err(void *arg, err_t err) { /** Nothing atm*/ }
static err_t _client_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    client_t *client = (client_t *)arg;
    client->state = CLIENT_CONNECTED;
    printf("Client connected\n");
}

/**
 * @brief Converts an IP address string to an ip_addr_t structure.
 *
 * @param ip_str The IP address string (e.g., "192.168.1.1").
 * @param ip_addr Pointer to the ip_addr_t structure to store the result.
 * @return int Returns 0 on success, -1 on failure.
 */
int _client_ip_string_to_ip_addr(const char *ip_str, ip_addr_t *ip_addr)
{
    if (ip_str == NULL || ip_addr == NULL)
    {
        return -1; // Invalid arguments
    }

    if (!ipaddr_aton(ip_str, ip_addr))
    {
        return -1; // Conversion failed
    }

    return 0; // Success
}
