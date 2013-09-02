/* Copyright (C) 2012 Sean D. Stewart (sdstewar@gmail.com)
 * All rights reserved. 
 * 
 * This program reads the absorbance/time data out of .BKN files produced
 * by Varian Cary WinUV software. 
 * 
 * Use:
 * read_bkn [filename]
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pcre.h>

#include "read_bkn.h"
#include "json.h"


/*
 * These define the maximum size of a metadata field, and the maximum size of the units 
 * descriptor for the value of that field, respectively. 
 */
#define COLLECTION_FIELD 0  // [field name]: [field value]
#define METHOD_FIELD 1  // [field name] ([units]) [whitespace] [field value] [whitespace]
#define VALUE_FIELD 2 // [field value]


/*
 * Represents a batch kinetics file (.bkn) buffer and it's size. 
 */
typedef struct {
    char *buffer;
    size_t size;
} kFile;


/*
 * Trims whitespace from a string while keeping 
 * the buffer size constant.
 *
 * s -- A pointer to the string to trim.
 *
 */
static void trim(char * s) {
    char * p = s;
    int l = strlen(p);

    while(isspace(p[l - 1])) p[--l] = 0;
    while(* p && isspace(* p)) ++p, --l;

    memmove(s, p, l + 1);
}


/*
 * Searches a buffer for the first occurrence of a given sequence of bytes. 
 *
 * buf -- The buffer to search.
 * bufSize -- The size of buf.
 * seq -- The sequence of bytes locate in buf.
 * seqSize -- The size of seq.
 * offset -- An offeset from which to begin searching. 
 *
 * Returns:
 * The index immediately following seq in the buffer if seq is found, -1 if
 * seq is not found.
 *
 */
static long buf_find(char *buf, size_t bufSize, char *seq, size_t seqSize, long offset) {
    
    long bufPos, seqPos = 0; 
    for (bufPos = offset; bufPos < bufSize; bufPos++) {
        if (buf[bufPos] == seq[seqPos]) { 
            if (++seqPos == seqSize)
                return bufPos + 1;
        }
        else {
            if (seqPos != 0)
                bufPos -= seqPos;
            seqPos = 0;
        }
    }

    return -1;
}


/*
 * Extracts the absorbance time data from a kFile. 
 *
 * file -- The kFile.
 * start -- The start index of the data in the kFile. 
 * end -- The end index of the data in the kFile.
 * set -- A pointer to the dSet to read the data into.
 *
 */
static void read_points(kFile *file, long start, int num_points, dSet *set) {
    
    dPoint **points = (dPoint**)malloc(sizeof(dPoint *)*num_points);

    int i;
    for (i = 0; i < num_points; i++) {
        dPoint *point = (dPoint*)malloc(sizeof(dPoint));
        memcpy(point, file->buffer + start, 8);
        points[i] = point;
        start += 8;
    }

    set->points = points;
}


/*
 * Extracts a metadata field from a kFile. 
 *
 * file -- The kFile.
 * offset -- The number of bytes from the start of kFile that the field begins.
 * field -- A pointer to the dField that the field data will be stored in.
 * field_type -- The type of field being read.
 *
 * Returns:
 * The number of bytes read. 
 *
 */
// TODO: field_name will be removed once we make this function read in field names as well
static int read_field(kFile *file, long offset, char* field_name, dField *field, int field_type) {

    int field_length;
    char field_contents[FIELD_SIZE];
    pcre *re;
    const char *error;
    int erroffset;
    int rc;
    int ovector[30];
    
    // TODO: Will be replaced when this function reads in the field name as well
    memcpy(field->name, field_name, strlen(field_name) + 1);
    

    // the length of the field is the first 4 bytes
    memcpy(&field_length, file->buffer + offset, 4);
    
    // get the contents of the field 
    if (field_length >= FIELD_SIZE) {
        memcpy(field_contents, file->buffer + offset + 4, FIELD_SIZE - 1);
        field_contents[FIELD_SIZE - 1] = '\0';
    }
    else {
        memcpy(field_contents, file->buffer + offset + 4, field_length);
        field_contents[field_length] = '\0';
    }
        
    if (field_type == COLLECTION_FIELD) { // match value
        // match value
        char *value_pattern = ":(\\s+.*)"; 
        re = pcre_compile(value_pattern, 0, &error, &erroffset, NULL);    

        if (re == NULL) {
            fprintf(stderr, "Unable to compile regex. Error message: %s\n", error);
            exit(EXIT_FAILURE);
        }
    
        rc = pcre_exec(re, NULL, field_contents, strlen(field_contents), 0, 0, ovector, 30); 
        pcre_free(re);
        
        if (rc >= 1) {
            if (rc == 2) {
                pcre_copy_substring(field_contents, ovector, rc, 1, field->value, FIELD_SIZE);
                trim(field->value);
            }
            else {
                field->value[0] = '\0';
            }
        }
        else {
            fprintf(stderr, "Pattern not matched. Error code: %d\n", rc);
            exit(EXIT_FAILURE);
        }
        
        // there are no units for this type of field
        field->units[0] = '\0';
    }
    else if (field_type == VALUE_FIELD) {  // copy the value
        memcpy(field->value, field_contents, strlen(field_contents) + 1);

        // there are no units for this type of field
        field->units[0] = '\0';
    }
    else { // match value and units (if any)
        char *value_pattern = "\\s{3,}(.+)";
        re = pcre_compile(value_pattern, 0, &error, &erroffset, NULL);            
        
        if (re == NULL) {
            fprintf(stderr, "Unable to compile regex. Error message: %s\n", error);
            exit(EXIT_FAILURE);
        }

        rc = pcre_exec(re, NULL, field_contents, strlen(field_contents), 0, 0, ovector, 30); 
        pcre_free(re);

        if (rc >= 1) {
            if (rc == 2) {
                pcre_copy_substring(field_contents, ovector, rc, 1, field->value, FIELD_SIZE);
                trim(field->value);
            }
            else {
                field->value[0] = '\0';
            }
        }
        else {
            fprintf(stderr, "Pattern not matched. Error code: %d\n", rc);
            exit(EXIT_FAILURE);
        }

        // match units
        char *units_pattern = "\\((.+)\\)";
        re = pcre_compile(units_pattern, 0, &error, &erroffset, NULL);            
        
        if (re == NULL) {
            fprintf(stderr, "Unable to compile regex. Error message: %s\n", error);
            exit(EXIT_FAILURE);
        }
        
        rc = pcre_exec(re, NULL, field_contents, strlen(field_contents), 0, 0, ovector, 30); 
        pcre_free(re);

        if (rc == 2) {
            pcre_copy_substring(field_contents, ovector, rc, 1, field->units, UNIT_SIZE);
            trim(field->units);
        }
        else {
            field->units[0] = '\0';
        }
    }
    
    return 4 + field_length;
}


/* 
 * Reads a batch kinetics file (.bkn) into a kFile. 
 *
 * path -- The path to the file.
 *
 * Returns:
 * A pointer to a kFile. 
 *
 */
static void read_file(kFile* f, char* path) {

    FILE *file;
    char *fBuf;
    long fSize;
    size_t rResult;

    file = fopen(path, "rb");
    
    if (file == NULL) {
        fprintf(stderr, "Unable to open '%s'.\n", path);
        exit(EXIT_FAILURE);
    }

    // get the size of the file
    fseek(file, 0, SEEK_END);
    fSize = ftell(file);
    rewind(file);
    
    fBuf = (char *)malloc(sizeof(char)*fSize);

    if(fBuf == NULL) {
        fprintf(stderr, "Unable to allocate memory for kinetics file buffer.\n");
        exit(EXIT_FAILURE);
    }

    rResult = fread(fBuf, 1, fSize, file);
    fclose(file);

    if (rResult != fSize) {
        fprintf(stderr, "Error reading %s into the kinetics file buffer.\n", path);
        free(fBuf);
        exit(EXIT_FAILURE);
    }
    
    f->buffer = fBuf;
    f->size = rResult;
}


/*
 * Prints the contents of a dSet to stdout.
 *
 * set -- The dSet to print.
 *
 */
static void display_set(dSet *set) {
    printf("Name: %s\n", set->name.value);
    printf("Collection Time: %s\n", set->collection_time.value);
    printf("Operator Name: %s\n", set->operator_name.value);
    printf("Kinetics Software Version: %s\n", set->kinetics_version.value);
    printf("Parameter List: %s\n", set->param_list.value);
    printf("Instrument: %s\n", set->instrument.value);
    printf("Instrument Version: %s\n", set->instrument_version.value);
    printf("Wavelength: %s %s\n", set->wavelength.value, set->wavelength.units);
    printf("Ordinate Mode: %s\n", set->ordinate_mode.value);
    printf("SBW: %s %s\n", set->sbw.value, set->sbw.units);
    printf("Averaging Time: %s %s\n", set->ave_time.value, set->ave_time.units);
    printf("Dwell Time: %s %s\n", set->dwell_time.value, set->dwell_time.units);
    printf("Cycle Time: %s %s\n", set->cycle_time.value, set->cycle_time.units);
    printf("Stop Time: %s %s\n", set->stop_time.value, set->cycle_time.units);
    printf("Number of Points: %d\n", set->num_points);
    printf("Time:                 Abs:\n");
    
    int i;
    for (i = 0; i < set->num_points; i++) {
        printf("%.10f          %.10f\n", set->points[i]->time, set->points[i]->abs);
    }
    printf("--------------------------------------------\n");
}


/*
 * Frees the memory consumed by a dSet.
 *
 * set -- The dSet memory to free.
 *
 */
static void free_sets(dSet** sets, int count) 
{
    int i;
    int j;
    
    for (i = 0; i < count; i++) {
        // data points
        for (j = 0; j < sets[i]->num_points; j++) {
            free(sets[i]->points[j]);
        }

        free(sets[i]->points);
        free(sets[i]);
    }

    free(sets);
}


/*
 * Extracts method data starting from a given offset.
 *
 * file -- The kFile containing the BKN data.
 * offset -- The offset in the file from which to begin extracting the 
 * method data. 
 *
 * Returns:
 * A dSet populated with the method data.
 *
 */
static dSet* extract_method_data(kFile *file, long offset) {
    
    dSet *set = (dSet*)malloc(sizeof(dSet));
    
    // jump to the number of points in the method
    offset += 28; 

    int num_points = 0;
    memcpy(&num_points, file->buffer + offset, 4);
    set->num_points = num_points;

    // jump to the start of the data
    offset += 0x3EC;
    read_points(file, offset, num_points, set);
    
    // jump to the start of the metadata
    offset += num_points * 8;

    // TODO: Later on, we could just read in fields generically, and extract the name of the field
    // as a parameter in a dField
    
    // read metadata fields
    offset += read_field(file, offset, "name", &set->name, VALUE_FIELD); 
    offset += read_field(file, offset, "time", &set->collection_time, COLLECTION_FIELD);
    offset += read_field(file, offset, "Operator Name", &set->operator_name, COLLECTION_FIELD);
    offset += read_field(file, offset, "Kinetics Version", &set->kinetics_version, COLLECTION_FIELD);
    offset += read_field(file, offset, "Parameter List", &set->param_list, COLLECTION_FIELD);
    offset += 4;
    offset += read_field(file, offset, "Instrument", &set->instrument, METHOD_FIELD);
    offset += read_field(file, offset, "Instrument Version", &set->instrument_version, METHOD_FIELD);
    offset += read_field(file, offset, "wavelength", &set->wavelength, METHOD_FIELD);
    offset += read_field(file, offset, "Ordinate Mode", &set->ordinate_mode, METHOD_FIELD);
    offset += read_field(file, offset, "SBW", &set->sbw, METHOD_FIELD);
    offset += read_field(file, offset, "Averaging Time", &set->ave_time, METHOD_FIELD);
    offset += read_field(file, offset, "Dwell Time", &set->dwell_time, METHOD_FIELD);
    offset += read_field(file, offset, "Cycle Time", &set->cycle_time, METHOD_FIELD);
    offset += read_field(file, offset, "Stop Time", &set->stop_time, METHOD_FIELD);
    
    return set;
} 

/*
 * Main application.
 *
 * Args:
 * [1] -- The path to the kinetics batch file (.bkn) to read.
 *
 */
int main(int argc, char **argv) {

    if (argc != 2) {
        fprintf(stdout, "Specify a single kinetics file to read.\n");
        return EXIT_SUCCESS;
    }
    
    // TODO: Verify file extension?
    // TODO: Make sure long is big enough for file offsets, if not use size_t
    
    kFile file; 
    char *method_prefix = "TContinuumStore";
    long method_start = 0; 
    dSet** sets = NULL;
    int sets_count = 0;
    
    // read file into a buffer
    read_file(&file, argv[1]);

    while (1) {
        method_start = buf_find(file.buffer, file.size, method_prefix, 15, method_start);

        if (method_start == -1)
            // we're at the end of the file
            break;
        
        sets = (dSet**)realloc(sets, sizeof(dSet*) * (sets_count + 1));
        
        if (sets == NULL) {
            fprintf(stderr, "Unable to allocate memory for data set %d\n.", sets_count);
            exit(EXIT_FAILURE);
        }
        
        sets[sets_count] = extract_method_data(&file, method_start);
        //display_set(sets[sets_count]);

        sets_count++;
    }

    //printf("--------------------------------------------\n");
    
    // output as JSON string
    char* json = json_serialize(sets, sets_count);
    fprintf(stdout, "%s\n", json);
    free(json);
    
    free_sets(sets, sets_count);

    free(file.buffer);

    //printf("Done.\n");
    //printf("Extracted data for %d methods.\n", sets_count);
    
    return EXIT_SUCCESS;
}
