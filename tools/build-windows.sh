#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build="$root/build/windows"
prefix="$build/prefix"
sources="$build/sources"
jobs=${JOBS:-4}
for program in x86_64-w64-mingw32-gcc x86_64-w64-mingw32-g++ cmake make curl tar; do
    if ! command -v "$program" >/dev/null 2>&1; then
        echo "Falta $program. No Arch: sudo pacman -S --needed mingw-w64-gcc cmake make curl" >&2
        exit 1
    fi
done
mkdir -p "$sources" "$prefix" "$root/dist/windows"
fetch() {
    name=$1
    url=$2
    if [ ! -d "$sources/$name" ]; then
        curl --fail --location --retry 3 "$url" -o "$sources/$name.tar.gz.part"
        tar -xzf "$sources/$name.tar.gz.part" -C "$sources"
        rm "$sources/$name.tar.gz.part"
    fi
}
fetch raylib-5.5 https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
fetch zlib-1.3.1 https://github.com/madler/zlib/archive/refs/tags/v1.3.1.tar.gz
cmake -S "$sources/raylib-5.5" -B "$build/raylib" \
    -DCMAKE_TOOLCHAIN_FILE="$root/cmake/windows-mingw.cmake" \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$prefix" -DBUILD_SHARED_LIBS=OFF -DBUILD_EXAMPLES=OFF
cmake --build "$build/raylib" --parallel "$jobs"
cmake --install "$build/raylib"
cmake -S "$sources/zlib-1.3.1" -B "$build/zlib" \
    -DCMAKE_TOOLCHAIN_FILE="$root/cmake/windows-mingw.cmake" \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$prefix" -DZLIB_BUILD_EXAMPLES=OFF
cmake --build "$build/zlib" --target zlibstatic --parallel "$jobs"
cp "$build/zlib/libzlibstatic.a" "$prefix/lib/libz.a"
cp "$sources/zlib-1.3.1/zlib.h" "$build/zlib/zconf.h" "$prefix/include/"
make -C "$root" -j"$jobs" CXX=x86_64-w64-mingw32-g++ \
    TARGET=dist/windows/minecraft_clone.exe BUILD_DIR=build/windows/objects \
    CPPFLAGS="-Iinclude -isystem \"$prefix/include\" -D_WIN32_WINNT=0x0601" \
    CXXFLAGS="-std=c++17 -O3 -DNDEBUG -Wall -Wextra -Wpedantic" \
    LDFLAGS="-static -static-libgcc -static-libstdc++ -mwindows -L\"$prefix/lib\"" \
    LDLIBS="-lraylib -lz -lopengl32 -lgdi32 -lwinmm -lws2_32 -ldnsapi -liphlpapi -pthread" all
cp "$root/README.md" "$root/dist/windows/README.md"
if [ -d "$root/LICENSES" ]; then cp -R "$root/LICENSES" "$root/dist/windows/"; fi
cp "$sources/raylib-5.5/LICENSE" "$root/dist/windows/LICENSE-raylib.txt"
cp "$sources/zlib-1.3.1/LICENSE" "$root/dist/windows/LICENSE-zlib.txt"
printf '\nWindows x64: %s\n' "$root/dist/windows/minecraft_clone.exe"
