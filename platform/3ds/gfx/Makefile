# Host tests for the GX backend's pure modules (no libctru); the rest of gfx/ is built into the game
# by platform/3ds/CMakeLists.txt.
#
#   make test       build and run every test
#   make coverage   run the tests with clang source-based coverage and print a report
#   make clean

HOSTCC        ?= cc
# pc/include gives the tests the PC port's recorded TEV configs (pc_shader_seed.h).
HOST_CFLAGS   := -std=c11 -Wall -Wextra -Werror -O2 -I. -I../../../pc/include
BUILD         := build/host
MODULES       := gx_tev gx_alpha_test gx_uv gx_dlist gx_fog gx_screen gx_tev_cache

COV_CC        ?= clang
ifeq ($(shell uname -s),Darwin)
LLVM_PROFDATA ?= xcrun llvm-profdata
LLVM_COV      ?= xcrun llvm-cov
else
LLVM_PROFDATA ?= llvm-profdata
LLVM_COV      ?= llvm-cov
endif
COV_CFLAGS    := -std=c11 -O0 -I. -I../../../pc/include -fprofile-instr-generate -fcoverage-mapping

.PHONY: test coverage clean

test:
	@mkdir -p $(BUILD)
	@for m in $(MODULES); do \
		echo "host test: $$m"; \
		$(HOSTCC) $(HOST_CFLAGS) tests/test_$$m.c $$m.c -lm -o $(BUILD)/test_$$m || exit 1; \
		$(BUILD)/test_$$m || exit 1; \
	done

coverage:
	@mkdir -p $(BUILD)/cov
	@for m in $(MODULES); do \
		$(COV_CC) $(COV_CFLAGS) tests/test_$$m.c $$m.c -lm -o $(BUILD)/cov/test_$$m || exit 1; \
		LLVM_PROFILE_FILE=$(BUILD)/cov/$$m.profraw $(BUILD)/cov/test_$$m > /dev/null || exit 1; \
	done
	@$(LLVM_PROFDATA) merge -sparse $(BUILD)/cov/*.profraw -o $(BUILD)/cov/all.profdata
	@$(LLVM_COV) report $(BUILD)/cov/test_$(firstword $(MODULES)) \
		$(foreach m,$(wordlist 2,$(words $(MODULES)),$(MODULES)),-object $(BUILD)/cov/test_$(m)) \
		-instr-profile=$(BUILD)/cov/all.profdata $(foreach m,$(MODULES),$(m).c)

clean:
	@rm -rf build
