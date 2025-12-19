#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/ip4_addr.h"
#include "dhcpserver.h"
#include "dnsserver.h"

#define AP_SSID     "PicoW-Estacionamento"
#define AP_PASSWORD "12345678"

#define AP_IP_1 192
#define AP_IP_2 168
#define AP_IP_3 4
#define AP_IP_4 1

static dhcp_server_t dhcp_server;
static dns_server_t  dns_server;

void wifi_ap_init(void) {

    

    cyw43_arch_enable_ap_mode(
        AP_SSID,
        AP_PASSWORD,
        CYW43_AUTH_WPA2_AES_PSK
    );

    ip4_addr_t ip, mask;
    IP4_ADDR(&ip,   AP_IP_1, AP_IP_2, AP_IP_3, AP_IP_4);
    IP4_ADDR(&mask, 255, 255, 255, 0);

    dhcp_server_init(&dhcp_server, &ip, &mask);
    dns_server_init(&dns_server, &ip);

    printf("\n=== WIFI AP ATIVO ===\n");
    printf("SSID: %s\n", AP_SSID);
    printf("IP:   %d.%d.%d.%d\n",
           AP_IP_1, AP_IP_2, AP_IP_3, AP_IP_4);
    printf("====================\n\n");
}
