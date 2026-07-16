#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
TOOLS_DIR="$SCRIPT_DIR/tools"
BUILD_DIR="$PROJECT_DIR/build-appimage"
APPDIR="$BUILD_DIR/AppDir"

# Read version from CMakeLists.txt
VERSION=$(grep -oP '(?<=project\(UplinkIRC VERSION )\S+' "$PROJECT_DIR/CMakeLists.txt")
ARCH=$(uname -m)

echo "==> Uplink $VERSION  ($ARCH)"

# ---------------------------------------------------------------------------
# Pinned tool versions
# Bump version + checksums together when upgrading a tool.
# appimagetool has no versioned releases upstream; pin by hash only.
# ---------------------------------------------------------------------------
LD_VERSION="1-alpha-20251107-1"
LD_QT_VERSION="1-alpha-20250213-1"

# Known SHA256 checksums keyed by filename.
# Add an entry for each arch you build on; unknown arches skip verification
# with a warning. Compute with: sha256sum <file>
declare -A TOOL_SHA256
TOOL_SHA256["linuxdeploy-x86_64.AppImage"]="c20cd71e3a4e3b80c3483cef793cda3f4e990aca14014d23c544ca3ce1270b4d"
TOOL_SHA256["linuxdeploy-plugin-qt-x86_64.AppImage"]="15106be885c1c48a021198e7e1e9a48ce9d02a86dd0a1848f00bdbf3c1c92724"
TOOL_SHA256["appimagetool-x86_64.AppImage"]="a6d71e2b6cd66f8e8d16c37ad164658985e0cf5fcaa950c90a482890cb9d13e0"

verify_sha256() {
    local file="$1"
    local name
    name="$(basename "$file")"
    local expected="${TOOL_SHA256[$name]:-}"
    if [[ -z "$expected" ]]; then
        echo "WARNING: no known checksum for $name — skipping verification" >&2
        return 0
    fi
    local actual
    actual="$(sha256sum "$file" | awk '{print $1}')"
    if [[ "$actual" != "$expected" ]]; then
        echo "ERROR: checksum mismatch for $name" >&2
        echo "  expected: $expected" >&2
        echo "  got:      $actual" >&2
        rm -f "$file"
        exit 1
    fi
    echo "==> Verified $name"
}

# ---------------------------------------------------------------------------
# Tools: linuxdeploy + Qt plugin
# ---------------------------------------------------------------------------
mkdir -p "$TOOLS_DIR"

LD="$TOOLS_DIR/linuxdeploy-$ARCH.AppImage"
LD_QT="$TOOLS_DIR/linuxdeploy-plugin-qt-$ARCH.AppImage"

if [[ ! -x "$LD" ]]; then
    echo "==> Downloading linuxdeploy ${LD_VERSION}..."
    curl -fL "https://github.com/linuxdeploy/linuxdeploy/releases/download/${LD_VERSION}/linuxdeploy-$ARCH.AppImage" -o "$LD"
    chmod +x "$LD"
fi
verify_sha256 "$LD"

if [[ ! -x "$LD_QT" ]]; then
    echo "==> Downloading linuxdeploy-plugin-qt ${LD_QT_VERSION}..."
    curl -fL "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/${LD_QT_VERSION}/linuxdeploy-plugin-qt-$ARCH.AppImage" -o "$LD_QT"
    chmod +x "$LD_QT"
fi
verify_sha256 "$LD_QT"

# ---------------------------------------------------------------------------
# Icon
# ---------------------------------------------------------------------------
ICON_PNG="$SCRIPT_DIR/io.github.noderelay.UplinkIRC.png"

# ---------------------------------------------------------------------------
# Build
# ---------------------------------------------------------------------------
echo "==> Building (Release)..."
cmake -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    ${UPLINK_EXTRA_CMAKE_ARGS:-} \
    "$PROJECT_DIR"
cmake --build "$BUILD_DIR" -j"$(nproc)"

# ---------------------------------------------------------------------------
# Install into AppDir
# ---------------------------------------------------------------------------
echo "==> Installing into AppDir..."
rm -rf "$APPDIR"
DESTDIR="$APPDIR" cmake --install "$BUILD_DIR"

# ---------------------------------------------------------------------------
# Build AppImage
# ---------------------------------------------------------------------------
echo "==> Running linuxdeploy..."

# Point linuxdeploy-plugin-qt at Qt6's qmake
export QMAKE
QMAKE=$(command -v qmake6 2>/dev/null || command -v qmake 2>/dev/null || true)
if [[ -z "$QMAKE" ]]; then
    echo "WARNING: qmake not found in PATH — Qt plugin may not bundle libraries correctly"
fi

# Some Qt imageformat plugins (e.g. kimg_jxr from kimageformats) depend on
# libraries not present on the build host (e.g. libjxrglue). Provide stub
# shared libraries for any missing deps so linuxdeploy-plugin-qt can proceed.
# The stubs are compiled into a temp dir added to LD_LIBRARY_PATH.
STUB_DIR=$(mktemp -d)
trap 'rm -rf "$STUB_DIR"' EXIT
QT_PLUGIN_SRC=$(${QMAKE} -query QT_INSTALL_PLUGINS 2>/dev/null || echo "/usr/lib/qt6/plugins")
while IFS= read -r missing; do
    soname="$missing"
    echo "==> Creating stub for missing dep: $soname"
    printf 'void _stub(void){}' | \
        gcc -x c -shared -fPIC -o "$STUB_DIR/$soname" - -Wl,-soname,"$soname"
done < <(find "$QT_PLUGIN_SRC" -name "*.so" -exec ldd {} 2>/dev/null \; \
         | awk '/not found/{print $1}' | sort -u)
if [[ -n "$(ls -A "$STUB_DIR" 2>/dev/null)" ]]; then
    export LD_LIBRARY_PATH="$STUB_DIR:${LD_LIBRARY_PATH:-}"
fi

OUTPUT_FILE="Uplink-$VERSION-$ARCH.AppImage"

# ---------------------------------------------------------------------------
# Download appimagetool (used for the final packaging step — separate from
# linuxdeploy so its bundled strip is never called on our AppDir).
# ---------------------------------------------------------------------------
APPIMAGETOOL="$TOOLS_DIR/appimagetool-$ARCH.AppImage"
if [[ ! -x "$APPIMAGETOOL" ]]; then
    echo "==> Downloading appimagetool (continuous — no versioned releases upstream)..."
    curl -fL "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-$ARCH.AppImage" \
         -o "$APPIMAGETOOL"
    chmod +x "$APPIMAGETOOL"
fi
verify_sha256 "$APPIMAGETOOL"

# ---------------------------------------------------------------------------
# Phase 1: extract linuxdeploy + its Qt plugin and patch BOTH bundled strips
# with a no-op. The bundled strip binaries cannot handle .relr.dyn ELF
# sections produced by modern Arch/Fedora toolchains (gcc >= 12 / binutils
# >= 2.39 with -z pack-relative-relocs). Replacing them with no-op shell
# scripts lets linuxdeploy finish deploying all libraries without stripping.
# ---------------------------------------------------------------------------
LD_EXTRACT=$(mktemp -d)
LD_QT_EXTRACT=$(mktemp -d)
trap 'rm -rf "$LD_EXTRACT" "$LD_QT_EXTRACT"' EXIT

echo "==> Patching linuxdeploy strip (bundled strip can't handle .relr.dyn)..."
(cd "$LD_EXTRACT" && "$LD" --appimage-extract >/dev/null 2>&1)
printf '#!/bin/sh\nexit 0\n' > "$LD_EXTRACT/squashfs-root/usr/bin/strip"
chmod +x "$LD_EXTRACT/squashfs-root/usr/bin/strip"
LD_PATCHED="$LD_EXTRACT/squashfs-root/AppRun"

echo "==> Patching linuxdeploy-plugin-qt strip..."
(cd "$LD_QT_EXTRACT" && "$LD_QT" --appimage-extract >/dev/null 2>&1)
printf '#!/bin/sh\nexit 0\n' > "$LD_QT_EXTRACT/squashfs-root/usr/bin/strip"
chmod +x "$LD_QT_EXTRACT/squashfs-root/usr/bin/strip"
# Create a wrapper with the expected filename so linuxdeploy finds it in PATH
# before the original AppImage, and it runs from the patched squashfs-root.
WRAPPER_DIR=$(mktemp -d)
trap 'rm -rf "$LD_EXTRACT" "$LD_QT_EXTRACT" "$WRAPPER_DIR"' EXIT
WRAPPER_APPRUN="$LD_QT_EXTRACT/squashfs-root/AppRun"
cat > "$WRAPPER_DIR/linuxdeploy-plugin-qt-${ARCH}.AppImage" << WRAPPER
#!/bin/sh
exec "$WRAPPER_APPRUN" "\$@"
WRAPPER
chmod +x "$WRAPPER_DIR/linuxdeploy-plugin-qt-${ARCH}.AppImage"

# ---------------------------------------------------------------------------
# Phase 2: deploy Qt libs and plugins into AppDir using patched linuxdeploy.
# ---------------------------------------------------------------------------
echo "==> Deploying Qt libraries..."
export PATH="$WRAPPER_DIR:$TOOLS_DIR:$PATH"
"$LD_PATCHED" \
    --appdir "$APPDIR" \
    --plugin qt \
    --desktop-file "$SCRIPT_DIR/io.github.noderelay.UplinkIRC.desktop" \
    --icon-file "$ICON_PNG"

# Phase 2b: bundle color emoji font as fallback for hosts that don't have one
mkdir -p "$APPDIR/usr/share/fonts/truetype"
EMOJI_FONT=$(find /usr/share/fonts /usr/local/share/fonts -name "NotoColorEmoji.ttf" 2>/dev/null | head -1)
if [[ -z "$EMOJI_FONT" ]]; then
    EMOJI_FONT=$(fc-match --format="%{file}" "emoji:color=1" 2>/dev/null || true)
fi
if [[ -f "$EMOJI_FONT" ]]; then
    cp "$EMOJI_FONT" "$APPDIR/usr/share/fonts/truetype/NotoColorEmoji.ttf"
    echo "==> Bundled emoji font: $EMOJI_FONT"
else
    echo "WARNING: No color emoji font found on build host — emoji may render without color"
fi

# Phase 2c: ensure AppDir root has the required files for appimagetool
cp -n "$APPDIR/usr/share/applications/io.github.noderelay.UplinkIRC.desktop" "$APPDIR/" 2>/dev/null || true
cp -n "$APPDIR/usr/share/icons/hicolor/256x256/apps/io.github.noderelay.UplinkIRC.png" "$APPDIR/" 2>/dev/null || true
if [[ -f "$ICON_PNG" ]] && [[ ! -f "$APPDIR/io.github.noderelay.UplinkIRC.png" ]]; then
    cp "$ICON_PNG" "$APPDIR/io.github.noderelay.UplinkIRC.png"
fi
if [[ ! -f "$APPDIR/AppRun" ]]; then
    cat > "$APPDIR/AppRun" << 'APPRUN'
#!/bin/bash
HERE="$(dirname "$(readlink -f "$0")")"

# Self-integrate on first run when running as an AppImage
if [[ -n "${APPIMAGE:-}" && ! -f "${HOME}/.local/share/applications/io.github.noderelay.UplinkIRC.desktop" ]]; then
    mkdir -p "${HOME}/.local/share/applications"
    mkdir -p "${HOME}/.local/share/icons/hicolor/256x256/apps"
    cat > "${HOME}/.local/share/applications/io.github.noderelay.UplinkIRC.desktop" << DESKTOP
[Desktop Entry]
Name=Uplink
Exec=${APPIMAGE}
Icon=io.github.noderelay.UplinkIRC
Type=Application
Categories=Network;IRCClient;
Terminal=false
StartupWMClass=Uplink
DESKTOP
    if [[ -f "$HERE/usr/share/icons/hicolor/256x256/apps/io.github.noderelay.UplinkIRC.png" ]]; then
        cp "$HERE/usr/share/icons/hicolor/256x256/apps/io.github.noderelay.UplinkIRC.png" \
           "${HOME}/.local/share/icons/hicolor/256x256/apps/io.github.noderelay.UplinkIRC.png"
    fi
    command -v update-desktop-database &>/dev/null && \
        update-desktop-database "${HOME}/.local/share/applications" 2>/dev/null || true
fi

export LD_LIBRARY_PATH="$HERE/usr/lib:${LD_LIBRARY_PATH:-}"
export QT_PLUGIN_PATH="$HERE/usr/plugins:${QT_PLUGIN_PATH:-}"
export QT_QPA_PLATFORM_PLUGIN_PATH="$HERE/usr/plugins/platforms"
export XDG_DATA_DIRS="$HERE/usr/share:${XDG_DATA_DIRS:-/usr/local/share:/usr/share}"

# Generate a fontconfig at runtime so system fonts take priority;
# the bundled color emoji font is listed last as a fallback only.
if [[ -d "$HERE/usr/share/fonts" ]]; then
    _FC_DIR="$HERE/var/cache/fontconfig-uplink"
    mkdir -p "$_FC_DIR"
    _FC_CONF="$_FC_DIR/fonts.conf"
    cat > "$_FC_CONF" << FONTSCONF
<?xml version="1.0"?>
<!DOCTYPE fontconfig SYSTEM "fonts.dtd">
<fontconfig>
  <dir>~/.fonts</dir>
  <dir>/usr/share/fonts</dir>
  <dir>/usr/local/share/fonts</dir>
  <dir prefix="xdg">fonts</dir>
  <dir>$HERE/usr/share/fonts</dir>
  <cachedir>$_FC_DIR</cachedir>
  <include ignore_missing="yes">/etc/fonts/conf.d</include>
</fontconfig>
FONTSCONF
    export FONTCONFIG_FILE="$_FC_CONF"
fi

exec "$HERE/usr/bin/Uplink" "$@"
APPRUN
    chmod +x "$APPDIR/AppRun"
fi

# ---------------------------------------------------------------------------
# Phase 3: package AppDir into an AppImage with appimagetool.
# If UPDATE_INFORMATION is set (e.g. by CI), embed zsync metadata.
# ---------------------------------------------------------------------------
echo "==> Packaging AppImage..."
APPIMAGETOOL_ARGS=()
if [[ -n "${UPDATE_INFORMATION:-}" ]]; then
    echo "==> Embedding update info: $UPDATE_INFORMATION"
    APPIMAGETOOL_ARGS+=( --updateinformation "$UPDATE_INFORMATION" )
fi
"$APPIMAGETOOL" "${APPIMAGETOOL_ARGS[@]}" "$APPDIR" "$OUTPUT_FILE"

echo ""
echo "==> Done: $OUTPUT_FILE"
