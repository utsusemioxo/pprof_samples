"""
Entry point for simpleperf recording.
Records twice per run:
  1. on-CPU  : cpu-cycles (perf_<name>.data)
  2. on+off-CPU: cpu-clock + --trace-offcpu (perf_<name>_offcpu.data)
"""

import os
import platform
import subprocess
import sys
from pathlib import Path

from dotenv import load_dotenv

load_dotenv()


def adb(*args: str) -> subprocess.CompletedProcess:
    sdk = os.getenv("ANDROID_SDK_HOME", "")
    adb_bin = Path(sdk) / "platform-tools" / ("adb.exe" if platform.system() == "Windows" else "adb")
    cmd = [str(adb_bin), *args]
    return subprocess.run(cmd, check=True, text=True, capture_output=True)


def device_serial() -> str | None:
    result = adb("devices")
    lines = result.stdout.strip().splitlines()[1:]
    for line in lines:
        parts = line.split()
        if len(parts) >= 2 and parts[1] == "device":
            return parts[0]
    return None


def push_simpleperf(serial: str) -> str:
    ndk = os.getenv("ANDROID_NDK_HOME", "")
    result = subprocess.run(
        ["adb", "-s", serial, "shell", "getprop", "ro.product.cpu.abi"],
        check=True, text=True, capture_output=True,
    )
    device_abi = result.stdout.strip()
    abi_map = {"arm64-v8a": "arm64", "armeabi-v7a": "arm", "x86_64": "x86_64", "x86": "x86"}
    ndk_abi = abi_map.get(device_abi, device_abi)
    simpleperf_src = Path(ndk) / "simpleperf" / "bin" / "android" / ndk_abi / "simpleperf"
    device_path = "/data/local/tmp/simpleperf"
    adb("-s", serial, "push", str(simpleperf_src), device_path)
    adb("-s", serial, "shell", "chmod", "+x", device_path)
    return device_path


def push_target_exe(serial: str) -> str:
    local_exe = os.getenv("ANDROID_TARGET_EXE", "")
    if not local_exe:
        print("ANDROID_TARGET_EXE is not set in .env", file=sys.stderr)
        sys.exit(1)
    device_path = os.getenv("ANDROID_TARGET_DEVICE_PATH", "/data/local/tmp/target_exe")
    adb("-s", serial, "push", local_exe, device_path)
    adb("-s", serial, "shell", "chmod", "+x", device_path)
    return device_path


def perf_name() -> str:
    local_exe = os.getenv("ANDROID_TARGET_EXE", "")
    stem = Path(local_exe).stem if local_exe else "unknown"
    return f"perf_{stem}"


def _run_record(serial: str, simpleperf_path: str, target_device_path: str,
                event: str, device_output: str, extra: list[str]) -> None:
    duration = os.getenv("SIMPLEPERF_DURATION", "10")
    call_graph = os.getenv("SIMPLEPERF_CALL_GRAPH", "fp")
    cmd = [
        "adb", "-s", serial, "shell",
        simpleperf_path, "record",
        "-e", event,
        "--duration", duration,
        "--call-graph", call_graph,
        *extra,
        "-o", device_output,
        target_device_path,
    ]
    subprocess.run(cmd, check=True)


def _pull(serial: str, device_path: str, local_path: Path) -> None:
    local_path.parent.mkdir(exist_ok=True)
    adb("-s", serial, "pull", device_path, str(local_path))


def record_oncpu(serial: str, simpleperf_path: str, target_device_path: str) -> Path:
    name = perf_name()
    device_out = f"/data/local/tmp/{name}.data"
    print("=== [on-CPU] recording cpu-cycles ===")
    _run_record(serial, simpleperf_path, target_device_path, "cpu-cycles", device_out, [])
    local = Path(__file__).resolve().parent.parent / "report" / f"{name}.data"
    print("Pulling on-CPU perf.data...")
    _pull(serial, device_out, local)
    print(f"Saved: {local}")
    return local


def record_offcpu(serial: str, simpleperf_path: str, target_device_path: str) -> Path:
    name = perf_name()
    device_out = f"/data/local/tmp/{name}_offcpu.data"
    print("=== [on+off-CPU] recording cpu-clock + --trace-offcpu ===")
    _run_record(serial, simpleperf_path, target_device_path,
                "cpu-clock", device_out, ["--trace-offcpu"])
    local = Path(__file__).resolve().parent.parent / "report" / f"{name}_offcpu.data"
    print("Pulling off-CPU perf.data...")
    _pull(serial, device_out, local)
    print(f"Saved: {local}")
    return local


def main() -> None:
    print(f"Host: {platform.system()} {platform.machine()}")

    serial = device_serial()
    if not serial:
        print("No Android device connected.", file=sys.stderr)
        sys.exit(1)
    print(f"Device: {serial}")

    simpleperf_path = push_simpleperf(serial)
    print(f"simpleperf pushed to {simpleperf_path}")

    target_path = push_target_exe(serial)
    print(f"target exe pushed to {target_path}")

    record_oncpu(serial, simpleperf_path, target_path)
    record_offcpu(serial, simpleperf_path, target_path)


if __name__ == "__main__":
    main()
