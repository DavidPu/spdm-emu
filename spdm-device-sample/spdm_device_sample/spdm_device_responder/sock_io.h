/**
 *  Copyright Notice:
 *  Copyright 2021-2022 DMTF. All rights reserved.
 *  License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/spdm-emu/blob/main/LICENSE.md
 **/

#ifndef __SPDM_TEST_COMMAND_H__
#define __SPDM_TEST_COMMAND_H__

#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "errno.h"
#include "sys/socket.h"
#include "arpa/inet.h"

struct proxy_buf {
    uint32_t len;
    uint8_t  data[16U * 1024U];
};

extern struct proxy_buf to_psc_buf;
extern struct proxy_buf from_psc_buf;

typedef int SOCKET;
#define closesocket(x) close(x)
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)

#define DEFAULT_SPDM_PLATFORM_PORT 2323
#define TCP_SPDM_PLATFORM_PORT 4194


/* Client->Server/Server->Client
 *   command/response: 4 bytes (big endian)
 *   transport_type: 4 bytes (big endian)
 *   PayloadSize (excluding command and PayloadSize): 4 bytes (big endian)
 *   payload (SPDM message, starting from SPDM_HEADER): PayloadSize (little endian)*/


#define SOCKET_TRANSPORT_TYPE_NONE 0x00
#define SOCKET_TRANSPORT_TYPE_MCTP 0x01
#define SOCKET_TRANSPORT_TYPE_PCI_DOE 0x02
#define SOCKET_TRANSPORT_TYPE_TCP 0x03

#define SOCKET_TCP_NO_HANDSHAKE 0x00
#define SOCKET_TCP_HANDSHAKE 0x01

#define SOCKET_SPDM_COMMAND_NORMAL 0x0001
#define SOCKET_SPDM_COMMAND_OOB_ENCAP_KEY_UPDATE 0x8001
#define SOCKET_SPDM_COMMAND_CONTINUE 0xFFFD
#define SOCKET_SPDM_COMMAND_SHUTDOWN 0xFFFE
#define SOCKET_SPDM_COMMAND_UNKOWN 0xFFFF
#define SOCKET_SPDM_COMMAND_TEST 0xDEAD

#include <stdint.h>
bool create_socket(uint16_t port_number, SOCKET *listen_socket);

bool send_platform_data(SOCKET socket, uint32_t command,
                        const uint8_t *send_buffer, size_t bytes_to_send);

bool receive_platform_data(SOCKET socket, uint32_t *command,
                           uint8_t *receive_buffer,
                           size_t *bytes_to_receive);

extern int m_server_socket;
extern uint32_t m_command;

void dump_data(const uint8_t *buffer, size_t buffer_size);

#endif
