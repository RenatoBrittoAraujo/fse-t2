#ifndef SHARED_UTIL_H
#define SHARED_UTIL_H 1

#define LEVEL_DEBUG 0
#define LEVEL_INFO 1
#define LEVEL_WARNING 2
#define LEVEL_ERROR 3

#include <shared/inc/errors.h>

void set_level(int level);
void log_print(char *str, int level);

char *itoa(int x);

char *get_env_str();
int get_env_int();
int is_dev();
int is_dev_test();
int is_prod();
t_error set_env(int);
t_error get_env(int *);

// if index is not present, may set index to a negative number
int read_env_int_index(char *name, int index);

// if index is not present, may set index to a negative number
char *read_env_str_index(char *name, int index);

// if index is not present, may set index to a negative number
double read_env_double_index(char *name, int index);

#endif