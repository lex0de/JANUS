# JANUS - Danyal A. Samak <dabsamak@tuta.com>
# Makefile: bootstrap checks and experimental hosted M0 model.
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
	    'make test-sanitize CC=gcc - model under ASan/UBSan (also clang)'

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
