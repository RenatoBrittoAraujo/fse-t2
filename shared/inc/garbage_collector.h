#ifndef GARBAGE_COLLECTOR_H
#define GARBAGE_COLLECTOR_H 1

#include <stdlib.h>

struct AllocatedMemory
{
    void *ptr;
    struct AllocatedMemory *prev;
};
typedef struct AllocatedMemory AllocatedMemory;

struct OpenFile
{
    int fd;
    struct OpenFile *prev;
};
typedef struct OpenFile OpenFile;

struct GarbageCollector
{
    AllocatedMemory *ptrs;
    int size;

    OpenFile *files;
    int files_size;
};
typedef struct GarbageCollector GarbageCollector;

GarbageCollector *create_garbage_collector();
void *add_to_garbage_collector(void *p);
void *g_malloc(size_t size);
int add_file_to_garbage_collector(int file_descriptor);
void cleanup_garbage(GarbageCollector *g);

#endif