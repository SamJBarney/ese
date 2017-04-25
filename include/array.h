#pragma once

#define ARRAY_DEFINITION_(type) \
typedef struct \
{ \
    size_t size; \
    size_t count; \
    type * values; \
} type ## _array_t; \
\
\
\
void type ## _array_append(type ## _array_t *, type * value); \
void type ## _array_remove(type ## _array_t *, size_t idx); \
void type ## _array_shrink(type ## _array_t *);\
void type ## _array_reserve(type ## _array_t *, size_t);
#define ARRAY_DEFINITION(type) ARRAY_DEFINITION_(type)

#define ARRAY_VARIABLE_(type, name) type ## _array_t name = {0,0,NULL}
#define ARRAY_VARIABLE(type, name) ARRAY_VARIABLE_(type, name)

#define ARRAY_ITERATE(array, action) \
for(size_t index = 0; index < array.count; ++index) \
{ \
    action; \
}

#define ARRAY_IMPLEMENTATION_(type) \
\
\
\
void type ##_array_append(type ## _array_t * array, type * value) \
{ \
    if (array->count < array->size) \
    { \
        array->values[array->count] = *value; \
        ++array->count; \
    } \
    else \
    { \
        array->size = (array->size / 5) + 10; \
        array->values = realloc(array->values, array->size * sizeof(type)); \
        array->values[array->count] = *value; \
        ++array->count; \
    } \
} \
\
\
\
void type ## _array_remove(type ## _array_t * array, size_t idx) \
{ \
    if (idx < array->count) \
    { \
        array->values[idx] = array->values[--array->count]; \
    } \
} \
\
\
\
void type ## _array_shrink(type ## _array_t * array) \
{ \
    if (array->size > array->count) \
    { \
        array->values = realloc(array->values, array->count * sizeof(type)); \
        array->size = array->count; \
    } \
}\
\
\
\
void type ## _array_reserve(type ## _array_t * array, size_t amount) \
{ \
    if (amount > (array->size - array->count)) \
    { \
        array->size = array->size + (array->size - array->count); \
        array->values = realloc(array->values, array->size * sizeof(type)); \
    } \
}
#define ARRAY_IMPLEMENTATION(type) ARRAY_IMPLEMENTATION_(type)