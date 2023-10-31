#include <shared/inc/garbage_collector.h>

GarbageCollector *GARBAGE_COLLECTOR;

void *add_to_garbage_collector(void *p)
{
    GarbageCollector *g = GARBAGE_COLLECTOR;

    AllocatedMemory *mem = (AllocatedMemory *)malloc(sizeof(AllocatedMemory));
    mem->prev = g->ptrs;
    mem->ptr = p;
    g->size++;
    g->ptrs = mem;
    return p;
}

void *g_malloc(size_t size)
{
    if (GARBAGE_COLLECTOR == NULL)
        GARBAGE_COLLECTOR = create_garbage_collector();
    return add_to_garbage_collector(malloc(size));
}

int add_file_to_garbage_collector(int file_descriptor)
{
    GarbageCollector *g = GARBAGE_COLLECTOR;

    OpenFile *of = (OpenFile *)g_malloc(sizeof(OpenFile));
    of->prev = g->files;
    of->fd = file_descriptor;
    g->files_size++;
    g->files = of;
    return file_descriptor;
}

GarbageCollector *create_garbage_collector()
{
    GarbageCollector *g = (GarbageCollector *)malloc(sizeof(GarbageCollector));

    g->ptrs = NULL;
    g->size = 0;

    g->files = NULL;
    g->files_size = 0;

    GARBAGE_COLLECTOR = g;

    add_to_garbage_collector(g);

    return g;
}

// if global garbage collector, just send NULL
void cleanup_garbage(GarbageCollector *g)
{
    if (g == NULL)
        g = GARBAGE_COLLECTOR;

    int rev_order = 0;
    while (g->files != NULL)
    {
        close(g->files->fd);
        g->files = g->files->prev;
        rev_order++;
    }

    rev_order = 0;
    while (g->ptrs != NULL)
    {
        void *allocated_mem = g->ptrs;
        void *prev_allocated_mem = g->ptrs->prev;
        free(g->ptrs->ptr);
        free(allocated_mem);
        rev_order++;
        g->ptrs = prev_allocated_mem;
    }
}