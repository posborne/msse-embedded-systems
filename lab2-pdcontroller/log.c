#include <pololu/orangutan.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include "log.h"
#include "deque.h"

typedef struct {
  int length;
  bool in_use;
  char buf[100];
} write_buffer_t;

static log_serial_state_t g_serial_state;
static struct deque_t deque;
static Deque d = &deque;
static write_buffer_t write_buffers[DEQUE_MAX_NODES];
static bool in_flight = false;
static bool starting = true;
static char SEND_BUFFER[256];

static write_buffer_t *
alloc_buffer(void)
{
  int i = 0;
  write_buffer_t * buf = NULL;
  for (i = 0; i < DEQUE_MAX_NODES; i++) {
    buf = &write_buffers[i];
    if (!buf->in_use) {
      buf->in_use = true;
      return buf;
    }
  }
  return NULL;
}

static void
free_buffer(write_buffer_t * buf)
{
  buf->in_use = false;
}

/* assuming the use of USB_COMM */
void log_init() {
  int i;
  g_serial_state.bytes_buffered = 0;
  deque_init(d, NULL);
//  serial_set_baud_rate(USB_COMM, 115200);
  for (i = 0; i < DEQUE_MAX_NODES; i++)
    write_buffers[i].in_use = false;
}

void log_start() {
	starting = false;
}

void log_service(void) {
	write_buffer_t * buf;
	uint32_t count = deque_count(d);
	if (serial_send_buffer_empty(USB_COMM) && count > 0) {
		/* in flight, free the old */
		if (in_flight) {
			buf = deque_popleft(d);
			free_buffer(buf);
			in_flight = false;
		}

		if (count > 1) { // more than 0 now
			buf = deque_peekleft(d);
			serial_send(USB_COMM, buf->buf, buf->length);
			in_flight = true;
		}
	}
}

void log_message(log_level_e lvl, char *fmt, ...) {
	int msg_len;
	write_buffer_t * buf;
	va_list args;
	va_start(args, fmt);
	vsprintf(SEND_BUFFER, fmt, args);
	msg_len = strlen(SEND_BUFFER);
	va_end(args);
	if (starting) {
		buf = alloc_buffer();
		if (buf != NULL) { // if NULL, just drop
			strncpy(buf->buf, SEND_BUFFER, msg_len);
			buf->length = msg_len;
			deque_append(d, buf);
		}
	} else {
		serial_send_blocking(USB_COMM, SEND_BUFFER, msg_len);
	}
}
