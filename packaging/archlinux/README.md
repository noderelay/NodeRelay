# AUR package (uplink-irc)

The PKGBUILD here is the source of truth. The AUR repo at
ssh://aur@aur.archlinux.org/uplink-irc.git mirrors it.

scripts/release.sh bumps pkgver and resets pkgrel. The checksum can't be
updated until the tag exists on GitHub, so after each release:

```bash
cd packaging/archlinux
updpkgsums
makepkg --printsrcinfo > .SRCINFO
makepkg -f          # test build
git add PKGBUILD .SRCINFO && git commit

# mirror to AUR
cp PKGBUILD .SRCINFO /path/to/aur/uplink-irc/
cd /path/to/aur/uplink-irc
git add -A && git commit -m "<version>" && git push
```

makepkg leaves src/, pkg/, tarballs and built packages in this
directory. They're gitignored, don't commit them.
