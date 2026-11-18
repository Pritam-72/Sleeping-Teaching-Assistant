#ifndef UTIL_H
#define UTIL_H

#include "shared.h"

void init_logger(void);
void destroy_logger(void);
void log_event(const char *fmt, ...);
void random_sleep_ms(int min_ms, int max_ms, unsigned int *seed);

int semx_init(SemX *s, unsigned int value, const char *tag);
void semx_destroy(SemX *s);
void semx_wait(SemX *s);
void semx_post(SemX *s);

#endif /* UTIL_H */
