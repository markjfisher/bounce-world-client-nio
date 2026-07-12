# MS-DOS disk-image packaging.
#
# Included from build.mk when CURRENT_TARGET=msdos. Uses shared variables:
#   BUILD_DIR, OBJDIR, DISK_IMAGE_DIR, PROGRAM, PROGRAM_TGT

CREATE_MSDOS_IMG ?= ../nio-apps/msdos/scripts/create_msdos_img.py
MSDOS_IMAGE ?= $(DISK_IMAGE_DIR)/$(PROGRAM)-msdos.img
MSDOS_LABEL ?= BWCMSDOS
MSDOS_EXE ?= BWCN.EXE

DISK_STAGE := $(OBJDIR)/diskimg/msdos

DISK_ARTIFACTS += $(MSDOS_IMAGE)

$(DISK_STAGE):
	mkdir -p $@

$(DISK_STAGE)/$(MSDOS_EXE): $(BUILD_DIR)/$(PROGRAM_TGT) | $(DISK_STAGE)
	cp $< $@

$(MSDOS_IMAGE): $(DISK_STAGE)/$(MSDOS_EXE) | $(DISK_IMAGE_DIR)
	@echo "Packaging $@..."
	python3 "$(CREATE_MSDOS_IMG)" -i "$(DISK_STAGE)" -o "$@" -l "$(MSDOS_LABEL)"
