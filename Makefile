# JANUS - Danyal A. Samak <dabsamak@tuta.com>
# Makefile: integrity, M0 model, M1 hosted and M2 native experiments.
PYTHON = python3
CC = cc
MODEL_FLAGS = -std=c17 -Wall -Wextra -Wpedantic -Werror -Wconversion \
	-Wshadow -Wstrict-prototypes -Wmissing-prototypes -g -O1
MODEL_SOURCES = lib/contract/model.c tests/contract/test_model.c

.PHONY: all help check toolchain preflight deps-plan test test-sanitize

all: help

help:
	@printf '%s\n' \
	    'make check       - bootstrap integrity/configuration/syntax' \
	    'make toolchain   - explicit GCC/Clang C/C++ and sanitizer probes' \
	    'make preflight   - non-elevated local host inventory' \
	    'make deps-plan   - Debian 13 missing-package simulation' \
	    'make test CC=gcc - hosted M0 contract model (also use CC=clang)' \
	    'make test-sanitize CC=gcc - model under ASan/UBSan (also clang)' \
	    'make hosted CC=gcc - Linux hosted broker and client' \
	    'make test-hosted CC=gcc - deterministic hosted tests' \
	    'make test-hosted-live FIXTURE=artifacts/new-lab - explicit disposable VM' \
	    'make native CC=gcc - M2 object service/tools and static note' \
	    'make test-native CC=gcc - M2 unit/storage process-crash tests' \
	    'make test-native-live CC=gcc FIXTURE=artifacts/new-m2 - explicit M2 lab'

check:
	$(PYTHON) tools/check-bootstrap.py

toolchain:
	./tools/probe-toolchain.sh

preflight:
	./tools/host-preflight.sh

deps-plan:
	./tools/install-deps.sh

test:
	mkdir -p out
	$(CC) $(CPPFLAGS) $(MODEL_FLAGS) $(CFLAGS) -Iinclude $(MODEL_SOURCES) \
	    $(LDFLAGS) -o out/test-model-$(CC)
	./out/test-model-$(CC)

test-sanitize:
	mkdir -p out
	$(CC) $(CPPFLAGS) $(MODEL_FLAGS) $(CFLAGS) -Iinclude \
	    -fno-omit-frame-pointer -fsanitize=address,undefined \
	    -fno-sanitize-recover=all $(MODEL_SOURCES) $(LDFLAGS) \
	    -o out/test-model-sanitize-$(CC)
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
	    UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
	    ./out/test-model-sanitize-$(CC)

ANALYZE ?= -fanalyzer

HOSTED_COMMON = lib/hosted/protocol.c lib/hosted/files.c lib/hosted/state.c \
	lib/hosted/viewer.c
HOSTED_FLAGS = $(MODEL_FLAGS) -Iinclude $(shell pkg-config --cflags libvirt)
HOSTED_LIBS = $(shell pkg-config --libs libvirt)
.PHONY: hosted test-hosted test-hosted-sanitize analyze-hosted format-hosted
hosted:
	mkdir -p out
	$(CC) $(HOSTED_FLAGS) $(CFLAGS) $(HOSTED_COMMON) lib/hosted/libvirt.c \
	    cmd/janusd/main.c $(LDFLAGS) $(HOSTED_LIBS) -o out/janusd
	$(CC) $(HOSTED_FLAGS) $(CFLAGS) lib/hosted/protocol.c lib/hosted/files.c \
	    cmd/janusctl/main.c $(LDFLAGS) -o out/janusctl

test-hosted:
	mkdir -p out
	$(CC) $(HOSTED_FLAGS) $(CFLAGS) $(HOSTED_COMMON) tests/hosted/test_hosted.c \
	    $(LDFLAGS) -o out/test-hosted-$(CC)
	./out/test-hosted-$(CC)

test-hosted-sanitize:
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 $(MAKE) test-hosted CC=$(CC) \
	    CFLAGS='-fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all' \
	    LDFLAGS='-fsanitize=address,undefined'

analyze-hosted:
	mkdir -p out
	@set -e; for src in $(HOSTED_COMMON) lib/hosted/libvirt.c cmd/janusd/main.c cmd/janusctl/main.c tests/hosted/test_hosted.c tests/hosted/guest_init.c; do \
	    echo "Analyzing $$src"; \
	    $(CC) $(HOSTED_FLAGS) $(ANALYZE) -c "$$src" -o out/analyze.o; \
	done

format-hosted:
	clang-format --dry-run --Werror include/janus/hosted/*.h lib/hosted/*.c \
	    cmd/janusd/*.c cmd/janusctl/*.c tests/hosted/*.c

.PHONY: test-hosted-live
test-hosted-live: hosted
	@test -n "$(FIXTURE)" || { echo 'Set FIXTURE to a NEW directory under artifacts/'; exit 2; }
	$(PYTHON) tests/hosted/live.py $(if $(filter 1,$(RETRY)),--retry,--create) "$(FIXTURE)"

NATIVE_FLAGS = $(MODEL_FLAGS) -Iinclude
NATIVE_COMMON = lib/native/protocol.c lib/native/local.c lib/native/sandbox.c lib/native/cli.c
NATIVE_STORE = lib/native/store.c
.PHONY: native
native:
	mkdir -p out
	$(CC) $(NATIVE_FLAGS) $(CFLAGS) $(NATIVE_COMMON) $(NATIVE_STORE) cmd/janus-objectd/main.c $(LDFLAGS) -lsqlite3 -o out/janus-objectd
	$(CC) $(NATIVE_FLAGS) $(CFLAGS) $(NATIVE_COMMON) cmd/janus-objectctl/main.c $(LDFLAGS) -o out/janus-objectctl
	$(CC) $(NATIVE_FLAGS) $(CFLAGS) $(NATIVE_COMMON) cmd/janus-run/main.c $(LDFLAGS) -o out/janus-run
	$(CC) $(NATIVE_FLAGS) -static lib/native/protocol.c lib/native/cli.c cmd/janus-note/main.c -o out/janus-note

.PHONY: test-native test-native-sanitize analyze-native format-native
test-native:
	mkdir -p out
	$(CC) $(NATIVE_FLAGS) $(CFLAGS) $(NATIVE_COMMON) $(NATIVE_STORE) tests/native/test_native.c $(LDFLAGS) -lsqlite3 -o out/test-native-$(CC)
	./out/test-native-$(CC)

test-native-sanitize:
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 $(MAKE) test-native CC=$(CC) CFLAGS='-fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all' LDFLAGS='-fsanitize=address,undefined'

analyze-native:
	mkdir -p out
	@set -e; for src in $(NATIVE_COMMON) $(NATIVE_STORE) cmd/janus-objectd/main.c cmd/janus-objectctl/main.c cmd/janus-run/main.c cmd/janus-note/main.c tests/native/test_native.c; do \
	    echo "Analyzing $$src"; $(CC) $(NATIVE_FLAGS) $(ANALYZE) -c "$$src" -o out/analyze-native.o; \
	done

format-native:
	clang-format --dry-run --Werror include/janus/native/*.h lib/native/*.c cmd/janus-objectd/*.c cmd/janus-objectctl/*.c cmd/janus-run/*.c cmd/janus-note/*.c tests/native/*.c

.PHONY: test-native-live
test-native-live: native hosted test-native
	@test -n "$(FIXTURE)" || { echo 'Set FIXTURE to a NEW directory under artifacts/'; exit 2; }
	$(PYTHON) tests/native/live.py --fixture "$(FIXTURE)" --cc "$(CC)"
