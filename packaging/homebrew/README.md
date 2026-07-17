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
