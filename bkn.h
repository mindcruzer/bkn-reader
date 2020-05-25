#ifndef BKN_H
#define BKN_H

#include <stdio.h>
#include <stdint.h>

#define out_of_memory()                               \
            fprintf(stderr, "Ran out of memory.\n");  \
            exit(EXIT_FAILURE);

struct bkn_point {
    float time; 
    float absorbance;
};

struct bkn_method {
    struct bkn_point* points;
    uint32_t numPoints;
    char** metadata;
    uint32_t numMetadata;
};

struct bkn_data {
    struct bkn_method* methods;
    uint32_t numMethods;
};

#endif