#ifndef UTIL_H
#define UTIL_H

void init_logger(void);
void destroy_logger(void);
void log_event(const char *fmt, ...);
void random_sleep_ms(int min_ms, int max_ms, unsigned int *seed);

#endif /* UTIL_H */
