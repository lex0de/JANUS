# JANUS - Danyal A. Samak <dabsamak@tuta.com>
# Makefile: bootstrap checks, M0 model and experimental M1 hosted boundary.
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
	    'make test-hosted-live FIXTURE=artifacts/new-lab - explicit disposable VM'

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
