# bounce-world-client-nio Makefile
#
# Usage:
#   export FUJINET_NIO_LIB=/path/to/fujinet-nio-lib
#   make           - Build all targets
#   make atari     - Build for Atari
#   make disk-msdos - Build an MS-DOS FAT image
#
# Other targets: bbc, linux, msdos, amiga, disk-bbc

TARGETS = atari bbc linux msdos amiga
PROGRAM := bwcn

.PHONY: all clean $(TARGETS) disk disk-% test-host-coords test-host-vectors test-host

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

disk-%:
	$(MAKE) --no-print-directory -f makefiles/build.mk CURRENT_TARGET=$* PROGRAM=$(PROGRAM) disk

test-host-coords:
	@mkdir -p build
	gcc -Wall -Wextra -O2 -std=c99 -Isrc/include \
	  src/common/shape_decode.c tests/host/test_coord_decode.c \
	  -o build/test_coord_decode.host
	build/test_coord_decode.host

test-host-vectors:
	@mkdir -p build
	gcc -Wall -Wextra -O2 -std=c99 -Isrc/include -Isrc/amiga \
	  src/amiga/vector_outline.c src/common/embedded_shapes.c tests/host/test_vector_outline.c \
	  -o build/test_vector_outline.host
	build/test_vector_outline.host

test-host: test-host-coords test-host-vectors
