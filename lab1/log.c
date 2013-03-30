#include <pololu/orangutan.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "log.h"

static log_serial_state_t g_serial_state;

/* assuming the use of USB_COMM */
void log_init() {
	g_serial_state.bytes_buffered = 0;
}

void log_service() {
	serial_check();
}

void log_message(log_level_e lvl, char *fmt, ...) {
	int sent_bytes;
	int msg_len;
	char tmpbuf[LOG_BUFFER_SIZE];

	/* service any bytes that we might have gotten recently */
	if ((sent_bytes = serial_get_sent_bytes(USB_COMM)) > 0) {
		g_serial_state.bytes_buffered -= sent_bytes;
		if (g_serial_state.bytes_buffered > 0) {
			strncpy(&g_serial_state.serbuf[sent_bytes],
					&g_serial_state.serbuf[0], g_serial_state.bytes_buffered);
		}
	}

	va_list args;
	va_start(args, fmt);
	vsprintf(tmpbuf, fmt, args);

	// at this point everything there will be bytes_buffered
	// bytes sitting at the start of the buffer
	msg_len = strlen(tmpbuf);
	if ((msg_len + 2 + g_serial_state.bytes_buffered)
			> sizeof g_serial_state.serbuf) {
		g_serial_state.bytes_buffered = 0;
	}
	strncpy(&g_serial_state.serbuf[g_serial_state.bytes_buffered], tmpbuf,
			msg_len);
	va_end(args);

	g_serial_state.serbuf[g_serial_state.bytes_buffered + msg_len] = '\r';
	g_serial_state.serbuf[g_serial_state.bytes_buffered + msg_len + 1] = '\n';
	g_serial_state.bytes_buffered += (msg_len + 2);
	serial_send_blocking(USB_COMM, g_serial_state.serbuf, g_serial_state.bytes_buffered);
}
