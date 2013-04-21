#ifndef _LOG_H
#define _LOG_H
#include <stdint.h>


typedef enum {
  LVL_DEBUG = 0,
  LVL_INFO = 1,
  LVL_WARN = 2,
  LVL_ERROR = 3,
  LVL_CRITICAL = 4,
} log_level_e;

#define LOG(args...)           (log_message(LVL_INFO, ##args))
#define LOG_DEBUG(args...)     (log_message(LVL_DEBUG, ##args))
#define LOG_INFO(args...)      (log_message(LVL_INFO, ##args))
#define LOG_ERROR(args...)     (log_message(LVL_ERROR, ##args))
#define LOG_CRITICAL(args...)  (log_message(LVL_CRITICAL, ##args))

#ifndef LOG_BUFFER_SIZE
#define LOG_BUFFER_SIZE (256)
#endif

typedef struct {
  char serbuf[LOG_BUFFER_SIZE];
  int bytes_buffered;
} log_serial_state_t;
void log_init();
void log_message(log_level_e lvl, char *fmt, ...);
void log_service(void);
void log_start(void);

#endif
