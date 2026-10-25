#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
android_sdk=${ANDROID_SDK_ROOT:-${ANDROID_HOME:-"$root/build/android/sdk"}}
if [ ! -d "$android_sdk/platforms/android-35" ]; then
    echo "SDK Android 35 nao encontrado. Instale pelo Android Studio ou use tools/setup-android-sdk.sh." >&2
    echo "Depois informe ANDROID_HOME=/caminho/do/SDK make android" >&2
    exit 1
fi
if [ -n "${ANDROID_JAVA_HOME:-}" ]; then JAVA_HOME=$ANDROID_JAVA_HOME
elif [ -x "$root/build/android/jdk/bin/javac" ]; then JAVA_HOME="$root/build/android/jdk"
elif [ -x /usr/lib/jvm/java-17-openjdk/bin/javac ]; then JAVA_HOME=/usr/lib/jvm/java-17-openjdk
elif [ -z "${JAVA_HOME:-}" ]; then echo "Configure JAVA_HOME para um JDK 17 (no Arch: jdk17-openjdk)." >&2;exit 1
fi
if [ ! -x "$JAVA_HOME/bin/javac" ]; then echo "JDK completo necessario: no Arch, instale jdk17-openjdk." >&2;exit 1;fi
export JAVA_HOME
export ANDROID_HOME="$android_sdk" ANDROID_SDK_ROOT="$android_sdk"
export GRADLE_USER_HOME="${GRADLE_USER_HOME:-$root/build/android/gradle-user}"
export ANDROID_USER_HOME="${ANDROID_USER_HOME:-$root/build/android/user}"
mkdir -p "$GRADLE_USER_HOME" "$ANDROID_USER_HOME" "$root/dist/android"
cd "$root/android"
./gradlew --no-daemon --console=plain -PandroidAbis="${ANDROID_ABIS:-arm64-v8a,armeabi-v7a}" assembleDebug
cp "$root/build/android/gradle/app/outputs/apk/debug/app-debug.apk" "$root/dist/android/MineClone-debug.apk"
sha256sum "$root/dist/android/MineClone-debug.apk" > "$root/dist/android/MineClone-debug.apk.sha256"
printf '\nAPK de teste: %s\n' "$root/dist/android/MineClone-debug.apk"
