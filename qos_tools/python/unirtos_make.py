#!/usr/bin/env python3

from dataclasses import dataclass
import os
from pathlib import Path
import shutil

from unirtos_common import (
    ARM_ARCH_LIST,
    DEFAULT_BUILD_MODEMAPP,
    DEFAULT_BUILD_OEM,
    DEFAULT_BUILD_QINF,
    DEFAULT_BUILD_SUBVER,
    DEFAULT_BUILD_SVN,
    DEFAULT_BUILD_TASKNUM,
    EXIT_ARGUMENT_ERROR,
    EXIT_CMAKE_ERROR,
    EXIT_NINJA_ERROR,
    EXIT_PLATFORM_BUILD_ERROR,
    EXIT_PREREQUISITE_ERROR,
    EXIT_PROJECT_ERROR,
    EXIT_VERDEF_ERROR,
    BuildError,
    arch_project_dir,
    build_dir,
    gccout_archive_path,
    gccout_dir_path,
    firmware_pack_path,
    info,
    normalize_path,
    output_dir,
    qos_build_dir_path,
    resolve_build_arch,
    resolve_python,
    resolve_toolchain_root,
    run_command,
    sdk_root,
)


@dataclass(frozen=True)
class BuildConfig:
    project: str
    version: str
    build_type: str
    operation: str
    build_tasknum: str
    build_subver: str = DEFAULT_BUILD_SUBVER
    build_qinf: str = DEFAULT_BUILD_QINF
    build_certver: str = ""
    build_svn: str = DEFAULT_BUILD_SVN
    build_modemapp: str = DEFAULT_BUILD_MODEMAPP
    build_oem: str = DEFAULT_BUILD_OEM


def resolve_output_root() -> Path:
    """Resolve output root directory (prefer external app dir when provided)."""
    external_app_dir = os.environ.get("UNIRTOS_EXTERNAL_APP_DIR", "").strip()
    print(f"external_app_dir: {external_app_dir}")
    if external_app_dir:
        external_root = Path(external_app_dir).expanduser().resolve()
        external_root.mkdir(parents=True, exist_ok=True)
        return external_root
    return sdk_root()


def print_make_usage():
    print("Usage:")
    print("  unirtos.exe make <project> <version> [all|app] [new|r]")
    print("  unirtos.exe make --project <project> --version <version> [--type all|app] [--operation new|r] [--jobs N]")
    print("")
    print("Notes:")
    print("  default type is all; default operation is new")
    print("  app path is verified in the current environment")
    print("  all path is implemented but not validated in the current environment")
    print("")
    print("Examples:")
    print("  unirtos.exe make EC800ZCN_LA EC800ZCNLA0E423_BETA000034")
    print("  unirtos.exe make EC800ZCN_LA EC800ZCNLA0E423_BETA000034 app r")
    print("  unirtos.exe make EG800ZCN_LA EG800ZCNLAR01A01M04_BETA0403_OCPU all new")
    print("  unirtos.exe make --project EG800ZCN_LA --version EG800ZCNLAR01A01M04_BETA0403_OCPU --type app --operation r --jobs 4")


def parse_make_args(args):
    if not args or args[0] in {"-h", "--help", "help"}:
        print_make_usage()
        return None

    project = None
    version = None
    build_type = "app"
    operation = "r"
    jobs = DEFAULT_BUILD_TASKNUM
    positional = []

    i = 0
    while i < len(args):
        arg = args[i]
        if arg in {"-h", "--help", "help"}:
            print_make_usage()
            return None
        if arg == "--project":
            i += 1
            if i >= len(args):
                raise BuildError("--project requires a value", EXIT_ARGUMENT_ERROR)
            project = args[i]
        elif arg == "--version":
            i += 1
            if i >= len(args):
                raise BuildError("--version requires a value", EXIT_ARGUMENT_ERROR)
            version = args[i]
        elif arg == "--type":
            i += 1
            if i >= len(args):
                raise BuildError("--type requires a value", EXIT_ARGUMENT_ERROR)
            build_type = args[i]
        elif arg == "--operation":
            i += 1
            if i >= len(args):
                raise BuildError("--operation requires a value", EXIT_ARGUMENT_ERROR)
            operation = args[i]
        elif arg == "--jobs":
            i += 1
            if i >= len(args):
                raise BuildError("--jobs requires a value", EXIT_ARGUMENT_ERROR)
            jobs = args[i]
        elif arg.startswith("--"):
            raise BuildError(f"unknown option: {arg}", EXIT_ARGUMENT_ERROR)
        else:
            positional.append(arg)
        i += 1

    if project is None and len(positional) >= 1:
        project = positional[0]
    if version is None and len(positional) >= 2:
        version = positional[1]
    if len(positional) >= 3:
        build_type = positional[2]
    if len(positional) >= 4:
        operation = positional[3]
    if len(positional) > 4:
        raise BuildError("too many positional arguments for make", EXIT_ARGUMENT_ERROR)

    if build_type not in {"all", "app"}:
        raise BuildError("type is invalid, should be app/all", EXIT_ARGUMENT_ERROR)
    if operation not in {"new", "r"}:
        raise BuildError("operation is invalid, should be new/r", EXIT_ARGUMENT_ERROR)
    if not jobs.isdigit() or int(jobs) <= 0:
        raise BuildError("jobs is invalid, should be a positive integer", EXIT_ARGUMENT_ERROR)
    if not project or not version:
        raise BuildError("make requires <project> <version> or --project/--version", EXIT_ARGUMENT_ERROR)

    return BuildConfig(
        project=project,
        version=version,
        build_type=build_type,
        operation=operation,
        build_tasknum=jobs,
        build_certver=version,
    )


def build_environment(toolchain_root: Path, config: BuildConfig, arch: str):
    # Mirror the env contract expected by the existing CMake and platform scripts.
    env = os.environ.copy()
    toolchain_root_posix = normalize_path(toolchain_root)
    path_entries = [
        str(toolchain_root / "7z"),
        str(toolchain_root / "tar"),
        str(toolchain_root / "make"),
        str(toolchain_root / "ninja-win-1.12.1"),
        str(toolchain_root / "cmake-3.28.0" / "bin"),
        str(toolchain_root / "Python312"),
        str(toolchain_root / "Git"),
        str(toolchain_root / "Git" / "bin"),
        str(toolchain_root / "Git" / "usr" / "bin"),
    ]

    if arch not in ARM_ARCH_LIST:
        raise BuildError(f"unsupported build arch: {arch}", EXIT_PROJECT_ERROR)
        
    if arch == "ecr6600":
        compiler_path = f"{toolchain_root_posix}/nds32le-elf-mculib-v3s-win"
        compiler_bin = f"{compiler_path}/bin"
        path_entries.append(str(toolchain_root / "nds32le-elf-mculib-v3s-win" / "bin"))
    elif arch == "bk7258" or arch == "bk7238":
        compiler_path = f"{toolchain_root_posix}/gcc-arm-none-eabi-10.3-2021.10-win32"
        compiler_bin = f"{compiler_path}/bin"    
        path_entries.append(str(toolchain_root / "gcc-arm-none-eabi-10.3-2021.10-win32" / "bin"))
    else:
        compiler_path = f"{toolchain_root_posix}/gcc-arm-none-eabi-10.2.1"
        compiler_bin = f"{compiler_path}/bin"
        path_entries.append(str(toolchain_root / "gcc-arm-none-eabi-10.2.1" / "bin"))
    
    env["PATH"] = os.pathsep.join(path_entries + [env.get("PATH", "")])

    env["CMAKE_SYSTEM_NAME"] = "Generic"
    env["UNIRTOS_TOOLCHAIN_PATH"] = toolchain_root_posix
    env["UNIRTOS_TOOLCHAIN_PATH_WIN"] = str(toolchain_root)
    env["build_project"] = config.project
    env["build_arch"] = arch
    env["build_version"] = config.version
    env["build_tasknum"] = config.build_tasknum
    env["COMPILER_PATH"] = compiler_path
    if arch == "ecr6600":
        env["COMPILER_AS"] = f"{compiler_bin}/nds32le-elf-as.exe"
        env["COMPILER_CC"] = f"{compiler_bin}/nds32le-elf-gcc.exe"
        env["COMPILER_CXX"] = f"{compiler_bin}/nds32le-elf-g++.exe"
        env["COMPILER_AR"] = f"{compiler_bin}/nds32le-elf-ar.exe"
        env["COMPILER_LD"] = f"{compiler_bin}/nds32le-elf-ld.exe"
        env["COMPILER_OBJCOPY"] = f"{compiler_bin}/nds32le-elf-objcopy.exe"
        env["COMPILER_SIZE"] = f"{compiler_bin}/nds32le-elf-size.exe"
        env["COMPILER_STRIP"] = f"{compiler_bin}/nds32le-elf-strip.exe"
        env["COMPILER_GDB"] = f"{compiler_bin}/nds32le-elf-gdb.exe"
        env["COMPILER_OBJDUMP"] = f"{compiler_bin}/nds32le-elf-objdump.exe"
        env["COMPILER_READELF"] = f"{compiler_bin}/nds32le-elf-readelf.exe"
        env["COMPILER_NM"] = f"{compiler_bin}/nds32le-elf-nm.exe"
    else:
        env["COMPILER_AS"] = f"{compiler_bin}/arm-none-eabi-as.exe"
        env["COMPILER_CC"] = f"{compiler_bin}/arm-none-eabi-gcc.exe"
        env["COMPILER_CXX"] = f"{compiler_bin}/arm-none-eabi-g++.exe"
        env["COMPILER_AR"] = f"{compiler_bin}/arm-none-eabi-ar.exe"
        env["COMPILER_LD"] = f"{compiler_bin}/arm-none-eabi-ld.exe"
        env["COMPILER_OBJCOPY"] = f"{compiler_bin}/arm-none-eabi-objcopy.exe"
        env["COMPILER_SIZE"] = f"{compiler_bin}/arm-none-eabi-size.exe"
        env["COMPILER_STRIP"] = f"{compiler_bin}/arm-none-eabi-strip.exe"
        env["COMPILER_GDB"] = f"{compiler_bin}/arm-none-eabi-gdb.exe"
        env["COMPILER_OBJDUMP"] = f"{compiler_bin}/arm-none-eabi-objdump.exe"
        env["COMPILER_READELF"] = f"{compiler_bin}/arm-none-eabi-readelf.exe"
        env["COMPILER_NM"] = f"{compiler_bin}/arm-none-eabi-nm.exe"

    return env


def clean_for_new(output_root: Path):
    current_build_dir = build_dir(output_root)
    current_output_dir = output_dir(output_root)
    root_config = sdk_root() / ".config"
    if current_build_dir.exists():
        shutil.rmtree(current_build_dir)
    if current_output_dir.exists():
        shutil.rmtree(current_output_dir)
    if root_config.exists():
        root_config.unlink()


def ensure_gccout_for_app(sdk_root_path: Path, output_root: Path, arch: str, project: str, toolchain_root: Path):
    firmware_pack = firmware_pack_path(output_root)
    gccout_dir = gccout_dir_path(output_root)
    gccout_libraries = gccout_dir / "libraries"

    if firmware_pack.exists() and gccout_libraries.exists():
        info(f"reuse existing gccout: {gccout_dir}")
        return

    if arch != "eigen_718":
        raise BuildError(
            f"app build requires existing gccout for arch {arch}: {firmware_pack}",
            EXIT_PREREQUISITE_ERROR,
        )

    archive = gccout_archive_path(sdk_root_path, arch, project)
    if not archive.exists():
        raise BuildError(
            f"missing gccout and archive not found: {archive}",
            EXIT_PREREQUISITE_ERROR,
        )

    seven_zip = toolchain_root / "7z" / "7z.exe"
    if not seven_zip.exists():
        raise BuildError(
            f"7z tool not found: {seven_zip}",
            EXIT_PREREQUISITE_ERROR,
        )

    target_qos_build = qos_build_dir_path(output_root)
    target_qos_build.mkdir(exist_ok=True)
    if gccout_dir.exists():
        shutil.rmtree(gccout_dir)

    info(f"extract gccout archive: {archive}")
    run_command(
        [str(seven_zip), "x", "-y", str(archive), f"-o{target_qos_build}"],
        cwd=sdk_root_path,
        label="extract gccout archive",
        exit_code=EXIT_PREREQUISITE_ERROR,
    )

    if not firmware_pack.exists() or not gccout_libraries.exists():
        raise BuildError(
            f"invalid gccout archive content: expected {firmware_pack} and {gccout_libraries}",
            EXIT_PREREQUISITE_ERROR,
        )

    info("gccout prepared for app build")


def ensure_compatible_build_cache(source_root: Path, output_root: Path):
    current_build_dir = build_dir(output_root)
    cache_file = current_build_dir / "CMakeCache.txt"
    if not cache_file.exists():
        return

    # Older bat-based flows could generate cache entries from a virtual drive path.
    # Reuse only when the current SDK root matches the cached home directory.
    expected_home = normalize_path(source_root)
    current_home = None
    with cache_file.open("r", encoding="utf-8", errors="ignore") as fp:
        for line in fp:
            if line.startswith("CMAKE_HOME_DIRECTORY:INTERNAL="):
                current_home = line.strip().split("=", 1)[1]
                break

    if current_home and current_home != expected_home:
        info(
            "incompatible cmake cache detected, removing existing qos_build/build "
            f"(cache home: {current_home}, expected: {expected_home})"
        )
        shutil.rmtree(current_build_dir)


def run_platform_build(source_root: Path, config: BuildConfig, arch: str, python_exe: Path):
    arch_prj_path = arch_project_dir(source_root, arch)
    if not arch_prj_path.exists():
        raise BuildError(f"platform project path not found: {arch_prj_path}", EXIT_PLATFORM_BUILD_ERROR)

    (source_root / "qos_build").mkdir(exist_ok=True)

    pre_write_root_path = source_root / "qos_tools" / "python" / "pre_write_root_path.py"
    qos_components_path = source_root / "qos_components"

    run_command(
        [str(python_exe), str(pre_write_root_path), str(arch_prj_path), str(qos_components_path)],
        cwd=source_root,
        label="prepare platform root path",
        exit_code=EXIT_PLATFORM_BUILD_ERROR,
    )

    cmd = [
        "cmd.exe",
        "/c",
        "buildlib.bat",
        config.operation,
        config.project,
        config.version,
        config.build_subver,
        config.build_qinf,
        config.build_certver,
        config.build_svn,
        config.build_modemapp,
    ]
    if config.build_oem:
        cmd.append(config.build_oem)
    run_command(cmd, cwd=arch_prj_path, label="platform build", exit_code=EXIT_PLATFORM_BUILD_ERROR)

def run_cmake_and_ninja(source_root: Path, output_root: Path, env, toolchain_root: Path, python_exe: Path):
    current_build_dir = build_dir(output_root)
    current_build_dir.mkdir(parents=True, exist_ok=True)
    cmake_exe = toolchain_root / "cmake-3.28.0" / "bin" / "cmake.exe"
    ninja_exe = toolchain_root / "ninja-win-1.12.1" / "ninja.exe"

    ext_app_dir = env.get("UNIRTOS_EXTERNAL_APP_DIR", "")
    ext_app_name = env.get("UNIRTOS_EXTERNAL_APP_NAME", "")

    cmake_cmd = [
        str(cmake_exe),
        "-B",
        str(current_build_dir),
        "-G",
        "Ninja",
        f"-DCMAKE_MAKE_PROGRAM={ninja_exe}",
        f"-DPython_EXECUTABLE={python_exe}",
    ]
    if ext_app_dir:
        print(f"ext_app_dir: {ext_app_dir}")
        cmake_cmd.append(f"-DUNIRTOS_EXTERNAL_APP_DIR={ext_app_dir}")
    if ext_app_name:
        cmake_cmd.append(f"-DUNIRTOS_EXTERNAL_APP_NAME={ext_app_name}")

    run_command(
        cmake_cmd,
        cwd=source_root,
        env=env,
        label="cmake configure",
        exit_code=EXIT_CMAKE_ERROR,
    )
    run_command(
        [str(ninja_exe), f"-j{env['build_tasknum']}"],
        cwd=current_build_dir,
        env=env,
        label="ninja build",
        exit_code=EXIT_NINJA_ERROR,
    )


def print_summary(config: BuildConfig, arch: str, toolchain_root: Path, source_root: Path, output_root: Path):
    info("configuration")
    print(f"  project   : {config.project}")
    print(f"  version   : {config.version}")
    print(f"  type      : {config.build_type}")
    print(f"  operation : {config.operation}")
    print(f"  jobs      : {config.build_tasknum}")
    print(f"  arch      : {arch}")
    print(f"  toolchain : {toolchain_root}")
    print(f"  source    : {source_root}")
    print(f"  output    : {output_root}")


def handle_make(args):
    config = parse_make_args(args)
    if config is None:
        return 0

    source_root = sdk_root()
    arch = resolve_build_arch(source_root, config.project)
    output_root = resolve_output_root()
    external_app_mode = bool(os.environ.get("UNIRTOS_EXTERNAL_APP_DIR", "").strip())
    toolchain_root = resolve_toolchain_root()
    python_exe = resolve_python(toolchain_root)
    
    env = build_environment(toolchain_root, config, arch)
    env["UNIRTOS_OUTPUT_ROOT"] = str(output_root)

    print_summary(config, arch, toolchain_root, source_root, output_root)

    if external_app_mode and config.build_type != "app":
        raise BuildError(
            "external app mode supports only type=app when all outputs must stay in app directory",
            EXIT_ARGUMENT_ERROR,
        )

    if config.build_type == "all":
        info("warning: all path is implemented but not validated in the current environment")
    else:
        info("using verified app build path")

    # new does a clean build; r keeps the build tree when the cache is still reusable.
    if config.operation == "new":
        clean_for_new(output_root)
    else:
        ensure_compatible_build_cache(source_root, output_root)

    # all first prepares platform-side artifacts; app reuses or restores gccout prereq.
    if config.build_type == "all":
        run_platform_build(source_root, config, arch, python_exe)
    elif arch not in {"bk7258", "bk7238", "ecr6600"}:
        ensure_gccout_for_app(source_root, output_root, arch, config.project, toolchain_root)

    run_cmake_and_ninja(source_root, output_root, env, toolchain_root, python_exe)

    info("build succeeded")
    print(f"[unirtos make] project : {config.project}")
    print(f"[unirtos make] version : {config.version}")
    print(f"[unirtos make] output  : {output_root / 'qos_build' / 'release' / config.version}")
    return 0
