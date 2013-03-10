#ifndef _LOG_H
#define _LOG_H
#include <stdint.h>

#ifndef LOG_DISABLE_LOGGING
#define LOG(lvl, fmt, args...) (log_message(lvl, fmt, ##args))
#else
#define LOG(lvl, msg) ()
#endif
#define printf(args...) (LOG(LVL_DEBUG, ##args))

#ifndef LOG_BUFFER_SIZE
#define LOG_BUFFER_SIZE (256)
#endif

typedef struct {
  char serbuf[LOG_BUFFER_SIZE];
  int bytes_buffered;
} log_serial_state_t;

typedef enum {
  LVL_DEBUG = 0,
  LVL_INFO = 1,
  LVL_WARN = 2,
  LVL_ERROR = 3,
  LVL_CRITICAL = 4
} log_level_e;

void log_init();
void log_service();
void log_message(log_level_e lvl, char *fmt, ...);

#endif
