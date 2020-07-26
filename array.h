#ifndef ARRAYH
#define ARRAYH

#include "types.h"
#include <string.h>

#define RESIZE_CALLBACK(cb_name) size_t cb_name(void* arr)
typedef RESIZE_CALLBACK(resize_callback);


typedef struct Array
{
    u32 element_size;
    u32 element_count;
    u32 max_element_count;
    resize_callback *resize_func;

    u32 _block_size;
    byte* base_ptr;
    byte* _head_ptr;
} Array;


void array_init(Array &arr, u32 element_size, u32 max_element_count)
{
    arr.element_count = 0;
    arr.element_size = element_size;
    arr.max_element_count = max_element_count;
    arr._block_size = arr.element_size * arr.max_element_count;
    arr.base_ptr = (byte*) calloc(arr.max_element_count, arr._block_size);
    arr._head_ptr = arr.base_ptr;
}


void array_realloc(Array& arr, size_t new_size)
{
    arr.base_ptr = (byte*)realloc(arr.base_ptr, new_size);
    arr._block_size = new_size;
    arr.max_element_count = new_size / arr.element_size;
    arr._head_ptr = arr.base_ptr + arr.element_size * arr.element_count;
    printf("max element count, %i\n", arr.max_element_count);
    printf("element count, %i\n", arr.element_count);

}


void array_resize_noop() {}


bool array_check_bounds(Array& arr, u32 count)
{
    if (arr.element_count + count > arr.max_element_count)
    {
        printf("Not enough memory to perform addition.\n");
        return false;
    }
    return true;
}


void array_extend(Array& arr, void* elements, u32 count)
{
    if(array_check_bounds(arr, count))
    {
        memcpy((void*)arr._head_ptr, elements, arr.element_size * count);
        arr._head_ptr += arr.element_size * count;
        arr.element_count += count;
    }
    else
    {
        //size_t new_size = arr.resize_func(&arr);
        size_t new_size = arr._block_size * 2;
        array_realloc(arr, new_size);
    }
}


void array_append(Array& arr, void* element)
{
    array_extend(arr, element, 1);
}


void* array_pop(Array& arr)
{
    if (arr.element_count > 0)
    {
        void* addr = arr._head_ptr;
        arr.element_count--;
        arr._head_ptr -= arr.element_size;
        return addr;
        //u32 byte_offset = arr.element_size * ;
        //return (void*) (arr.base_ptr + byte_offset);
    }
    return NULL;
}


void array_insert(Array& arr, void* element, u32 index)
{
    u32 byte_offset = index * arr.element_size;
    memcpy((void*) (arr.base_ptr + byte_offset), element, arr.element_size);
}


void* array_get_index(Array& arr, u32 index)
{
    u32 byte_offset = arr.element_size * index;
    return (void*) (arr.base_ptr + byte_offset);
}


void array_free(Array& arr)
{
    free(arr.base_ptr);
}
#endif // ARRAYH

