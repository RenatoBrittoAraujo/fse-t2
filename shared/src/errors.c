#include <stdio.h>
#include <string.h>

#include <shared/inc/shared_util.h>
#include <shared/inc/errors.h>

// exits with error code
t_error fatal_error(t_error err_num, char *err_msg)
{
    int size = strlen(err_msg);
    char buff[size + 50];
    sprintf(buff, "fatal error (code: %d): %s", err_num, err_msg);
    if (err_msg[size - 1] != '\n')
        strcat(buff, "\n");
    log_print(buff, LEVEL_ERROR);
    exit(err_num);
}

t_error handle_error(t_error err_num, char *err_msg)
{
    int size = strlen(err_msg);
    char buff[size + 50];
    sprintf(buff, "error (code: %d): %s", err_num, err_msg);
    if (err_msg[size - 1] != '\n')
        strcat(buff, "\n");
    log_print(buff, LEVEL_ERROR);
    exit(err_num);
}