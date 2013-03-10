// Author: Maxwell Walter, 2006

#ifndef __CBUF_H
#define __CBUF_H

#include "inttypes.h"

//TODO:
//  Maybe add an element to the struct for 
//  size - 1, since I use it so often.
//  Don't know if saving a single subtraction is worth the
//  extra memory

//TODO:
//  Perhaps make these all functions instead of defines.
//  That would make it much easier to read...

//note that size **MUST** be a power of 2
//if it isn't you will get strange (broken) behavior
//This works becuase  x & ( n - 1) is the same as
//  x mod n, if n is a power of 2

//concept shamelessly taken from 
//http://www.ganssle.com/tem/tem110.pdf
//the code from which was released to the public domain

//To use a cbuf, create a struct and buffer in your code, 
//  and then call the CBUF_INIT function.  As an example
//
//#define BUF_SIZE (1<<6)
//uint8_t buf[BUF_SIZE]
//cbuf_t circular_buffer;
//CBUF_INIT(&circular_buffer, &buf, BUF_SIZE); //(1<<6) = 2^7 = 128
//
//TODO: make the caller allocate memory, so there is no malloc?
//   This would allow for easier determination of available/used
//   memory at compile time.
typedef struct __cbuf_t {
  uint16_t tail, head;
  uint16_t size;
  uint8_t *buf;
} cbuf_t;

//fill in the struct, with the buffer being an already
//  existing array of unsigned char 
#define CBUF_INIT(cbuf, buffer, sz)		\
  (cbuf)->buf = (buffer);			\
  (cbuf)->head = 0; (cbuf)->tail = 0;		\
  (cbuf)->size = sz;

//we have to make sure the value returned is unsigned
#define CBUF_LEN(cbuf)				\
  (uint16_t)(((cbuf)->head) - ((cbuf)->tail))

#define CBUF_EMPTY(cbuf)			\
  (CBUF_LEN(cbuf) <= 0)

#define CBUF_FULL(cbuf)				\
  ( (CBUF_LEN(cbuf)) >= (cbuf)->size)


//!!push and pop do not do any sanity checking!!
//  that is left to the application programmer

//if you don't do the checking before a push, 
//  you may overwrite items in your buffer
#define CBUF_PUSH(cbuf, item)				\
  ((cbuf)->buf)[((cbuf)->head++) & ((cbuf)->size - 1)] = (item)

//Unpush an item from your buffer.  Shouldn't be done on an empty
//  cbuf.
#define CBUF_UNPUSH(cbuf)  \
  ((cbuf)->buf)[((cbuf)->head--) & ((cbuf)->size - 1)]

//if you pop an empty cbuf, it will really screw things up
//  you will wind up with a tail that advances past the head...
#define CBUF_POP(cbuf)					\
  ((cbuf)->buf)[((cbuf)->tail++) & ((cbuf)->size - 1)]

//Get an element at a specific index
#define CBUF_GET(cbuf, x)				\
  ((cbuf)->buf)[((cbuf)->tail + x) & ((cbuf)->size - 1)]

#endif
