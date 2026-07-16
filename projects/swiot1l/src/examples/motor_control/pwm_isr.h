/***************************************************************************//**
 *   @file   pwm_isr.h
 *   @brief  Timer-ISR software PWM output on P1.28 (PMOD pin 4, MOSI).
 *   @author Tudor Gansca (tudor.gansca@analog.com)
********************************************************************************
 * Copyright 2026(c) Analog Devices, Inc.
*******************************************************************************/
#ifndef __PWM_ISR_H__
#define __PWM_ISR_H__

#include <stdint.h>

int pwm_isr_init(void);
void pwm_isr_set_duty(uint8_t percent);
uint8_t pwm_isr_get_duty(void);

#endif /* __PWM_ISR_H__ */
