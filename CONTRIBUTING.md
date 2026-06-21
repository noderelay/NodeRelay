# Contributing to Uplink

Thanks for your interest in Uplink. Before writing any code, please read this.

## Open an issue first

Do not submit a pull request without opening an issue first. Describe what you want to change and why. Wait for a maintainer to confirm it fits the project's direction before writing code. PRs without a prior discussion will be closed.

## Project goals

Uplink aims to be **stable, secure, fast, and lightweight**. Every change is weighed against these goals. Features that add resource overhead, complexity, or attack surface without a clear need will be rejected.

## Testing

All submissions are built and run against the project's test suites, including unit tests (IRC parser, chat formatting) and fuzz testing. PRs that break existing tests or introduce untested behavior will not be merged. If your change touches protocol handling or message rendering, run `ctest --test-dir build` before submitting.

## Code style

- Read the existing code before submitting changes. Match the style exactly.
- Minimal comments. Don't add block descriptions above simple functions.
- Keep changes small and focused. One fix or feature per PR.
- Don't add dependencies unless absolutely necessary.
- Follow C++17 and Qt6 conventions used throughout the codebase.

## What we won't merge

- Large unsolicited features that weren't discussed in an issue
- AI-generated code that hasn't been adapted to match the project's style
- Changes that increase memory usage or resource overhead without justification
- Code that introduces bugs, skips error handling, or weakens security
