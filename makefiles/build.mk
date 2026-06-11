SHELL := /usr/bin/env bash

SRCDIR   := src
BUILD_DIR := build
OBJDIR   := obj

# Platform mapping: target -> platform directory name
CURRENT_PLATFORM_atari := atari
CURRENT_PLATFORM_bbc   := bbc
CURRENT_PLATFORM_linux := linux

CURRENT_PLATFORM = $(CURRENT_PLATFORM_$(CURRENT_TARGET))

ifeq ($(CURRENT_PLATFORM),)
$(error Unknown target: $(CURRENT_TARGET). Supported: atari bbc linux)
endif

# fujinet-nio-lib base directory
NIO_LIB_DIR := ../fujinet-nio-lib

# Library include path and library file per target
NIO_LIB_INC  := $(NIO_LIB_DIR)/include
NIO_LIB_FILE_atari := $(NIO_LIB_DIR)/build/fujinet-nio-atari.lib
NIO_LIB_FILE_bbc   := $(NIO_LIB_DIR)/build/fujinet-nio-bbc.lib
NIO_LIB_FILE_linux := $(NIO_LIB_DIR)/build/fujinet-nio-linux.a

NIO_LIB_FILE = $(NIO_LIB_FILE_$(CURRENT_TARGET))

PROGRAM_TGT := $(PROGRAM).$(CURRENT_TARGET)

# Source files: root src/, common/, and platform-specific
SOURCES :=
SOURCES += $(wildcard $(SRCDIR)/*.c)
SOURCES += $(wildcard $(SRCDIR)/common/*.c)
SOURCES += $(wildcard $(SRCDIR)/common/*.s)
SOURCES += $(wildcard $(SRCDIR)/$(CURRENT_PLATFORM)/*.c)
SOURCES += $(wildcard $(SRCDIR)/$(CURRENT_PLATFORM)/*.s)

ifeq ($(CURRENT_TARGET),bbc)
SOURCES := $(filter-out $(SRCDIR)/common/hex_dump.c,$(SOURCES))
endif

SOURCES := $(strip $(SOURCES))

# Object files: mirror src/* path under obj/<target>/
OBJ_TMP  := $(SOURCES:.c=.o)
OBJECTS  := $(OBJ_TMP:.s=.o)
OBJECTS  := $(OBJECTS:$(SRCDIR)/%=$(OBJDIR)/$(CURRENT_TARGET)/%)

DEPENDS  := $(OBJECTS:.o=.d)

# --------------------------------------------------------------------
# Compiler configuration
# --------------------------------------------------------------------
CC_atari := cl65
CC_bbc   := cl65
CC_linux := gcc
CC = $(CC_$(CURRENT_TARGET))

CFLAGS :=
CFLAGS_atari_common := -Osir
CFLAGS_bbc_common   := -Osir
CFLAGS_linux_common := -Wall -Wextra -O2 -std=c99
CFLAGS += $(CFLAGS_$(CURRENT_TARGET)_common)
CFLAGS += -I$(SRCDIR)/include
CFLAGS += -I$(SRCDIR)/$(CURRENT_PLATFORM)
CFLAGS += -I$(SRCDIR)/common
CFLAGS += -I$(SRCDIR)
CFLAGS += -I$(NIO_LIB_INC)

ASFLAGS :=
ASFLAGS += --asm-include-dir $(SRCDIR)/include
ASFLAGS += --asm-include-dir $(SRCDIR)/$(CURRENT_PLATFORM)
ASFLAGS += --asm-include-dir $(SRCDIR)/common
ASFLAGS += --asm-include-dir $(SRCDIR)
ifeq ($(CURRENT_TARGET),bbc)
ASFLAGS += --asm-include-dir /home/markf/dev/bbc/cc65/libsrc/bbc
ASFLAGS += --asm-include-dir /home/markf/dev/bbc/cc65/asminc
endif

LDFLAGS_atari := -C cfg/atari.cfg
LDFLAGS_bbc   := -C cfg/bbc.cfg
LDFLAGS_linux :=
LDFLAGS = $(LDFLAGS_$(CURRENT_TARGET))

LIBS  = $(NIO_LIB_FILE)

# Additional target-specific CFLAGS
CFLAGS_atari := -DBWC_CUSTOM_CPUTC
CFLAGS_bbc   :=
CFLAGS_linux :=
CFLAGS += $(CFLAGS_$(CURRENT_TARGET))

# Dependency files  
-include $(DEPENDS)

.PHONY: all clean

all: $(BUILD_DIR)/$(PROGRAM_TGT)

# Compile .c -> .o
ifeq ($(CURRENT_TARGET),linux)
$(OBJDIR)/$(CURRENT_TARGET)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) -MMD -MF $(@:.o=.d) -o $@ $<
else
$(OBJDIR)/$(CURRENT_TARGET)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	@mkdir -p $(dir $@)
	$(CC) -t $(CURRENT_TARGET) -c --create-dep $(@:.o=.d) --listing $(@:.o=.lst) $(CFLAGS) -o $@ $<
endif

# Assemble .s -> .o
$(OBJDIR)/$(CURRENT_TARGET)/%.o: $(SRCDIR)/%.s | $(OBJDIR)
	@mkdir -p $(dir $@)
	$(CC) -t $(CURRENT_TARGET) -c --create-dep $(@:.o=.d) $(ASFLAGS) --listing $(@:.o=.lst) -o $@ $<

# Ensure lib exists
$(NIO_LIB_FILE):
	@echo "Building fujinet-nio-lib for $(CURRENT_TARGET)..."
	$(MAKE) -C $(NIO_LIB_DIR) $(CURRENT_TARGET)

# Link
ifeq ($(CURRENT_TARGET),linux)
$(BUILD_DIR)/$(PROGRAM_TGT): $(OBJECTS) $(NIO_LIB_FILE) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS)
else
$(BUILD_DIR)/$(PROGRAM_TGT): $(OBJECTS) $(NIO_LIB_FILE) | $(BUILD_DIR)
	$(CC) -t $(CURRENT_TARGET) $(LDFLAGS) --mapfile $@.map -Ln $@.lbl -o $@ $(OBJECTS) $(LIBS)
endif

$(OBJDIR):
	mkdir -p $@

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)/$(PROGRAM_TGT) $(OBJDIR)/$(CURRENT_TARGET)
