#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include <shared/inc/shared_util.h>
#include <shared/inc/proto.h>
#include <shared/inc/time.h>
#include <shared/inc/threads.h>
#include <shared/inc/tcp_ip.h>
#include <shared/inc/errors.h>

#define SHARED_THREADS_ERROR_INVALID_ENV 1001

#define ENV_PROD 1
#define ENV_DEV_TEST 2
#define ENV_DEV 3

char BUFF[10000];
int GLOBAL_LOG_LEVEL = 0;
int ENV = ENV_PROD;

void set_level(int level)
{
    GLOBAL_LOG_LEVEL = level;
}

void log_print(char *str, int level)
{
    if (level >= GLOBAL_LOG_LEVEL)
    {
        switch (level)
        {
        case LEVEL_DEBUG:
            printf("[DEBUG] ");
            break;
        case LEVEL_INFO:
            printf("[INFO] ");
            break;
        case LEVEL_WARNING:
            printf("[WARNING] ");
            break;
        case LEVEL_ERROR:
            printf("[ERROR] ");
            break;
        default:
            break;
        }
        printf("%s", str);
        int len = strlen(str);
        if (len > 0 && str[len - 1] != '\n')
        {
            printf("\n");
        }
    }
    fflush(stdout);
}

char *itoa(int x)
{
    char *buff = (char *)malloc(sizeof(char) * 20);
    sprintf(buff, "%d", x);
    return buff;
}

// if index is not present, may set index to a negative number
char *read_env_str_index(char *name, int index)
{
    char key[10000];
    if (index < 0)
    {
        sprintf(key, "%s", name);
    }
    else
    {
        sprintf(key, "%s%d", name, index);
    }

    char *val = getenv(key);
    if (val == NULL)
    {
        // sprintf(BUFF, "[read_env_str_index] NO KEY FOR ENV VAR '%s'\n", key);
        log_print(BUFF, 1);
        return NULL;
    }
    // sprintf(BUFF, "[read_env_str_index] KEY VAL '%s = %s'\n", key, val);
    // log_print(BUFF, 0);
    return val;
}

// if index is not present, may set index to a negative number
int read_env_int_index(char *name, int index)
{
    char *val = read_env_str_index(name, index);
    if (val == NULL)
        return 0;
    return atoi(val);
}

// if index is not present, may set index to a negative number
double read_env_double_index(char *name, int index)
{
    char *val = read_env_str_index(name, index);
    if (val == NULL)
        return 0;
    return atof(val);
}

char *get_env_str()
{
    return read_env_str_index("ENV", -1);
}

int get_env_int()
{
    return ENV;
}

int is_dev()
{
    return strcmp("DEV", get_env_str()) == 0;
}

int is_dev_test()
{
    return strcmp("DEV_TEST", get_env_str()) == 0;
}

int is_prod()
{
    return !is_dev() && !is_dev_test();
}

t_error set_env(int new_env)
{
    if (new_env != ENV_DEV && new_env != ENV_DEV_TEST && new_env != ENV_PROD)
    {
        return handle_error(SHARED_THREADS_ERROR_INVALID_ENV,
                            "[shared.shared_util] [set_env] invalid env");
    }
    ENV = new_env;
}

t_error get_env(int *env)
{
#ifdef TEST_MODE
    log_print("ENV=dev_test", LEVEL_INFO);
    *env = ENV_DEV_TEST;
    return NULL;
#else
    if (is_dev())
    {
        log_print("ENV=dev", LEVEL_INFO);
        *env = ENV_DEV;
    }
    else if (is_prod())
    {
        log_print("ENV=prod", LEVEL_INFO);
        *env = ENV_PROD;
    }
    else
    {
        return handle_error(SHARED_THREADS_ERROR_INVALID_ENV,
                            "[shared.shared_util] [get_env] env not found");
    }
    return NO_ERROR;
#endif
}