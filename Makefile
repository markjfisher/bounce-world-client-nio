# bounce-world-client-nio Makefile
#
# Usage:
#   export FUJINET_NIO_LIB=/path/to/fujinet-nio-lib
#   make           - Build all targets
#   make atari     - Build for Atari
#
# Other targets: bbc, linux, msdos

TARGETS = atari bbc linux msdos
PROGRAM := bwcn

.PHONY: all clean $(TARGETS) disk

all:
	@for target in $(TARGETS); do \
		echo "-------------------------------------"; \
		echo "Building $$target"; \
		echo "-------------------------------------"; \
		$(MAKE) --no-print-directory -f makefiles/build.mk CURRENT_TARGET=$$target PROGRAM=$(PROGRAM); \
	done

$(TARGETS):
	$(MAKE) --no-print-directory -f makefiles/build.mk CURRENT_TARGET=$@ PROGRAM=$(PROGRAM)

clean:
	@for d in build obj disk-images; do \
		if [ -d "./$$d" ]; then \
			echo "Removing $$d"; \
			rm -rf ./$$d; \
		fi; \
	done

disk:
	$(MAKE) --no-print-directory -f makefiles/build.mk CURRENT_TARGET=bbc PROGRAM=$(PROGRAM) $(MAKECMDGOALS)
