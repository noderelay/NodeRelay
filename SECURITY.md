# Security Policy

## Supported versions

Only the latest release is supported with security fixes. Uplink uses calendar
versioning (`year.month.fix`, e.g. `2026.7.0`) — a security fix ships as a new
release with the last digit bumped (e.g. `2026.7.1`).

## Reporting a vulnerability

Please do **not** open a public GitHub issue for security vulnerabilities.

Report security issues privately to the maintainer:

- GitHub: [@noderelay](https://github.com/noderelay)
- Email: joseph.d.harris78@gmail.com

Include:
- A clear description of the vulnerability
- Steps to reproduce or a proof-of-concept
- The version of Uplink affected
- Any suggested mitigations

You can expect an acknowledgement within a few days and a fix or status update within 30 days.

## Scope

In-scope:
- Remote code execution or command injection via IRC messages or DCC
- SSRF via URL preview fetching
- Path traversal in DCC file receive
- Config file exposure (passwords stored in plaintext)
- Any vulnerability allowing an IRC peer to read local files or execute commands

Out of scope:
- Issues requiring physical access to the machine
- Social engineering
- Vulnerabilities in upstream dependencies (report those to the upstream project)

## Security notes

- IRC server, NickServ, and SASL passwords are stored in the OS keychain — the config file only holds a `<keychain>` sentinel. If no keychain service is available, passwords are kept in memory for the session and not persisted. Passwords typed directly into `config.toml` by hand are migrated to the keychain when saved from the server dialog.
- DCC file transfers connect directly to the peer's IP address. Only accept DCC offers from trusted users.
- URL previews fetch metadata from linked sites, which receives your IP address. Previews are disabled by default. Enable with `link_previews = true` in `[privacy]`.
