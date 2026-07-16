/***************************************************************************//**
 *   @file   apard_led_control_example.c
 *   @brief  Implementation of the APARD LED control example.
 *   @author Tudor Gansca (tudor.gansca@analog.com)
********************************************************************************
 * Copyright 2025(c) Analog Devices, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of Analog Devices, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES, INC. “AS IS” AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL ANALOG DEVICES, INC. BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#include <stdio.h>
#include <string.h>
#include "common_data.h"

#include "lwip_socket.h"
#include "lwip_adin1110.h"
#include "tcp_socket.h"
#include "no_os_error.h"
#include "no_os_gpio.h"
#include "adin1110.h"
#include "network_interface.h"

#define SERVER_PORT	10000
#define CMD_BUF_SIZE	64
#define RESP_BUF_SIZE	64

static struct no_os_gpio_desc *led_desc;

static int process_command(const char *cmd, char *resp,
			   struct no_os_gpio_desc *gpio_led)
{
	uint8_t val;

	if (strcmp(cmd, "LED_ON") == 0) {
		no_os_gpio_set_value(gpio_led, NO_OS_GPIO_HIGH);
		return sprintf(resp, "OK\n");
	}

	if (strcmp(cmd, "LED_OFF") == 0) {
		no_os_gpio_set_value(gpio_led, NO_OS_GPIO_LOW);
		return sprintf(resp, "OK\n");
	}

	if (strcmp(cmd, "LED_STATUS") == 0) {
		no_os_gpio_get_value(gpio_led, &val);
		return sprintf(resp, "LED:%s\n", val ? "ON" : "OFF");
	}

	return sprintf(resp, "ERR:UNKNOWN_CMD\n");
}

/***************************************************************************//**
 * @brief Configure the output port of the AD-APARDPFWD-SL, initialize LED GPIO,
 *        then open a TCP socket to accept commands.
 * @return ret - Result of the example execution.
*******************************************************************************/
int example_main()
{
	struct lwip_network_param lwip_ip = {
		.platform_ops = &adin1110_lwip_ops,
		.mac_param = &adin1110_ip,
	};

	struct tcp_socket_desc *server_socket;
	struct tcp_socket_desc *client_socket;
	struct lwip_network_desc *lwip_desc;
	struct tcp_socket_init_param tcp_ip = {
		.max_buff_size = 0
	};

	struct no_os_uart_desc *uart_desc;
	struct adin1110_desc *adin_desc;

	char cmd_buf[CMD_BUF_SIZE];
	char resp_buf[RESP_BUF_SIZE];
	int cmd_idx = 0;

	uint32_t device_id;
	uint8_t read_byte;
	int ret;

	ret = no_os_uart_init(&uart_desc, &uart_ip);
	if (ret) {
		pr_err("UART initialization failed (%d)\n", ret);
		return ret;
	}

	no_os_uart_stdio(uart_desc);

	ret = spi_cfg_0(adin1110_spi_cfg_0);
	if (ret) {
		pr_err("ADIN2111 SPI configuration failed (%d)\n", ret);
		goto remove_uart;
	}

	/* Enable Port 2 on PFWD shield (LOW = enabled) */
	ret = port2_cfg(port2_cfg_0, NO_OS_GPIO_LOW);
	if (ret) {
		pr_err("AD-APARDPFWD output port configuration failed (%d)\n", ret);
		goto remove_uart;
	}

	pr_info("AD-APARDPFWD COMMAND SERVER.\n");

	memcpy(lwip_ip.hwaddr, adin1110_ip.mac_address, NETIF_MAX_HWADDR_LEN);

	ret = no_os_lwip_init(&lwip_desc, &lwip_ip);
	if (ret) {
		pr_err("LWIP initialization failed (%d)\n", ret);
		goto remove_uart;
	}

	adin_desc = (struct adin1110_desc *)lwip_desc->mac_desc;

	if (adin_desc) {
		printf("MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
		       adin_desc->mac_address[0],
		       adin_desc->mac_address[1],
		       adin_desc->mac_address[2],
		       adin_desc->mac_address[3],
		       adin_desc->mac_address[4],
		       adin_desc->mac_address[5]);
	} else {
		pr_err("MAC address is NULL (%d)\n", ret);
		goto remove_lwip;
	}

	/*
	 * Remove broadcast filter so unmatched broadcasts are forwarded
	 * between T1L ports by the ADIN2111 switch.
	 */
	adin1110_broadcast_filter(adin_desc, false);

	/*
	 * Enable FWD_UNK2HOST so the host still receives unmatched frames
	 * (including broadcasts for ARP) while the switch forwards them.
	 */
	adin1110_set_promisc(adin_desc, 0, true);

	ret = adin1110_reg_read(adin_desc, ADIN1110_PHY_ID_REG, &device_id);
	if (ret) {
		pr_err("Error reading the ADIN1110's device id (%d)\n", ret);
		goto remove_lwip;
	}

	pr_info("Got device id 0x%X\n", device_id);

	/* Initialize LED GPIO */
	ret = no_os_gpio_get(&led_desc, &led_gpio_ip);
	if (ret) {
		pr_err("LED GPIO init failed (%d)\n", ret);
		goto remove_lwip;
	}

	ret = no_os_gpio_direction_output(led_desc, NO_OS_GPIO_LOW);
	if (ret) {
		pr_err("LED GPIO direction set failed (%d)\n", ret);
		goto remove_led;
	}

	pr_info("LED GPIO initialized on P%d.%d\n",
		led_gpio_ip.port, led_gpio_ip.number);

	tcp_ip.net = &lwip_desc->no_os_net;

	ret = socket_init(&server_socket, &tcp_ip);
	if (ret) {
		pr_err("Socket initialization failed (%d)\n", ret);
		goto remove_led;
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

	pr_info("Command server listening on port %d\n", SERVER_PORT);

	bool connected = false;
	cmd_idx = 0;

	while (1) {
		no_os_lwip_step(server_socket->net->net, NULL);

		if (connected) {
			ret = socket_recv(client_socket, &read_byte, 1);
			if (ret > 0) {
				if (read_byte == '\n' || read_byte == '\r') {
					if (cmd_idx > 0) {
						cmd_buf[cmd_idx] = '\0';
						pr_info("CMD: %s\n", cmd_buf);

						int resp_len = process_command(
							cmd_buf, resp_buf,
							led_desc);
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
				pr_err("Socket recv failed (%d), closing\n",
				       ret);
				socket_remove(client_socket);
				connected = false;
				cmd_idx = 0;
			} else if (ret == 0) {
				pr_info("Client disconnected\n");
				socket_remove(client_socket);
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

remove_led:
	no_os_gpio_remove(led_desc);

remove_lwip:
	no_os_lwip_remove(lwip_desc);

remove_uart:
	no_os_uart_remove(uart_desc);

	return 0;
}
