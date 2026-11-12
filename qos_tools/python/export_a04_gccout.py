"""Package an explicitly user-built private producer. Does not compile anything."""
import argparse
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

from unirtos_platform_artifact import (COMMON, SDK_VERSION, STAMP, sha256,
                                       snapshot_allowed, validate_tree)


def run(command, cwd=None):
    result = subprocess.run([str(x) for x in command], cwd=str(cwd) if cwd else None,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode:
        raise RuntimeError(result.stderr.decode(errors='replace'))
    return result.stdout.decode('utf-8', errors='replace')


def export(producer, repo, project, toolchain):
    producer, repo, toolchain = Path(producer).resolve(), Path(repo).resolve(), Path(toolchain).resolve()
    if not re.fullmatch(r'[A-Z0-9_]+', project):
        raise RuntimeError('Invalid project name')
    if not (producer / 'A04_BUILD_RECEIPT.json').is_file():
        raise RuntimeError('No user build receipt. Run the separate Build-Producer.ps1 first.')
    origin = json.loads((producer / 'SDK_ORIGIN.json').read_text(encoding='utf-8-sig'))
    receipt = json.loads((producer / 'A04_BUILD_RECEIPT.json').read_text(encoding='utf-8-sig'))
    if (origin.get('sdk_version') != SDK_VERSION or receipt.get('sdk_version') != SDK_VERSION
            or receipt.get('project') != project or receipt.get('status') != 'success'):
        raise RuntimeError('A successful matching Build-Producer.ps1 receipt is required')
    gccout = producer / 'qos_build/gccout'
    base = gccout / 'images/firmware_base_release_pack.7z'
    if not base.is_file() or sha256(base) != receipt.get('firmware_sha256'):
        raise RuntimeError('Producer firmware does not match the successful build receipt')
    nm = toolchain / 'gcc-arm-none-eabi-10.2.1/bin/arm-none-eabi-nm.exe'
    seven_zip = toolchain / '7z/7z.exe'
    if not nm.is_file() or not seven_zip.is_file():
        raise RuntimeError('ARM nm and 7z are required for artifact checks')
    object_proof = {}
    for name in ('bsp_usart', 'ql_update_check'):
        matches = list(producer.rglob(name + '.o'))
        if not matches:
            raise RuntimeError('Closed-source implementation object missing: ' + name)
        definitions = [(p, run([nm, '--defined-only', '--extern-only', p])) for p in matches]
        definitions = [(p, text) for p, text in definitions if any(
            ' T ' in line or ' D ' in line for line in text.splitlines())]
        if not definitions:
            raise RuntimeError('No definitions in the closed-source object: ' + name)
        object_proof[name] = [{'path': str(p.relative_to(producer)).replace('\\', '/'),
                               'sha256': sha256(p)} for p, _ in definitions]
    maps = [p for p in producer.rglob('*.map') if 'ap_application' not in p.name]
    linked = []
    for path in maps:
        text = path.read_text(encoding='utf-8', errors='replace')
        if 'bsp_usart.o' in text and 'ql_update_check.o' in text:
            linked.append({'path': str(path.relative_to(producer)).replace('\\', '/'),
                           'sha256': sha256(path)})
    if not linked:
        raise RuntimeError('No platform MAP references both closed-source objects; inspect the producer build')
    # A MAP can list discarded sections too. Keep this proof explicitly below runtime acceptance.
    with tempfile.TemporaryDirectory(prefix='a04-export-') as temporary:
        staging = Path(temporary)
        payload = staging / 'gccout'
        for directory in ('images', 'libraries', 'scripts'):
            source = gccout / directory
            if not source.is_dir():
                raise RuntimeError('Missing producer directory: ' + str(source))
            shutil.copytree(str(source), str(payload / directory))
        for path in payload.rglob('*'):
            if path.is_file() and path.suffix.lower() in ('.c', '.cpp', '.cc', '.s'):
                raise RuntimeError('Unexpected source inside the platform payload: ' + str(path))
        snapshots = {}
        for path in (producer / COMMON).rglob('*.h'):
            relative = str(path.relative_to(producer)).replace('\\', '/')
            if snapshot_allowed(relative):
                target = payload / 'sdk-snapshot' / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(str(path), str(target))
                snapshots[relative] = sha256(target)
        if COMMON + 'ver_tag/qosa_version.h' not in snapshots:
            raise RuntimeError('Missing A04 generated version header snapshot')
        data = {'schema_version': 1, 'sdk_version': SDK_VERSION, 'arch': 'eigen_718',
                'project': project, 'artifact_status': 'USER-BUILT-PLATFORM',
                'build_version': receipt['version'], 'build_started_utc': receipt['started_utc'],
                'files': {str(p.relative_to(payload)).replace('\\', '/'): sha256(p)
                          for p in sorted(payload.rglob('*')) if p.is_file()},
                'sdk_snapshots': snapshots, 'closed_source_objects': object_proof,
                'platform_maps': linked, 'device_validation': 'NOT-RUN',
                'app_only_validation': 'PENDING'}
        (payload / STAMP).write_text(json.dumps(data, indent=2) + '\n', encoding='utf-8')
        validate_tree(payload, data)
        run([seven_zip, 't', base])
        listing = run([seven_zip, 'l', '-slt', '-ba', base])
        for line in listing.splitlines():
            if line.startswith('Path = ') and Path(line[7:]).suffix.lower() in ('.c', '.cpp', '.cc', '.s'):
                raise RuntimeError('Source file inside nested firmware archive: ' + line[7:])
        packed = staging / 'gccout.7z'
        run([seven_zip, 'a', '-t7z', '-mx=5', packed, 'gccout'], cwd=staging)
        run([seven_zip, 't', packed])
        data['archive_sha256'] = sha256(packed)
        destination = repo / 'qos_kernel/eigen_718/images' / project
        if (destination / 'gccout.7z').exists() or (destination / 'platform-manifest.json').exists():
            raise RuntimeError('Artifact already exists; preserve/review the previous package before replacing it')
        destination.mkdir(parents=True, exist_ok=True)
        shutil.copy2(str(packed), str(destination / 'gccout.7z'))
        (destination / 'platform-manifest.json').write_text(json.dumps(data, indent=2) + '\n', encoding='utf-8')
        print('EXPORTED_USER_BUILT_PLATFORM=' + str(destination))
        print('APPLICATION_AND_DEVICE_VALIDATION=STILL-PENDING')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--producer', required=True)
    parser.add_argument('--project', required=True)
    parser.add_argument('--repo', default=str(Path(__file__).resolve().parents[2]))
    parser.add_argument('--toolchain', default=r'D:\unirtos-toolchain')
    options = parser.parse_args()
    try:
        export(options.producer, options.repo, options.project, options.toolchain)
    except (RuntimeError, ValueError, OSError) as exc:
        parser.exit(1, 'A04 export refused: {}\n'.format(exc))