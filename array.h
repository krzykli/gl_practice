#ifndef ARRAYH
#define ARRAYH

#include "types.h"
#include <string.h>

typedef struct Array
{
    u32 element_size;
    u32 max_element_count;
    u32 element_count;

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


bool ArrayCheckAppendBounds(Array& arr, u32 count)
{
    if (arr.element_count + count > arr.max_element_count)
    {
        printf("Not enough memory to perform addition.");
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

