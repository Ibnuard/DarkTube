#!/bin/bash
set -e

mkdir -p build/tmp_pkgs
cd build/tmp_pkgs

BASE_URL="https://github.com/xfangfang/wiliwili/releases/download/v0.1.0/"

PKGS=(
    "deko3d-8939ff80f94d061dbc7d107e08b8e3be53e2938b-1-any.pkg.tar.zst"
    "libuam-f8c9eef01ffe06334d530393d636d69e2b52744b-1-any.pkg.tar.zst"
    "switch-libass-0.17.1-1-any.pkg.tar.zst"
    "switch-ffmpeg-7.1-1-any.pkg.tar.zst"
    "switch-libmpv-0.36.0-3-any.pkg.tar.zst"
    "switch-nspmini-48d4fc2-1-any.pkg.tar.xz"
    "hacBrewPack-3.05-1-any.pkg.tar.zst"
)

echo "Installing switch-glfw from repository..."
sudo dkp-pacman -S --noconfirm --needed switch-glfw

for PKG in "${PKGS[@]}"; do
    echo "Downloading and installing ${PKG}..."
    [ -f "${PKG}" ] || curl -LO ${BASE_URL}${PKG}
    sudo dkp-pacman -U --noconfirm ${PKG}
done

# Fix ownership if run with sudo so build dir can be deleted by user
if [ -n "$SUDO_USER" ]; then
    chown -R "$SUDO_USER" build/tmp_pkgs
fi
