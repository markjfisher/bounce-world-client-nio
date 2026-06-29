SHELL := /usr/bin/env bash

.DEFAULT_GOAL := all

SRCDIR   := src
BUILD_DIR := build
OBJDIR   := obj

# Disk image output directory (per-target to support future atari .atr etc.)
DISK_IMAGE_DIR := disk-images/$(CURRENT_TARGET)

# Platform mapping: target -> platform directory name
CURRENT_PLATFORM_atari := atari
CURRENT_PLATFORM_bbc   := bbc
CURRENT_PLATFORM_linux := linux
CURRENT_PLATFORM_msdos := msdos

CURRENT_PLATFORM = $(CURRENT_PLATFORM_$(CURRENT_TARGET))

ifeq ($(CURRENT_PLATFORM),)
$(error Unknown target: $(CURRENT_TARGET). Supported: atari bbc linux msdos)
endif

# fujinet-nio-lib base directory (required; relative to project root or absolute)
ifeq ($(FUJINET_NIO_LIB),)
$(error FUJINET_NIO_LIB is not defined. Set it to the fujinet-nio-lib repository.)
endif

ifeq ($(wildcard $(FUJINET_NIO_LIB)/include/fujinet-nio.h),)
$(error FUJINET_NIO_LIB does not exist or is invalid: $(FUJINET_NIO_LIB))
endif

NIO_LIB_DIR := $(FUJINET_NIO_LIB)

# Library include path and library file per target
NIO_LIB_INC  := $(NIO_LIB_DIR)/include
NIO_LIB_FILE_atari := $(NIO_LIB_DIR)/build/fujinet-nio-atari.lib
NIO_LIB_FILE_bbc   := $(NIO_LIB_DIR)/build/fujinet-nio-bbc.lib
NIO_LIB_FILE_linux := $(NIO_LIB_DIR)/build/fujinet-nio-linux.a
MSDOS_NIO_BACKEND ?= ioctl
BWC_CLIENT_IO_POLICY ?= stop
NIO_LIB_FILE_msdos-serial := $(NIO_LIB_DIR)/build/fujinet-nio-msdos-serial.lib
NIO_LIB_FILE_msdos-ioctl  := $(NIO_LIB_DIR)/build/fujinet-nio-msdos-ioctl.lib
NIO_LIB_FILE_msdos-f5     := $(NIO_LIB_DIR)/build/fujinet-nio-msdos-f5.lib
NIO_LIB_FILE_msdos := $(NIO_LIB_FILE_msdos-$(MSDOS_NIO_BACKEND))

NIO_LIB_FILE = $(NIO_LIB_FILE_$(CURRENT_TARGET))

PROGRAM_TGT := $(PROGRAM).$(CURRENT_TARGET)
ifeq ($(CURRENT_TARGET),msdos)
PROGRAM_TGT := $(PROGRAM).msdos.exe
endif

# Source files: root src/, common/, and platform-specific
SOURCES :=
SOURCES += $(wildcard $(SRCDIR)/*.c)
SOURCES += $(wildcard $(SRCDIR)/common/*.c)
SOURCES += $(wildcard $(SRCDIR)/common/*.s)
SOURCES += $(wildcard $(SRCDIR)/$(CURRENT_PLATFORM)/*.c)
SOURCES += $(wildcard $(SRCDIR)/$(CURRENT_PLATFORM)/*.s)

ifeq ($(CURRENT_TARGET),bbc)
SOURCES := $(filter-out $(SRCDIR)/common/hex_dump.c,$(SOURCES))
SOURCES := $(filter-out $(SRCDIR)/common/embedded_shapes.c,$(SOURCES))
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
CC_msdos := wcc
LD_msdos := wcl
CC = $(CC_$(CURRENT_TARGET))
LD = $(LD_$(CURRENT_TARGET))
ifeq ($(LD),)
LD := $(CC)
endif

CFLAGS :=
CFLAGS_atari_common := -Osir
CFLAGS_bbc_common   := -Osir
CFLAGS_linux_common := -Wall -Wextra -O2 -std=c99
CFLAGS_msdos_common := -0 -bt=dos -os -ms -s -q
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
LDFLAGS_msdos := -q -0 -bt=dos -ms
LDFLAGS = $(LDFLAGS_$(CURRENT_TARGET))

LIBS  = $(NIO_LIB_FILE)

# Additional target-specific CFLAGS
CFLAGS_atari := -DBWC_CUSTOM_CPUTC
CFLAGS_bbc   :=
CFLAGS_linux :=
CFLAGS_msdos :=
ifeq ($(CURRENT_TARGET),msdos)
ifeq ($(MSDOS_NIO_BACKEND),ioctl)
CFLAGS_msdos += -DBWC_MSDOS_IOCTL_DIAG
endif
ifeq ($(BWC_CLIENT_IO_POLICY),continue)
CFLAGS_msdos += -DBWC_IGNORE_TRANSIENT_CLIENT_IO
endif
endif
CFLAGS += $(CFLAGS_$(CURRENT_TARGET))

# Dependency files  
-include $(DEPENDS)

# Disk image artifacts — populated by platform-specific include below
DISK_ARTIFACTS :=

# Platform-specific disk packaging rules
-include makefiles/disk-$(CURRENT_TARGET).mk

.PHONY: all clean disk gfx-shapes embedded-shapes

gfx-shapes:
	python3 scripts/gen_gfx_shapes.py \
	  ../kotlin/bounce-world/server/src/jvmMain/resources/shapes \
	  src/bbc/gfx_shapes.c

embedded-shapes:
	python3 scripts/gen_embedded_shapes.py \
	  ../kotlin/bounce-world/server/src/jvmMain/resources/shapes \
	  src/common/embedded_shapes.c \
	  src/include/embedded_shapes.h

all: $(BUILD_DIR)/$(PROGRAM_TGT)

disk: $(DISK_ARTIFACTS)

# Compile .c -> .o
ifeq ($(CURRENT_TARGET),linux)
$(OBJDIR)/$(CURRENT_TARGET)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) -MMD -MF $(@:.o=.d) -o $@ $<
else ifeq ($(CURRENT_TARGET),msdos)
$(OBJDIR)/$(CURRENT_TARGET)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -fo=$@ $<
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
ifeq ($(CURRENT_TARGET),msdos)
	$(MAKE) -C $(NIO_LIB_DIR) msdos-$(MSDOS_NIO_BACKEND)
else
	$(MAKE) -C $(NIO_LIB_DIR) $(CURRENT_TARGET)
endif

# Link
ifeq ($(CURRENT_TARGET),linux)
$(BUILD_DIR)/$(PROGRAM_TGT): $(OBJECTS) $(NIO_LIB_FILE) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS)
else ifeq ($(CURRENT_TARGET),msdos)
$(BUILD_DIR)/$(PROGRAM_TGT): $(OBJECTS) $(NIO_LIB_FILE) | $(BUILD_DIR)
	$(LD) $(LDFLAGS) -fe=$@ $(OBJECTS) $(LIBS)
else
$(BUILD_DIR)/$(PROGRAM_TGT): $(OBJECTS) $(NIO_LIB_FILE) | $(BUILD_DIR)
	$(CC) -t $(CURRENT_TARGET) $(LDFLAGS) --mapfile $@.map -Ln $@.lbl -o $@ $(OBJECTS) $(LIBS)
endif

$(OBJDIR):
	mkdir -p $@

$(BUILD_DIR):
	mkdir -p $@

$(DISK_IMAGE_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)/$(PROGRAM_TGT) $(OBJDIR)/$(CURRENT_TARGET) $(DISK_IMAGE_DIR)
