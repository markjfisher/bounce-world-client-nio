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
NON_AMIGA_TARGETS = atari bbc linux msdos
PROGRAM := bwcn
AMIGA_PROFILES := wb31 wb32
AMIGA_PROFILE ?=
AMIGA_CRT_wb31 := clib2
AMIGA_CRT_wb32 := clib2

.PHONY: all clean $(TARGETS) disk disk-% test-host-coords test-host-vectors test-host-csv test-host-interpolation test-host-pacing test-host

all:
	@for target in $(TARGETS); do \
		echo "-------------------------------------"; \
		echo "Building $$target"; \
		echo "-------------------------------------"; \
		$(MAKE) --no-print-directory $$target PROGRAM=$(PROGRAM); \
	done

$(NON_AMIGA_TARGETS):
	$(MAKE) --no-print-directory -f makefiles/build.mk CURRENT_TARGET=$@ PROGRAM=$(PROGRAM)

# Amiga is only released for the explicitly supported Workbench profiles.
# Keeping both the executable and objects profile-qualified prevents a
# Workbench 3.x clib2 executable being mistaken for a future WB1.3 build.
amiga:
ifeq ($(AMIGA_PROFILE),)
	@for profile in $(AMIGA_PROFILES); do \
		$(MAKE) --no-print-directory -f makefiles/build.mk \
			CURRENT_TARGET=amiga PROGRAM=$(PROGRAM) \
			AMIGA_PROFILE=$$profile AMIGA_CRT=clib2 \
			BUILD_DIR=build/amiga/$$profile OBJDIR=obj/amiga/$$profile || exit $$?; \
	done
else
ifeq ($(filter $(AMIGA_PROFILE),$(AMIGA_PROFILES)),)
$(error Unsupported Amiga profile '$(AMIGA_PROFILE)'; supported profiles: $(AMIGA_PROFILES))
endif
	$(MAKE) --no-print-directory -f makefiles/build.mk \
		CURRENT_TARGET=amiga PROGRAM=$(PROGRAM) \
		AMIGA_PROFILE=$(AMIGA_PROFILE) AMIGA_CRT=$(AMIGA_CRT_$(AMIGA_PROFILE)) \
		BUILD_DIR=build/amiga/$(AMIGA_PROFILE) OBJDIR=obj/amiga/$(AMIGA_PROFILE)
endif

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

test-host-csv:
	@mkdir -p build
	gcc -Wall -Wextra -O2 -std=c99 -Isrc/include \
	  src/common/add_client_csv.c tests/host/test_add_client_csv.c \
	  -o build/test_add_client_csv.host
	build/test_add_client_csv.host

test-host-interpolation:
	@mkdir -p build
	gcc -Wall -Wextra -O2 -std=c99 -Isrc/include \
	  src/common/bwc_interpolation_math.c tests/host/test_interpolation_math.c \
	  -o build/test_interpolation_math.host
	build/test_interpolation_math.host

test-host-pacing:
	@mkdir -p build
	gcc -Wall -Wextra -O2 -std=c99 -Isrc/include \
	  src/common/fetch_pacing.c tests/host/test_fetch_pacing.c \
	  -o build/test_fetch_pacing.host
	build/test_fetch_pacing.host

test-host: test-host-coords test-host-vectors test-host-csv test-host-interpolation test-host-pacing
