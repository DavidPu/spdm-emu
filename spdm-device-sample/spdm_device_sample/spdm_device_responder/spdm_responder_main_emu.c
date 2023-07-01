/**
 *  Copyright Notice:
 *  Copyright 2021-2022 DMTF. All rights reserved.
 *  License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/libspdm/blob/main/LICENSE.md
 **/
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "errno.h"
#include "sys/socket.h"
#include "arpa/inet.h"
typedef int SOCKET;
#define closesocket(x) close(x)
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)

#include "spdm_responder.h"
#include "hal/base.h"
#include "library/spdm_responder_lib.h"
#include "library/spdm_transport_pcidoe_lib.h"
#include "library/pci_doe_common_lib.h"
#include "library/pci_doe_responder_lib.h"

#include "library/malloclib.h"
#include "hal/library/debuglib.h"
#include "hal/library/memlib.h"

#include "sock_io.h"
int m_server_socket = -1;

extern void *m_pci_doe_context;
extern uint8_t m_send_receive_buffer[LIBSPDM_MAX_SENDER_RECEIVER_BUFFER_SIZE];
extern size_t m_send_receive_buffer_size;


/* Disable optimization to avoid code removal with VS2019.*/

#if defined(_MSC_EXTENSIONS)
#pragma optimize("", off)
#elif defined (__clang__)
#pragma clang optimize off
#endif

static void *spdm_context;

bool platform_server(const SOCKET socket)
{
    bool result;
    libspdm_return_t status;
    uint8_t response[LIBPCIDOE_MAX_NON_SPDM_MESSAGE_SIZE];
    size_t response_size;

    printf("%s:%d\n", __func__, __LINE__);
    while (true) {
        status = libspdm_responder_dispatch_message(spdm_context);
        if (status == LIBSPDM_STATUS_SUCCESS) {
            /* success dispatch SPDM message*/
        }
        if ((status == LIBSPDM_STATUS_SEND_FAIL) ||
            (status == LIBSPDM_STATUS_RECEIVE_FAIL)) {
            printf("Server Critical Error - STOP\n");
            return false;
        }
        if (status != LIBSPDM_STATUS_UNSUPPORTED_CAP) {
            continue;
        }
        switch (m_command) {
        case SOCKET_SPDM_COMMAND_TEST:
            result = send_platform_data(socket,
                                        SOCKET_SPDM_COMMAND_TEST,
                                        (uint8_t *)"Server Hello!",
                                        sizeof("Server Hello!"));
            if (!result) {
                printf("send_platform_data Error - %x\n", errno);
                return true;
            }
            break;

        case SOCKET_SPDM_COMMAND_OOB_ENCAP_KEY_UPDATE:
#if (LIBSPDM_ENABLE_CAPABILITY_MUT_AUTH_CAP) || (LIBSPDM_ENABLE_CAPABILITY_ENCAP_CAP)
            LIBSPDM_ASSERT(false);
            libspdm_init_key_update_encap_state(m_spdm_context);
            result = send_platform_data(
                socket,
                SOCKET_SPDM_COMMAND_OOB_ENCAP_KEY_UPDATE, NULL,
                0);
            if (!result) {
                printf("send_platform_data Error - %x\n", errno);
                return true;
            }
#endif
            break;

        case SOCKET_SPDM_COMMAND_SHUTDOWN:
            result = send_platform_data(
                socket, SOCKET_SPDM_COMMAND_SHUTDOWN, NULL, 0);
            if (!result) {
                printf("send_platform_data Error - %x\n", errno);
                return true;
            }
            return false;
            break;

        case SOCKET_SPDM_COMMAND_CONTINUE:
            result = send_platform_data(
                socket, SOCKET_SPDM_COMMAND_CONTINUE, NULL, 0);
            if (!result) {
                printf("send_platform_data Error - %x\n", errno);
                return true;
            }
            return true;
            break;

        case SOCKET_SPDM_COMMAND_NORMAL:
        {
                response_size = sizeof(response);
                status = pci_doe_get_response_doe_request (m_pci_doe_context,
                                                           m_send_receive_buffer,
                                                           m_send_receive_buffer_size, response,
                                                           &response_size);
                if (LIBSPDM_STATUS_IS_ERROR(status)) {
                    /* unknown message*/
                    return true;
                }
                result = send_platform_data(
                    socket, SOCKET_SPDM_COMMAND_NORMAL,
                    response, response_size);
                if (!result) {
                    printf("send_platform_data Error - %x\n", errno);
                    return true;
                }
            }
            break;

        default:
            printf("Unrecognized platform interface command %x\n",
                   m_command);
            result = send_platform_data(
                socket, SOCKET_SPDM_COMMAND_UNKOWN, NULL, 0);
            if (!result) {
                printf("send_platform_data Error - %x\n", errno);
                return true;
            }
            return true;
        }
    }
}

bool platform_server_routine(uint16_t port_number)
{
    SOCKET responder_socket;
    struct sockaddr_in peer_address;
    bool result;
    uint32_t length;
    bool continue_serving;

    result = create_socket(port_number, &responder_socket);
    if (!result) {
        printf("Create platform service socket fail\n");
#ifdef _MSC_VER
        WSACleanup();
#endif
        return false;
    }

    do {
        printf("Platform server listening on port %d\n", port_number);

        length = sizeof(peer_address);
        m_server_socket =
            accept(responder_socket, (struct sockaddr *)&peer_address,
                    (socklen_t *)&length);
        if (m_server_socket == INVALID_SOCKET) {
            closesocket(responder_socket);
            printf("Accept error.  Error is 0x%x\n", errno);
            return false;
        }
        continue_serving = platform_server(m_server_socket);
        closesocket(m_server_socket);

    } while (continue_serving);

    closesocket(responder_socket);
    return true;
}

void spdm_dispatch(void)
{
    // void *spdm_context;
    libspdm_return_t status;

    spdm_context = spdm_server_init();
    if (spdm_context == NULL) {
        return;
    }

    status = pci_doe_init_responder ();
    if (status != LIBSPDM_STATUS_SUCCESS) {
        return;
    }

    platform_server_routine(DEFAULT_SPDM_PLATFORM_PORT);

    return;
}

/**
 * Main entry point.
 *
 * @return This function should never return.
 *
 **/
int main(void)
{
    spdm_dispatch();

    return 0;
}
