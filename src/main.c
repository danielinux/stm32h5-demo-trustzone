/* app_stm32h5.c
 *
 * Test bare-metal application.
 *
 * Copyright (C) 2024 wolfSSL Inc.
 *
 * This file is part of wolfBoot.
 *
 * wolfBoot is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfBoot is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
//#include "hal.h"
#include "wolfboot/wolfboot.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "pico_stack.h"
#include "pico_socket.h"

#include "stm32h5xx_hal_eth.h"
#include "stm32h5xx_hal_gpio.h"
#include "stm32h5xx_hal_dma.h"
#include "stm32h5xx_hal_rcc.h"

#ifdef SECURE_PKCS11
#include "wcs/user_settings.h"
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/wc_pkcs11.h>
#include <wolfssl/wolfcrypt/random.h>
extern const char pkcs11_library_name[];
extern const CK_FUNCTION_LIST wolfpkcs11nsFunctionList;
#endif

#define LED_BOOT_PIN (4) /* PG4 - Nucleo - Red Led */
#define LED_USR_PIN  (0) /* PB0 - Nucleo - Green Led */
#define LED_EXTRA_PIN  (4) /* PF4 - Nucleo - Orange Led */



#define GPIOG_MODER (*(volatile uint32_t *)(GPIOG_BASE + 0x00))
#define GPIOG_PUPDR (*(volatile uint32_t *)(GPIOG_BASE + 0x0C))
#define GPIOG_BSRR  (*(volatile uint32_t *)(GPIOG_BASE + 0x18))

#define GPIOB_MODER (*(volatile uint32_t *)(GPIOB_BASE + 0x00))
#define GPIOB_PUPDR (*(volatile uint32_t *)(GPIOB_BASE + 0x0C))
#define GPIOB_BSRR  (*(volatile uint32_t *)(GPIOB_BASE + 0x18))

#define GPIOF_MODER (*(volatile uint32_t *)(GPIOF_BASE + 0x00))
#define GPIOF_PUPDR (*(volatile uint32_t *)(GPIOF_BASE + 0x0C))
#define GPIOF_BSRR  (*(volatile uint32_t *)(GPIOF_BASE + 0x18))

#define RCC_AHB2ENR1_CLOCK_ER (*(volatile uint32_t *)(RCC_BASE + 0x8C ))
#define GPIOG_AHB2ENR1_CLOCK_ER (1 << 6)
#define GPIOF_AHB2ENR1_CLOCK_ER (1 << 5)
#define GPIOB_AHB2ENR1_CLOCK_ER (1 << 1)

static struct pico_stack *S;
ETH_TxPacketConfig TxConfig;
ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT]; /* Ethernet Tx DMA Descriptors */

ETH_HandleTypeDef heth;

static void boot_led_on(void)
{
    uint32_t reg;
    uint32_t pin = LED_BOOT_PIN;

    RCC_AHB2ENR1_CLOCK_ER|= GPIOG_AHB2ENR1_CLOCK_ER;
    /* Delay after an RCC peripheral clock enabling */
    reg = RCC_AHB2ENR1_CLOCK_ER;

    reg = GPIOG_MODER & ~(0x03 << (pin * 2));
    GPIOG_MODER = reg | (1 << (pin * 2));
    GPIOG_PUPDR &= ~(0x03 << (pin * 2));
    GPIOG_BSRR |= (1 << (pin));
}

static void boot_led_off(void)
{
    GPIOG_BSRR |= (1 << (LED_BOOT_PIN + 16));
}

void usr_led_on(void)
{
    uint32_t reg;
    uint32_t pin = LED_USR_PIN;

    RCC_AHB2ENR1_CLOCK_ER|= GPIOB_AHB2ENR1_CLOCK_ER;
    /* Delay after an RCC peripheral clock enabling */
    reg = RCC_AHB2ENR1_CLOCK_ER;

    reg = GPIOB_MODER & ~(0x03 << (pin * 2));
    GPIOB_MODER = reg | (1 << (pin * 2));
    GPIOB_PUPDR &= ~(0x03 << (pin * 2));
    GPIOB_BSRR |= (1 << (pin));
}

void usr_led_off(void)
{
    GPIOB_BSRR |= (1 << (LED_USR_PIN + 16));
}

void extra_led_on(void)
{
    uint32_t reg;
    uint32_t pin = LED_EXTRA_PIN;

    RCC_AHB2ENR1_CLOCK_ER|= GPIOF_AHB2ENR1_CLOCK_ER;
    /* Delay after an RCC peripheral clock enabling */
    reg = RCC_AHB2ENR1_CLOCK_ER;

    reg = GPIOF_MODER & ~(0x03 << (pin * 2));
    GPIOF_MODER = reg | (1 << (pin * 2));
    GPIOF_PUPDR &= ~(0x03 << (pin * 2));
    GPIOF_BSRR |= (1 << (pin));
}

void extra_led_off(void)
{
    GPIOF_BSRR |= (1 << (LED_EXTRA_PIN + 16));
}

static char CaBuf[2048];
static uint8_t my_pubkey[200];

extern int ecdsa_sign_verify(int devId);

void MainTask(void *arg)
{
    int ret;
    uint32_t rand;
    uint32_t i;
    uint32_t klen = 200;
    int otherkey_slot;
    unsigned int devId = 0;

#ifdef SECURE_PKCS11
    WC_RNG rng;
    Pkcs11Token token;
    Pkcs11Dev PKCS11_d;
    unsigned long session;
    char TokenPin[] = "0123456789ABCDEF";
    char UserPin[] = "ABCDEF0123456789";
    char SoPinName[] = "SO-PIN";
#endif
    (void)arg;

    /* Turn on boot LED */
    boot_led_on();

#ifdef SECURE_PKCS11
    wolfCrypt_Init();

    PKCS11_d.heap = NULL,
    PKCS11_d.func = (CK_FUNCTION_LIST *)&wolfpkcs11nsFunctionList;

    ret = wc_Pkcs11Token_Init(&token, &PKCS11_d, 1, "EccKey",
            (const byte*)TokenPin, strlen(TokenPin));

    if (ret == 0) {
        ret = wolfpkcs11nsFunctionList.C_OpenSession(1,
                CKF_SERIAL_SESSION | CKF_RW_SESSION,
                NULL, NULL, &session);
    }
    if (ret == 0) {
        ret = wolfpkcs11nsFunctionList.C_InitToken(1,
                (byte *)TokenPin, strlen(TokenPin), (byte *)SoPinName);
    }

    if (ret == 0) {
        extra_led_on();
        ret = wolfpkcs11nsFunctionList.C_Login(session, CKU_SO,
                (byte *)TokenPin,
                strlen(TokenPin));
    }
    if (ret == 0) {
        ret = wolfpkcs11nsFunctionList.C_InitPIN(session,
                (byte *)TokenPin,
                strlen(TokenPin));
    }
    if (ret == 0) {
        ret = wolfpkcs11nsFunctionList.C_Logout(session);
    }
    if (ret != 0) {
        while(1)
            ;
    }
    if (ret == 0) {
        ret = wc_CryptoDev_RegisterDevice(devId, wc_Pkcs11_CryptoDevCb,
                &token);
        if (ret != 0) {
            while(1)
                ;
        }
        if (ret == 0) {
#ifdef HAVE_ECC
            ret = ecdsa_sign_verify(devId);
            if (ret != 0)
                ret = 1;
            else
                usr_led_on();
#endif
        }
        wc_Pkcs11Token_Final(&token);
    }

#else
    /* Check if version > 1 and turn on user led */
    if (wolfBoot_current_firmware_version() > 1) {
        usr_led_on();
    }
#endif /* SECURE_PKCS11 */
    while(1)
        ;

    /* Never reached */
}

extern unsigned int _stored_data;
extern unsigned int _start_data;
extern unsigned int _end_data;
extern unsigned int _start_bss;
extern unsigned int _end_bss;
extern unsigned int _end_stack;
extern unsigned int _start_heap;

static volatile struct pico_socket *cli = NULL;
static SemaphoreHandle_t picotcp_started;
static SemaphoreHandle_t picotcp_rx_data;

static void reboot(void)
{
#   define SCB_AIRCR         (*((volatile uint32_t *)(0xE000ED0C)))
#   define AIRCR_VECTKEY     (0x05FA0000)
#   define SYSRESET          (1 << 2)
    SCB_AIRCR = AIRCR_VECTKEY | SYSRESET;
}

/* TCP/IP: Ethernet initialization */
static uint8_t MACAddr[6] = {0x00, 0x80, 0xE1, 0x00, 0x01, 0x02};

static int eth_mac_init(void)
{
    heth.Instance = ETH;
    heth.Init.MACAddr = &MACAddr[0];
    heth.Init.MediaInterface = HAL_ETH_RMII_MODE;
    heth.Init.TxDesc = DMATxDscrTab;
    heth.Init.RxDesc = DMARxDscrTab;
    heth.Init.RxBuffLen = 1524;
    if (HAL_ETH_Init(&heth) != HAL_OK)
        return -1;

    memset(&TxConfig, 0 , sizeof(ETH_TxPacketConfig));
    TxConfig.Attributes = ETH_TX_PACKETS_FEATURES_CSUM | ETH_TX_PACKETS_FEATURES_CRCPAD;
    TxConfig.ChecksumCtrl = ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC;
    TxConfig.CRCPadCtrl = ETH_CRC_PAD_INSERT;
    return 0;
}

static void eth_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    __HAL_RCC_ETH_CLK_ENABLE();
    __HAL_RCC_ETHTX_CLK_ENABLE();
    __HAL_RCC_ETHRX_CLK_ENABLE();

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**ETH GPIO Configuration
      PC1     ------> ETH_MDC
      PA1     ------> ETH_REF_CLK
      PA2     ------> ETH_MDIO
      PA5     ------> ETH_TX_EN
      PA7     ------> ETH_CRS_DV
      PC4     ------> ETH_RXD0
      PC5     ------> ETH_RXD1
      PB12     ------> ETH_TXD0
      PB15     ------> ETH_TXD1
      */
    GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_5|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

struct ethdev {
    struct pico_device dev;
    void *base; 
};

static int ethernet_poll(struct pico_device *dev, int loop_score)
{
    struct pico_device_enet *enet = (struct pico_device_enet *)dev;
    uint32_t size;
    while(loop_score > 0) {
        //pico_stack_recv(dev, frame, size);
        loop_score--;
    }
    return loop_score;
}

static int ethernet_send(struct pico_device *dev, void *buf, int len)
{
    struct pico_device_enet *enet = (struct pico_device_enet *)dev;
    return 0;
}

static void ethernet_destroy(struct pico_device *dev)
{

}

struct pico_device *pico_eth_create(const char *name)
{
    struct ethdev *enet = PICO_ZALLOC(sizeof(struct ethdev));
    uint32_t duplex, speed;

    duplex = ETH_FULLDUPLEX_MODE;
    speed = ETH_SPEED_100M;

    ETH_MACConfigTypeDef MACConf = {0};
    
    if( 0 != pico_device_init(S, (struct pico_device *)enet, name, MACAddr)) {
        dbg("Eth init failed.\n");
        ethernet_destroy((struct pico_device *)enet);
        return NULL;
    }
    enet->base = &heth;
    enet->dev.send = ethernet_send;
    enet->dev.poll = ethernet_poll;
    enet->dev.destroy = ethernet_destroy;

    eth_gpio_init();
    if (eth_mac_init() < 0) {
        return NULL;
    }
    HAL_ETH_GetMACConfig(&heth, &MACConf);
    MACConf.DuplexMode = duplex;
    MACConf.Speed = speed;
    HAL_ETH_SetMACConfig(&heth, &MACConf);
    HAL_ETH_Start_IT(&heth);
    return (struct pico_device *)enet;
}


/* TCP/IP: attach pico_rand function to NCS
 * wcs_get_random()
 */

uint32_t pico_rand(void)
{
    uint32_t rnd;
    int ret;
    ret = wcs_get_random((void *)&rnd, sizeof(uint32_t));
    if (ret < 0) {
        /* Panic if unable to use wcs_get_random()
         */
        while(1)
            ;
    }
    return rnd;
}


/* TCP/IP main loop. */
void PicoTask(void *pv) {
    struct pico_device *dev = NULL;
    struct pico_ip4 addr, mask, gw, any;
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = 5;
    uint32_t ad4, nm4, gw4;
    ad4 = addr.addr;
    nm4 = mask.addr;
    gw4 = gw.addr;
    pico_string_to_ipv4("192.168.178.211", &ad4);
    pico_string_to_ipv4("255.255.255.0", &nm4);
    pico_string_to_ipv4("192.168.178.1", &gw4);
    any.addr = 0;
    pico_stack_init(&S);
    dev = pico_eth_create("en0");
    if (dev) {
       pico_ipv4_link_add(S, dev, addr, mask);
       pico_ipv4_route_add(S, any, any, gw, 1, NULL);
       usr_led_on();
    }
    xSemaphoreGive(&picotcp_started);
    pico_stack_tick(S);

    xLastWakeTime = xTaskGetTickCount();
    while(1) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        pico_stack_tick(S);
    }
}


void main(void) {
    picotcp_started = xSemaphoreCreateBinary();
    picotcp_rx_data = xSemaphoreCreateBinary();
    if (xTaskCreate(
        PicoTask,  /* pointer to the task */
        "picoTCP", /* task name for kernel awareness debugging */
        400, /* task stack size */
        (void*)NULL, /* optional task startup argument */
        tskIDLE_PRIORITY,  /* initial priority */
        (xTaskHandle*)NULL /* optional task handle to create */
      ) != pdPASS) {
       for(;;){} /* error! probably out of memory */
    }
    if (xTaskCreate(
        MainTask,  /* pointer to the task */
        "Main", /* task name for kernel awareness debugging */
        1200, /* task stack size */
        (void*)NULL, /* optional task startup argument */
        tskIDLE_PRIORITY,  /* initial priority */
        (xTaskHandle*)NULL /* optional task handle to create */
      ) != pdPASS) {
       for(;;){} /* error! probably out of memory */
    }

    vTaskStartScheduler();
    while(1) {
    }
    return 0 ;
}

