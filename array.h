#ifndef ARRAYH
#define ARRAYH

#include "types.h"
#include <string.h>

void* debug_malloc(size_t size, const char* file, u32 line)
{
     printf("Allocating %lu bytes in file:%s:%i\n", size, file, line);
     return malloc(size);
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
    #define realloc(ptr, size) debug_realloc(ptr, size, __FILE__, __LINE__)
    #define free(ptr) debug_free(ptr, __FILE__, __LINE__)
#endif

#define RESIZE_CALLBACK(cb_name) size_t cb_name(void* arr)
typedef RESIZE_CALLBACK(resize_callback);


typedef struct Array
{
    u32 element_size;
    u32 max_element_count;
    u32 element_count;
    resize_callback *resize_func;

    u32 _block_size;
    byte* _base_pointer;
    byte* _head_pointer;
} Array;


void ArrayInit(Array &arr)
{
    arr._block_size = arr.element_size * arr.max_element_count;
    arr._base_pointer = (byte*) malloc(arr._block_size);
    arr._head_pointer = arr._base_pointer;
}


void ArrayRealloc(Array& arr, size_t new_size)
{
    arr._base_pointer = (byte*)realloc(arr._base_pointer, new_size);
    arr._block_size = new_size;
    arr.max_element_count = new_size / arr.element_size;
    arr._head_pointer = arr._base_pointer + arr.element_size * arr.element_count;
    printf("max element count, %i\n", arr.max_element_count);
    printf("element count, %i\n", arr.element_count);

}


void ArrayResizeNoop() {}


bool ArrayCheckAppendBounds(Array& arr, u32 count)
{
    if (arr.element_count + count > arr.max_element_count)
    {
        printf("Not enough memory to perform addition.\n");
        return false;
    }
    return true;
}


void ArrayExtend(Array& arr, void* elements, u32 count)
{
    if(ArrayCheckAppendBounds(arr, count))
    {
        memcpy((void*)arr._head_pointer, elements, arr.element_size * count);
        arr._head_pointer += arr.element_size * count;
        arr.element_count += count;
    }
    else
    {
        //size_t new_size = arr.resize_func(&arr);
        size_t new_size = arr._block_size * 2;
        ArrayRealloc(arr, new_size);
    }
}


void ArrayAppend(Array& arr, void* element)
{
    ArrayExtend(arr, element, 1);
}


void* ArrayPop(Array& arr)
{
    if (arr.element_count > 0)
    {
        void* addr = arr._head_pointer;
        arr.element_count--;
        arr._head_pointer -= arr.element_size;
        return addr;
        //u32 byte_offset = arr.element_size * ;
        //return (void*) (arr._base_pointer + byte_offset);
    }
    return NULL;
}


void ArrayInsert(Array& arr, void* element, u32 index)
{
    u32 byte_offset = index * arr.element_size;
    memcpy((void*) (arr._base_pointer + byte_offset), element, arr.element_size);
}


void* ArrayGetIndex(Array& arr, u32 index)
{
    u32 byte_offset = arr.element_size * index;
    return (void*) (arr._base_pointer + byte_offset);
}


void ArrayFree(Array& arr)
{
    free(arr._base_pointer);
}
#endif // ARRAYH

