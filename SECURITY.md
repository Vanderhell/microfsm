# Security Policy

microfsm is a source-distributed library. This repository snapshot does not claim a hardened or fully verified release.

## Supported versions

Security fixes are expected against the latest maintained branch state rather than an asserted released series.

## Reporting a vulnerability

Provide:

- the affected file or commit
- a minimal reproducer
- the observed impact
- any configuration details that matter

If private security reporting is available on the hosting platform, use it. Otherwise open a minimal issue requesting a private contact path without publishing sensitive details.

## Scope notes

- The library validates machine definitions and runtime state, but application callbacks remain application code.
- The library does not provide sandboxing, privilege separation, durable storage, rollback, or fault recovery.
- ISR dispatch, asynchronous callback escape, and externally unsynchronized shared-instance use are out of contract.
