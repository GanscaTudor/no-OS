/***************************************************************************//**
 *   @file   motor_control.c
 *   @brief  SWIOT1L motor control example.
 *
 *   Drives a brushed DC motor via a DRV8874 H-bridge with a 10 kHz software
 *   PWM signal on the PMOD connector (pin 4 = P1.28, MOSI). Exposes
 *   SET_PWM / GET_PWM / STOP commands over a TCP socket on the ADIN1110 link.
 *
 *   @author Tudor Gansca (tudor.gansca@analog.com)
********************************************************************************
 * Copyright 2026(c) Analog Devices, Inc.
*******************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "motor_control.h"
#include "motor_control_data.h"
#include "pwm_isr.h"

#include "lwip_socket.h"
#include "lwip_adin1110.h"
#include "tcp_socket.h"
#include "no_os_error.h"
#include "no_os_gpio.h"
#include "no_os_print_log.h"
#include "adin1110.h"
#include "network_interface.h"

#define SERVER_PORT   10000
#define CMD_BUF_SIZE  64
#define RESP_BUF_SIZE 64

static int parse_uint8(const char *s, uint8_t *out)
{
	char *end;
	unsigned long v;

	if (!s || !*s)
		return -EINVAL;

	v = strtoul(s, &end, 10);
	if (*end != '\0' && *end != ' ' && *end != '\r' && *end != '\n')
		return -EINVAL;

	if (v > 100)
		return -ERANGE;

	*out = (uint8_t)v;
	return 0;
}

static int process_command(const char *cmd, char *resp)
{
	if (strncmp(cmd, "SET_PWM ", 8) == 0) {
		uint8_t duty;
		int ret;

		ret = parse_uint8(cmd + 8, &duty);
		if (ret)
			return sprintf(resp, "ERR:BAD_VALUE\n");

		pwm_isr_set_duty(duty);
		return sprintf(resp, "OK\n");
	}

	if (strcmp(cmd, "GET_PWM") == 0)
		return sprintf(resp, "PWM:%u\n", pwm_isr_get_duty());

	if (strcmp(cmd, "STOP") == 0) {
		pwm_isr_set_duty(0);
		return sprintf(resp, "OK\n");
	}

	return sprintf(resp, "ERR:UNKNOWN_CMD\n");
}

/***************************************************************************//**
 * @brief Initialize the ADIN1110 network stack, start the PWM output, and
 *        run a TCP command server on port 10000.
 *
 * @return ret - Result of the example execution.
*******************************************************************************/
int motor_control_main(void)
{
	uint8_t adin1110_mac_address[6] = {0x00, 0x18, 0x80, 0x03, 0x25, 0x60};

	struct lwip_network_param lwip_ip = {
		.platform_ops = &adin1110_lwip_ops,
		.mac_param = &adin1110_ip,
	};

	struct tcp_socket_desc *server_socket;
	struct tcp_socket_desc *client_socket = NULL;
	struct lwip_network_desc *lwip_desc;
	struct tcp_socket_init_param tcp_ip = {
		.max_buff_size = 0
	};

	struct adin1110_desc *adin_desc;
	struct no_os_gpio_desc *adin1110_swpd_gpio = NULL;
	struct no_os_gpio_desc *adin1110_tx2p4_gpio = NULL;
	struct no_os_gpio_desc *adin1110_mssel_gpio = NULL;
	struct no_os_gpio_desc *adin1110_cfg0_gpio = NULL;
	struct no_os_gpio_desc *adin1110_cfg1_gpio = NULL;
	struct no_os_gpio_desc *adin1110_int_gpio = NULL;
	struct no_os_gpio_desc *swiot_led1_gpio = NULL;
	struct no_os_gpio_desc *swiot_led2_gpio = NULL;

	char cmd_buf[CMD_BUF_SIZE];
	char resp_buf[RESP_BUF_SIZE];
	int cmd_idx = 0;
	uint8_t read_byte;
	bool connected = false;
	int ret;

	/*
	 * Bring up the same board-level GPIOs the stock firmware configures for
	 * the ADIN1110 (reset, MSSEL, TX2P4, config strap pins, LEDs). Without
	 * these the T1L link will not come up.
	 */
	no_os_gpio_get(&swiot_led1_gpio, &swiot_led1_ip);
	no_os_gpio_get(&swiot_led2_gpio, &swiot_led2_ip);
	no_os_gpio_get(&adin1110_swpd_gpio, &adin1110_swpd_ip);
	no_os_gpio_get(&adin1110_tx2p4_gpio, &adin1110_tx2p4_ip);
	no_os_gpio_get(&adin1110_mssel_gpio, &adin1110_mssel_ip);
	no_os_gpio_get(&adin1110_cfg0_gpio, &adin1110_cfg0_ip);
	no_os_gpio_get(&adin1110_cfg1_gpio, &adin1110_cfg1_ip);
	no_os_gpio_get(&adin1110_int_gpio, &adin1110_int_ip);

	no_os_gpio_direction_output(swiot_led1_gpio, 1);
	no_os_gpio_direction_output(swiot_led2_gpio, 1);
	no_os_gpio_direction_output(adin1110_swpd_gpio, 1);
	no_os_gpio_direction_output(adin1110_tx2p4_gpio, 0);
	no_os_gpio_direction_output(adin1110_mssel_gpio, 1);
	no_os_gpio_direction_input(adin1110_int_gpio);

	if (SWIOT1L_OA_TC6_SPI) {
		no_os_gpio_direction_output(adin1110_cfg1_gpio, 0);
		no_os_gpio_direction_output(adin1110_cfg0_gpio, 1);
	} else {
		no_os_gpio_direction_output(adin1110_cfg1_gpio, 1);
		no_os_gpio_direction_output(adin1110_cfg0_gpio, 1);
	}

	memcpy(adin1110_ip.mac_address, adin1110_mac_address,
	       NETIF_MAX_HWADDR_LEN);
	memcpy(lwip_ip.hwaddr, adin1110_mac_address, NETIF_MAX_HWADDR_LEN);

	pr_info("SWIOT1L motor control firmware.\n");

	ret = no_os_lwip_init(&lwip_desc, &lwip_ip);
	if (ret) {
		pr_err("LWIP initialization failed (%d)\n", ret);
		return ret;
	}

	adin_desc = (struct adin1110_desc *)lwip_desc->mac_desc;
	if (adin_desc) {
		pr_info("MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
			adin_desc->mac_address[0], adin_desc->mac_address[1],
			adin_desc->mac_address[2], adin_desc->mac_address[3],
			adin_desc->mac_address[4], adin_desc->mac_address[5]);
	} else {
		pr_err("MAC address is NULL\n");
		ret = -ENODEV;
		goto remove_lwip;
	}

	ret = pwm_isr_init();
	if (ret) {
		pr_err("PWM init failed (%d)\n", ret);
		goto remove_lwip;
	}

	pr_info("PWM initialized on P1.28 (PMOD pin 4) at 10 kHz\n");

	tcp_ip.net = &lwip_desc->no_os_net;

	ret = socket_init(&server_socket, &tcp_ip);
	if (ret) {
		pr_err("Socket initialization failed (%d)\n", ret);
		goto remove_lwip;
	}

	ret = socket_bind(server_socket, SERVER_PORT);
	if (ret) {
		pr_err("Socket bind failed (%d)\n", ret);
		goto remove_server_socket;
	}

	ret = socket_listen(server_socket, MAX_BACKLOG);
	if (ret) {
		pr_err("Socket listen failed (%d)\n", ret);
		goto remove_server_socket;
	}

	pr_info("Motor command server listening on port %d\n", SERVER_PORT);

	while (1) {
		no_os_lwip_step(server_socket->net->net, NULL);

		if (connected) {
			ret = socket_recv(client_socket, &read_byte, 1);
			if (ret > 0) {
				if (read_byte == '\n' || read_byte == '\r') {
					if (cmd_idx > 0) {
						int resp_len;

						cmd_buf[cmd_idx] = '\0';
						pr_info("CMD: %s\n", cmd_buf);

						resp_len = process_command(cmd_buf,
									   resp_buf);
						socket_send(client_socket,
							    (uint8_t *)resp_buf,
							    resp_len);

						pr_info("RSP: %s", resp_buf);
						cmd_idx = 0;
					}
				} else if (cmd_idx < CMD_BUF_SIZE - 1) {
					cmd_buf[cmd_idx++] = (char)read_byte;
				} else {
					pr_err("Command too long, discarding\n");
					cmd_idx = 0;
				}
			} else if (ret < 0 && ret != -EAGAIN) {
				pr_err("Socket recv failed (%d), closing\n", ret);
				socket_remove(client_socket);
				client_socket = NULL;
				connected = false;
				cmd_idx = 0;
			} else if (ret == 0) {
				pr_info("Client disconnected\n");
				socket_remove(client_socket);
				client_socket = NULL;
				connected = false;
				cmd_idx = 0;
			}
		} else {
			ret = socket_accept(server_socket, &client_socket);
			if (ret && ret != -EAGAIN) {
				pr_err("Socket accept failed (%d)\n", ret);
				goto remove_server_socket;
			}

			if (!ret) {
				connected = true;
				cmd_idx = 0;
				pr_info("Client connected\n");
			}
		}
	}

remove_server_socket:
	socket_remove(server_socket);

remove_lwip:
	no_os_lwip_remove(lwip_desc);

	return ret;
}
