#include "usart.h"
#include "cbuf.h"

#include <avr/interrupt.h>

//our transmit and receive buffers
static uint8_t tx_buf[DEFAULT_BUF_SIZE];
static volatile cbuf_t  usart1_tx;

static uint8_t rx_buf[DEFAULT_BUF_SIZE];
static volatile cbuf_t  usart1_rx;

#define INPUT_BUFFER_SIZE 40

static uint8_t usart_input[INPUT_BUFFER_SIZE];

extern uint8_t G_flag;

//setup USART1 to recieve/transmit
void init_USART1() {

  //calculate the value for the UBRR register using the equation
  //given on page 175
  //  UBRR = ( osc / (16 * BAUD) ) - 1
  uint16_t ubrr_val = (CLOCK_FREQ / (16*BAUD) ) - 1;

  //set up the baud speed
  UBRR1H = (ubrr_val >> 8) & 0xFF;
  UBRR1L = ubrr_val & 0xFF;

  //now we have to configure the USART
  //There are three registers we have to set; UCSR0(A,B,C)
  UCSR1A = 0;        //page 190 (status bits and double speed)
  UCSR1B =           //page 191
    ((1 << RXCIE1) | //enable interrupt on data receipt
     (1 << TXEN1)  | //enable transmitter
     (1 << RXEN1)    //enable receiver
     );              //done

  //this sets the serial port transmot parameters
  //we will be using 8n1, which is 8 data bits, no parity, 1 stop bit
  UCSR1C =            //page 192
    ((1 << UCSZ11) |  //these two specify 8 data bits
     (1 << UCSZ10) |
     (1 << USBS1)     //and a stop bit
     );

  //now we have to initialize our transmit and recieve buffers
  CBUF_INIT(&usart1_tx, tx_buf, DEFAULT_BUF_SIZE);
  CBUF_INIT(&usart1_rx, rx_buf, DEFAULT_BUF_SIZE);
}


//the code on page 179 of the ATMega128 datashee 
//  has a simple send routine
void USART1_send_byte(int8_t data) {
  //if we are supposed to send a newline '\n', we
  //need to add a carriage reutrn as well
  //to make things look pretty for terminals
  if(data == '\n') {
    while(CBUF_FULL(&usart1_tx)) {}
    CBUF_PUSH(&usart1_tx, '\r');
  }
  //push the data byte into the transmit buffer
  //first we have to wait till the transmit buffer has space
  while(CBUF_FULL(&usart1_tx)) {}
  CBUF_PUSH(&usart1_tx, data);

  //now we enable the USART data register empty interrupt
  //this will allow us to send data without having to
  //poll 
  UCSR1B |= (1<<UDRIE1);
}

//used for fdevopen
int USART1_stdio_send(char data, FILE * f) {
  USART1_send_byte(data);
  return 0;
}

//check to see if the input buffer contains a newline character
//  This will tell us if gets, fgets and whatnot will return or block
int USART1_contains_newline() {
  int len = CBUF_LEN(&usart1_rx);

  int i; char c;

  for(i = 0; i < len; i++) {
    c = CBUF_GET(&usart1_rx, i);
    if(c == '\r' || c == '\n') {
      return 1;
    }
  }

  return 0;
}

//We have a command if the buffer is either full or contains a newline
//  AND we've parsed out the command line to figure out what we want to do.
int USART1_input_ready() {
 	 return CBUF_FULL(&usart1_rx) || USART1_contains_newline();
}


//Returns the current input
uint8_t* USART1_get_input() {
	int idx = 0;
	for (; idx < CBUF_LEN(&usart1_rx) && idx < INPUT_BUFFER_SIZE; idx++)
		usart_input[idx] = CBUF_GET(&usart1_rx, idx);

	usart_input[idx++] = '\0';

	while(!CBUF_EMPTY(&usart1_rx))
		CBUF_POP(&usart1_rx);

	return &usart_input;
}

//check if there is a byte available in the recieve
//buffer.  This will tell if get_byte will block or
//not
int USART1_byte_available() {
  return  ! CBUF_EMPTY(&usart1_rx);
}

//if there is no byte available, block!
//  check with byte_available if you need non-blocking
int8_t USART1_get_byte() {
  while(CBUF_EMPTY(&usart1_rx)) {}
  
  uint8_t ret = CBUF_POP(&usart1_rx);

  return ret;
}

//for use with fdevopen
int USART1_stdio_get(FILE * f) {
  return USART1_get_byte();
}


/**********************************************************
 *  SIGNAL HANDLERS
 *  
 *  this is where the actual sending and recieving of data
 *  happens
 *
 *  see page 59 for available interrupts
 *  signal names defined in avr/iom128.h
 **********************************************************/

//called when there is data to recieve
ISR(SIG_USART1_RECV) {

	G_flag = 1;

  //the data is stored in the UDR register
  uint8_t ch = UDR1;

  //if we got a character return '\r' we need to
  //replace it with a newline (for stdio functions)
  if(ch == '\r') ch = '\n';

  //printf("%c", ch);

  //If we have a backspace character and the buffer isn't empty,
  //we need to remove the most-recently added char from the buffer.
  if (ch == 127 && ! CBUF_EMPTY(&usart1_rx)) { //backspace
  	CBUF_UNPUSH(&usart1_rx);
	return;
  }
  
  //if the recieve buffer is full, we lose the data
  if( ! CBUF_FULL(&usart1_rx)) {
    CBUF_PUSH(&usart1_rx, ch);
  }
}

//called with the data register is empty
//  the data register is where we put the data we want 
//  to send.
ISR(SIG_USART1_DATA) {
  if(! CBUF_EMPTY(&usart1_tx)) {
    //pop data from the transmit buffer and put it in the
    //data register

    uint8_t ch = CBUF_POP(&usart1_tx);
    UDR1 = ch;
  } else {
    //nothing to send, so turn off the interrupt
    UCSR1B &= ~(1 << UDRIE1);
  }
}
