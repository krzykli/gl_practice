#ifndef DICTH
#define DICTH

#include "types.h"
#include <string.h>

#include "array.h"

typedef struct Dict
{
     Array keys;
     Array values;
} Dict;


void dict_init(Dict &dict, Array &keys, Array &values)
{
     dict.keys = keys;
     dict.values = values;
}


void* dict_get(Dict &dict, const char* key, u32 &out_index)
{
    u32 element_count = dict.keys.element_count;
    for (u32 i=0; i<element_count; ++i)
    {
        void** ptr = (void**)array_get_index(dict.keys, i);  // assuming strings as keys
        const char* current_key= (const char*)*ptr;
        if(strcmp(current_key, key) == 0)
        {
            out_index = i;
            return (void*)array_get_index(dict.values, i);
        }
    }
    return NULL;
}


void dict_map(Dict &dict, const char* key, void* value)
{
    u32 index;
    void* result = dict_get(dict, key, index);
    if(result != NULL)
    {
        print ("inserting %p %i", value, index);
        array_insert(dict.values, value, index);
    }
    else
    {
        print ("appending %p %i", value, index);
        array_append(dict.keys, &key);
        array_append(dict.values, value);
    }
}

#endif // DICTH
