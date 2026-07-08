#!/usr/bin/env bash
# Sync docs/index.html, docs/quality.html, and README.md version strings to match CMakeLists.txt.
# Run manually at session close, or automatically via release.yml after a tag build.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

VERSION=$(grep -oP 'project\(UplinkIRC VERSION \K[0-9]+\.[0-9]+\.[0-9]+' CMakeLists.txt)
echo "sync-site: target version = v${VERSION}"

changed=0

sync_file() {
    local file="$1"
    # Find the current version embedded in the file (bare digits, no leading v)
    local old
    old=$(grep -oP '[0-9]+\.[0-9]+\.[0-9]+' "$file" | head -1)
    if [ -z "$old" ]; then
        echo "  $file: no version string found, skipping"
        return
    fi
    if [ "$old" = "$VERSION" ]; then
        echo "  $file: already v${VERSION}"
        return
    fi
    # Escape dots for sed BRE
    local old_esc="${old//./\\.}"
    sed -i "s/${old_esc}/${VERSION}/g" "$file"
    echo "  $file: ${old} → ${VERSION}"
    changed=1
}

sync_file "docs/index.html"
sync_file "docs/quality.html"
sync_file "README.md"

if [ "$changed" -eq 1 ]; then
    echo "sync-site: files updated. Commit with:"
    echo "  git add docs/index.html docs/quality.html README.md && git commit -m \"docs: sync site to v${VERSION}\""
else
    echo "sync-site: nothing to update."
fi
