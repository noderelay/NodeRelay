# winget (Windows)

The three manifests here are the source of truth; they live upstream at
microsoft/winget-pkgs under manifests/n/NodeRelay/Uplink/<version>/.

Users install with:

```
winget install NodeRelay.Uplink
```

After each release: bump PackageVersion + InstallerUrl, update
InstallerSha256 (uppercase; `sha256sum Uplink-v<ver>-windows-x64.zip`),
update ReleaseNotesUrl, then PR the new version folder to
microsoft/winget-pkgs (the noderelay fork exists; files can be added
via the GitHub contents API without cloning the huge repo). Their
pipeline validates; a moderator merges.
