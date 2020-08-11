#ifndef DEBUGH
#define DEBUGH

#include <stdarg.h>

#define print(format, ...) \
    printf("%s\t| %s:%d\t| " format "\n", __FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#ifdef DEBUG

void* debug_malloc(size_t size, const char* file, u32 line)
{
    void* ptr = malloc(size);
    print("Allocating %lu bytes in file:%s:%i @%p", size, file, line, ptr);
    return ptr;
}

void* debug_calloc(size_t number_of_items, size_t size, const char* file, u32 line)
{
    void* ptr = calloc(number_of_items, size);
    print("Allocating %lu bytes in file:%s:%i @%p", size, file, line, ptr);
    return ptr;
}

void* debug_realloc(void* ptr, size_t size, const char* file, u32 line)
{
    void* result = realloc(ptr, size);
    print("Reallocating %lu bytes in file:%s:%i @%p->@%p", size, file, line, ptr, result);
    return result;
}

void debug_free(void* ptr, const char* file, u32 line)
{
    print("Freeing memory location %p in file:%s:%i", ptr, file, line);
    return free(ptr);
}


#ifdef DEBUG

#define malloc(size) debug_malloc(size, __FILE__, __LINE__)
#define calloc(number_of_items, size) debug_calloc(number_of_items, size, __FILE__, __LINE__)
#define realloc(ptr, size) debug_realloc(ptr, size, __FILE__, __LINE__)
#define free(ptr) debug_free(ptr, __FILE__, __LINE__)

#endif

#endif // DEBUGH
