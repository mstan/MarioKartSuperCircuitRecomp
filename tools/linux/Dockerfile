# MarioKartSuperCircuitRecomp Linux release builder.
#
# Ubuntu 22.04 (glibc 2.35) so the AppImage runs on current desktop distros
# and SteamOS. GCC 12 for the engine's C++20. SDL2 is NOT taken from the
# distro (2.0.20 there): build_appimage.sh builds the engine's pinned SDL fork
# (the same 2.32.x the Android build ships) and bundles it.
#
#   docker build -t mariokartsupercircuitrecomp-linux-builder tools/linux
FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential gcc-12 g++-12 cmake ninja-build pkg-config python3 git \
        file patchelf ca-certificates wget \
        libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxss-dev \
        libxfixes-dev libxinerama-dev libxkbcommon-dev libwayland-dev wayland-protocols \
        libdecor-0-dev libegl-dev libgles-dev libgl-dev libdrm-dev libgbm-dev \
        libpulse-dev libasound2-dev libpipewire-0.3-dev libdbus-1-dev libudev-dev \
        libibus-1.0-dev libfreetype-dev libharfbuzz-dev \
    && rm -rf /var/lib/apt/lists/*

# AppImage tooling (appimagetool + static type-2 runtime), unpacked so it runs
# without FUSE inside the container.
RUN mkdir -p /opt/appimage && cd /opt/appimage \
    && wget -q https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage \
    && wget -q https://github.com/AppImage/type2-runtime/releases/download/continuous/runtime-x86_64 \
    && chmod +x appimagetool-x86_64.AppImage \
    && ./appimagetool-x86_64.AppImage --appimage-extract >/dev/null \
    && mv squashfs-root appimagetool \
    && sha256sum appimagetool-x86_64.AppImage runtime-x86_64 > SHA256SUMS
