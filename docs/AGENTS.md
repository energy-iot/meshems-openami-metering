# Agent Instructions

## Code Style

### Do
- use feature flags (`#ifdef ENABLE_*`) to guard all optional functionality
- keep each source file focused on a single peripheral or subsystem
- prefer small, targeted diffs — fix the thing that needs fixing, nothing more
- use `#pragma once` for all header guards
- use `const` and `constexpr` wherever values are known at compile time
- match the naming conventions already present in the file you are editing

### Don't
- do not hard-code pin numbers or timing values — use the constants in `config.cpp` / headers
- do not add new library dependencies without approval
- do not rewrite unrelated files as part of a focused fix
- do not leave debug `Serial.print` calls in committed code unless they are guarded by `#ifdef ENABLE_DEBUG`

## Commenting Style

### Do
- write a brief block comment above every function explaining **what** it does and any non-obvious side-effects
- use inline comments sparingly — only to explain **why**, not **what**
- document hardware-specific magic numbers (register addresses, bitmasks, timing constants) with a short note referencing the datasheet or protocol
- keep comments up to date when you change the code they describe

### Don't
- do not write comments that simply restate the code (`i++; // increment i`)
- do not leave commented-out code blocks — remove dead code instead
- do not use multi-line `/* */` block comments for function-level documentation; prefer `//` line comments

## README Maintenance

Whenever you add, remove, or rename a section in `README.md`, update the Table of Contents to match — section titles, anchors, and ordering must stay in sync with the document body.

### Do

- update the ToC immediately after editing any heading in `README.md`
- verify anchor links are correct (lowercase, spaces replaced with `-`, punctuation stripped)

### Don't

- do not leave the ToC out of date after a README change, no matter how minor

## Changelog Maintenance

Whenever you make any change to the codebase — bug fixes, new features, refactors, dependency updates, build system changes, or documentation updates — you **must** update `CHANGELOG.md` before finishing the task.

### Do
- add all new entries under the `## [Unreleased]` section at the top (create it if absent)
- group entries under `### Added`, `### Changed`, `### Fixed`, `### Removed`, or `### Security`
- reference the affected file(s) in backticks with a concise description of **what** changed and **why**
- group related file changes under a single bullet when they are part of the same fix or feature

### Don't
- do not skip the changelog — every change, no matter how small, requires an entry
- do not modify released version sections — only add to `## [Unreleased]`
