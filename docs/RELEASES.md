# Release Policy

microfsm uses a tag-based release workflow.

## Rules

- Branch pushes do not create releases.
- Pushing a tag matching `v*` is the only release trigger.
- The workflow verifies the tag and generates release notes through GitHub CLI.
- Untracked local build artifacts are not part of the release process.

See [`.github/workflows/release.yml`](../.github/workflows/release.yml) and [CHANGELOG.md](../CHANGELOG.md).
