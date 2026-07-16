# AUR packages

Three packages, one directory each. The PKGBUILDs here are the source of
truth; the AUR repos mirror them.

| package        | what it ships                        | AUR repo                                    |
|----------------|--------------------------------------|---------------------------------------------|
| uplink-irc     | latest release, built from source    | ssh://aur@aur.archlinux.org/uplink-irc.git     |
| uplink-irc-bin | prebuilt binary from the release     | ssh://aur@aur.archlinux.org/uplink-irc-bin.git |
| uplink-irc-git | main HEAD, built from source         | ssh://aur@aur.archlinux.org/uplink-irc-git.git |

scripts/release.sh bumps pkgver and resets pkgrel for uplink-irc and
uplink-irc-bin. uplink-irc-git never needs a bump, its pkgver comes from
git describe at build time.

Checksums can't be updated until the tag (and for -bin, the release
artifacts from release.yml) exist on GitHub. So after each release, for
uplink-irc and uplink-irc-bin:

```bash
cd packaging/archlinux/<package>
updpkgsums
makepkg --printsrcinfo > .SRCINFO
makepkg -f          # test build
git add PKGBUILD .SRCINFO && git commit

# mirror to AUR (working clones live in ~/Projects/aur/)
cp PKGBUILD .SRCINFO ~/Projects/aur/<package>/
cd ~/Projects/aur/<package>
git add -A && git commit -m "<version>" && git push
```

makepkg leaves src/, pkg/, tarballs and built packages in these
directories. They're gitignored, don't commit them.
