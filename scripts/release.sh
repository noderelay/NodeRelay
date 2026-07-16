#!/usr/bin/env bash
# Usage: scripts/release.sh 0.25.10
# Bumps version everywhere, commits, tags, and pushes.
set -euo pipefail

VER="${1:?Usage: $0 <version>  e.g. 0.25.10}"
TAG="v${VER}"

# Validate semver-ish
if ! [[ "$VER" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "Error: version must be X.Y.Z" >&2
    exit 1
fi

echo "Bumping to ${TAG}..."

# CMakeLists.txt
sed -i -E "s/project\(UplinkIRC VERSION [0-9]+\.[0-9]+\.[0-9]+/project(UplinkIRC VERSION ${VER}/" CMakeLists.txt

# README.md — download badge hrefs
sed -i -E \
    "s|Uplink-[0-9]+\.[0-9]+\.[0-9]+-x86_64\.AppImage|Uplink-${VER}-x86_64.AppImage|g;
     s|Uplink-v[0-9]+\.[0-9]+\.[0-9]+-linux-x86_64\.tar\.gz|Uplink-${TAG}-linux-x86_64.tar.gz|g;
     s|Uplink-v[0-9]+\.[0-9]+\.[0-9]+-windows-x64\.zip|Uplink-${TAG}-windows-x64.zip|g;
     s|Uplink-v[0-9]+\.[0-9]+\.[0-9]+-macos-arm64\.dmg|Uplink-${TAG}-macos-arm64.dmg|g" \
    README.md

# docs/index.html — download card hrefs + display strings
sed -i -E \
    "s|Uplink-[0-9]+\.[0-9]+\.[0-9]+-x86_64\.AppImage|Uplink-${VER}-x86_64.AppImage|g;
     s|Uplink-v[0-9]+\.[0-9]+\.[0-9]+-linux-x86_64\.tar\.gz|Uplink-${TAG}-linux-x86_64.tar.gz|g;
     s|Uplink-v[0-9]+\.[0-9]+\.[0-9]+-windows-x64\.zip|Uplink-${TAG}-windows-x64.zip|g;
     s|Uplink-v[0-9]+\.[0-9]+\.[0-9]+-macos-arm64\.dmg|Uplink-${TAG}-macos-arm64.dmg|g;
     s|(<b>)v[0-9]+\.[0-9]+\.[0-9]+(</b>)|\1${TAG}\2|g;
     s|(footer-logo\">Uplink )v[0-9]+\.[0-9]+\.[0-9]+(</div>)|\1${TAG}\2|g;
     s|(Uplink <span>)v[0-9]+\.[0-9]+\.[0-9]+(</span>)|\1${TAG}\2|g;
     s|(Download )v[0-9]+\.[0-9]+\.[0-9]+|\1${TAG}|g;
     s|v[0-9]+\.[0-9]+\.[0-9]+ — Latest release|${TAG} — Latest release|g;
     s|(class=\"version-tag\">)v[0-9]+\.[0-9]+\.[0-9]+(</div>)|\1${TAG}\2|g" \
    docs/index.html

# docs/quality.html — page-eyebrow <b> tag + footer-logo display strings.
# Targeted patterns only, so historical version references and the Qt test
# version (e.g. "QtTest 6.11.1") are left untouched.
sed -i -E \
    "s|(<b>)v[0-9]+\.[0-9]+\.[0-9]+(</b>)|\1${TAG}\2|g;
     s|(footer-logo\">Uplink )v[0-9]+\.[0-9]+\.[0-9]+(</div>)|\1${TAG}\2|g" \
    docs/quality.html

# packaging/archlinux/PKGBUILD — pkgver bump, pkgrel reset. The sha256 can
# only be refreshed once the tag's tarball exists on GitHub, so the AUR
# update itself stays manual (see packaging/archlinux/README.md).
sed -i -E \
    "s|^pkgver=[0-9]+\.[0-9]+\.[0-9]+|pkgver=${VER}|;
     s|^pkgrel=[0-9]+|pkgrel=1|" \
    packaging/archlinux/PKGBUILD

echo "Files updated. Staging..."
git add CMakeLists.txt README.md docs/index.html docs/quality.html packaging/archlinux/PKGBUILD
git diff --cached --stat

echo ""
read -rp "Commit, tag ${TAG}, and push? [y/N] " confirm
[[ "$confirm" =~ ^[Yy]$ ]] || { echo "Aborted."; exit 0; }

git commit -m "docs: bump download links to ${TAG}"
git tag -a "${TAG}" -m "${TAG}"
git push origin main
git push origin "${TAG}"

echo "Done. CI will build and publish binaries for ${TAG}."
