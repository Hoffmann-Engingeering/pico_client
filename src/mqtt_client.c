/** Includes *************************************************************************************/
#include "mqtt_client.h"
/** Defines **************************************************************************************/
/** Typedefs *************************************************************************************/
/** Variables ************************************************************************************/
/** Prototypes ***********************************************************************************/
/** Functions ************************************************************************************/

/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

//
// Created by elliot on 25/05/24.
//
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/unique_id.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/adc.h"
#include "lwip/apps/mqtt.h"
#include "lwip/apps/mqtt_priv.h" // needed to set hostname
#include "lwip/dns.h"
#include "lwip/altcp_tls.h"

#ifndef MQTT_TOPIC_LEN
#define MQTT_TOPIC_LEN 100
#endif

typedef enum
{
    MQTT_CLIENT_DISCONNECTED,
    MQTT_CLIENT_CONNECTING,
    MQTT_CLIENT_CONNECTED,
    MQTT_CLIENT_SUBSCRIBED,
} mqtt_client_state_t;

typedef struct
{
    mqtt_client_t *mqttClientInst;
    struct mqtt_connect_client_info_t mqttClientInfo;
    char data[MQTT_OUTPUT_RINGBUF_SIZE];
    char topic[MQTT_TOPIC_LEN];
    uint32_t len;
    ip_addr_t mqtt_server_address;
    bool connect_done;
    int subscribe_count;
    bool stop_client;
    mqtt_client_state_t taskState;
} MqttClientData_t;

/** Ensure that the client data is initialised to 0 */
static MqttClientData_t MqttClient = {0};

#ifndef DEBUG_printf
#ifndef NDEBUG
#define DEBUG_printf printf
#else
#define DEBUG_printf(...)
#endif
#endif

#ifndef INFO_printf
#define INFO_printf printf
#endif

#ifndef ERROR_printf
#define ERROR_printf printf
#endif

// keep alive in seconds
#define MQTT_KEEP_ALIVE_S 60

// qos passed to mqtt_subscribe
// At most once (QoS 0)
// At least once (QoS 1)
// Exactly once (QoS 2)
#define MQTT_SUBSCRIBE_QOS 1
#define MQTT_PUBLISH_QOS 1
#define MQTT_PUBLISH_RETAIN 0

#define MQTT_CLIENT_TASK_TIMEOUT_ms 100

static void pub_request_cb(__unused void *arg, err_t err)
{
    if (err != 0)
    {
        ERROR_printf("pub_request_cb failed %d", err);
    }
}

static void sub_request_cb(void *arg, err_t err)
{
    MqttClientData_t *state = (MqttClientData_t *)arg;
    if (err != 0)
    {
        panic("subscribe request failed %d", err);
    }

    INFO_printf("Subscribed to topic\n");
    state->subscribe_count++;
}

static void unsub_request_cb(void *arg, err_t err)
{
    MqttClientData_t *state = (MqttClientData_t *)arg;
    if (err != 0)
    {
        panic("unsubscribe request failed %d", err);
    }
    state->subscribe_count--;
    assert(state->subscribe_count >= 0);

    // Stop if requested
    if (state->subscribe_count <= 0 && state->stop_client)
    {
        mqtt_disconnect(state->mqttClientInst);
    }
}

static void sub_unsub_topics(MqttClientData_t *state, bool sub)
{
    // Subscribe to topics
    INFO_printf("Subscribing to topics\n");
    mqtt_request_cb_t cb = sub ? sub_request_cb : unsub_request_cb;
    
    /** TODO: Need to connect one at a time and then verify connected. */
    mqtt_sub_unsub(state->mqttClientInst, "/led", MQTT_SUBSCRIBE_QOS, cb, state, sub);
}

static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
    MqttClientData_t *state = (MqttClientData_t *)arg;
    /** Need to handle NULL */
    if (data == NULL || len == 0)
    {
        return;
    }

    const char *basic_topic = state->topic;
    strncpy(state->data, (const char *)data, len);
    state->len = len;
    state->data[len] = '\0';

    DEBUG_printf("Topic: %s, Message: %s\n", state->topic, state->data);
}

static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
{
    MqttClientData_t *state = (MqttClientData_t *)arg;
    strncpy(state->topic, topic, sizeof(state->topic));
}

static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
    MqttClientData_t *state = (MqttClientData_t *)arg;

    if (status == MQTT_CONNECT_ACCEPTED)
    {
        INFO_printf("Connected to mqtt server\n");
        state->connect_done = true;
        sub_unsub_topics(state, true); // subscribe;
    }
    else if (status == MQTT_CONNECT_DISCONNECTED)
    {
        if (!state->connect_done)
        {
            panic("Failed to connect to mqtt server");
        }
    }
    else
    {
        panic("Unexpected status");
    }
}

static void start_client(MqttClientData_t *state)
{
    INFO_printf("Starting mqtt client\n");
    const int port = MQTT_PORT;
    INFO_printf("Warning: Not using TLS\n");
    /** TODO: CH - Before cleaning out the memory ensure that the mqttClientInst is free */
    if (state->mqttClientInst != NULL)
    {
        mqtt_client_free(state->mqttClientInst);
        state->mqttClientInst = NULL;
    }

    /** Ensure that the client structure is cleaned out */
    memset(state, 0, sizeof(MqttClientData_t));

    state->mqttClientInfo.client_id = CLIENT_ID;          /** See CMakeLists.txt */
    state->mqttClientInfo.keep_alive = MQTT_KEEP_ALIVE_S; // Keep alive in sec
    state->mqttClientInfo.will_topic = NULL;

    state->mqttClientInst = mqtt_client_new();
    if (!state->mqttClientInst)
    {
        panic("MQTT client instance creation error");
    }
    INFO_printf("IP address of this device %s\n", ipaddr_ntoa(&(netif_list->ip_addr)));
    INFO_printf("Connecting to mqtt server at %s\n", ipaddr_ntoa(&state->mqtt_server_address));

    cyw43_arch_lwip_begin();
    if (mqtt_client_connect(state->mqttClientInst, &state->mqtt_server_address, port, mqtt_connection_cb, state, &state->mqttClientInfo) != ERR_OK)
    {
        panic("MQTT broker connection error");
    }

    mqtt_set_inpub_callback(state->mqttClientInst, mqtt_incoming_publish_cb, mqtt_incoming_data_cb, state);
    cyw43_arch_lwip_end();
}

int mqtt_client_task(void)
{

    /** Implement state machine with timeout and rollover protection */
    static uint32_t timeLastRunMs = 0;
    uint32_t currentTimeMs = to_ms_since_boot(get_absolute_time());

    // clang-format off
    uint32_t timePassedMs = currentTimeMs < timeLastRunMs
                          ? UINT32_MAX - timeLastRunMs + currentTimeMs
                          : currentTimeMs - timeLastRunMs;
    // clang-format on

    if (timePassedMs < MQTT_CLIENT_TASK_TIMEOUT_ms)
    {
        return 0;
    }

    /** Update the last run time and catch the roll-over */
    timeLastRunMs = currentTimeMs;

    switch (MqttClient.taskState)
    {
    case MQTT_CLIENT_DISCONNECTED:
        start_client(&MqttClient);
        MqttClient.taskState = MQTT_CLIENT_CONNECTING;
        break;
    case MQTT_CLIENT_CONNECTING:
        if (MqttClient.connect_done)
        {
            /** We are connected yay */
            MqttClient.taskState = MQTT_CLIENT_CONNECTED;
            INFO_printf("MQTT client connected\n");
        }
        break;
        /* code */
        break;

    default:
        break;
    }

    while (!state.connect_done || mqtt_client_is_connected(state.mqttClientInst))
    {
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(10000));
    }

    INFO_printf("mqtt client exiting\n");
    return 0;
}