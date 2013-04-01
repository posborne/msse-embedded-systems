#include <pololu/orangutan.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include "log.h"

static log_serial_state_t g_serial_state;

/* assuming the use of USB_COMM */
void log_init() {
	g_serial_state.bytes_buffered = 0;
}

void log_message(log_level_e lvl, char *fmt, ...) {
	int msg_len;
	char tmpbuf[LOG_BUFFER_SIZE];
    va_list args;
    va_start(args, fmt);
    vsprintf(tmpbuf, fmt, args);
    msg_len = strlen(tmpbuf);
    va_end(args);
    strncpy(g_serial_state.serbuf, tmpbuf, msg_len);
    serial_send_blocking(USB_COMM, g_serial_state.serbuf, msg_len);
}
