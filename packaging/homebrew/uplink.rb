cask "uplink" do
  version "2026.8.2"
  sha256 "082cafc2d07bc6316b480753fb212daff5a239bf80ba0ad619c96b8c3de96784"

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
