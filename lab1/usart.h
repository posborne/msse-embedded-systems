/*
 * Author: Maxwell Walter, 2006
 *
 *
 * USART (Universal synchronous/asynchronous recieve/transmit
 *   -we will be using it in asynchronous mode, as a serial port
 *
 * USART documentation begins on page 174 of the ATMega128 datasheet
 *   Pages for individual settings will be noted in the code
 * The ATMega128 has 2 USART ports; we will only be using USART0 
 *   in this code.  To use USART0, you can just copy all this
 *   and replace the 0's with 1's
 *
 * See the comments in the usart.c file for more details
 *
 * TODO: Make this generic across both UARTS, perhaps as a compile time parameter
 */

#ifndef __USART_H
#define __USART_H

#include <inttypes.h>
#include <stdio.h>

#define CLOCK_FREQ 16000000

//default baud rate.
#define BAUD 57600

//default size of the transmit and recieve buffers
//should be big enough that it doesn't fill up
//MUST be a power of 2 (see cbuf.h for details)
#define DEFAULT_BUF_SIZE 1<<8

/*************************
 *     funtions          *
 *************************/

//initialize USART1
void init_USART1();

//find out if there is a byte to read 
//  this allows one to avoid blocking
int USART1_byte_available();

//Will return true if there is a newline OR the buffer is
//  full.
int USART1_input_ready();

//Will return the input that has been entered.
uint8_t* USART1_get_input();

//will return true if there is a newline in the input stream
//  This can be used to determine if the stdio functions will
//  return
int USART1_contains_newline();

//functions to read a byte
int8_t USART1_get_byte();
int USART1_stdio_get(FILE * f);

//functions to write a byte
void USART1_send_byte(int8_t data);
int USART1_stdio_send(char data, FILE * f);


#endif
