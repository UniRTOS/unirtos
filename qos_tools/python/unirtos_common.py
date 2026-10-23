#!/usr/bin/env python3

import json
import os
from pathlib import Path
import shutil
import subprocess
import sys


ARM_ARCH_LIST = {"rda_8910_R05", "eigen_718", "qcx216", "qcx217", "ASR","bk7258","bk7238","ecr6600"}
DEFAULT_BUILD_SUBVER = "V01"
DEFAULT_BUILD_QINF = "FFF"
DEFAULT_BUILD_SVN = "01"
DEFAULT_BUILD_MODEMAPP = "01.001"
DEFAULT_BUILD_OEM = ""
DEFAULT_BUILD_TASKNUM = "4"

EXIT_OK = 0
EXIT_ARGUMENT_ERROR = 2
EXIT_UNSUPPORTED_COMMAND = 3
EXIT_TOOLCHAIN_ERROR = 10
EXIT_PROJECT_ERROR = 11
EXIT_PREREQUISITE_ERROR = 12
EXIT_PLATFORM_BUILD_ERROR = 20
EXIT_VERDEF_ERROR = 21
EXIT_CMAKE_ERROR = 22
EXIT_NINJA_ERROR = 23
EXIT_BUILD_ERROR = 24
EXIT_DERIVE_MANIFEST_ERROR = 30
EXIT_DERIVE_OUTPUT_ERROR = 31
EXIT_DERIVE_COPY_ERROR = 32
EXIT_DERIVE_VALIDATION_ERROR = 33
EXIT_PROJECT_CREATE_ERROR = 40
EXIT_PROJECT_TEMPLATE_ERROR = 41
EXIT_PROJECT_COPY_ERROR = 42


class BuildError(Exception):
    def __init__(self, message: str, exit_code: int):
        super().__init__(message)
        self.exit_code = exit_code


class CommandStepError(BuildError):
    def __init__(self, message: str, exit_code: int, command):
        super().__init__(message, exit_code)
        self.command = command


def eprint(message):
    print(message, file=sys.stderr)


def info(message):
    print(f"[unirtos make] {message}")


def derive_info(message):
    print(f"[unirtos internal-sdk-derive] {message}")


def project_info(message):
    print(f"[unirtos project] {message}")


def sdk_root() -> Path:
    return Path(__file__).resolve().parents[2]


def variants_dir(root: Path) -> Path:
    return root / "variants"


def build_dir(root: Path) -> Path:
    return root / "qos_build" / "build"


def qos_build_dir_path(root: Path) -> Path:
    return root / "qos_build"


def gccout_dir_path(root: Path) -> Path:
    return qos_build_dir_path(root) / "gccout"


def output_dir(root: Path) -> Path:
    return root / "output"


def firmware_pack_path(root: Path) -> Path:
    return gccout_dir_path(root) / "images" / "firmware_base_release_pack.7z"


def gccout_archive_path(root: Path, arch: str, project: str) -> Path:
    return root / "qos_kernel" / arch / "images" / project / "gccout.7z"


def arch_project_dir(root: Path, arch: str) -> Path:
    return root / "qos_kernel" / arch / "arch_prj"


def normalize_path(path: Path) -> str:
    return str(path).replace("\\", "/")


def delete_path(target: Path):
    if not target.exists():
        return
    if target.is_dir():
        shutil.rmtree(target)
    else:
        target.unlink()


def run_command(cmd, *, cwd, env=None, label="command", exit_code=EXIT_BUILD_ERROR, logger=info):
    # Keep subprocess failure handling centralized so CLI errors stay uniform.
    logger(f"run: {' '.join(str(part) for part in cmd)}")
    result = subprocess.run(cmd, cwd=str(cwd), env=env)
    if result.returncode != 0:
        raise CommandStepError(
            f"{label} failed with exit code {result.returncode}",
            exit_code,
            cmd,
        )


def load_platform_map(root: Path):
    platform_json = root / "qos_components" / "system" / "platform" / "platform.json"
    with platform_json.open("r", encoding="utf-8") as fp:
        return json.load(fp)


def resolve_build_arch(root: Path, project: str) -> str:
    platforms = load_platform_map(root)
    for arch, boards in platforms.items():
        if project in boards:
            return arch
    raise BuildError(f"unsupported project: {project}", EXIT_PROJECT_ERROR)


def resolve_toolchain_root() -> Path:
    unirtos_path = shutil.which("unirtos.exe")
    if not unirtos_path:
        raise BuildError("not find unirtos.exe in PATH", EXIT_TOOLCHAIN_ERROR)
    return Path(unirtos_path).resolve().parents[1]


def resolve_python(toolchain_root: Path) -> Path:
    py = toolchain_root / "Python312" / "python.exe"
    if py.exists():
        return py
    return Path(sys.executable)
