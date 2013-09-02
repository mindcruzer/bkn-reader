/* Copyright (C) 2012 Sean D. Stewart (sdstewar@gmail.com)
 * All rights reserved. 
 * 
 * Provides json_serialize, which converts a collection of dSets 
 * to a JSON string representation. 
 *
 */


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "read_bkn.h"
#include "json.h"

#define SB_INIT_SIZE 17

#define out_of_memory()                               \
            fprintf(stderr, "Ran out of memory.\n");  \
            exit(EXIT_FAILURE);


/*
 * Tracks the size and status of a string buffer.
 */
typedef struct {
    char *start;
    char *cur;
    char *end;
} SB;


/*
 * Initializes a new string buffer. 
 *
 * sb -- A pointer to a string buffer to initialize.
 *
 */
static void sb_init(SB *sb) 
{
    sb->start = (char*) malloc(SB_INIT_SIZE);

    if (sb->start == NULL) {
        out_of_memory();
    }
    
    sb->cur = sb->start;
    sb->end = sb->start + (SB_INIT_SIZE - 1);
}


/*
 * Grows a string buffer.
 *
 * sb -- A previously initialized string buffer to grow. 
 * need -- The minimum number of bytes to grow the string by.
 *
 */
static void sb_grow(SB *sb, int need) 
{
    size_t length = sb->cur - sb->start;
    size_t alloc = sb->end - sb->start;

    do {
        alloc *= 2;
    } while (alloc < (length + need));

    sb->start = (char*)realloc(sb->start, alloc + 1);
    
    if (sb->start == NULL) {
        out_of_memory();
    }

    sb->cur = sb->start + length;
    sb->end = sb->start + alloc;
}


/*
 * Grows a string buffer if there are not enough bytes to accommodate a need. 
 *
 * sb -- A previously initialized string buffer.
 * need -- The number of bytes needed in sb. 
 *
 */
static void sb_need(SB* sb, int need)
{
    if ((sb->end - sb->cur) < need) {
        sb_grow(sb, need);
    }
}


/*
 * Copies a set of bytes to the end of a string buffer. 
 *
 * sb -- A previously initialized string buffer.
 * bytes -- A pointer to an array of bytes that will be copied to sb.
 * count -- The number of bytes to copy to sb.
 *
 */
static void sb_put(SB *sb, const char* bytes, int count)
{
    sb_need(sb, count);
    memcpy(sb->cur, bytes, count);
    sb->cur += count;
}


/*
 * Copies a string to the end of a string buffer. 
 *
 * sb -- A previously initialized string buffer.
 * str -- A pointer to a string that will be copied to sb.
 *
 */
static void sb_puts(SB *sb, const char *str)
{
    sb_put(sb, str, strlen(str));
}


/*
 * Copies a single character to the end of a string buffer.
 *
 * sb -- A previously initialized string buffer. 
 * c -- The char to copy to the end of sb. 
 *
 */
static void sb_putc(SB *sb, char c)
{
    if (sb->cur >= sb->end) {
        sb_grow(sb, 1);
    }

    *sb->cur++ = c;
}


/*
 * Stringifies a float and appends it to a string buffer.
 *
 * sb -- A previously initialized string buffer.
 * num -- The float to append to sb.
 *
 */
static void sb_putf(SB *sb, float num)
{
	char buf[64];
	sprintf(buf, "%.10f", num);
	sb_puts(sb, buf);
}


/*
 * Stringifies a dField with the name and value, then appends it to a string buffer.
 *
 * sb -- A previously initialized string buffer.
 * field -- The dField to append to sb.
 *
 */
static void sb_put_nv_field(SB *sb, dField* field)
{
    sb_putc(sb, '\"');
    sb_puts(sb, field->name);         // 'field name'
    sb_putc(sb, '\"');

    sb_putc(sb, ':');                 // :
    
    sb_putc(sb, '\"');
    sb_puts(sb, field->value);        // 'field value'
    sb_putc(sb, '\"');
}


/*
 * Stringifies a dField with the name, value, and units, then appends it to a string buffer.
 *
 * sb -- A previously initialized string buffer.
 * field -- The dField to append to sb.
 *
 */
static void sb_put_nvu_field(SB *sb, dField* field)
{
    sb_putc(sb, '\"');
    sb_puts(sb, field->name);         // 'field name'
    sb_putc(sb, '\"');

    sb_putc(sb, ':');                 // :
    
    sb_putc(sb, '{');                 //     {   

    sb_putc(sb, '\"');
    sb_puts(sb, "value");             //       'value'
    sb_putc(sb, '\"');
    
    sb_putc(sb, ':');                 //       :  

    sb_putc(sb, '\"');
    sb_puts(sb, field->value);        //       'field value'
    sb_putc(sb, '\"');
    
    sb_putc(sb, ',');                 //       ,

    sb_putc(sb, '\"');
    sb_puts(sb, "units");             //       'units'
    sb_putc(sb, '\"');
    
    sb_putc(sb, ':');                 //       :

    sb_putc(sb, '\"');
    sb_puts(sb, field->units);        //       'field units'
    sb_putc(sb, '\"');
    
    sb_putc(sb, '}');                 //    }
}


/*
 * Stringifies a dField with the name, value, and units, then appends it to a string buffer.
 *
 * sb -- A previously initialized string buffer.
 * field -- The dField to append to sb.
 *
 */
static void sb_put_meta_field(SB *sb, dField* field)
{
    
    sb_putc(sb, '{');                 //     {   

    sb_putc(sb, '\"');
    sb_puts(sb, "name");             //       'name'
    sb_putc(sb, '\"');
    
    sb_putc(sb, ':');                 //       :  

    sb_putc(sb, '\"');
    sb_puts(sb, field->name);        //       'field name'
    sb_putc(sb, '\"');
    
    sb_putc(sb, ',');                 //       ,
    
    sb_putc(sb, '\"');
    sb_puts(sb, "value");             //       'value'
    sb_putc(sb, '\"');
    
    sb_putc(sb, ':');                 //       :  

    sb_putc(sb, '\"');
    sb_puts(sb, field->value);        //       'field value'
    sb_putc(sb, '\"');
    
    sb_putc(sb, ',');                 //       ,

    sb_putc(sb, '\"');
    sb_puts(sb, "units");             //       'units'
    sb_putc(sb, '\"');
    
    sb_putc(sb, ':');                 //       :

    sb_putc(sb, '\"');
    sb_puts(sb, field->units);        //       'field units'
    sb_putc(sb, '\"');
    
    sb_putc(sb, '}');                 //    }
}


/*
 * Stringifies a dPoint and appends it to a string buffer.
 *
 * sb -- A previously initialized string buffer.
 * point -- The dPoint to append to sb.
 *
 */
static void sb_put_point(SB *sb, dPoint* point)
{
    sb_putc(sb, '[');                 //     [   

    sb_putf(sb, point->abs);          //       abs value
    
    sb_putc(sb, ',');                 //       ,
    
    sb_putf(sb, point->time);         //       time value

    sb_putc(sb, ']');                 //    ]
}


/*
 * Stringifies a dSet and appends it to a string buffer.
 *
 * sb -- A previously initialized string buffer.
 * set -- A pointer to a dSet to add to sb.
 *
 */
static void sb_put_dset(SB* sb, dSet* set) 
{
    sb_putc(sb, '{');
    
        // standard fields
        sb_put_nv_field(sb, &set->name);
        sb_putc(sb, ',');
            
        sb_put_nv_field(sb, &set->collection_time);
        sb_putc(sb, ',');
    
        sb_put_nvu_field(sb, &set->wavelength);
        sb_putc(sb, ',');
        
        // data points
        sb_putc(sb, '\"');
        sb_puts(sb, "points");
        sb_putc(sb, '\"');
        
        sb_putc(sb, ':');
        
        sb_putc(sb, '[');
        
            int i;
            for (i = 0; i < set->num_points; i++) {
                sb_put_point(sb, set->points[i]);

                if (i != set->num_points - 1) {
                    sb_putc(sb, ',');
                }
            }
        
        sb_putc(sb, ']');
        sb_putc(sb, ',');

        // metadata fields
        // TODO: this should really be a list, since we're just going to iterate over 
        // the meta fields when we display them
        sb_putc(sb, '\"');
        sb_puts(sb, "meta");
        sb_putc(sb, '\"');
        
        sb_putc(sb, ':');
        
        sb_putc(sb, '[');
        
        sb_put_meta_field(sb, &set->operator_name);
        sb_putc(sb, ',');
        
        sb_put_meta_field(sb, &set->kinetics_version);
        sb_putc(sb, ',');
        
        sb_put_meta_field(sb, &set->param_list);
        sb_putc(sb, ',');
        
        sb_put_meta_field(sb, &set->instrument);
        sb_putc(sb, ',');
        
        sb_put_meta_field(sb, &set->instrument_version);
        sb_putc(sb, ',');
        
        sb_put_meta_field(sb, &set->ordinate_mode);
        sb_putc(sb, ',');
        
        sb_put_meta_field(sb, &set->sbw);
        sb_putc(sb, ',');
        
        sb_put_meta_field(sb, &set->ave_time);
        sb_putc(sb, ',');
        
        sb_put_meta_field(sb, &set->dwell_time);
        sb_putc(sb, ',');
        
        sb_put_meta_field(sb, &set->cycle_time);
        sb_putc(sb, ',');
        
        sb_put_meta_field(sb, &set->stop_time);
        
        sb_putc(sb, ']');
        
    sb_putc(sb, '}');
}


/*
 * Finishes a string buffer by adding on the null character.
 *
 * sb -- A previously initialized string buffer to finish. 
 *
 */
static char* sb_finish(SB *sb) 
{
    *(sb->cur) = 0;
    assert(sb->start <= sb->cur && strlen(sb->start) == (size_t)(sb->cur - sb->start));
    return sb->start;
}


/*
 * JSON serializes a collection of dSets.
 *
 * sets -- A collection of dSet objects.
 * count -- The number of dSets in sets.
 *
 */
char* json_serialize(dSet** sets, int count)
{
    SB sb;

    sb_init(&sb);

    sb_putc(&sb, '[');

    int i;
    for (i = 0; i < count; i++) {
        sb_put_dset(&sb, sets[i]);
        
        if (i != count - 1) {
            sb_putc(&sb, ',');
        }
    }
    
    sb_putc(&sb, ']');
    
    sb_finish(&sb);

    return sb.start;
}
