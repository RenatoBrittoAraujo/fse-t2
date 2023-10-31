#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <shared/inc/shared_util.h>
#include <shared/inc/tcp_ip.h>

#define SHARED_TCP_IP_ERROR_SOCKET_CREATION_FAILED 1000
#define SHARED_TCP_IP_ERROR_INVALID_ADDRESS 1001
#define SHARED_TCP_IP_ERROR_CONNECTION_FAILED 1002
#define SHARED_TCP_IP_ERROR_BIND_FAILED 1003
#define SHARED_TCP_IP_ERROR_SOCKOPT_FAILED 1004
#define SHARED_TCP_IP_ERROR_LISTEN_FAILED 1005
#define SHARED_TCP_IP_ERROR_ACCEPT_FAILED 1006

// res_buff may be NULL, response will not be set
t_error call_tcp_ip_port(char *request, char *ip, int port, char *res_buff)
{
    log_print("[shared.tcp_ip] [listen_tcp_ip_port] call_tcp_ip_port()\n", LEVEL_DEBUG);
    size_t message_size = sizeof(char) * MAX_FRAME_SIZE;

    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char *buffer = (char *)malloc(message_size);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        return handle_error(SHARED_TCP_IP_ERROR_SOCKET_CREATION_FAILED, "[shared.tcp_ip] [call_tcp_ip_port] socket creation failed");
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0)
    {
        return handle_error(SHARED_TCP_IP_ERROR_INVALID_ADDRESS, "[shared.tcp_ip] [call_tcp_ip_port] address not found / invalid address");
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        return handle_error(SHARED_TCP_IP_ERROR_CONNECTION_FAILED, "[shared.tcp_ip] [call_tcp_ip_port] connection failed");
    }

    send(sock, request, strlen(request), 0);
    valread = read(sock, buffer, 1024);
    if (res_buff != NULL)
    {
        memcpy(res_buff, message_size, buffer);
    }
    return NULL;
}

t_error listen_tcp_ip_port(char *(*get_response)(void *), char *ip, int port)
{
    log_print("[shared.tcp_ip] [listen_tcp_ip_port] listen_tcp_ip_port\n", LEVEL_DEBUG);

    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        return handle_error(SHARED_TCP_IP_ERROR_SOCKET_CREATION_FAILED, "[shared.tcp_ip] [listen_tcp_ip_port] socket creation failed");
    }
    log_print("[shared.tcp_ip] [listen_tcp_ip_port] socket opened\n", LEVEL_DEBUG);

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEADDR, &opt, sizeof(opt)))
    {
        return handle_error(SHARED_TCP_IP_ERROR_SOCKOPT_FAILED, "[shared.tcp_ip] [listen_tcp_ip_port] setsockopt failed");
    }
    log_print("[shared.tcp_ip] [listen_tcp_ip_port] socket options set\n", LEVEL_DEBUG);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        return handle_error(SHARED_TCP_IP_ERROR_BIND_FAILED, "[shared.tcp_ip] [listen_tcp_ip_port] bind failed");
    }
    log_print("[shared.tcp_ip] [listen_tcp_ip_port] socket binded\n", LEVEL_DEBUG);

    if (listen(server_fd, 3) < 0)
    {
        return handle_error(SHARED_TCP_IP_ERROR_LISTEN_FAILED, "[shared.tcp_ip] [listen_tcp_ip_port] listen failed");
    }
    log_print("[shared.tcp_ip] [listen_tcp_ip_port] listen worked!\n", LEVEL_DEBUG);

    while (1)
    {
        log_print("[shared.tcp_ip] [listen_tcp_ip_port] waiting for connection\n", LEVEL_ERROR);
        printf("Waiting for connection...\n");
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            return handle_error(SHARED_TCP_IP_ERROR_ACCEPT_FAILED, "[shared.tcp_ip] [listen_tcp_ip_port] accept failed");
        }
        log_print("[shared.tcp_ip] [listen_tcp_ip_port] request received\n", LEVEL_DEBUG);

        valread = read(new_socket, buffer, 1024);
        char *response = get_response(buffer);
        log_print("[shared.tcp_ip] [listen_tcp_ip_port] response created\n", LEVEL_DEBUG);

        send(new_socket, &response, strlen(response), 0);
        log_print("[shared.tcp_ip] [listen_tcp_ip_port] response sent\n", LEVEL_DEBUG);

        close(new_socket);
    }
    return NULL;
}
