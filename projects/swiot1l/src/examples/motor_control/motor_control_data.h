/***************************************************************************//**
 *   @file   motor_control_data.h
 *   @brief  Init parameters used by the motor_control firmware.
 *
 *   Kept separate from common_data.h so the motor firmware does not depend
 *   on MAX14906, AD74413R, ADT75 headers (those drivers are not linked in
 *   this build).
 *
 *   @author Tudor Gansca (tudor.gansca@analog.com)
********************************************************************************
 * Copyright 2026(c) Analog Devices, Inc.
*******************************************************************************/
#ifndef __MOTOR_CONTROL_DATA_H__
#define __MOTOR_CONTROL_DATA_H__

#include "platform_includes.h"
#include "adin1110.h"

#ifndef SWIOT1L_OA_TC6_SPI
#define SWIOT1L_OA_TC6_SPI 0
#endif

extern struct no_os_uart_init_param uart_ip;
extern struct adin1110_init_param adin1110_ip;
extern const struct no_os_spi_init_param adin1110_spi_ip;

extern const struct no_os_gpio_init_param adin1110_rst_gpio_ip;
extern const struct no_os_gpio_init_param adin1110_swpd_ip;
extern const struct no_os_gpio_init_param adin1110_tx2p4_ip;
extern const struct no_os_gpio_init_param adin1110_mssel_ip;
extern const struct no_os_gpio_init_param adin1110_cfg0_ip;
extern const struct no_os_gpio_init_param adin1110_cfg1_ip;
extern const struct no_os_gpio_init_param adin1110_int_ip;
extern const struct no_os_gpio_init_param swiot_led1_ip;
extern const struct no_os_gpio_init_param swiot_led2_ip;

#endif /* __MOTOR_CONTROL_DATA_H__ */
