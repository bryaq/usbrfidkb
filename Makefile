NAME=usbrfidkb
OBJS=main.o keyboard.o usb.o cdc.o usbdrv/usbdrv.o usbdrv/usbdrvasm.o
CPU=atmega8
CC=avr-gcc
F_CPU=12000000ul
CFLAGS=-std=gnu99 -Wall -Wno-overflow -Wno-volatile-register-var -pedantic -O2 -g -mmcu=$(CPU) -DF_CPU=$(F_CPU) -I.
ASFLAGS=-g -mmcu=$(CPU) -DF_CPU=$(F_CPU) -I.
LDFLAGS=-mmcu=$(CPU)

.PHONY:	all fuse prog clean

all:	$(NAME).elf

$(NAME).elf:	$(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)
	avr-size $@

fuse:
	avrdude -p $(CPU) -c usbtiny -U hfuse:w:0xc1:m -U lfuse:w:0x1f:m

prog:	$(NAME).hex
	avrdude -p $(CPU) -c usbtiny -U flash:w:$(NAME).hex:i

$(NAME).hex:	$(NAME).elf
	avr-objcopy -j .text -j .data -O ihex $(NAME).elf $(NAME).hex

$(NAME).lst:	$(NAME).elf
	avr-objdump -S $(NAME).elf >$@

$(OBJS):	types.h usbconfig.h keyboard.h usb.h hw.h cdc.h

clean:
	rm -f $(NAME).elf $(NAME).hex $(NAME).lst $(OBJS)
