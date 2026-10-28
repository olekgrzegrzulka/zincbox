#!/usr/bin/env python3
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path
from typing import NoReturn

IS_WINDOWS = platform.system() == "Windows"
EXE_NAME = "zincbox.exe" if IS_WINDOWS else "zincbox"
BUILD_DIR = Path("build")


def usage() -> None:
    print(f"Usage: {sys.argv[0]} [flags]")
    print("Flags:")
    print("  asan      - Enable AddressSanitizer")
    print("  release   - Build in release mode")
    print("  debug     - Build in debug mode (default)")
    print("  clang     - Use Clang (default on Linux)")
    print("  gcc       - Use GCC")
    print("  msvc      - Use MSVC (default on Windows)")
    print("  no-ccache - Disable ccache")
    print("  ninja     - Use Ninja build system (default)")
    print("  make      - Use Make build system")
    print("  run       - Run the program after build")
    print("  clean     - Remove build directory")
    print("  help      - Display this help")
    sys.exit(0)


def check_cmd(cmd: str) -> str | None:
    return shutil.which(cmd)


def fail(msg: str) -> NoReturn:
    print(f"Error: {msg}", file=sys.stderr)
    sys.exit(1)


def resolve_compiler(flags: set[str]) -> tuple[str, str]:
    requested = flags & {"gcc", "clang", "msvc"}
    if len(requested) > 1:
        fail(f"{', '.join(requested)} are mutually exclusive.")

    choice = next(iter(requested), None)

    if choice == "gcc":
        cc, cxx = check_cmd("gcc"), check_cmd("g++")
        if not (cc and cxx):
            fail("'gcc' was explicitly requested but is not installed.")
        return cc, cxx

    if choice == "clang":
        cc, cxx = check_cmd("clang"), check_cmd("clang++")
        if cc is None or cxx is None:
            fail("'clang' was explicitly requested but is not installed.")
        return cc, cxx

    if choice == "msvc":
        cl = check_cmd("cl")
        if cl is None:
            fail("'msvc' (cl.exe) was explicitly requested but is not found in PATH.")
        return cl, cl

    if IS_WINDOWS:
        cl = check_cmd("cl")
        if cl:
            return cl, cl

    cc, cxx = check_cmd("clang"), check_cmd("clang++")
    if cc and cxx:
        return cc, cxx

    cc, cxx = check_cmd("gcc"), check_cmd("g++")
    if cc and cxx:
        return cc, cxx

    fail("No suitable C/C++ compiler (clang, gcc, msvc) found on the system.")
    return "", ""


def resolve_generator(flags: set[str]) -> str:
    if "make" in flags and "ninja" in flags:
        fail("'make' and 'ninja' are mutually exclusive.")

    if "ninja" in flags:
        if not check_cmd("ninja"):
            fail("'ninja' was explicitly requested but is not installed.")
        return "Ninja"

    if "make" in flags:
        if not check_cmd("make"):
            fail("'make' was explicitly requested but is not installed.")
        return "Unix Makefiles"

    if check_cmd("ninja"):
        return "Ninja"
    if check_cmd("make"):
        return "Unix Makefiles"

    fail("Neither 'ninja' nor 'make' was found on the system.")
    return ""


def main() -> None:
    valid_flags = {
        "clean",
        "asan",
        "release",
        "debug",
        "gcc",
        "clang",
        "msvc",
        "no-ccache",
        "ninja",
        "make",
        "run",
        "help",
    }

    flags = set(sys.argv[1:])
    unknown = flags - valid_flags
    if unknown:
        print(f"Unknown parameter: {next(iter(unknown))}")
        usage()

    if "help" in flags:
        usage()

    if "clean" in flags:
        print("Cleaning build directory...")
        shutil.rmtree(BUILD_DIR, ignore_errors=True)
        sys.exit(0)

    if "asan" in flags and "release" in flags:
        fail("'asan' and 'release' are mutually exclusive.")

    if "debug" in flags and "release" in flags:
        fail("'debug' and 'release' are mutually exclusive.")

    build_type = "Release" if "release" in flags else "Debug"
    asan = "ON" if "asan" in flags else "OFF"
    use_ccache = "no-ccache" not in flags
    run_app = "run" in flags

    cc_path, cxx_path = resolve_compiler(flags)
    generator = resolve_generator(flags)

    if sys.stdout.isatty() and not os.environ.get("CI"):
        os.system("cls" if IS_WINDOWS else "clear")

    exe_dest = Path(EXE_NAME)
    exe_dest.unlink(missing_ok=True)

    needs_configure = not (
        (BUILD_DIR / "build.ninja").exists() or (BUILD_DIR / "Makefile").exists()
    )

    if needs_configure:
        cmake_cmd = [
            "cmake",
            "-G",
            generator,
            "-B",
            str(BUILD_DIR),
            "-S",
            ".",
            f"-DCMAKE_C_COMPILER={cc_path}",
            f"-DCMAKE_CXX_COMPILER={cxx_path}",
            f"-DENABLE_ASAN={asan}",
            f"-DCMAKE_BUILD_TYPE={build_type}",
            "-DMY_FLAGS=",
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        ]

        if use_ccache and check_cmd("ccache"):
            cmake_cmd.extend(
                [
                    "-DCMAKE_C_COMPILER_LAUNCHER=ccache",
                    "-DCMAKE_CXX_COMPILER_LAUNCHER=ccache",
                ]
            )

        subprocess.run(cmake_cmd, check=True)

    jobs = os.cpu_count() or 1
    subprocess.run(
        ["cmake", "--build", str(BUILD_DIR), "--config", build_type, "-j", str(jobs)],
        check=True,
    )

    candidates = [
        BUILD_DIR / EXE_NAME,
        BUILD_DIR / build_type / EXE_NAME,
    ]
    built_exe = next((p for p in candidates if p.exists()), None)
    if not built_exe:
        fail(f"Built binary '{EXE_NAME}' not found in {BUILD_DIR}.")

    shutil.copy2(built_exe, exe_dest)

    if run_app:
        env = os.environ.copy()
        if asan == "ON":
            env["ASAN_OPTIONS"] = "symbolize=1:handle_abort=1:print_stacktrace=1"
            env["UBSAN_OPTIONS"] = "print_stacktrace=1"
        subprocess.run([str(exe_dest.resolve())], env=env, check=True)


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as e:
        sys.exit(e.returncode)
