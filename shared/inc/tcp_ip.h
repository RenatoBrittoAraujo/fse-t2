#ifndef COMUNICACAO_H
#define COMUNICACAO_H 1

#define MAX_FRAME_SIZE 10000

#include <shared/inc/errors.h>

t_error call_tcp_ip_port(char *request, char *ip, int port, char* res_buff);
t_error listen_tcp_ip_port(char *(*get_response)(void *), char *ip, int port);

#endif