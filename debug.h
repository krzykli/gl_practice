#ifndef DEBUGH
#define DEBUGH

#include <stdarg.h>

void* debug_malloc(size_t size, const char* file, u32 line)
{
     printf("Allocating %lu bytes in file:%s:%i\n", size, file, line);
     return malloc(size);
}

void* debug_calloc(size_t number_of_items, size_t size, const char* file, u32 line)
{
     printf("Callocating %lu bytes in file:%s:%i\n", size, file, line);
     return calloc(number_of_items, size);
}

void* debug_realloc(void* ptr, size_t size, const char* file, u32 line)
{
     printf("Reallocating %lu bytes in file:%s:%i\n", size, file, line);
     return realloc(ptr, size);
}

void debug_free(void* ptr, const char* file, u32 line)
{
     printf("Freeing memory location %s", (byte*)ptr);
     return free(ptr);
}


#ifdef DEBUG
    #define malloc(size) debug_malloc(size, __FILE__, __LINE__)
    #define calloc(number_of_items, size) debug_calloc(number_of_items, size, __FILE__, __LINE__)
    #define realloc(ptr, size) debug_realloc(ptr, size, __FILE__, __LINE__)
    #define free(ptr) debug_free(ptr, __FILE__, __LINE__)
    #define printf(format, ...) \
        printf(format " %s %d\n", ##__VA_ARGS__, __FILE__, __LINE__)
#endif

#endif // DEBUGH
