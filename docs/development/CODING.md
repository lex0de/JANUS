# C/C++ engineering conventions

The user requests OpenBSD-quality code and conventions where possible. Apply the
review discipline, small interfaces, explicit ownership and conservative failure
handling from the supplied guide. OpenBSD style(9) is a style reference, not a
certification JANUS can acquire by using a formatter. Source: [STYLE] in
`docs/SOURCES.md`.

## New code and inherited code

Use tabs at eight-column stops, roughly 80-column lines, K&R statement braces,
separate-line return types for function definitions, `snake_case`, and
`/* ... */` comments. Keep header ordering intentional: system headers, relevant
network headers, other standard/library headers, then local headers, with useful
group separation. Avoid cosmetic churn in imports. No fake `$OpenBSD$` revision
markers or claims that OpenBSD maintains this code.

The `.clang-format` file is an approximation for new standalone code, not an
exact implementation of style(9). Review its diff. It must not reformat imported
subtrees automatically. Header names/prototypes and debugger visibility follow
the actual component and substrate, not blindly copied BSD implementation macros.
Use standard `extern "C"` guards in portable C headers shared with C++.

## C baseline

For hosted M0 experiments, start with C17. Prefer `size_t` for in-memory sizes,
`ssize_t` for POSIX I/O results, and fixed-width integers only where the contract
needs them. Validate conversion between widths and signedness. Keep private
userspace helpers `static`; declare exported interfaces in narrow headers. Avoid
opaque typedefs that conceal ownership or pointer types without a good reason.

A minimal style example, not a JANUS API:

```c
static int
size_product(size_t a, size_t b, size_t *out)
{
	if (out == NULL || (b != 0 && a > SIZE_MAX / b))
		return (-1);
	*out = a * b;
	return (0);
}
```

Include the correct standard headers in real code. Do not transplant compiler or
kernel-private types into public portable headers. Keep wire formats explicitly
encoded, bounded and versioned; do not serialize an in-memory struct.

## Restrained C++

C++17 is the initial hosted-test baseline, not a requirement to use C++ in every
component. Prefer RAII for file descriptors, mappings, locks and allocations with
explicit, non-throwing cleanup. Use value semantics and clear ownership transfer.
Keep C-compatible boundaries free of exceptions and C++ runtime object layouts.

Select an error model per component. A component that catches allocation errors
at its boundary is different from one built without exception support. Under a
no-exceptions policy, ordinary standard-container allocation may still terminate;
do not claim recoverability without a real fallible or preallocated strategy.
Similarly, `noexcept` is a contract, not an error-handling implementation.

Avoid deep inheritance, owning raw pointers, unnecessary templates, global
initialisation side effects, and implicit broad dependencies. Prefer a small
explicit state machine over a generic framework for state machines. Document
whether exceptions, RTTI and dynamic allocation are permitted, and why. Existing
substrates may impose their own C++ version, runtime and build requirements.

## Trust boundaries and error paths

Check all input lengths, enum values, integer products/sums, alignment and bounds.
Validate `snprintf` for both a negative result and truncation. Parse numeric input
with a checked conversion and full-string/range validation, not `atoi`.
Use constant format strings. Avoid unbounded stack allocation and input-sized
VLAs. Initialise outbound data and do not leak padding or stale bytes.

Document pointer ownership and lifetime, resource-release order, and what changes
on failure. Handle short read/write, `EINTR`, cancellation, timeout and allocation
failure where possible. Use one clear failure-unwind path when it improves review;
structured `goto` cleanup is acceptable in C. Never continue after a failed
security prerequisite or return success for an unimplemented operation.

Do not pass user-controlled strings to `system`, `popen`, or a shell. Use an API or
an explicit argument vector. A validated domain name is not permission to manage
any libvirt domain: validate the mapping to the authorised UUID/backend too.

OpenBSD APIs such as pledge/unveil, BSD allocation helpers and compiler annotations
must not be assumed available or semantically equivalent on Debian. Portability
adapters need real tests and honest reduced guarantees; a no-op security shim is
not sandboxing. Use established libraries for cryptography; do not create novel
cryptographic algorithms as part of an unrelated bootstrap task.

## Concurrency, kernel and device code

Specify lock ordering, which context may block, lifetime while callbacks run,
shared-memory ownership and message completion. `volatile` is not synchronisation.
Avoid clever lock-free code without a need, memory-order argument and tests.

Future kernel/driver code must use the selected substrate's actual allocators,
interrupt and DMA interfaces. Record mapping lifetimes, ordering/fences, detach,
reset and cancellation. Stop DMA/interrupts and outstanding callbacks before
releasing associated resources. A userspace unit-test build is not target evidence.
Never load a new driver on the Debian host merely because it compiled.

## Compiler and analysis policy

For new hosted code, begin with supported warnings such as `-Wall -Wextra
-Wpedantic -Werror`, and C-only prototype warnings on C files. Evaluate stricter
conversion/shadow warnings per component; fix causes instead of broadly disabling
warnings. Do not impose hosted Linux flags on a future freestanding kernel.
No `-march=native` in portable reference builds. Keep fixed-baseline portability
runs separate from host-feature/nested-virtualisation tests.

Run both GCC and Clang. Use AddressSanitizer/UndefinedBehaviorSanitizer where the
runtime works, with non-recovering diagnostics. Use static analysis and fuzzing
against real malformed inputs. A failed sanitizer startup is a failure or blocker,
not a clean application test; do not silently rerun without instrumentation and
report that as a sanitizer pass. Formatting does not replace these checks.

The supplied `make toolchain` target compiles tiny fixtures only. Add component
build/test targets as real code appears. Keep expected test outcomes independent
of the implementation's tables and include regression tests for each bug.

## Shell and other tooling

Use POSIX shell for simple host automation and quote paths/arguments. No untrusted
`.env` sourcing, curl-to-shell bootstrap, `eval`, secret tracing, or broad recursive
cleanup. Preserve command exit status; POSIX shell pipelines do not provide
portable `pipefail`. Use explicit conditionals where a failure is expected.

Python is acceptable for development helpers, not required in JANUS early boot.
Prefer its standard library, tabs, single-quoted strings, accurate type hints,
small functions and concise docstrings. No unnecessary asynchronous framework.
Use spaces where a format such as YAML requires them. Keep generated output in
ignored directories and never ignore source merely because it is called build.
