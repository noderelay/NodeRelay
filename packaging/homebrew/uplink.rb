cask "uplink" do
  version "2026.7.6"
  sha256 "0ec7553eb26a575a78cae351ad253e5d4ed4b90a5596af8ac2e97ea69d111a62"

  url "https://github.com/noderelay/UplinkIRC/releases/download/v#{version}/Uplink-v#{version}-macos-arm64.dmg"
  name "Uplink"
  desc "Fast, secure, IRCv3-featured IRC client"
  homepage "https://uplinkirc.chat"

  livecheck do
    url :url
    strategy :github_latest
  end

  depends_on arch: :arm64

  app "Uplink.app"

  caveats <<~EOS
    Uplink is not notarized by Apple. If macOS blocks the first launch,
    allow it under System Settings -> Privacy & Security ("Open Anyway"),
    or install with --no-quarantine.
  EOS
end
