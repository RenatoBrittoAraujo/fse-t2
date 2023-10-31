#ifndef SHARED_ERRORS_H
#define SHARED_ERRORS_H 1

#include <stdlib.h>

typedef unsigned long t_error;
#define NO_ERROR 0

t_error fatal_error(t_error err_num, char *err_msg);
t_error handle_error(t_error err_num, char *err_msg);

#endif
