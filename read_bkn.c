#include <stdlib.h>
#include <string.h>

#include "read_bkn.h"


struct bkn_file {
    uint8_t *buffer;
    size_t size;
    size_t offset;
};


/**
 * Searches a BKN file for the first occurrence of a target sequence of bytes, starting 
 * from the current offset. 
 *
 * bknFile -- BKN file structure.
 * target -- The sequence of bytes locate in `buffer`.
 * targetSize -- The size of `target`.
 *
 * Returns:
 * The index immediately following `target` if `target` is found; -1 otherwise.
 */
static bool search_bkn_file(struct bkn_file* bknFile, uint8_t *target, size_t targetSize) {
    size_t targetIndex = 0;

    while (bknFile->offset < bknFile->size) {
        if (bknFile->buffer[bknFile->offset] == target[targetIndex]) { 
            if (++targetIndex == targetSize) {
                bknFile->offset++;
                return true;
            }
        }
        else {
            if (targetIndex != 0) {
                bknFile->offset -= targetIndex;
            }

            targetIndex = 0;
        }

        bknFile->offset++;
    } 
    
    bknFile->offset = bknFile->size -1;
    return false;
}


/**
 * Reads a float from the current offset.
 * 
 * bknFile -- BKN file structure.
 * 
 * Returns:
 * Float value.
 */
static float read_float(struct bkn_file* bknFile) {
    float value; 
    memcpy(&value, bknFile->buffer + bknFile->offset, sizeof(float));
    bknFile->offset += sizeof(float);
    return value;
}


/*
 * Reads the absorbance/time points from the current offset.
 *
 * bknFile -- BKN file structure.
 * numPoints -- Number of points to read.
 * 
 * Returns:
 * Array of points.
 */
static struct bkn_point* read_bkn_points(struct bkn_file* bknFile, int numPoints) {
    struct bkn_point* bknPoints = malloc(numPoints * sizeof(struct bkn_point));

    if (bknPoints == NULL) {
        out_of_memory(); 
    }

    uint32_t i;
    for (i = 0; i < numPoints; i++) {
        // A point is a pair of single-precision floating point values
        bknPoints[i].absorbance = read_float(bknFile);
        bknPoints[i].time = read_float(bknFile);
    }

    return bknPoints;
}


/*
 * Reads a metadata field from the current offset.
 *
 * bknFile -- BKN file structure.
 *
 * Returns:
 * Field value. 
 */
static char* read_bkn_field(struct bkn_file* bknFile) {
    // The field is preceded by a 32-bit integer indicating the number of 
    // bytes in the field
    uint32_t fieldLength;
    memcpy(&fieldLength, bknFile->buffer + bknFile->offset, 4);
    bknFile->offset += 4;

    // Read in the field value
    char* value = malloc((fieldLength * sizeof(char)) + 1);

    if (value == NULL) {
        out_of_memory();
    }

    memcpy(value, bknFile->buffer + bknFile->offset, fieldLength);
    bknFile->offset += fieldLength;
    value[fieldLength] = '\0';

    return value;
}


/*
 * Reads method data from the current offset.
 *
 * bknFile -- BKN file structure.
 * bknMethod -- Structure method data will be written into.
 *
 * Returns:
 * BKN method.
 */
static struct bkn_method* read_bkn_method(struct bkn_file* bknFile, struct bkn_method* bknMethod) {
    // Seek to the number of points in the method
    bknFile->offset += 0x1C; 

    // The number of points is indicated with a 32-bit integer
    memcpy(&bknMethod->numPoints, bknFile->buffer + bknFile->offset, 4);

    // Seek to the start of the points
    bknFile->offset += 0x3EC;
    bknMethod->points = read_bkn_points(bknFile, bknMethod->numPoints);

    // The metadata immediately follows the points
    char* value;
    char* endValue = "End Method";
    while(true) {
        value = read_bkn_field(bknFile);
        
        bknMethod->metadata = realloc(bknMethod->metadata, (bknMethod->numMetadata + 1) * sizeof(char*));

        if (bknMethod->metadata == NULL) {
            out_of_memory();
        }

        bknMethod->metadata[bknMethod->numMetadata] = value;
        bknMethod->numMetadata++;

        if (strncmp(endValue, value, strlen(endValue)) == 0) {
            break;
        }
    }
    
    return bknMethod;
} 


/* 
 * Reads a batch kinetics file (.bkn) into a file structure.
 *
 * bknFile -- BKN file structure.
 * filePath -- Path to the file.
 * 
 * Returns:
 * true if file is read successfully; false otherwise.
 */
static bool load_bkn_file(struct bkn_file* bknFile, char* filePath) {
    // Open the file
    FILE* fileHandle = fopen(filePath, "rb");

    if (fileHandle == NULL) {
        fprintf(stderr, "Unable to open '%s'.\n", filePath);
        return false;
    }

    // Get the file size in bytes
    fseek(fileHandle, 0, SEEK_END);
    long fileSize = ftell(fileHandle);
    rewind(fileHandle);
    
    // Allocate memory for the whole file 
    uint8_t* buffer = malloc(fileSize * sizeof(uint8_t));

    if(buffer == NULL) {
        out_of_memory();
    }

    // Read the file into memory
    size_t bytesRead = fread(buffer, 1, fileSize, fileHandle);
    fclose(fileHandle);

    if (bytesRead != fileSize) {
        fprintf(stderr, "Error reading %s.\n", filePath);
        free(buffer);
        return false;
    }
    
    bknFile->buffer = buffer;
    bknFile->size = (size_t)fileSize;
    bknFile->offset = 0;

    return true;
}


/**
 * Reads method data from a BKN file.
 * 
 * filePath -- Path to .BKN file.
 * 
 * Returns: 
 * true if read was successful; false otherwise.
 */ 
bool read_bkn(char* filePath, struct bkn_data* bknData) {
    // Load the file into memory
    struct bkn_file bknFile; 
    memset(&bknFile, 0, sizeof(struct bkn_file));
    
    if (!load_bkn_file(&bknFile, filePath)) {
        return false;
    }

    // Scan the file from top to bottom for method data 
    char* methodStartValue = "TContinuumStore";
    while (1) {
        if (!search_bkn_file(&bknFile, (uint8_t*)methodStartValue, strlen(methodStartValue))) {
            // No more method data in file
            break;   
        }
        
        bknData->methods = realloc(bknData->methods, (bknData->numMethods + 1) * sizeof(struct bkn_method));

        if (bknData->methods == NULL) {
            out_of_memory();
        }
        
        read_bkn_method(&bknFile, &bknData->methods[bknData->numMethods]);
        bknData->numMethods++;
    }

    return true;
}
