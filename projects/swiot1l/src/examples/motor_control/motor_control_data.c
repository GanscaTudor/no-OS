/***************************************************************************//**
 *   @file   motor_control_data.c
 *   @brief  Init-parameter definitions for the motor_control firmware.
 *
 *   Provides only what the motor firmware needs: UART, ADIN1110 SPI, and
 *   the GPIOs the ADIN1110 requires to bring up the T1L link. Everything
 *   related to MAX14906, AD74413R, ADT75, SWIOT state machine is omitted.
 *
 *   @author Tudor Gansca (tudor.gansca@analog.com)
********************************************************************************
 * Copyright 2026(c) Analog Devices, Inc.
*******************************************************************************/
#include "motor_control_data.h"

struct no_os_uart_init_param uart_ip = {
	.device_id = UART_DEVICE_ID,
	.irq_id = UART_IRQ_ID,
	.asynchronous_rx = true,
	.baud_rate = UART_BAUDRATE,
	.size = NO_OS_UART_CS_8,
	.parity = NO_OS_UART_PAR_NO,
	.platform_ops = &max_uart_ops,
	.stop = NO_OS_UART_STOP_1_BIT,
	.extra = UART_EXTRA,
};

const struct no_os_spi_init_param adin1110_spi_ip = {
	.device_id = 2,
	.max_speed_hz = 15000000,
	.bit_order = NO_OS_SPI_BIT_ORDER_MSB_FIRST,
	.mode = NO_OS_SPI_MODE_0,
	.platform_ops = &max_spi_ops,
	.chip_select = 0,
	.extra = SPI_EXTRA,
};

const struct no_os_gpio_init_param adin1110_rst_gpio_ip = {
	.port = 2,
	.number = 1,
	.pull = NO_OS_PULL_UP,
	.platform_ops = &max_gpio_ops,
	.extra = GPIO_EXTRA,
};

const struct no_os_gpio_init_param adin1110_swpd_ip = {
	.port = 2,
	.number = 25,
	.pull = NO_OS_PULL_UP,
	.platform_ops = &max_gpio_ops,
	.extra = GPIO_EXTRA,
};

const struct no_os_gpio_init_param adin1110_tx2p4_ip = {
	.port = 2,
	.number = 10,
	.pull = NO_OS_PULL_DOWN,
	.platform_ops = &max_gpio_ops,
	.extra = GPIO_EXTRA,
};

const struct no_os_gpio_init_param adin1110_mssel_ip = {
	.port = 2,
	.number = 9,
	.pull = NO_OS_PULL_NONE,
	.platform_ops = &max_gpio_ops,
	.extra = GPIO_EXTRA,
};

const struct no_os_gpio_init_param adin1110_cfg0_ip = {
	.port = 2,
	.number = 3,
	.pull = NO_OS_PULL_NONE,
	.platform_ops = &max_gpio_ops,
	.extra = GPIO_EXTRA,
};

const struct no_os_gpio_init_param adin1110_cfg1_ip = {
	.port = 2,
	.number = 0,
	.pull = NO_OS_PULL_UP,
	.platform_ops = &max_gpio_ops,
	.extra = GPIO_EXTRA,
};

const struct no_os_gpio_init_param adin1110_int_ip = {
	.port = 2,
	.number = 6,
	.pull = NO_OS_PULL_UP,
	.platform_ops = &max_gpio_ops,
	.extra = GPIO_EXTRA,
};

const struct no_os_gpio_init_param swiot_led1_ip = {
	.port = 2,
	.number = 15,
	.pull = NO_OS_PULL_UP,
	.platform_ops = &max_gpio_ops,
	.extra = GPIO_EXTRA,
};

const struct no_os_gpio_init_param swiot_led2_ip = {
	.port = 2,
	.number = 14,
	.pull = NO_OS_PULL_UP,
	.platform_ops = &max_gpio_ops,
	.extra = GPIO_EXTRA,
};

struct adin1110_init_param adin1110_ip = {
	.chip_type = ADIN1110,
	.comm_param = adin1110_spi_ip,
	.reset_param = adin1110_rst_gpio_ip,
	.append_crc = false,
	.oa_tc6_spi = SWIOT1L_OA_TC6_SPI,
};
