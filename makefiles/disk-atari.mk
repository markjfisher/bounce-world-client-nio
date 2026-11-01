# Bootable Atari disk: PicoBoot runs the only XEX in the staged directory.
DIR2ATR ?= dir2atr
ATARI_BOOT ?= PicoBoot406
ATARI_IMAGE := $(DISK_IMAGE_DIR)/$(PROGRAM).atr
DISK_ARTIFACTS += $(ATARI_IMAGE)

$(ATARI_IMAGE): $(BUILD_DIR)/$(PROGRAM_TGT) | $(DISK_IMAGE_DIR)
	@command -v "$(DIR2ATR)" >/dev/null || { echo "dir2atr is required to build the Atari disk" >&2; exit 1; }
	@stage=$$(mktemp -d); image="$@.tmp.atr"; \
	trap 'rm -rf "$$stage"; rm -f "$$image"' EXIT; \
	cp "$<" "$$stage/BWCN.XEX" && \
	$(DIR2ATR) -b "$(ATARI_BOOT)" "$$image" "$$stage" && \
	mv "$$image" "$@"
