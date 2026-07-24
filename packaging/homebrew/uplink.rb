cask "uplink" do
  version "2026.7.8"
  sha256 "227b94b5fea395e0ebbe69c2dbe0285b1557c7b5f010f63fbb531167f3ce0e7a"

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
