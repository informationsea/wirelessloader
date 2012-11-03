/*
 * debug.c
 *
 *  Created on: 2012/04/22
 *      Author: yasu
 */

#include <stdio.h>
#include <stdarg.h>

#include "debug.h"

static FILE *debug_file = NULL;
static bool write_to_stderr = false;

void gdebug_to_stderr(void)
{
    write_to_stderr = true;
}

bool gopen(const char *filepath, const char *mode)
{
    debug_file = fopen(filepath, mode);
    if (!debug_file) return false;
    return true;
}

int gprintf(const char *format, ...)
{
    va_list list;
    int ret;
    va_start(list, format);
    if (write_to_stderr) ret = vfprintf(stderr, format, list);
    if (debug_file) ret = vfprintf(debug_file, format, list);
    va_end(list);
    return ret;
}

bool gclose(void)
{
    if (fclose(debug_file)) return false;
    debug_file = NULL;
    return true;
}


bool gdebug_enabled(void){
    return debug_file || write_to_stderr;
}
