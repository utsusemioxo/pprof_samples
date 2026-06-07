"""
Cross-compile pprof_samples for Android arm64 using the NDK CMake toolchain.
Output binaries land in build_android/bin/.
"""

import os
import subprocess
import sys
from pathlib import Path

from dotenv import load_dotenv

load_dotenv()

ROOT = Path(__file__).resolve().parent.parent


def main() -> None:
    ndk = os.getenv("ANDROID_NDK_HOME", "")
    if not ndk:
        print("ANDROID_NDK_HOME is not set in .env", file=sys.stderr)
        sys.exit(1)

    toolchain = Path(ndk) / "build" / "cmake" / "android.toolchain.cmake"
    if not toolchain.exists():
        print(f"Toolchain not found: {toolchain}", file=sys.stderr)
        sys.exit(1)

    build_dir = ROOT / "build_android"
    build_dir.mkdir(exist_ok=True)

    abi = os.getenv("ANDROID_ABI", "arm64-v8a")
    platform = os.getenv("ANDROID_PLATFORM", "android-21")
    build_type = os.getenv("ANDROID_BUILD_TYPE", "RelWithDebInfo")

    cmake_args = [
        "cmake",
        "-B", str(build_dir),
        "-S", str(ROOT),
        f"-DCMAKE_TOOLCHAIN_FILE={toolchain}",
        f"-DANDROID_ABI={abi}",
        f"-DANDROID_PLATFORM={platform}",
        f"-DCMAKE_BUILD_TYPE={build_type}",
    ]

    print("Configuring...")
    subprocess.run(cmake_args, check=True)

    print("Building...")
    subprocess.run(["cmake", "--build", str(build_dir), "--parallel"], check=True)

    bin_dir = build_dir / "bin"
    binaries = sorted(bin_dir.iterdir()) if bin_dir.exists() else []
    print(f"\nBuilt {len(binaries)} binaries in {bin_dir}:")
    for b in binaries:
        print(f"  {b.name}")


if __name__ == "__main__":
    main()
