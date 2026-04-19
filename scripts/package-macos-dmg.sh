#!/usr/bin/env bash

set -euo pipefail

usage() {
    cat >&2 <<'EOF'
Usage: bash scripts/package-macos-dmg.sh [--skip-deploy] <path-to-app> [output-dmg]

Examples:
  bash scripts/package-macos-dmg.sh build/Release/LocalGen-new.app
  bash scripts/package-macos-dmg.sh --skip-deploy build/Release/LocalGen-new.app LocalGen-new.dmg
EOF
}

skip_deploy=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        --skip-deploy)
            skip_deploy=true
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        --)
            shift
            break
            ;;
        -*)
            usage
            exit 1
            ;;
        *)
            break
            ;;
    esac
done

if [[ $# -lt 1 || $# -gt 2 ]]; then
    usage
    exit 1
fi

app_input="$1"
output_input="${2:-}"

if [[ ! -d "$app_input" || "${app_input##*.}" != "app" ]]; then
    echo "Expected a macOS .app bundle, got: $app_input" >&2
    exit 1
fi

app_path="$(cd "$(dirname "$app_input")" && pwd)/$(basename "$app_input")"
app_name="$(basename "$app_path" .app)"
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
license_source="$(cd "$script_dir/.." && pwd)/LICENSE"

if [[ -n "$output_input" ]]; then
    case "$output_input" in
        /*)
            output_dmg="$output_input"
            ;;
        *)
            output_dmg="$(pwd)/$output_input"
            ;;
    esac
else
    output_dmg="$(dirname "$app_path")/${app_name}.dmg"
fi

mkdir -p "$(dirname "$output_dmg")"

for required_tool in codesign hdiutil ditto; do
    if ! command -v "$required_tool" >/dev/null 2>&1; then
        echo "Missing required tool: $required_tool" >&2
        exit 1
    fi
done

if ! $skip_deploy; then
    macdeployqt_path="${MACDEPLOYQT_EXECUTABLE:-$(command -v macdeployqt || true)}"
    if [[ -z "$macdeployqt_path" ]]; then
        echo "macdeployqt was not found in PATH or MACDEPLOYQT_EXECUTABLE." >&2
        exit 1
    fi

    "$macdeployqt_path" "$app_path" -always-overwrite
fi

staging_dir="$(mktemp -d "${TMPDIR:-/tmp}/${app_name}.dmg.XXXXXX")"
rw_image="$staging_dir/${app_name}-rw.sparseimage"
mount_point="$staging_dir/mount"
mounted_volume=""

cleanup() {
    if [[ -n "$mounted_volume" && -d "$mounted_volume" ]]; then
        hdiutil detach "$mounted_volume" >/dev/null 2>&1 || true
    fi

    rm -rf "$staging_dir"
}
trap cleanup EXIT

app_size_kb="$(du -sk "$app_path" | awk '{print $1}')"
image_size_kb="$((app_size_kb + app_size_kb / 3 + 65536))"

hdiutil create \
    -ov \
    -size "${image_size_kb}k" \
    -fs APFS \
    -volname "$app_name" \
    -type SPARSE \
    "$rw_image"

mkdir -p "$mount_point"
hdiutil attach -nobrowse -readwrite -mountpoint "$mount_point" "$rw_image" >/dev/null
mounted_volume="$mount_point"

if [[ -z "$mounted_volume" || ! -d "$mounted_volume" ]]; then
    echo "Failed to mount writable disk image." >&2
    exit 1
fi

mounted_app="$mounted_volume/${app_name}.app"

COPYFILE_DISABLE=1 ditto --norsrc "$app_path" "$mounted_app"
if [[ -f "$license_source" ]]; then
    COPYFILE_DISABLE=1 ditto --norsrc "$license_source" "$mounted_volume/LICENSE.txt"
fi
codesign --force --deep --sign - "$mounted_app"
codesign --verify --deep --strict "$mounted_app"
ln -s /Applications "$mounted_volume/Applications"

hdiutil detach "$mounted_volume"
mounted_volume=""

rm -f "$output_dmg"
hdiutil convert "$rw_image" -ov -format UDZO -o "$output_dmg"

echo "Created DMG: $output_dmg"
