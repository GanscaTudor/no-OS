/***************************************************************************//**
 *   @file   pwm_isr.c
 *   @brief  Timer-ISR software PWM output on P1.28 (PMOD pin 4, MOSI).
 *
 *   TMR1 is configured in continuous mode to fire at PWM_TICK_HZ (1 MHz).
 *   The ISR toggles P1.28 based on a global duty (0..PWM_STEPS). With
 *   PWM_STEPS = 100 this yields a 10 kHz output with 1% resolution.
 *
 *   TMR0 is intentionally NOT used because on SWIOT1L P3.7 (TMR0_OUT) is
 *   wired to MAX14906 D1 chip-select; even though MAX14906 is not
 *   initialized in this firmware, using TMR1 keeps the free-running timer
 *   away from the CS traces.
 *
 *   @author Tudor Gansca (tudor.gansca@analog.com)
********************************************************************************
 * Copyright 2026(c) Analog Devices, Inc.
*******************************************************************************/

#include "pwm_isr.h"

#include "mxc_device.h"
#include "gpio.h"
#include "tmr.h"
#include "nvic_table.h"

#define PWM_TIMER              MXC_TMR1
#define PWM_TIMER_IRQn         TMR1_IRQn
#define PWM_TIMER_IRQ_HANDLER  TMR1_IRQHandler

#define PWM_TICK_HZ            1000000U
#define PWM_STEPS              100U

#define PWM_GPIO_PORT          MXC_GPIO1
#define PWM_GPIO_MASK          (1U << 28)

static volatile uint8_t g_pwm_duty;
static volatile uint8_t s_phase;

void PWM_TIMER_IRQ_HANDLER(void)
{
	MXC_TMR_ClearFlags(PWM_TIMER);

	if (s_phase < g_pwm_duty)
		PWM_GPIO_PORT->out_set = PWM_GPIO_MASK;
	else
		PWM_GPIO_PORT->out_clr = PWM_GPIO_MASK;

	if (++s_phase >= PWM_STEPS)
		s_phase = 0;
}

int pwm_isr_init(void)
{
	mxc_gpio_cfg_t gpio_cfg = {
		.port = PWM_GPIO_PORT,
		.mask = PWM_GPIO_MASK,
		.func = MXC_GPIO_FUNC_OUT,
		.pad  = MXC_GPIO_PAD_NONE,
		.vssel = MXC_GPIO_VSSEL_VDDIOH,
	};
	int ret;

	ret = MXC_GPIO_Config(&gpio_cfg);
	if (ret != E_NO_ERROR)
		return ret;

	PWM_GPIO_PORT->out_clr = PWM_GPIO_MASK;

	g_pwm_duty = 0;
	s_phase = 0;

	MXC_TMR_Shutdown(PWM_TIMER);

	mxc_tmr_cfg_t tmr_cfg = {
		.pres = TMR_PRES_1,
		.mode = TMR_MODE_CONTINUOUS,
		.cmp_cnt = PeripheralClock / PWM_TICK_HZ,
		.pol = 0,
		.bitMode = TMR_BIT_MODE_32,
		.clock = MXC_TMR_APB_CLK,
	};

	ret = MXC_TMR_Init(PWM_TIMER, &tmr_cfg, false);
	if (ret != E_NO_ERROR)
		return ret;

	NVIC_SetPriority(PWM_TIMER_IRQn, 1);
	NVIC_ClearPendingIRQ(PWM_TIMER_IRQn);
	NVIC_EnableIRQ(PWM_TIMER_IRQn);

	MXC_TMR_Start(PWM_TIMER);

	return 0;
}

void pwm_isr_set_duty(uint8_t percent)
{
	if (percent > 100)
		percent = 100;

	g_pwm_duty = percent;
}

uint8_t pwm_isr_get_duty(void)
{
	return g_pwm_duty;
}
