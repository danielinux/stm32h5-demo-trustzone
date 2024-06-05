/* startup_arm.c
 *
 * Test bare-metal blinking led application
 *
 * Copyright (C) 2021 wolfSSL Inc.
 *
 * This file is part of wolfBoot.
 *
 * wolfBoot is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

extern unsigned int _stored_data;
extern unsigned int _start_data;
extern unsigned int _end_data;
extern unsigned int _start_bss;
extern unsigned int _end_bss;
extern unsigned int _end_stack;
extern unsigned int _start_heap;

#ifdef STM32
extern void isr_tim2(void);
#endif


static volatile unsigned int avail_mem = 0;

extern void main(void);

void isr_reset(void) {
    register unsigned int *src, *dst;
    src = (unsigned int *) &_stored_data;
    dst = (unsigned int *) &_start_data;
    while (dst < (unsigned int *)&_end_data) {
        *dst = *src;
        dst++;
        src++;
    }

    dst = &_start_bss;
    while (dst < (unsigned int *)&_end_bss) {
        *dst = 0U;
        dst++;
    }

    avail_mem = &_end_stack - &_start_heap;
    main();
}

void isr_fault(void)
{
    /* Panic. */
    while(1) ;;

}

void isr_memfault(void)
{
    /* Panic. */
    while(1) ;;
}

void isr_busfault(void)
{
    /* Panic. */
    while(1) ;;
}

void isr_usagefault(void)
{
    /* Panic. */
    while(1) ;;
}


void isr_empty(void)
{

}

extern void SVC_Handler(void);
extern void PendSV_Handler(void);
extern void SysTick_Handler(void);

__attribute__ ((section(".isr_vector")))
void (* const IV[])(void) =
{
	(void (*)(void))(&_end_stack),
	isr_reset,                   // Reset
	isr_fault,                   // NMI
	isr_fault,                   // HardFault
	isr_memfault,                // MemFault
	isr_busfault,                // BusFault
	isr_usagefault,              // UsageFault
	0,                           // SecureFault
	0,                          // reserved
	0,                          // reserved
	0,                          // reserved
	SVC_Handler,                   // SVC
	isr_empty,                   // DebugMonitor
	0,                           // reserved
	PendSV_Handler,                   // PendSV
	SysTick_Handler,                   // SysTick

    isr_empty, //	WWDG_IRQHandler
    isr_empty, //	PVD_PVM_IRQHandler
    isr_empty, //	RTC_IRQHandler
    isr_empty, //	RTC_S_IRQHandler
    isr_empty, //	TAMP_IRQHandler
    isr_empty, //	TAMP_S_IRQHandler
    isr_empty, //	FLASH_IRQHandler
    isr_empty, //	FLASH_S_IRQHandler
    isr_empty, //	GTZC_IRQHandler
    isr_empty, //	RCC_IRQHandler
    isr_empty, //	RCC_S_IRQHandler
    isr_empty, //	EXTI0_IRQHandler
    isr_empty, //	EXTI1_IRQHandler
    isr_empty, //	EXTI2_IRQHandler
    isr_empty, //	EXTI3_IRQHandler
    isr_empty, //	EXTI4_IRQHandler
    isr_empty, //	EXTI5_IRQHandler
    isr_empty, //	EXTI6_IRQHandler
    isr_empty, //	EXTI7_IRQHandler
    isr_empty, //	EXTI8_IRQHandler
    isr_empty, //	EXTI9_IRQHandler
    isr_empty, //	EXTI10_IRQHandler
    isr_empty, //	EXTI11_IRQHandler
    isr_empty, //	EXTI12_IRQHandler
    isr_empty, //	EXTI13_IRQHandler
    isr_empty, //	EXTI14_IRQHandler
    isr_empty, //	EXTI15_IRQHandler
    isr_empty, //	DMAMUX1_IRQHandler
    isr_empty, //	DMAMUX1_S_IRQHandler
    isr_empty, //	DMA1_Channel1_IRQHandler
    isr_empty, //	DMA1_Channel2_IRQHandler
    isr_empty, //	DMA1_Channel3_IRQHandler
    isr_empty, //	DMA1_Channel4_IRQHandler
    isr_empty, //	DMA1_Channel5_IRQHandler
    isr_empty, //	DMA1_Channel6_IRQHandler
    isr_empty, //	DMA1_Channel7_IRQHandler
    isr_empty, //	DMA1_Channel8_IRQHandler
    isr_empty, //	ADC1_2_IRQHandler
    isr_empty, //	DAC_IRQHandler
    isr_empty, //	FDCAN1_IT0_IRQHandler
    isr_empty, //	FDCAN1_IT1_IRQHandler
    isr_empty, //	TIM1_BRK_IRQHandler
    isr_empty, //	TIM1_UP_IRQHandler
    isr_empty, //	TIM1_TRG_COM_IRQHandler
    isr_empty, //	TIM1_CC_IRQHandler
    isr_empty, //	TIM2_IRQHandler
    isr_empty, //	TIM3_IRQHandler
    isr_empty, //	TIM4_IRQHandler
    isr_empty, //	TIM5_IRQHandler
    isr_empty, //	TIM6_IRQHandler
    isr_empty, //	TIM7_IRQHandler
    isr_empty, //	TIM8_BRK_IRQHandler
    isr_empty, //	TIM8_UP_IRQHandler
    isr_empty, //	TIM8_TRG_COM_IRQHandler
    isr_empty, //	TIM8_CC_IRQHandler
    isr_empty, //	I2C1_EV_IRQHandler
    isr_empty, //	I2C1_ER_IRQHandler
    isr_empty, //	I2C2_EV_IRQHandler
    isr_empty, //	I2C2_ER_IRQHandler
    isr_empty, //	SPI1_IRQHandler
    isr_empty, //	SPI2_IRQHandler
    isr_empty, //	USART1_IRQHandler
    isr_empty, //	USART2_IRQHandler
    isr_empty, //	USART3_IRQHandler
    isr_empty, //	UART4_IRQHandler
    isr_empty, //	UART5_IRQHandler
    isr_empty, //	LPUART1_IRQHandler
    isr_empty, //	LPTIM1_IRQHandler
    isr_empty, //	LPTIM2_IRQHandler
    isr_empty, //	TIM15_IRQHandler
    isr_empty, //	TIM16_IRQHandler
    isr_empty, //	TIM17_IRQHandler
    isr_empty, //	COMP_IRQHandler
    isr_empty, //	USB_FS_IRQHandler
    isr_empty, //	CRS_IRQHandler
    isr_empty, //	FMC_IRQHandler
    isr_empty, //	OCTOSPI1_IRQHandler
    isr_empty, //	0
    isr_empty, //	SDMMC1_IRQHandler
    isr_empty, //	0
    isr_empty, //	DMA2_Channel1_IRQHandler
    isr_empty, //	DMA2_Channel2_IRQHandler
    isr_empty, //	DMA2_Channel3_IRQHandler
    isr_empty, //	DMA2_Channel4_IRQHandler
    isr_empty, //	DMA2_Channel5_IRQHandler
    isr_empty, //	DMA2_Channel6_IRQHandler
    isr_empty, //	DMA2_Channel7_IRQHandler
    isr_empty, //	DMA2_Channel8_IRQHandler
    isr_empty, //	I2C3_EV_IRQHandler
    isr_empty, //	I2C3_ER_IRQHandler
    isr_empty, //	SAI1_IRQHandler
    isr_empty, //	SAI2_IRQHandler
    isr_empty, //	TSC_IRQHandler
    isr_empty, //	AES_IRQHandler
    isr_empty, //	RNG_IRQHandler
    isr_empty, //	FPU_IRQHandler
    isr_empty, //	HASH_IRQHandler
    isr_empty, //	PKA_IRQHandler
    isr_empty, //	LPTIM3_IRQHandler
    isr_empty, //	SPI3_IRQHandler
    isr_empty, //	I2C4_ER_IRQHandler
    isr_empty, //	I2C4_EV_IRQHandler
    isr_empty, //	DFSDM1_FLT0_IRQHandler
    isr_empty, //	DFSDM1_FLT1_IRQHandler
    isr_empty, //	DFSDM1_FLT2_IRQHandler
    isr_empty, //	DFSDM1_FLT3_IRQHandler
    isr_empty, //	UCPD1_IRQHandler
    isr_empty, //	ICACHE_IRQHandler
    isr_empty, //	OTFDEC1_IRQHandler
};
