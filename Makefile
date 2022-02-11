CC := gcc
CCFLAGS := \
  -std=c99 \
  -Wall \
  -Werror \
  -g \
  -O0 \
  -Isrc \
  -Isrc/audio \
  -Isrc/third_party \
  -Isrc/utils \
  -Ibuild/lib

LDFLAGS := \
  -Lbuild/lib \
  -lstt \
  -ltensorflowlite \
  -ltflitedelegates \
  -lpulse \
  -lpulse-simple

TEST_CCFLAGS := \
  -fsanitize=address \
  -fsanitize=undefined \
  -fno-sanitize-recover=all \
  -fsanitize=float-divide-by-zero \
  -fsanitize=float-cast-overflow \
  -fno-sanitize=null \
  -fno-sanitize=alignment \
  -fno-omit-frame-pointer

DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)$*.d
TEST_DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)$*_test.d

BUILDDIR = build/
OBJDIR := $(BUILDDIR)obj/
BINDIR := $(BUILDDIR)bin/
DEPDIR := $(BUILDDIR)dep/
LIBDIR := $(BUILDDIR)lib/

OBJS := $(addprefix $(OBJDIR),$(subst .c,.o,$(SRCS)))
TEST_OBJS := $(addprefix $(OBJDIR),$(subst .c,.o,$(TEST_SRCS)))

.PHONY: all clean test

all: \
  $(BINDIR)file_utils_test \
  $(BINDIR)string_utils_test \
  $(BINDIR)yargs_test \
  $(BINDIR)settings_test \
  $(BINDIR)app_main_test \
  $(BINDIR)spchcat

clean:
	rm -rf $(BUILDDIR)

clean_src:
	rm -rf $(OBJDIR)
	rm -rf $(BINDIR)
	rm -rf $(DEPDIR)

test: \
  run_file_utils_test \
  run_string_utils_test \
  run_yargs_test \
  run_settings_test \
  run_pa_list_devices_test \
  run_audio_buffer_test \
  run_wav_io_test \
  run_app_main_test

$(OBJDIR)%.o: %.c $(DEPDIR)/%.d | $(DEPDIR)
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(DEPDIR)$*_test.d)
	$(CC) $(CCFLAGS) $(DEPFLAGS) -c $< -o $@

$(OBJDIR)%_test.o: %_test.c $(DEPDIR)/%.d | $(DEPDIR)
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(DEPDIR)$*.d)
	$(CC) $(CCFLAGS) $(TEST_CCFLAGS) $(TEST_DEPFLAGS) -c $< -o $@

$(BINDIR)file_utils_test: \
  $(OBJDIR)src/utils/file_utils_test.o \
  $(OBJDIR)src/utils/string_utils.o
	@mkdir -p $(dir $@) 
	$(CC) $(CCFLAGS) $(TEST_CCFLAGS) $^ -o $@

run_file_utils_test: $(BINDIR)file_utils_test
	$<

$(BINDIR)string_utils_test: \
  $(OBJDIR)src/utils/string_utils_test.o
	@mkdir -p $(dir $@) 
	$(CC) $(CCFLAGS) $(TEST_CCFLAGS) $^ -o $@

run_string_utils_test: $(BINDIR)string_utils_test
	$<

$(BINDIR)yargs_test: \
  $(OBJDIR)src/utils/string_utils.o \
  $(OBJDIR)src/utils/yargs_test.o
	@mkdir -p $(dir $@) 
	$(CC) $(CCFLAGS) $(TEST_CCFLAGS) $^ -o $@

run_yargs_test: $(BINDIR)yargs_test
	$<

$(BINDIR)pa_list_devices_test: \
  $(OBJDIR)src/utils/string_utils.o \
  $(OBJDIR)src/audio/pa_list_devices_test.o
	@mkdir -p $(dir $@) 
	$(CC) $(CCFLAGS) $(TEST_CCFLAGS) -lpulse $^ -o $@

run_pa_list_devices_test: $(BINDIR)pa_list_devices_test
	$<

$(BINDIR)audio_buffer_test: \
  $(OBJDIR)src/audio/audio_buffer_test.o
	@mkdir -p $(dir $@) 
	$(CC) $(CCFLAGS) $(TEST_CCFLAGS) $^ -o $@

run_audio_buffer_test: $(BINDIR)audio_buffer_test
	$<

$(BINDIR)wav_io_test: \
  $(OBJDIR)src/utils/file_utils.o \
  $(OBJDIR)src/utils/string_utils.o \
  $(OBJDIR)src/audio/audio_buffer.o \
  $(OBJDIR)src/audio/wav_io_test.o
	@mkdir -p $(dir $@) 
	$(CC) $(CCFLAGS) $(TEST_CCFLAGS) -lm $^ -o $@

run_wav_io_test: $(BINDIR)wav_io_test
	$<

$(BINDIR)settings_test: \
  $(OBJDIR)src/settings_test.o \
  $(OBJDIR)src/utils/file_utils.o \
  $(OBJDIR)src/utils/string_utils.o \
  $(OBJDIR)src/utils/yargs.o
	@mkdir -p $(dir $@) 
	$(CC) $(CCFLAGS) $(TEST_CCFLAGS) $^ -o $@

run_settings_test: $(BINDIR)settings_test
	$<

$(BINDIR)app_main_test: \
 $(OBJDIR)src/app_main_test.o \
 $(OBJDIR)src/settings.o \
 $(OBJDIR)src/audio/audio_buffer.o \
 $(OBJDIR)src/audio/pa_list_devices.o \
 $(OBJDIR)src/audio/wav_io.o \
 $(OBJDIR)src/utils/file_utils.o \
 $(OBJDIR)src/utils/string_utils.o \
 $(OBJDIR)src/utils/yargs.o
	@mkdir -p $(dir $@) 
	$(CC) $(CCFLAGS) $(TEST_CCFLAGS) $^ -o $@ $(LDFLAGS)

run_app_main_test: $(BINDIR)app_main_test
	$<

$(BINDIR)spchcat: \
 $(OBJDIR)src/app_main.o \
 $(OBJDIR)src/main.o \
 $(OBJDIR)src/settings.o \
 $(OBJDIR)src/audio/audio_buffer.o \
 $(OBJDIR)src/audio/pa_list_devices.o \
 $(OBJDIR)src/audio/wav_io.o \
 $(OBJDIR)src/utils/file_utils.o \
 $(OBJDIR)src/utils/string_utils.o \
 $(OBJDIR)src/utils/yargs.o
	@mkdir -p $(dir $@) 
	$(CC) $^ -o $@ $(LDFLAGS)

$(DEPDIR): ; @mkdir -p $@

SRCS := $(shell find src/ -type f -name '*.c')
DEPFILES := $(SRCS:%.c=$(DEPDIR)/%.d)
$(DEPFILES):

include $(wildcard $(DEPFILES))