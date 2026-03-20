# Security Policy

## Supported Versions

microfsm is a small, source-distributed C library. Security fixes are applied
to the latest published state of the repository.

At this stage, support is provided for:

| Version | Supported |
|---------|-----------|
| 1.x     | Yes       |
| < 1.0   | No        |

## Reporting a Vulnerability

If you believe you have found a security issue in microfsm, please report it
responsibly and do not open a public issue with full exploit details first.

Please include:

- A clear description of the issue
- Affected version, commit, or file
- Reproduction steps or a minimal proof of concept
- Expected impact
- Any suggested mitigation, if known

## Preferred Disclosure Process

Please contact the maintainer, Vanderhell, through the repository contact
channel or a private security report mechanism if enabled on GitHub.

If private reporting is not available yet, open a minimal issue requesting
private contact without publishing sensitive technical details.

## Response Expectations

The project aims to:

- Acknowledge receipt of a report within 7 days
- Triage and assess impact as quickly as practical
- Coordinate a fix before public disclosure when reasonable
- Credit the reporter after disclosure, if they want attribution

## Scope

This policy applies to:

- The C library source in `include/` and `src/`
- Test and example code where a flaw could affect downstream users
- Build and repository configuration that could impact consumers

This policy does not guarantee fixes for issues caused solely by:

- Unsupported toolchains or heavily modified downstream forks
- Insecure application-specific integration code outside this repository
- Misuse that contradicts documented API constraints

## Security Notes for Users

microfsm is designed to be small and predictable, but integrators are still
responsible for:

- Validating untrusted inputs before dispatching events
- Avoiding unsafe callback behavior such as re-entrant dispatch
- Reviewing compile-time configuration for their target environment
- Testing their full application-level state transitions
