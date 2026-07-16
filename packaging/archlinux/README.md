# AUR package (uplink-irc)

The PKGBUILD here is the source of truth; the AUR repo at
`ssh://aur@aur.archlinux.org/uplink-irc.git` mirrors it.

`scripts/release.sh` bumps `pkgver` and resets `pkgrel` automatically.
The checksum can only be refreshed after the tag exists on GitHub, so the
AUR update is a manual step after each release:

```bash
cd packaging/archlinux
updpkgsums                          # refresh sha256 for the new tarball
makepkg --printsrcinfo > .SRCINFO
makepkg -f                          # sanity build (optional but recommended)
git add PKGBUILD .SRCINFO && git commit

# then mirror to AUR
cp PKGBUILD .SRCINFO /path/to/aur/uplink-irc/
cd /path/to/aur/uplink-irc
git add -A && git commit -m "Update to <version>" && git push
```

Note: `makepkg -f` leaves `src/`, `pkg/`, tarballs, and built packages in
this directory — they are untracked; don't commit them.
