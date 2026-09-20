# Evidence records

Store concise, sanitised validation reports here. Keep raw logs, system inventory,
crash dumps and VM outputs in ignored `artifacts/` unless deliberately reviewed
for inclusion. Use `docs/templates/EVIDENCE.md`.

A source filename or proposed command is not execution evidence. Distinguish
bootstrap validation, toolchain probes, contract models, real enforcing services,
actual guest boots, and physical-device results. Only observed tests get PASS.

`bundle-validation.md`, when present, describes tests performed while packaging
this archive in the assistant's environment. It is not a report about the user's
Debian machine and does not mark M0 or nested virtualisation complete.
