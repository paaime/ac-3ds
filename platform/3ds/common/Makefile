HOSTCC        ?= cc
HOST_CFLAGS   := -std=c11 -Wall -Wextra -Werror -O2 -I.
BUILD         := build/host
MODULES       := pica_tile gc_texture pot_pad gc_clock mem_layout pad_map audio_pacer wave_pool pcm_level pcm_limiter gc_ortho dsp_firmware quest_bits prof_window pcm_stereo nes_pixels feed_rate settings_file gc_pattern bottom_layout light_timer stall_watch tex_memo debounce press_hold tga acre_sync acre_select quick_save text_wrap disc_check

# clang only: gcc lacks source-based coverage. macOS keeps the llvm tools behind xcrun.
COV_CC        ?= clang
ifeq ($(shell uname -s),Darwin)
LLVM_PROFDATA ?= xcrun llvm-profdata
LLVM_COV      ?= xcrun llvm-cov
else
LLVM_PROFDATA ?= llvm-profdata
LLVM_COV      ?= llvm-cov
endif
COV_CFLAGS    := -std=c11 -O0 -I. -fprofile-instr-generate -fcoverage-mapping

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
