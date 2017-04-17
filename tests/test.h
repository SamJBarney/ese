#pragma once

int success_count = 0;
int failure_count = 0;

#define EXPECT(description, action) \
    if (action) \
    { \
        printf("SUCCESS: %s\n", description); \
        ++success_count;\
    } \
    else \
    { \
        printf("FAILURE: %s\n", description); \
        printf("\t%s\n", #action); \
        ++failure_count; \
        return 1; \
    }