#include <string.h>
#include <stdlib.h>

#include "json.h"
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

    // Output as JSON string
    char* json = json_serialize(bknData);
    printf("%s\n", json);
    
    return EXIT_SUCCESS;
}