#!/usr/bin/env python3

import argparse
import os
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path


ROOT_DIR = Path(__file__).resolve().parents[1]
RARS_RUNTIME = ROOT_DIR / "stdlib/rars.s"


@dataclass(frozen=True)
class SmokeCase:
    name: str
    sources: tuple[Path, ...]
    expected_stdout: str
    stdin: str = ""


CASES = (
    SmokeCase("print_arithmetic", (ROOT_DIR / "tests/rars/print_arithmetic.c",), "17"),
    SmokeCase("print_loop", (ROOT_DIR / "tests/rars/print_loop.c",), "6"),
    SmokeCase("arrays_globals_char", (ROOT_DIR / "tests/rars/arrays_globals_char.c",), "193"),
    SmokeCase("local_arrays_loop", (ROOT_DIR / "tests/rars/local_arrays_loop.c",), "16"),
    SmokeCase("global_arrays", (ROOT_DIR / "tests/rars/global_arrays.c",), "10"),
    SmokeCase("char_arrays", (ROOT_DIR / "tests/rars/char_arrays.c",), "459"),
    SmokeCase("pointers", (ROOT_DIR / "tests/rars/pointers.c",), "26"),
    SmokeCase("char_pointers", (ROOT_DIR / "tests/rars/char_pointers.c",), "230"),
    SmokeCase("pointer_arithmetic", (ROOT_DIR / "tests/rars/pointer_arithmetic.c",), "23"),
    SmokeCase("bitwise_shift", (ROOT_DIR / "tests/rars/bitwise_shift.c",), "33"),
    SmokeCase("update_compound_continue", (ROOT_DIR / "tests/rars/update_compound_continue.c",), "55"),
    SmokeCase("string_literals", (ROOT_DIR / "tests/rars/string_literals.c",), "S:GiA\nbc\n122"),
    SmokeCase("ptr_walk", (ROOT_DIR / "tests/rars/ptr_walk.c",), "118"),
    SmokeCase("fibonacci", (ROOT_DIR / "tests/rars/fibonacci.c",), "88"),
    SmokeCase("isort", (ROOT_DIR / "tests/rars/isort.c",), "1\n9\n35"),
    SmokeCase("gcd_lcm", (ROOT_DIR / "tests/rars/gcd_lcm.c",), "6\n36\n25"),
    SmokeCase(
        "matmul",
        (ROOT_DIR / "tests/rars/matmul.c",),
        "19 22\n43 50",
        stdin="2\n1\n2\n3\n4\n5\n6\n7\n8\n",  # A=[[1,2],[3,4]] B=[[5,6],[7,8]]
    ),
    SmokeCase(
        "plain_multifile",
        (
            ROOT_DIR / "tests/multifile/plain_a.c",
            ROOT_DIR / "tests/multifile/plain_b.c",
        ),
        "7",
    ),
    SmokeCase(
        "static_collision",
        (
            ROOT_DIR / "tests/multifile/static_collision_a.c",
            ROOT_DIR / "tests/multifile/static_collision_b.c",
        ),
        "3",
    ),
    SmokeCase(
        "extern_shared",
        (
            ROOT_DIR / "tests/multifile/extern_shared_a.c",
            ROOT_DIR / "tests/multifile/extern_shared_b.c",
        ),
        "10",
    ),
    SmokeCase(
        "static_function_vs_external_global",
        (
            ROOT_DIR / "tests/multifile/static_function_vs_external_global_a.c",
            ROOT_DIR / "tests/multifile/static_function_vs_external_global_b.c",
        ),
        "2",
    ),
    SmokeCase(
        "static_global_vs_external_function",
        (
            ROOT_DIR / "tests/multifile/static_global_vs_external_function_a.c",
            ROOT_DIR / "tests/multifile/static_global_vs_external_function_b.c",
        ),
        "9",
    ),
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Compile Small-C smoke cases and run the generated assembly in RARS.",
    )
    parser.add_argument(
        "--crv",
        default=os.environ.get("CRV", str(ROOT_DIR / "build/crv")),
        help="path to the crv compiler; defaults to CRV or build/crv",
    )
    parser.add_argument(
        "--rars-jar",
        default=os.environ.get("RARS_JAR"),
        help="path to rars1_6.jar; defaults to RARS_JAR",
    )

    return parser.parse_args()


def fail(message: str) -> int:
    print(message, file=sys.stderr)
    return 2


def check_tools(crv: Path, rars_jar: Path | None) -> int:
    if rars_jar is None:
        return fail(
            "RARS_JAR is not set. Example: "
            "RARS_JAR=/path/to/rars1_6.jar ./tests/rars_e2e.py"
        )

    if not rars_jar.is_file():
        return fail(f"RARS jar not found: {rars_jar}")

    if not crv.is_file() or not os.access(crv, os.X_OK):
        print(f"Compiler not found or not executable: {crv}", file=sys.stderr)
        print("Build it first: cmake --build build", file=sys.stderr)
        return 2

    if not RARS_RUNTIME.is_file():
        return fail(f"RARS runtime not found: {RARS_RUNTIME}")

    return 0


def compile_sources(crv: Path, sources: tuple[Path, ...], asm_path: Path) -> subprocess.CompletedProcess[str]:
    env = os.environ.copy()
    env.setdefault("ASAN_OPTIONS", "detect_leaks=0")

    result = subprocess.run(
        [str(crv), *(str(source) for source in sources)],
        check=False,
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

    if result.returncode == 0:
        asm_path.write_text(result.stdout)

    return result


def compile_case(crv: Path, case: SmokeCase, asm_path: Path) -> subprocess.CompletedProcess[str]:
    return compile_sources(crv, case.sources, asm_path)


def run_rars(rars_jar: Path, asm_path: Path, stdin: str = "") -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ["java", "-jar", str(rars_jar), "nc", "me", "sm", str(asm_path), str(RARS_RUNTIME)],
        check=False,
        input=stdin,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )


def normalize_stdout(stdout: str) -> str:
    return stdout.rstrip("\n")


def print_failure(case: SmokeCase, expected: str, actual: str, stderr: str) -> None:
    print(f"[FAIL] {case.name}", file=sys.stderr)
    print(f"expected: <{expected}>", file=sys.stderr)
    print(f"actual:   <{actual}>", file=sys.stderr)

    if stderr:
        print("RARS stderr:", file=sys.stderr)
        print(stderr, end="" if stderr.endswith("\n") else "\n", file=sys.stderr)


def run_case(crv: Path, rars_jar: Path, case: SmokeCase, tmp_dir: Path) -> bool:
    asm_path = tmp_dir / f"{case.name}.s"

    compile_result = compile_case(crv, case, asm_path)
    if compile_result.returncode != 0:
        print(f"[FAIL] {case.name}", file=sys.stderr)
        print("Compiler stderr:", file=sys.stderr)
        print(compile_result.stderr, end="" if compile_result.stderr.endswith("\n") else "\n", file=sys.stderr)
        return False

    rars_result = run_rars(rars_jar, asm_path, case.stdin)
    actual_stdout = normalize_stdout(rars_result.stdout)

    if rars_result.returncode != 0 or actual_stdout != case.expected_stdout:
        print_failure(case, case.expected_stdout, actual_stdout, rars_result.stderr)
        return False

    print(f"[PASS] {case.name}")
    return True


def main() -> int:
    args = parse_args()
    crv = Path(args.crv)
    rars_jar = Path(args.rars_jar) if args.rars_jar else None

    tools_status = check_tools(crv, rars_jar)
    if tools_status != 0:
        return tools_status

    assert rars_jar is not None

    with tempfile.TemporaryDirectory(prefix="crv-rars-") as tmp:
        tmp_dir = Path(tmp)
        all_passed = True

        for case in CASES:
            all_passed = run_case(crv, rars_jar, case, tmp_dir) and all_passed

    return 0 if all_passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
