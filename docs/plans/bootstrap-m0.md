# B0 and M0 execution plan

Scope: explicit 20 September 2026 bootstrap request; J-001 through J-018.
Integration owner: this session. No parallel workers. Stop at M0 READY FOR
REVIEW; no maintainer acceptance, M1 implementation or hardware assignment.

| Work | State | Acceptance |
| --- | --- | --- |
| Inspect sources, Git boundaries and actual host | PASS | No ancestor Git root; private inventory retained |
| Bootstrap and compiler checks | PASS | make check, deps-plan, toolchain; retain sandbox failures |
| Candidate contracts and threat model | PASS | Explicit boundaries, errors, persistence and authority |
| Portable bounded C model | PASS | C17, no runtime dependencies beyond libc |
| Independent expected-result tables | PASS | GCC/Clang, sanitizers, malformed state/input, ordered races |
| Boundary review and evidence | PASS | Trace entry points; record findings and limitations |
| Reviewed commits/private remote | PASS | Private lex0de/JANUS; implementation SHA verified; publication.md |

Paths: docs/contracts/, docs/decisions/, docs/evidence/, include/janus/,
lib/contract/, tests/contract/, Makefile, README.md, AI_CONTEXT.md and requirement
map. Whitepaper remains unchanged. Raw inventory/build logs stay in artifacts/.

Oracle: write candidate rules before code and encode expected outcomes separately
in test tables, including mutation-free rejection. One author writes both: this
is specification-derived testing, not independent human review. Exercise ordered
revocation/operation interleavings, not OS thread or DMA enforcement.

Observed setup: Debian 13; sandbox restricts libvirt and LeakSanitizer. Host
probes pass outside sandbox. Dependency simulation proposes eight new packages,
no upgrades/removals; installation completed; all required tools now present. No VM changes made. Existing Git
identity preserved. No kernel, ABI, storage format or licence selected.

Final local validation: 6920 assertions per GCC/Clang plain and sanitizer run;
static analyses and make check PASS. See docs/evidence/m0.md. Private implementation publication verified. No host configuration or guest changes.
