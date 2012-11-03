/*
 * debug.h
 *
 *  Created on: 2012/04/22
 *      Author: yasu
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEBUGF(format, ...) gprintf("" __FILE__ " [%4d, %s()] :" format, __LINE__, __func__ , ## __VA_ARGS__)
#define DEBUGC(format, ...) gprintf(format, ## __VA_ARGS__) // contine from last line

void gdebug_to_stderr(void);
bool gopen(const char *filepath, const char *mode);
int gprintf(const char *format, ...);
bool gclose(void);
bool gdebug_enabled(void);

#ifdef __cplusplus
}
#endif

#endif /* DEBUG_H_ */
