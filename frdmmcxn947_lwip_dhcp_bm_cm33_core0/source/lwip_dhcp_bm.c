/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2020,2023-2024 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*******************************************************************************
 * Includes
 ******************************************************************************/

#include "lwip/opt.h"

#if LWIP_IPV4 && LWIP_TCP

#include "lwip/timeouts.h"
#include "lwip/ip_addr.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "netif/ethernet.h"
#include "ethernetif.h"
#include "fsl_phy.h"
#include "board.h"
#include "app.h"
#include "tcp_client.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Must be after include of app.h */
#ifndef configMAC_ADDR
#include "fsl_silicon_id.h"
#endif

/* IP estatica del MCX N947 (misma subred que la K64: 10.10.10.0/24). */
#ifndef configIP_ADDR0
#define configIP_ADDR0 10
#endif
#ifndef configIP_ADDR1
#define configIP_ADDR1 10
#endif
#ifndef configIP_ADDR2
#define configIP_ADDR2 10
#endif
#ifndef configIP_ADDR3
#define configIP_ADDR3 101
#endif

#ifndef configNET_MASK0
#define configNET_MASK0 255
#endif
#ifndef configNET_MASK1
#define configNET_MASK1 255
#endif
#ifndef configNET_MASK2
#define configNET_MASK2 255
#endif
#ifndef configNET_MASK3
#define configNET_MASK3 0
#endif

#ifndef configGW_ADDR0
#define configGW_ADDR0 10
#endif
#ifndef configGW_ADDR1
#define configGW_ADDR1 10
#endif
#ifndef configGW_ADDR2
#define configGW_ADDR2 10
#endif
#ifndef configGW_ADDR3
#define configGW_ADDR3 100
#endif

/* Servidor TCP (FRDM-K64F). */
#ifndef configSERVER_ADDR0
#define configSERVER_ADDR0 10
#endif
#ifndef configSERVER_ADDR1
#define configSERVER_ADDR1 10
#endif
#ifndef configSERVER_ADDR2
#define configSERVER_ADDR2 10
#endif
#ifndef configSERVER_ADDR3
#define configSERVER_ADDR3 102
#endif
#ifndef configSERVER_PORT
#define configSERVER_PORT 7
#endif

#ifndef EXAMPLE_NETIF_INIT_FN
/*! @brief Network interface initialization function. */
#define EXAMPLE_NETIF_INIT_FN ethernetif0_init
#endif /* EXAMPLE_NETIF_INIT_FN */

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

static phy_handle_t phyHandle;
static netif_ext_callback_t linkStatusCallbackInfo;

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Link status callback - prints link status events.
 */
static void linkStatusCallback(struct netif *netif_, netif_nsc_reason_t reason, const netif_ext_callback_args_t *args)
{
    if (reason != LWIP_NSC_LINK_CHANGED)
        return;

    PRINTF("[LINK STATE] netif=%d, state=%s", netif_->num, args->link_changed.state ? "up" : "down");

    if (args->link_changed.state)
    {
        char *speedStr;
        switch (ethernetif_get_link_speed(netif_))
        {
            case kPHY_Speed10M:
                speedStr = "10M";
                break;
            case kPHY_Speed100M:
                speedStr = "100M";
                break;
            case kPHY_Speed1000M:
                speedStr = "1000M";
                break;
            default:
                speedStr = "N/A";
                break;
        }

        char *duplexStr;
        switch (ethernetif_get_link_duplex(netif_))
        {
            case kPHY_HalfDuplex:
                duplexStr = "half";
                break;
            case kPHY_FullDuplex:
                duplexStr = "full";
                break;
            default:
                duplexStr = "N/A";
                break;
        }

        PRINTF(", speed=%s_%s", speedStr, duplexStr);
    }

    PRINTF("\r\n");
}

/*!
 * @brief Interrupt service for SysTick timer.
 */
void SysTick_Handler(void)
{
    time_isr();
}

/*!
 * @brief Main function.
 */
int main(void)
{
    struct netif netif;
    ip4_addr_t netif_ipaddr, netif_netmask, netif_gw, server_ip;
    ethernetif_config_t enet_config = {.phyHandle   = &phyHandle,
                                       .phyAddr     = EXAMPLE_PHY_ADDRESS,
                                       .phyOps      = EXAMPLE_PHY_OPS,
                                       .phyResource = EXAMPLE_PHY_RESOURCE,
#ifdef configMAC_ADDR
                                       .macAddress = configMAC_ADDR
#endif
    };

    BOARD_InitHardware();

    time_init();

    /* Set MAC address. */
#ifndef configMAC_ADDR
    (void)SILICONID_ConvertToMacAddr(&enet_config.macAddress);
#endif

    /* Get clock after hardware init. */
    enet_config.srcClockHz = EXAMPLE_CLOCK_FREQ;

    lwip_init();

    netif_add_ext_callback(&linkStatusCallbackInfo, linkStatusCallback);

    IP4_ADDR(&netif_ipaddr, configIP_ADDR0, configIP_ADDR1, configIP_ADDR2, configIP_ADDR3);
    IP4_ADDR(&netif_netmask, configNET_MASK0, configNET_MASK1, configNET_MASK2, configNET_MASK3);
    IP4_ADDR(&netif_gw, configGW_ADDR0, configGW_ADDR1, configGW_ADDR2, configGW_ADDR3);
    IP4_ADDR(&server_ip, configSERVER_ADDR0, configSERVER_ADDR1, configSERVER_ADDR2, configSERVER_ADDR3);

    netif_add(&netif, &netif_ipaddr, &netif_netmask, &netif_gw, &enet_config, EXAMPLE_NETIF_INIT_FN, ethernet_input);
    netif_set_default(&netif);
    netif_set_up(&netif);

    while (ethernetif_wait_linkup(&netif, 5000) != ERR_OK)
    {
        PRINTF("PHY Auto-negotiation failed. Please check the cable connection and link partner setting.\r\n");
    }

    PRINTF("\r\n************************************************\r\n");
    PRINTF(" FRDM-MCXN947 TCP Client\r\n");
    PRINTF("************************************************\r\n");
    PRINTF(" IPv4 Address     : %s\r\n", ip4addr_ntoa(&netif_ipaddr));
    PRINTF(" IPv4 Subnet mask : %s\r\n", ip4addr_ntoa(&netif_netmask));
    PRINTF(" IPv4 Gateway     : %s\r\n", ip4addr_ntoa(&netif_gw));
    PRINTF(" MAC Address      : %02X:%02X:%02X:%02X:%02X:%02X\r\n", netif.hwaddr[0], netif.hwaddr[1], netif.hwaddr[2],
           netif.hwaddr[3], netif.hwaddr[4], netif.hwaddr[5]);
    PRINTF(" Servidor TCP     : %s:%u\r\n", ip4addr_ntoa(&server_ip), (unsigned)configSERVER_PORT);
    PRINTF("************************************************\r\n\r\n");

    tcp_client_init(&server_ip, configSERVER_PORT);

    while (1)
    {
        /* Poll the driver, get any outstanding frames */
        ethernetif_input(&netif);

        /* Handle all system timeouts for all core protocols */
        sys_check_timeouts();

        /* Lee la consola sin bloquear y envia la linea al servidor al presionar Enter */
        tcp_client_poll_console();
    }
}
#endif
