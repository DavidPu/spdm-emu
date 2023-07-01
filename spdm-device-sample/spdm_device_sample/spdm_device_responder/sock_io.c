/**
 *  Copyright Notice:
 *  Copyright 2021-2022 DMTF. All rights reserved.
 *  License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/spdm-emu/blob/main/LICENSE.md
 **/

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "sock_io.h"

/* hack to add MCTP header for PCAP*/
#include "industry_standard/mctp.h"
#include "hal/library/memlib.h"

uint32_t m_use_transport_layer = SOCKET_TRANSPORT_TYPE_PCI_DOE;

uint32_t m_use_tcp_handshake = SOCKET_TCP_NO_HANDSHAKE;
struct in_addr m_ip_address = { 0x0100007F };
static int debug_enable = 0;
#define debug(fmt, ...) do { if (debug_enable) { printf(fmt, ## __VA_ARGS__);}} while (0)
/**
 * Read number of bytes data in blocking mode.
 *
 * If there is no enough data in socket, this function will wait.
 * This function will return if enough data is read, or socket error.
 **/
bool read_bytes(const SOCKET socket, uint8_t *buffer,
                uint32_t number_of_bytes)
{
    int32_t result;
    uint32_t number_received;

    number_received = 0;
    while (number_received < number_of_bytes) {
        result = recv(socket, (char *)(buffer + number_received),
                      number_of_bytes - number_received, 0);
        if (result == -1) {
            printf("Receive error - 0x%x\n", errno);
            return false;
        }
        if (result == 0) {
            return false;
        }
        number_received += result;
    }
    return true;
}

bool read_data32(const SOCKET socket, uint32_t *data)
{
    bool result;

    result = read_bytes(socket, (uint8_t *)data, sizeof(uint32_t));
    if (!result) {
        return result;
    }
    *data = ntohl(*data);
    return true;
}

/**
 * Read multiple bytes in blocking mode.
 *
 * The length is presented as first 4 bytes in big endian.
 * The data follows the length.
 *
 * If there is no enough data in socket, this function will wait.
 * This function will return if enough data is read, or socket error.
 **/
bool read_multiple_bytes(const SOCKET socket, uint8_t *buffer,
                         uint32_t *bytes_received,
                         uint32_t max_buffer_length)
{
    uint32_t length;
    bool result;

    result = read_data32(socket, &length);
    if (!result) {
        return result;
    }
    debug("Platform port Receive size: ");
    length = ntohl(length);
    dump_data((uint8_t *)&length, sizeof(uint32_t));
    debug("\n");
    length = ntohl(length);

    *bytes_received = length;
    if (*bytes_received > max_buffer_length) {
        printf("buffer too small (0x%x). Expected - 0x%x\n",
               max_buffer_length, *bytes_received);
        return false;
    }
    if (length == 0) {
        return true;
    }
    result = read_bytes(socket, buffer, length);
    if (!result) {
        return result;
    }
    debug("Platform port Receive buffer:\n    ");
    dump_data(buffer, length);
    debug("\n");

    return true;
}

bool receive_platform_data(const SOCKET socket, uint32_t *command,
                           uint8_t *receive_buffer,
                           size_t *bytes_to_receive)
{
    bool result;
    uint32_t response;
    uint32_t transport_type;
    uint32_t bytes_received;

    result = read_data32(socket, &response);
    if (!result) {
        return result;
    }
    *command = response;
    debug("Platform port Receive command: ");
    response = ntohl(response);
    dump_data((uint8_t *)&response, sizeof(uint32_t));
    debug("\n");

    result = read_data32(socket, &transport_type);
    if (!result) {
        return result;
    }
    debug("Platform port Receive transport_type: ");
    transport_type = ntohl(transport_type);
    dump_data((uint8_t *)&transport_type, sizeof(uint32_t));
    debug("\n");
    transport_type = ntohl(transport_type);
    if (transport_type != m_use_transport_layer) {
        printf("transport_type mismatch\n");
        return false;
    }

    bytes_received = 0;
    result = read_multiple_bytes(socket, receive_buffer, &bytes_received,
                                 (uint32_t)*bytes_to_receive);
    if (!result) {
        return result;
    }
    if (bytes_received > (uint32_t)*bytes_to_receive) {
        return false;
    }
    *bytes_to_receive = bytes_received;

    switch (*command) {
    case SOCKET_SPDM_COMMAND_SHUTDOWN:
        break;
    case SOCKET_SPDM_COMMAND_NORMAL:
        break;
    }

    return result;
}

/**
 * Write number of bytes data in blocking mode.
 *
 * This function will return if data is written, or socket error.
 **/
bool write_bytes(const SOCKET socket, const uint8_t *buffer,
                 uint32_t number_of_bytes)
{
    int32_t result;
    uint32_t number_sent;

    number_sent = 0;
    while (number_sent < number_of_bytes) {
        result = send(socket, (char *)(buffer + number_sent),
                      number_of_bytes - number_sent, 0);
        if (result == -1) {
#ifdef _MSC_VER
            if (WSAGetLastError() == 0x2745) {
                printf("Client disconnected\n");
            } else {
#endif
            printf("Send error - 0x%x\n",
#ifdef _MSC_VER
                   WSAGetLastError()
#else
                   errno
#endif
                   );
#ifdef _MSC_VER
        }
#endif
            return false;
        }
        number_sent += result;
    }
    return true;
}

bool write_data32(const SOCKET socket, uint32_t data)
{
    data = htonl(data);
    return write_bytes(socket, (uint8_t *)&data, sizeof(uint32_t));
}

/**
 * Write multiple bytes.
 *
 * The length is presented as first 4 bytes in big endian.
 * The data follows the length.
 **/
bool write_multiple_bytes(const SOCKET socket, const uint8_t *buffer,
                          uint32_t bytes_to_send)
{
    bool result;

    result = write_data32(socket, bytes_to_send);
    if (!result) {
        return result;
    }
    debug("Platform port Transmit size: ");
    bytes_to_send = htonl(bytes_to_send);
    dump_data((uint8_t *)&bytes_to_send, sizeof(uint32_t));
    debug("\n");
    bytes_to_send = htonl(bytes_to_send);

    result = write_bytes(socket, buffer, bytes_to_send);
    if (!result) {
        return result;
    }
    debug("Platform port Transmit buffer:\n    ");
    dump_data(buffer, bytes_to_send);
    debug("\n");
    return true;
}

bool send_platform_data(const SOCKET socket, uint32_t command,
                        const uint8_t *send_buffer, size_t bytes_to_send)
{
    bool result;
    uint32_t request;
    uint32_t transport_type;

    request = command;
    result = write_data32(socket, request);
    if (!result) {
        return result;
    }
    debug("Platform port Transmit command: ");
    request = htonl(request);
    dump_data((uint8_t *)&request, sizeof(uint32_t));
    debug("\n");

    result = write_data32(socket, m_use_transport_layer);
    if (!result) {
        return result;
    }
    debug("Platform port Transmit transport_type: ");
    transport_type = ntohl(m_use_transport_layer);
    dump_data((uint8_t *)&transport_type, sizeof(uint32_t));
    debug("\n");

    result = write_multiple_bytes(socket, send_buffer,
                                  (uint32_t)bytes_to_send);
    if (!result) {
        return result;
    }
    return true;
}


bool init_client(SOCKET *sock, uint16_t port)
{
    SOCKET client_socket;
    struct sockaddr_in server_addr;
    int32_t ret_val;

#ifdef _MSC_VER
    WSADATA ws;
    if (WSAStartup(MAKEWORD(2, 2), &ws) != 0) {
        printf("Init Windows socket Failed - %x\n", WSAGetLastError());
        return false;
    }
#endif

    client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket == INVALID_SOCKET) {
        printf("Create socket Failed - %x\n",
#ifdef _MSC_VER
               WSAGetLastError()
#else
               errno
#endif
               );
        return false;
    }

    server_addr.sin_family = AF_INET;
    libspdm_copy_mem(&server_addr.sin_addr.s_addr, sizeof(struct in_addr), &m_ip_address,
                     sizeof(struct in_addr));
    server_addr.sin_port = htons(port);
    libspdm_zero_mem(server_addr.sin_zero, sizeof(server_addr.sin_zero));

    ret_val = connect(client_socket, (struct sockaddr *)&server_addr,
                      sizeof(server_addr));
    if (ret_val == SOCKET_ERROR) {
        printf("Connect Error - %x\n",
#ifdef _MSC_VER
               WSAGetLastError()
#else
               errno
#endif
               );
        closesocket(client_socket);
        return false;
    }

    printf("connect success!\n");

    *sock = client_socket;
    return true;
}

bool create_socket(uint16_t port_number, SOCKET *listen_socket)
{
    struct sockaddr_in my_address;
    int32_t res;

    /* Initialize Winsock*/
#ifdef _MSC_VER
    WSADATA ws;
    res = WSAStartup(MAKEWORD(2, 2), &ws);
    if (res != 0) {
        printf("WSAStartup failed with error: %d\n", res);
        return false;
    }
#endif

    *listen_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (INVALID_SOCKET == *listen_socket) {
        printf("Cannot create server listen socket.  Error is 0x%x\n",
#ifdef _MSC_VER
               WSAGetLastError()
#else
               errno
#endif
               );
        return false;
    }

    /* When the program stops unexpectedly the used port will stay in the TIME_WAIT
     * state which prevents other programs from binding to this port until a timeout
     * triggers. This timeout may be 30s to 120s. In this state the responder cannot
     * be restarted since it cannot bind to its port.
     * To prevent this SO_REUSEADDR is applied to the socket which allows the
     * responder to bind to this port even if it is still in the TIME_WAIT state.*/
    if (setsockopt(*listen_socket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        printf("Cannot configure server listen socket.  Error is 0x%x\n",
#ifdef _MSC_VER
               WSAGetLastError()
#else
               errno
#endif
               );
        closesocket(*listen_socket);
        return false;
    }

    libspdm_zero_mem(&my_address, sizeof(my_address));
    my_address.sin_port = htons((short)port_number);
    my_address.sin_family = AF_INET;

    res = bind(*listen_socket, (struct sockaddr *)&my_address,
               sizeof(my_address));
    if (res == SOCKET_ERROR) {
        printf("Bind error.  Error is 0x%x\n",
#ifdef _MSC_VER
               WSAGetLastError()
#else
               errno
#endif
               );
        closesocket(*listen_socket);
        return false;
    }

    res = listen(*listen_socket, 3);
    if (res == SOCKET_ERROR) {
        printf("Listen error.  Error is 0x%x\n",
#ifdef _MSC_VER
               WSAGetLastError()
#else
               errno
#endif
               );
        closesocket(*listen_socket);
        return false;
    }

    return true;
}
