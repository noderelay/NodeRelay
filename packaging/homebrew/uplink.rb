cask "uplink" do
  version "2026.8.1"
  sha256 "393ac1d437a218bd4bffb2d10c91307d98f151e0a19698e6a881dee0af0ae3e7"

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
