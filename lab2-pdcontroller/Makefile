DEVICE = atmega1284p
MCU = atmega1284p
AVRDUDE_DEVICE = m1284p
PORT ?= /dev/ttyACM0
DEVICE ?= atmega168
MCU ?= atmega168
AVRDUDE_DEVICE ?= m168

CFLAGS=-g -Wall -Werror -mcall-prologues -DF_CPU=20000UL -mmcu=$(MCU) $(DEVICE_SPECIFIC_CFLAGS) -O0
CC=avr-gcc
AS=avr-as
OBJ2HEX=avr-objcopy
LDFLAGS=-Wl,-gc-sections -lpololu_$(DEVICE) -Wl,-relax
PANDOC_HTML=pandoc --from markdown --to html --standalone
PANDOC_DOCX=pandoc --from markdown --to docx

PORT ?= /dev/ttyACM0
AVRDUDE=avrdude

TARGET=lab2
OBJECT_FILES=$(TARGET).o log.o timers.o scheduler.o motor.o cli.o deque.o interpolator.o

all: $(TARGET).hex

clean:
	rm -f *.o *.hex *.obj *.hex

docs: README.md
	$(PANDOC_HTML) README.md > README.html
	$(PANDOC_DOCX) README.md -o README.docx

%.hex: $(TARGET).obj
	$(OBJ2HEX) -R .eeprom -O ihex $< $@

%.obj: $(OBJECT_FILES)
	$(CC) $(CFLAGS) $(OBJECT_FILES) $(LDFLAGS) -o $@

program: $(TARGET).hex
	$(AVRDUDE) -p $(AVRDUDE_DEVICE) -c avrisp2 -P $(PORT) -U flash:w:$(TARGET).hex

.PHONY: release
release:
	tar -czvf seng5831-embedded-lab2-`date +%m_%d_%y`.tar.gz Makefile *.c *.h *.md assets/*
