"""Identity and integrity checks for user-built A04 EC718 platform artifacts."""
import hashlib
import json
from pathlib import Path, PurePosixPath
import shutil

from unirtos_common import BuildError, EXIT_PREREQUISITE_ERROR, run_command

SDK_VERSION = 'QSR02A04_C_SDK_LTE_E_BETA261029'
COMMON = 'qos_kernel/eigen_718/arch_prj/ql_common/'
STAMP = 'a04-platform.json'


def sha256(path):
    digest = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def fail(message):
    raise BuildError(message, EXIT_PREREQUISITE_ERROR)


def read_json(path):
    try:
        value = json.loads(Path(path).read_text(encoding='utf-8-sig'))
    except (OSError, ValueError) as exc:
        fail('Cannot read platform metadata {}: {}'.format(path, exc))
    if not isinstance(value, dict):
        fail('Platform metadata must be a JSON object: ' + str(path))
    return value


def safe_relative(value):
    if not isinstance(value, str):
        fail('Platform manifest path must be a string')
    path = PurePosixPath(value)
    if (not value or str(path) != value or value == '.' or '\\' in value
            or ':' in value or path.is_absolute() or '..' in path.parts):
        fail('unsafe platform manifest path: ' + value)
    return path


def snapshot_allowed(value):
    safe_relative(value)
    return (value.startswith(COMMON + 'unirtos_adapter/dynamic_create/') and value.endswith('.h')
            or value.startswith(COMMON + 'ver_tag/') and value.endswith('.h')
            or value in {COMMON + 'middleware/quectel/cust/inc/QosaPrjName.h',
                         COMMON + 'middleware/developed/debug/inc/dbversion.h',
                         COMMON + 'middleware/developed/debug/inc/debug_log_ap.h'})


def compiled_package(data):
    return data.get('build_type') == 'platform-only-clean-new'


def valid_digest(value):
    return (isinstance(value, str) and len(value) == 64
            and all(char in '0123456789abcdefABCDEF' for char in value))


def file_matches(path, digest):
    return valid_digest(digest) and path.is_file() and sha256(path) == digest.lower()


def generated_snapshot_allowed(value, project):
    return (snapshot_allowed(value)
            or value == COMMON + 'middleware/quectel/cust/inc/QosaCurPrj.txt'
            or value == COMMON + 'middleware/quectel/project/config/public/' + project + '/system.config')


def read_compiled_manifest(root, project, sidecar, origin):
    data = read_json(sidecar)
    if (data.get('schema_version') != 1 or data.get('sdk_version') != SDK_VERSION
            or data.get('sub_version') != 'V01' or data.get('project') != project
            or not compiled_package(data) or data.get('archive_layout') != 'gccout/'
            or not valid_digest(data.get('archive_sha256'))
            or not valid_digest(data.get('payload_manifest_sha256'))
            or data.get('source_manifest_sha256') != origin.get('source_manifest_sha256')
            or data.get('git_base') != origin.get('git_base')
            or not valid_digest(data.get('source_manifest_sha256'))):
        fail('Compiled A04 package identity/manifest mismatch: ' + str(sidecar))
    snapshots = data.get('generated_configuration')
    if not isinstance(snapshots, list) or not snapshots:
        fail('Compiled A04 package has no generated configuration: ' + str(sidecar))
    names = set()
    for row in snapshots:
        if (not isinstance(row, dict) or not isinstance(row.get('path'), str)
                or not generated_snapshot_allowed(row['path'], project)
                or not valid_digest(row.get('sha256')) or row['path'].casefold() in names):
            fail('Invalid generated configuration in: ' + str(sidecar))
        names.add(row['path'].casefold())
    required = {COMMON + 'ver_tag/qosa_version.h',
                COMMON + 'middleware/quectel/cust/inc/QosaPrjName.h',
                COMMON + 'middleware/quectel/cust/inc/QosaCurPrj.txt'}
    if not required <= {row['path'] for row in snapshots}:
        fail('Required per-project generated headers are missing: ' + str(sidecar))
    return data


def read_manifest(root, project):
    root = Path(root)
    if (not isinstance(project, str) or not project
            or any(char not in 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_' for char in project)):
        fail('Invalid platform project name')
    origin = read_json(root / 'SDK_ORIGIN.json')
    if origin.get('sdk_version') != SDK_VERSION:
        fail('SDK_ORIGIN.json is not the expected A04 SDK identity')
    folder = root / 'qos_kernel/eigen_718/images' / project
    compiled = folder / 'platform-build.json'
    if compiled.is_file():
        return read_compiled_manifest(root, project, compiled, origin), folder / 'gccout.7z'
    sidecar = folder / 'platform-manifest.json'
    if not sidecar.is_file():
        fail('Missing A04 package metadata for {}. Expected platform-build.json '
             'or platform-manifest.json beside gccout.7z. See PLATFORM_BUILD_STATUS.md. '
             '0511 archives and an unstamped cached gccout are not accepted.'.format(project))
    data = read_json(sidecar)
    if (data.get('schema_version') != 1 or data.get('sdk_version') != SDK_VERSION
            or data.get('project') != project or data.get('arch') != 'eigen_718'
            or data.get('artifact_status') != 'USER-BUILT-PLATFORM'
            or not isinstance(data.get('files'), dict) or not data['files']
            or not isinstance(data.get('sdk_snapshots', {}), dict)):
        fail('Platform identity/project/schema mismatch: ' + str(sidecar))
    for relative in data['files']:
        safe_relative(relative)
    for relative in data.get('sdk_snapshots', {}):
        if not snapshot_allowed(relative):
            fail('Unapproved generated-header snapshot target: ' + relative)
    return data, folder / 'gccout.7z'


def validate_compiled_tree(gccout, data):
    embedded = read_json(gccout / STAMP)
    identity = ('schema_version', 'sdk_version', 'sub_version', 'project',
                'source_manifest_sha256', 'git_base', 'build_type', 'built_at',
                'customer_build_label', 'generated_configuration', 'closed_source_compiled_objects')
    if any(key not in embedded or embedded[key] != data.get(key) for key in identity):
        fail('Cached gccout does not match the compiled A04 project/package; clean the app cache and retry')
    manifest = gccout / 'a04-files.json'
    if not file_matches(manifest, data['payload_manifest_sha256']):
        fail('A04 payload manifest missing or changed: ' + str(manifest))
    try:
        rows = json.loads(manifest.read_text(encoding='utf-8-sig'))
    except (OSError, ValueError) as exc:
        fail('Cannot read A04 payload manifest: ' + str(exc))
    if not isinstance(rows, list) or not rows:
        fail('A04 payload manifest must be a nonempty array')
    names = set()
    for row in rows:
        if not isinstance(row, dict):
            fail('Invalid payload manifest entry')
        relative = str(safe_relative(row.get('path')))
        if relative.casefold() in names:
            fail('Duplicate A04 payload manifest path: ' + relative)
        names.add(relative.casefold())
        file = gccout / relative
        if (not isinstance(row.get('bytes'), int) or not file_matches(file, row.get('sha256'))
                or file.stat().st_size != row['bytes']):
            fail('Platform file missing or changed: ' + str(file))
    # Firmware images are unpacked by CMake beneath images/. Only that derived
    # directory may add files; bridge/linker/snapshot inputs must remain exact.
    derived = 'images/firmware_base_release_pack/'
    actual = {p.relative_to(gccout).as_posix().casefold() for p in gccout.rglob('*')
              if p.is_file() and not p.relative_to(gccout).as_posix().casefold().startswith(derived)}
    if actual != names | {'a04-files.json'}:
        fail('Unexpected stale or missing files in compiled A04 gccout')
    for directory in ('images', 'libraries', 'scripts', 'generated'):
        if not (gccout / directory).is_dir():
            fail('Invalid compiled gccout directory: ' + directory)
    for name in ('ql_app_main.o', 'ql_app_pre_init.o', 'ql_syscalls.o', 'qos_app_api_map.o'):
        if not (gccout / 'libraries' / name).is_file():
            fail('Missing A04 bridge object: ' + name)
    if not list((gccout / 'scripts').glob('*.ld')):
        fail('Missing A04 linker script')
    if not (gccout / 'images/firmware_base_release_pack.7z').is_file():
        fail('A04 firmware base archive is missing')


def validate_tree(gccout, data):
    gccout = Path(gccout)
    if compiled_package(data):
        validate_compiled_tree(gccout, data)
        return
    stamp = gccout / STAMP
    if not stamp.is_file():
        fail('Unstamped/legacy gccout cache; remove its qos_build/gccout directory and retry')
    expected = dict(data)
    expected.pop('archive_sha256', None)
    if read_json(stamp) != expected:
        fail('Cached gccout does not match this A04 project/package; remove it and retry')
    for relative, digest in data['files'].items():
        path = gccout / relative
        if not path.is_file() or sha256(path) != digest:
            fail('Platform file missing or changed: ' + str(path))
    for directory in ('libraries', 'scripts'):
        actual = {str(p.relative_to(gccout)).replace('\\', '/')
                  for p in (gccout / directory).rglob('*') if p.is_file()}
        expected_files = {p for p in data['files'] if p.startswith(directory + '/')}
        if actual != expected_files:
            fail('Unexpected stale platform inputs in gccout/' + directory)
    for directory in ('images', 'libraries', 'scripts'):
        if not (gccout / directory).is_dir():
            fail('Invalid gccout directory: ' + directory)
    if not (gccout / 'images/firmware_base_release_pack.7z').is_file():
        fail('Platform firmware base archive is missing')


def restore_snapshots(root, gccout, data):
    if compiled_package(data):
        snapshots = [(row['path'], row['sha256']) for row in data['generated_configuration']]
        prefix = 'generated'
    else:
        snapshots = list(data.get('sdk_snapshots', {}).items())
        prefix = 'sdk-snapshot'
    # Check every source before modifying any SDK header.
    for relative, digest in snapshots:
        source = gccout / prefix / relative
        if not file_matches(source, digest):
            fail('Generated-header snapshot integrity failure: ' + relative)
    for relative, digest in snapshots:
        source = gccout / prefix / relative
        target = Path(root) / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        if not file_matches(target, digest):
            shutil.copy2(str(source), str(target))


def ensure_a04_gccout(root, output_root, project, toolchain_root):
    data, archive = read_manifest(root, project)
    if not file_matches(archive, data.get('archive_sha256')):
        fail('A04 gccout archive missing or SHA256 mismatch: ' + str(archive))
    gccout = Path(output_root) / 'qos_build/gccout'
    if not gccout.exists():
        seven_zip = Path(toolchain_root) / '7z/7z.exe'
        if not seven_zip.is_file():
            fail('7z executable is missing: ' + str(seven_zip))
        gccout.parent.mkdir(parents=True, exist_ok=True)
        run_command([str(seven_zip), 'x', '-y', str(archive), '-o' + str(gccout.parent)],
                    cwd=root, label='extract validated A04 platform',
                    exit_code=EXIT_PREREQUISITE_ERROR)
    validate_tree(gccout, data)
    restore_snapshots(root, gccout, data)


if __name__ == '__main__':
    import argparse
    import sys
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', required=True)
    parser.add_argument('--project', required=True)
    parser.add_argument('--gccout', required=True)
    args = parser.parse_args()
    try:
        metadata, archive_path = read_manifest(args.root, args.project)
        if not file_matches(archive_path, metadata.get('archive_sha256')):
            fail('Platform archive is missing or changed: ' + str(archive_path))
        validate_tree(Path(args.gccout), metadata)
        restore_snapshots(args.root, Path(args.gccout), metadata)
    except (BuildError, ValueError, OSError) as exc:
        print('[A04 platform validation] ' + str(exc), file=sys.stderr)
        sys.exit(EXIT_PREREQUISITE_ERROR)