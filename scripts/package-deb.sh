#!/usr/bin/env bash
# scripts/package-deb.sh — Build a .deb package from a static binary.
#
# Usage:  ./scripts/package-deb.sh <binary> <arch> [output-dir]
#
# Creates a .deb in the specified output directory (default: dist/).
# Uses ar + tar — no dpkg-deb required, works on any Linux.

set -euo pipefail

BIN="${1:?Usage: $0 <binary> <arch> [output-dir]}"
ARCH="${2:?Usage: $0 <binary> <arch> [output-dir]}"
OUT_DIR="${3:-dist}"

PKG_NAME="upgrade_tool"
VER="${VER:-0.0.0}"

if [ ! -f "${BIN}" ]; then
  echo "ERROR: binary not found: ${BIN}" >&2
  exit 1
fi

DEB_NAME="${PKG_NAME}-static_${VER}_${ARCH}"
DEB_ROOT="deb-pkg/${DEB_NAME}"

echo "Packaging ${DEB_NAME}.deb ..."
rm -rf "deb-pkg"
mkdir -p "${DEB_ROOT}/DEBIAN" \
         "${DEB_ROOT}/usr/bin" \
         "${DEB_ROOT}/usr/share/doc/${PKG_NAME}"

install -m 755 "${BIN}" "${DEB_ROOT}/usr/bin/upgrade_tool"

# DEBIAN/control
python3 -c "
control = '''Package: ${PKG_NAME}-static
Version: ${VER}
Architecture: ${ARCH}
Maintainer: linaro <linaro@localhost>
Section: electronics
Priority: optional
Conflicts: upgrade-tool
Replaces: upgrade-tool
Description: Rockchip firmware upgrade tool — static build
 Command-line tool for flashing firmware to Rockchip SoC devices
 over USB. Supports reading/writing firmware images, device
 enumeration, and boot mode control.
 Fully static binary — no external library dependencies.
'''
with open('${DEB_ROOT}/DEBIAN/control', 'w') as f:
    f.write(control)
"

# copyright
cat > "${DEB_ROOT}/usr/share/doc/${PKG_NAME}/copyright" <<'COPY'
Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/
License: GPL-3.0+
COPY

# Build .deb with ar + tar
cd "${DEB_ROOT}"
echo "2.0" > debian-binary
tar czf ../data.tar.gz --exclude=DEBIAN --exclude=debian-binary .
tar czf ../control.tar.gz -C DEBIAN .
cp debian-binary ..
cd ..
mkdir -p "${OUT_DIR}"
ar rcs "../${OUT_DIR}/${DEB_NAME}.deb" debian-binary control.tar.gz data.tar.gz
cd - >/dev/null

rm -rf "deb-pkg"
echo "  -> ${OUT_DIR}/${DEB_NAME}.deb"
