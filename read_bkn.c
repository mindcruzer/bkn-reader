/* 
 * Copyright (C) 2012 Sean D. Stewart (sdstewar@gmail.com)
 * All rights reserved. 
 * 
 * This program reads the absorbance/time data out of .BKN files produced
 * by Varian Cary WinUV software. 
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json.h"
#include "read_bkn.h"


struct bkn_file {
    uint8_t *buffer;
    size_t size;
    size_t offset;
};


/**
 * Searches a buffer for the first occurrence of a target sequence of bytes. 
 *
 * buffer -- The buffer to search.
 * bufferSize -- The size of `buffer`.
 * target -- The sequence of bytes locate in `buffer`.
 * targetSize -- The size of `target`.
 * offset -- An offset to start the search from.
 *
 * Returns:
 * The index immediately following `target` in `buffer` if `target` is found; -1 otherwise.
 */
static size_t search_buffer(uint8_t *buffer, size_t bufferSize, uint8_t *target, size_t targetSize, size_t offset) {
    size_t bufferIndex = 0;
    size_t targetIndex = 0; 

    for (bufferIndex = offset; bufferIndex < bufferSize; bufferIndex++) {
        if (buffer[bufferIndex] == target[targetIndex]) { 
            if (++targetIndex == targetSize)
                return bufferIndex + 1;
        }
        else {
            if (targetIndex != 0)
                bufferIndex -= targetIndex;
            targetIndex = 0;
        }
    }

    return -1;
}


/**
 * Reads a float from the current offset.
 * 
 * file -- BKN file structure.
 * 
 * Returns:
 * Float value.
 */
static float read_float(struct bkn_file* file) {
    float value; 
    memcpy(&value, file->buffer + file->offset, sizeof(float));
    file->offset += sizeof(float);
    return value;
}


/*
 * Reads the absorbance/time points from the current offset.
 *
 * file -- BKN file structure.
 * numPoints -- Number of points to read.
 * 
 * Returns:
 * Array of points.
 */
static struct data_point* read_points(struct bkn_file* file, int numPoints) {
    struct data_point* points = malloc(numPoints * sizeof(struct data_point));

    int32_t i;
    for (i = 0; i < numPoints; i++) {
        // A point is a pair of single-precision floating point values
        points[i].absorbance = read_float(file);
        points[i].time = read_float(file);
    }

    return points;
}


/*
 * Reads a metadata field from the current offset.
 *
 * file -- BKN file structure.
 *
 * Returns:
 * Field value. 
 */
static char* read_metadata_field(struct bkn_file* file) {
    // The field is preceded by a 32-bit integer indicating the number of 
    // bytes in the field
    int32_t fieldLength;
    memcpy(&fieldLength, file->buffer + file->offset, 4);
    file->offset += 4;

    // Read in the field value
    char* value = malloc((fieldLength * sizeof(char)) + 1);
    memcpy(value, file->buffer + file->offset, fieldLength);
    file->offset += fieldLength;
    value[fieldLength] = '\0';

    return value;
}


/*
 * Reads method data from the current offset.
 *
 * file -- BKN file structure.
 *
 * Returns:
 * Data set.
 */
static struct data_set* extract_method_data(struct bkn_file* file) {
    struct data_set* dataSet = malloc(sizeof(struct data_set));
    memset(dataSet, 0, sizeof(struct data_set));

    // Seek to the number of points in the method
    file->offset += 0x1C; 

    // The number of points is indicated with a 32-bit integer
    memcpy(&dataSet->numPoints, file->buffer + file->offset, 4);

    // Seek to the start of the points
    file->offset += 0x3EC;
    dataSet->points = read_points(file, dataSet->numPoints);

    // Read in the metadata
    char* endValue = "End Method";
    while(true) {
        char* value = read_metadata_field(file);
        
        dataSet->metadata = realloc(dataSet->metadata, (dataSet->numMetadata + 1) * sizeof(char*));
        dataSet->metadata[dataSet->numMetadata] = value;
        dataSet->numMetadata++;

        if (strncmp(endValue, value, strlen(endValue)) == 0) {
            break;
        }
    }
    
    return dataSet;
} 


/* 
 * Reads a batch kinetics file (.bkn) into a file structure.
 *
 * file -- BKN file structure.
 * path -- Path to the file.
 * 
 * Returns:
 * true if file is read successfully; false otherwise.
 */
static bool load_file(struct bkn_file* file, char* path) {
    // Attempt to open the file
    FILE* fileHandle = fopen(path, "rb");

    if (fileHandle == NULL) {
        fprintf(stderr, "Unable to open '%s'.\n", path);
        return false;
    }

    // Get the file size
    fseek(fileHandle, 0, SEEK_END);
    long fileSize = ftell(fileHandle);
    rewind(fileHandle);
    
    // Allocate memory for the whole file 
    uint8_t* buffer = malloc(fileSize * sizeof(uint8_t));

    if(buffer == NULL) {
        fprintf(stderr, "Not enough memory to read the file.\n");
        return false;
    }

    // Read the file into memory
    size_t bytesRead = fread(buffer, 1, fileSize, fileHandle);
    fclose(fileHandle);

    if (bytesRead != fileSize) {
        fprintf(stderr, "Error reading %s.\n", path);
        free(buffer);
        return false;
    }
    
    file->buffer = buffer;
    file->size = fileSize;
    file->offset = 0;

    return true;
}


/*
 * Application entry point.
 *
 * Args:
 * [1] -- Path to the BKN file.
 *
 */
int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Specify a single BKN file.\n");
        return EXIT_FAILURE;
    }
    
    // Read the file 
    struct bkn_file file; 
    if (!load_file(&file, argv[1])) {
        return EXIT_FAILURE;
    }

    // Find each method 
    char* methodStartValue = "TContinuumStore";
    struct data_set** dataSets = NULL;
    int numDataSets = 0;
    while (1) {
        file.offset = search_buffer(file.buffer, file.size, (uint8_t*)methodStartValue, strlen(methodStartValue), file.offset);

        if (file.offset == -1) {
            // End of methods
            break;
        }
        
        dataSets = realloc(dataSets, (numDataSets + 1) * sizeof(struct data_set*));

        if (dataSets == NULL) {
            printf("Memory allocation failed :(\n.");
            return EXIT_FAILURE;
        }
        
        dataSets[numDataSets] = extract_method_data(&file);
        numDataSets++;
    }

    // Output as JSON string
    char* json = json_serialize(dataSets, numDataSets);
    printf("%s\n", json);
    
    return EXIT_SUCCESS;
}
