# bounce-world-client-nio Makefile
#
# Usage:
#   make           - Build all targets
#   make atari     - Build for Atari
#   make bbc       - Build for BBC Micro
#   make clean     - Remove build artifacts

TARGETS = atari bbc linux
PROGRAM := bwcn

.PHONY: all clean $(TARGETS)

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
	@for d in build obj; do \
		if [ -d "./$$d" ]; then \
			echo "Removing $$d"; \
			rm -rf ./$$d; \
		fi; \
	done
