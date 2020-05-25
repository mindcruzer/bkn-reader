#include <string.h>
#include <stdlib.h>

#include "read_bkn.h"


/*
 * Application entry point.
 *
 * Args:
 * [1] -- Path to the BKN file.
 */
int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Specify a single BKN file.\n");
        return EXIT_FAILURE;
    }

    struct bkn_data* bknData = malloc(sizeof(struct bkn_data));
    
    if (bknData == NULL) {
        out_of_memory();
    }
    
    memset(bknData, 0, sizeof(struct bkn_data));
    
    // Read data
    if (!read_bkn(argv[1], bknData)) {
        return EXIT_FAILURE;
    }    

    // Write data to stdout
    uint32_t i, j;
    for (i = 0; i < bknData->numMethods; i++) {
        printf("[points]\n");
        for (j = 0; j < bknData->methods[i].numPoints; j++) {
            printf("%.10f,%.10f\n", bknData->methods[i].points[j].time, bknData->methods[i].points[j].absorbance);
        } 
        printf("[metadata]\n");
        for (j = 0; j < bknData->methods[i].numMetadata; j++) {
            printf("%s\n", bknData->methods[i].metadata[j]);
        } 
    }

    return EXIT_SUCCESS;
}