#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "json.h"

#define SB_INIT_SIZE 17


typedef struct {
    char *start;
    char *cur;
    char *end;
} SB;


/**
 * Initializes a new string buffer. 
 *
 * sb -- A pointer to a string buffer to initialize.
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


/**
 * Grows a string buffer.
 *
 * sb -- A previously initialized string buffer to grow. 
 * need -- The minimum number of bytes to grow the string by.
 */
static void sb_grow(SB *sb, uint32_t need) 
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


/**
 * Grows a string buffer if there are not enough bytes to accommodate a need. 
 *
 * sb -- A previously initialized string buffer.
 * need -- The number of bytes needed in sb. 
 */
static void sb_need(SB* sb, uint32_t need)
{
    if ((sb->end - sb->cur) < need) {
        sb_grow(sb, need);
    }
}


/**
 * Copies a set of bytes to the end of a string buffer. 
 *
 * sb -- A previously initialized string buffer.
 * bytes -- A pointer to an array of bytes that will be copied to sb.
 * count -- The number of bytes to copy to sb.
 */
static void sb_put(SB *sb, const char* bytes, uint32_t count)
{
    sb_need(sb, count);
    memcpy(sb->cur, bytes, count);
    sb->cur += count;
}


/**
 * Copies a string to the end of a string buffer. 
 *
 * sb -- A previously initialized string buffer.
 * str -- A pointer to a string that will be copied to sb.
 */
static void sb_puts(SB *sb, const char *str)
{
    sb_put(sb, str, strlen(str));
}


/**
 * Copies a single character to the end of a string buffer.
 *
 * sb -- A previously initialized string buffer. 
 * c -- The char to copy to the end of sb. 
 */
static void sb_putc(SB *sb, char c)
{
    if (sb->cur >= sb->end) {
        sb_grow(sb, 1);
    }

    *sb->cur++ = c;
}


/**
 * Stringifies a float and appends it to a string buffer.
 *
 * sb -- A previously initialized string buffer.
 * num -- The float to append to sb.
 */
static void sb_putf(SB *sb, float num)
{
	char buf[64];
	sprintf(buf, "%.10f", num);
	sb_puts(sb, buf);
}


/**
 * Finishes a string buffer by adding on the null character.
 *
 * sb -- A previously initialized string buffer to finish. 
 */
static char* sb_finish(SB *sb) 
{
    *(sb->cur) = 0;
    assert(sb->start <= sb->cur && strlen(sb->start) == (size_t)(sb->cur - sb->start));
    return sb->start;
}


/**
 * Stringifies a bkn method and appends it to a string buffer.
 *
 * sb -- A previously initialized string buffer.
 * set -- A pointer to a dSet to add to sb.
 */
static void sb_put_bkn_method(SB* sb, struct bkn_method* bknMethod) 
{
    sb_putc(sb, '{');
    sb_putc(sb, '\"');
    sb_puts(sb, "points");
    sb_putc(sb, '\"');
    sb_putc(sb, ':');
    sb_putc(sb, '[');

    uint32_t i;
    for (i = 0; i < bknMethod->numPoints; i++) {
        sb_putc(sb, '{');
        sb_putc(sb, '\"');
        sb_puts(sb, "time");
        sb_putc(sb, '\"');
        sb_putc(sb, ':');
        sb_putf(sb, bknMethod->points[i].absorbance);
        sb_putc(sb, ',');             
        sb_putc(sb, '\"');
        sb_puts(sb, "absorbance");
        sb_putc(sb, '\"');
        sb_putc(sb, ':');
        sb_putf(sb, bknMethod->points[i].time); 
        sb_putc(sb, '}');

        if (i != bknMethod->numPoints - 1) {
            sb_putc(sb, ',');
        }
    }
    
    sb_putc(sb, ']');
    sb_putc(sb, ',');
    sb_putc(sb, '\"');
    sb_puts(sb, "metadata");
    sb_putc(sb, '\"');
    sb_putc(sb, ':');
    sb_putc(sb, '[');
    
    for (i = 0; i < bknMethod->numMetadata; i++) {
        sb_putc(sb, '\"');
        sb_puts(sb, bknMethod->metadata[i]); 
        sb_putc(sb, '\"');
        
        if (i != bknMethod->numMetadata - 1) {
            sb_putc(sb, ',');
        }
    }
    
    sb_putc(sb, ']');
    sb_putc(sb, '}');
}


/**
 * JSON serializes BKN data.
 *
 * data -- BKN data.
 */
char* json_serialize(struct bkn_data* bknData) 
{
    SB sb;
    sb_init(&sb);
    sb_putc(&sb, '[');

    uint32_t i;
    for (i = 0; i < bknData->numMethods; i++) {
        sb_put_bkn_method(&sb, bknData->methods[i]);
        
        if (i != bknData->numMethods - 1) {
            sb_putc(&sb, ',');
        }
    }
    
    sb_putc(&sb, ']');
    sb_finish(&sb);

    return sb.start;
}
