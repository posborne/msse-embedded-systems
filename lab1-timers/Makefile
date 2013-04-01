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
PANDOC=pandoc --from markdown --to html --standalone

PORT ?= /dev/ttyACM0
AVRDUDE=avrdude

TARGET=lab1
OBJECT_FILES=$(TARGET).o log.o leds.o timers.o scheduler.o menu.o

all: $(TARGET).hex

clean:
	rm -f *.o *.hex *.obj *.hex

docs: README.md
	$(PANDOC) README.md > README.html

%.hex: $(TARGET).obj
	$(OBJ2HEX) -R .eeprom -O ihex $< $@

%.obj: $(OBJECT_FILES)
	$(CC) $(CFLAGS) $(OBJECT_FILES) $(LDFLAGS) -o $@

%.s: %.c
	$(CC) -mmcu=$(MCU) -fverbose-asm -S $<

lab1.lst: lab1.obj
	avr-objdump -d -M mips -S lab1.obj > lab1.lst

program: $(TARGET).hex
	$(AVRDUDE) -p $(AVRDUDE_DEVICE) -c avrisp2 -P $(PORT) -U flash:w:$(TARGET).hex

.PHONY: release
release:
	tar -czvf seng5831-embedded-lab1-`date +%m_%d_%y`.tar.gz Makefile *.txt *.c *.h
