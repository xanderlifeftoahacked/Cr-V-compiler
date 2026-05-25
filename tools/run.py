#!/usr/bin/env python3

import os
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT_DIR = Path(__file__).resolve().parents[1]

CRV = Path(os.environ.get("CRV", ROOT_DIR / "build/crv"))
RARS_JAR = Path(os.environ.get("RARS_JAR", "/tmp/rars1_6.jar"))
RARS_RUNTIME = ROOT_DIR / "stdlib/rars.s"


def main() -> int:
    if len(sys.argv) < 2:
        print(f"usage: {sys.argv[0]} <file.c>", file=sys.stderr)
        return 1

    source = Path(sys.argv[1])

    if not source.is_file():
        print(f"error: file not found: {source}", file=sys.stderr)
        return 1

    if not CRV.is_file():
        print(f"error: compiler not found: {CRV} (run: cmake --build build)", file=sys.stderr)
        return 1

    if not RARS_JAR.is_file():
        print(f"error: RARS jar not found: {RARS_JAR} (set RARS_JAR=/path/to/rars1_6.jar)", file=sys.stderr)
        return 1

    if not RARS_RUNTIME.is_file():
        print(f"error: RARS runtime not found: {RARS_RUNTIME}", file=sys.stderr)
        return 1

    env = os.environ.copy()
    env.setdefault("ASAN_OPTIONS", "detect_leaks=0")

    with tempfile.NamedTemporaryFile(suffix=".s", delete=False) as tmp:
        asm_path = Path(tmp.name)

    try:
        compile_result = subprocess.run(
            [str(CRV), str(source)],
            stdout=asm_path.open("w"),
            stderr=subprocess.PIPE,
            env=env,
            text=True,
        )
        if compile_result.returncode != 0:
            print(compile_result.stderr, end="", file=sys.stderr)
            return compile_result.returncode

        rars_result = subprocess.run(
            ["java", "-jar", str(RARS_JAR), "nc", "me", "sm", str(asm_path), str(RARS_RUNTIME)],
        )
        return rars_result.returncode
    finally:
        asm_path.unlink(missing_ok=True)


if __name__ == "__main__":
    raise SystemExit(main())
