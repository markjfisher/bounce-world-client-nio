# BBC disk-image packaging.
#
# Included from build.mk when CURRENT_TARGET=bbc.  Uses shared variables:
#   BUILD_DIR, OBJDIR, DISK_IMAGE_DIR, PROGRAM

CREATE_SSD := scripts/create_ssd.py

# DFS leaf name — 7-char uppercase limit
DFS_LEAF  := $(shell printf '%s' '$(PROGRAM)' | cut -c1-7 | tr a-z A-Z)

# Staging directory for the raw binary + .inf sidecar
DISK_STAGE := $(OBJDIR)/diskimg/bbc

# Single SSD artifact
DISK_ARTIFACTS += $(DISK_IMAGE_DIR)/$(PROGRAM).ssd

# ---- Rules ----

$(DISK_STAGE):
	mkdir -p $@

$(DISK_STAGE)/$(DFS_LEAF): $(BUILD_DIR)/$(PROGRAM_TGT) | $(DISK_STAGE)
	cp $< $@
	printf '$.%s 001900 001900\n' '$(DFS_LEAF)' > $@.inf

$(DISK_ARTIFACTS): $(DISK_STAGE)/$(DFS_LEAF) | $(DISK_IMAGE_DIR)
	@echo "Packaging $@..."
	python3 "$(CREATE_SSD)" -i "$(DISK_STAGE)" -o "$@" -t "$(DFS_LEAF)"