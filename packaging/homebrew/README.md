# Homebrew tap (macOS)

Casks/uplink.rb here is the source of truth; the tap repo at
https://github.com/noderelay/homebrew-uplink mirrors it.

Users install with:

```
brew tap noderelay/uplink
brew trust noderelay/uplink
brew install --cask uplink
```

The trust step is a one-time thing: current Homebrew refuses to load
casks from third-party taps until the tap is marked trusted.

After each release: bump version, update sha256
(`sha256sum Uplink-v<ver>-macos-arm64.dmg` from the release assets),
copy uplink.rb to the tap repo's Casks/ and push. Sanity check on the
MacBook: `brew style noderelay/uplink && brew audit --cask --online
noderelay/uplink/uplink`.

`release.sh` does NOT touch this file — it bumps the AUR PKGBUILDs but
not the cask, because the sha256 cannot be known until the release
artifacts exist. That makes it easy to update the tap and forget the
copy here, which is exactly what happened during 2026.7.8 (this file
sat at 2026.7.7 while the tap served .8). To check for drift:

```
diff <(gh api repos/noderelay/homebrew-uplink/contents/Casks/uplink.rb \
        --jq .content | base64 -d) packaging/homebrew/uplink.rb
```

GitHub also publishes the artifact hash, so the sha256 can be taken
without downloading the dmg:

```
gh api repos/noderelay/UplinkIRC/releases/tags/v<ver> \
  --jq '.assets[] | select(.name|test("macos")) | .digest'
```
