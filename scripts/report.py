"""
Post-process perf.data files:
  1. Build binary_cache
  2. Convert perf_<name>.data      → perf_<name>.proto      (on-CPU)
  3. Convert perf_<name>_offcpu.data → perf_<name>_offcpu.proto (on+off-CPU)
"""

import os
import subprocess
import sys
from pathlib import Path

from dotenv import load_dotenv

load_dotenv()

ROOT = Path(__file__).resolve().parent.parent


def simpleperf_scripts_dir() -> Path:
    ndk = os.getenv("ANDROID_NDK_HOME", "")
    if not ndk:
        print("ANDROID_NDK_HOME is not set in .env", file=sys.stderr)
        sys.exit(1)
    return Path(ndk) / "simpleperf"


def build_binary_cache(perf_data: Path) -> None:
    scripts = simpleperf_scripts_dir()
    print("Building binary_cache...")
    subprocess.run(
        [sys.executable, str(scripts / "binary_cache_builder.py"), "-i", str(perf_data)],
        check=True,
        cwd=ROOT,
    )
    print("binary_cache ready.")


def to_pprof_proto(perf_data: Path, output: Path) -> None:
    scripts = simpleperf_scripts_dir()
    subprocess.run(
        [sys.executable, str(scripts / "pprof_proto_generator.py"),
         "-i", str(perf_data), "-o", str(output)],
        check=True,
        cwd=ROOT,
    )
    print(f"pprof proto saved: {output.resolve()}")


def perf_name() -> str:
    local_exe = os.getenv("ANDROID_TARGET_EXE", "")
    stem = Path(local_exe).stem if local_exe else "unknown"
    return f"perf_{stem}"


def process(data_file: Path, proto_file: Path) -> None:
    if not data_file.exists():
        print(f"skip: {data_file} not found")
        return
    build_binary_cache(data_file)
    print(f"Converting {data_file.name} → {proto_file.name}...")
    to_pprof_proto(data_file, proto_file)


def main() -> None:
    name = perf_name()
    report_dir = ROOT / "report"

    process(report_dir / f"{name}.data", report_dir / f"{name}.proto")
    process(report_dir / f"{name}_offcpu.data", report_dir / f"{name}_offcpu.proto")


if __name__ == "__main__":
    main()
