# Changelog

All notable changes to this project are documented here.

## Unreleased

- No unreleased changes recorded.

## 1.0.0 - 2026-07-02

### Changed

- Replaced compiler-dependent public error enums with fixed-width ABI types and fixed values.
- Added an authoritative generated configuration header contract for source and installed consumers.
- Stabilized public struct layouts across names-enabled, names-disabled, trace-enabled, and trace-disabled builds.
- Reworked initialization, validation, runtime state checks, query APIs, reset semantics, tracing, and same-instance reentrancy handling.
- Added CMake package support, standalone consumer fixtures, compile-fail fixtures, and expanded CI and release workflows.
- Rewrote repository documentation to reflect current limits, verification status, and tag-based release policy.
