# ---------------------------------------------------------------------------
#  Greenhouse controller - Arduino Mega 2560  (GNU make + avr-gcc)
#
#    make                  build  -> build/greenhouse.hex
#    make flash PORT=COM3  upload through the Mega USB bootloader (Linux: PORT=/dev/ttyACM0)
#    make clean
# ---------------------------------------------------------------------------
TARGET := greenhouse
MCU    := atmega2560
F_CPU  := 16000000UL
PORT   ?= /dev/ttyACM0

BUILD := build
SRCS  := $(wildcard src/*.c) $(wildcard lib/*/*.c)
OBJS  := $(SRCS:%.c=$(BUILD)/%.o)
DEPS  := $(OBJS:.o=.d)
INCS  := -Iinclude $(addprefix -I,$(wildcard lib/*))

CC      := avr-gcc
OBJCOPY := avr-objcopy
SIZE    := avr-size

CFLAGS  := -mmcu=$(MCU) -DF_CPU=$(F_CPU) -std=gnu99 -Os -Wall -Wextra \
           -ffunction-sections -fdata-sections $(INCS) -MMD -MP
LDFLAGS := -mmcu=$(MCU) -Wl,--gc-sections

ELF := $(BUILD)/$(TARGET).elf
HEX := $(BUILD)/$(TARGET).hex

.PHONY: all size flash clean rebuild

all: $(HEX) size

$(ELF): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex -R .eeprom $< $@

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

size: $(ELF)
	$(SIZE) --format=avr --mcu=$(MCU) $(ELF)

flash: $(HEX)
	avrdude -p m2560 -c wiring -P $(PORT) -b 115200 -D -U flash:w:$(HEX):i

clean:
	rm -rf $(BUILD)

rebuild: clean all

-include $(DEPS)
