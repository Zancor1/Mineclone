#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
android_sdk=${ANDROID_SDK_ROOT:-${ANDROID_HOME:-"$root/build/android/sdk"}}
if [ -n "${ANDROID_JAVA_HOME:-}" ]; then JAVA_HOME=$ANDROID_JAVA_HOME
elif [ -x "$root/build/android/jdk/bin/javac" ]; then JAVA_HOME="$root/build/android/jdk"
elif [ -x /usr/lib/jvm/java-17-openjdk/bin/javac ]; then JAVA_HOME=/usr/lib/jvm/java-17-openjdk
fi
: "${JAVA_HOME:?Instale jdk17-openjdk e configure JAVA_HOME para o JDK 17}"
export JAVA_HOME
export ANDROID_USER_HOME="${ANDROID_USER_HOME:-$root/build/android/user}"
mkdir -p "$android_sdk" "$root/build/android/tooling" "$ANDROID_USER_HOME"
if [ ! -x "$android_sdk/cmdline-tools/latest/bin/sdkmanager" ]; then
    archive="$root/build/android/tooling/commandlinetools.zip"
    curl -fL --retry 2 https://dl.google.com/android/repository/commandlinetools-linux-13114758_latest.zip -o "$archive"
    printf '5fdcc763663eefb86a5b8879697aa6088b041e70  %s\n' "$archive" | sha1sum -c -
    mkdir -p "$android_sdk/cmdline-tools"
    unzip -q "$archive" -d "$android_sdk/cmdline-tools"
    mv "$android_sdk/cmdline-tools/cmdline-tools" "$android_sdk/cmdline-tools/latest"
fi
"$android_sdk/cmdline-tools/latest/bin/sdkmanager" --sdk_root="$android_sdk" --licenses
"$android_sdk/cmdline-tools/latest/bin/sdkmanager" --sdk_root="$android_sdk" 'platforms;android-35' 'build-tools;35.0.0' 'platform-tools' 'ndk;27.2.12479018' 'cmake;3.22.1'
