# Bootable Atari disk: PicoBoot runs the only XEX in the staged directory.
DIR2ATR ?= dir2atr
ATARI_BOOT ?= PicoBoot406
ATARI_IMAGE := $(DISK_IMAGE_DIR)/$(PROGRAM).atr
DISK_ARTIFACTS += $(ATARI_IMAGE)

$(BUILD_DIR)/atari-splash-screen.bin: scripts/atari_boot_splash.py | $(BUILD_DIR)
	python3 $< screen $@

$(BUILD_DIR)/atari-splash.bin: src/atari-loader/splash.s $(BUILD_DIR)/atari-splash-screen.bin cfg/atari-splash.cfg
	ca65 --bin-include-dir $(BUILD_DIR) $< -o $(BUILD_DIR)/atari-splash.o
	ld65 -C cfg/atari-splash.cfg $(BUILD_DIR)/atari-splash.o -o $@

$(BUILD_DIR)/bwcn-boot.xex: $(BUILD_DIR)/$(PROGRAM_TGT) $(BUILD_DIR)/atari-splash.bin scripts/atari_boot_splash.py
	python3 scripts/atari_boot_splash.py xex $@ $(BUILD_DIR)/atari-splash.bin $(BUILD_DIR)/$(PROGRAM_TGT)

$(ATARI_IMAGE): $(BUILD_DIR)/bwcn-boot.xex | $(DISK_IMAGE_DIR)
	@command -v "$(DIR2ATR)" >/dev/null || { echo "dir2atr is required to build the Atari disk" >&2; exit 1; }
	@stage=$$(mktemp -d); image="$@.tmp.atr"; \
	trap 'rm -rf "$$stage"; rm -f "$$image"' EXIT; \
	cp "$<" "$$stage/BWCN.XEX" && \
	$(DIR2ATR) -b "$(ATARI_BOOT)" "$$image" "$$stage" && \
	mv "$$image" "$@"
