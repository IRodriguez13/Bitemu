/**
 * Bitemu - Logging
 * Logging y debugging
 */

#ifndef BITEMU_LOG_H
#define BITEMU_LOG_H

void log_info(const char *fmt, ...);
void log_warn(const char *fmt, ...);
void log_error(const char *fmt, ...);
void log_debug(const char *fmt, ...);

#endif /* BITEMU_LOG_H */
