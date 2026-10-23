import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


SCRIPT_DIR = Path(__file__).resolve().parent
BK_PY_LIBS_DIR = SCRIPT_DIR.parent / "arch_prj" / "ql_tools"
OTA_TOOLS_DIR = BK_PY_LIBS_DIR / "ota_rbl"
sys.path.insert(0, str(BK_PY_LIBS_DIR))

from bk_misc import parse_format_size
from bk_packager import bk_packager_linear, bk_packager_linear_crc


def package_ota(workdir: Path, partition_config: Path, output_file: Path):
    # ponytail: non-AB OTA only; use the AB packager if configurationab.json is shipped.
    if (partition_config.parent / "configurationab.json").exists():
        raise RuntimeError("BK7258 CSDK OTA packaging does not support AB partitions")

    config = json.loads(partition_config.read_text(encoding="utf-8"))
    config["section"] = [
        part for part in config["section"] if "bootloader" not in part["partition"]
    ]
    config["count"] = len(config["section"])
    if config.get("crc_enable", False):
        # OTA contains raw binaries; convert flash addresses and sizes from 34 to 32 bytes.
        for part in config["section"]:
            part["start_addr"] = f"0x{int(part['start_addr'], 16) * 32 // 34:08x}"
            part["size"] = f"{parse_format_size(part['size']) * 32 // 34 // 1024}K"

    ota_config = workdir / "ota_apps_pack.json"
    ota_config.write_text(json.dumps(config, indent=4), encoding="utf-8")
    ota_bin = workdir / "app_pack.bin"
    bk_packager_linear(workdir, ota_config, ota_bin).pack()
    with ota_bin.open("ab") as fp:
        fp.write(b"\xff" * (-fp.tell() % 64))

    rbl_file = workdir / "app_pack.rbl"
    env = os.environ.copy()
    env["PARTITIONS_DIR"] = str(partition_config.parent)
    subprocess.run(
        [sys.executable, str(OTA_TOOLS_DIR / "ota_packager_python.py"),
         "-i", str(ota_bin), "-o", str(rbl_file), "-g", str(OTA_TOOLS_DIR), "packager"],
        env=env,
        check=True,
    )
    # The vendor tool can exit successfully without producing a file.
    if not rbl_file.is_file() or rbl_file.read_bytes()[:4] != b"RBL\x00":
        raise RuntimeError("OTA packager did not generate a valid app_pack.rbl")
    output_file.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(rbl_file, output_file)


def main() -> int:
    parser = argparse.ArgumentParser(description="Package bootloader, app and app1 into all-app.bin")
    parser.add_argument("--app", type=Path, required=True)
    parser.add_argument("--app1", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--bootloader", type=Path, required=True)
    parser.add_argument("--partition-config", type=Path, required=True)
    parser.add_argument("--ota-output", type=Path, help="Also generate a non-AB RBL OTA package")
    args = parser.parse_args()

    app_file = args.app.resolve()
    app1_file = args.app1.resolve()
    output_file = args.output.resolve()
    bootloader = args.bootloader.resolve()
    partition_config = args.partition_config.resolve()
    output_dir = output_file.parent
    required_files = {
        "app.bin": app_file,
        "app1.bin": app1_file,
        "bootloader.bin": bootloader,
        "partition config": partition_config,
    }
    missing_files = [
        f"{name}: {path}"
        for name, path in required_files.items()
        if not path.is_file()
    ]
    if missing_files:
        print("Missing packaging inputs:")
        print("\n".join(missing_files))
        return 1

    output_dir.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(prefix="bk_packager_") as temp_dir_name:
        temp_dir = Path(temp_dir_name)
        shutil.copy2(app_file, temp_dir / "app.bin")
        shutil.copy2(app1_file, temp_dir / "app1.bin")
        shutil.copy2(bootloader, temp_dir / "bootloader.bin")
        temp_config = temp_dir / "bk_package.json"
        shutil.copy2(partition_config, temp_config)

        packager = bk_packager_linear_crc(temp_dir, temp_config, output_file)
        packager.pack()
        if args.ota_output:
            package_ota(temp_dir, partition_config, args.ota_output.resolve())

    print(f"Generated: {output_file}")
    if args.ota_output:
        print(f"Generated: {args.ota_output.resolve()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
