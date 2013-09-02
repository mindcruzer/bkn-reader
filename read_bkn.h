/* Copyright (C) 2012 Sean D. Stewart (sdstewar@gmail.com)
 * All rights reserved. 
 */

#define FIELD_SIZE 105
#define UNIT_SIZE 5

/*
 * Represents an absorbance/time data point.
 */
typedef struct {
    float time; 
    float abs;
} dPoint;


/*
 * Represents a metadata field. 
 */
typedef struct {
    char name[FIELD_SIZE];
    char value[FIELD_SIZE];
    char units[UNIT_SIZE];
} dField;


/*
 * Represents a set of absorbance/time data points.
 */
typedef struct {
    // metadata
    dField name;
    dField collection_time;
    dField operator_name;
    dField kinetics_version;
    dField param_list;
    dField instrument;
    dField instrument_version;
    dField wavelength;
    dField ordinate_mode;
    dField sbw;
    dField ave_time;
    dField dwell_time;
    dField cycle_time;
    dField stop_time;

    // abs/time points
    dPoint **points;
    int num_points;
} dSet;

