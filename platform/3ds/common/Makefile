HOSTCC        ?= cc
HOST_CFLAGS   := -std=c11 -Wall -Wextra -Werror -O2 -I.
BUILD         := build/host
MODULES       := pica_tile gc_texture pot_pad gc_clock mem_layout pad_map audio_pacer wave_pool pcm_level pcm_limiter gc_ortho dsp_firmware quest_bits prof_window pcm_stereo nes_pixels feed_rate settings_file gc_pattern bottom_layout light_timer stall_watch tex_memo debounce press_hold tga acre_sync acre_select quick_save text_wrap disc_check ordinance gc_msg ordinance_talk
DEPS_ordinance_talk := ordinance.c gc_msg.c

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
	@set -e; $(foreach m,$(MODULES), \
		echo "host test: $(m)"; \
		$(HOSTCC) $(HOST_CFLAGS) tests/test_$(m).c $(m).c $(DEPS_$(m)) -lm -o $(BUILD)/test_$(m); \
		$(BUILD)/test_$(m);)

coverage:
	@mkdir -p $(BUILD)/cov
	@set -e; $(foreach m,$(MODULES), \
		$(COV_CC) $(COV_CFLAGS) tests/test_$(m).c $(m).c $(DEPS_$(m)) -lm -o $(BUILD)/cov/test_$(m); \
		LLVM_PROFILE_FILE=$(BUILD)/cov/$(m).profraw $(BUILD)/cov/test_$(m) > /dev/null;)
	@$(LLVM_PROFDATA) merge -sparse $(BUILD)/cov/*.profraw -o $(BUILD)/cov/all.profdata
	@$(LLVM_COV) report $(BUILD)/cov/test_$(firstword $(MODULES)) \
		$(foreach m,$(wordlist 2,$(words $(MODULES)),$(MODULES)),-object $(BUILD)/cov/test_$(m)) \
		-instr-profile=$(BUILD)/cov/all.profdata $(foreach m,$(MODULES),$(m).c)

clean:
	@rm -rf build
