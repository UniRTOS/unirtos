"""Non-compiling tests for A04 platform identity and cache isolation."""
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'qos_tools/python'))
import unirtos_platform_artifact as artifact
import unirtos_make
import export_a04_gccout
from unirtos_common import BuildError


class PlatformTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.project = 'EG800ZEU_LA'
        self.folder = self.root / 'qos_kernel/eigen_718/images' / self.project
        self.folder.mkdir(parents=True)
        (self.root / 'SDK_ORIGIN.json').write_text(json.dumps({'sdk_version': artifact.SDK_VERSION}), encoding='utf-8')
        self.gccout = self.root / 'qos_build/gccout'
        self.data = {'schema_version': 1, 'sdk_version': artifact.SDK_VERSION,
                     'arch': 'eigen_718', 'project': self.project,
                     'artifact_status': 'USER-BUILT-PLATFORM', 'files': {}, 'sdk_snapshots': {}}
        for name in ('images/firmware_base_release_pack.7z', 'libraries/bridge.o', 'scripts/link.ld'):
            path = self.gccout / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(b'isolated non-build fixture')
            self.data['files'][name] = artifact.sha256(path)

    def stamp(self):
        metadata = dict(self.data)
        metadata.pop('archive_sha256', None)
        (self.gccout / artifact.STAMP).write_text(json.dumps(metadata), encoding='utf-8')

    def sidecar(self):
        archive = self.folder / 'gccout.7z'
        archive.write_bytes(b'not a real archive: never extracted in this test')
        self.data['archive_sha256'] = artifact.sha256(archive)
        (self.folder / 'platform-manifest.json').write_text(json.dumps(self.data), encoding='utf-8')

    def test_missing_a04_package_fails_before_build(self):
        with self.assertRaises(BuildError):
            artifact.read_manifest(self.root, self.project)

    def test_export_requires_user_build_receipt(self):
        with self.assertRaises(RuntimeError):
            export_a04_gccout.export(self.root, self.root, self.project, self.root / 'tools')

    def test_export_rejects_failed_user_build(self):
        receipt = {'sdk_version': artifact.SDK_VERSION, 'project': self.project, 'status': 'failed'}
        (self.root / 'A04_BUILD_RECEIPT.json').write_text(json.dumps(receipt), encoding='utf-8')
        with self.assertRaises(RuntimeError):
            export_a04_gccout.export(self.root, self.root, self.project, self.root / 'tools')

    def test_wrong_project_is_rejected(self):
        self.data['project'] = 'EG800ZCN_LA'
        self.sidecar()
        with self.assertRaises(BuildError):
            artifact.read_manifest(self.root, self.project)

    def test_0511_identity_is_rejected(self):
        self.data['sdk_version'] = 'QSR01A03_C_SDK_LTE_E_BETA20260511'
        self.sidecar()
        with self.assertRaises(BuildError):
            artifact.read_manifest(self.root, self.project)

    def test_legacy_unstamped_cache_is_rejected(self):
        with self.assertRaises(BuildError):
            artifact.validate_tree(self.gccout, self.data)

    def test_valid_fixture_reuses_no_compiler_or_unpacker(self):
        self.sidecar()
        self.stamp()
        with patch.object(artifact, 'run_command', side_effect=AssertionError('unexpected command')):
            artifact.ensure_a04_gccout(self.root, self.root, self.project, self.root / 'tools')

    def test_changed_object_is_rejected(self):
        self.stamp()
        (self.gccout / 'libraries/bridge.o').write_bytes(b'changed')
        with self.assertRaises(BuildError):
            artifact.validate_tree(self.gccout, self.data)

    def test_extra_stale_object_is_rejected(self):
        self.stamp()
        (self.gccout / 'libraries/old.o').write_bytes(b'stale')
        with self.assertRaises(BuildError):
            artifact.validate_tree(self.gccout, self.data)

    def test_unsafe_manifest_path_is_rejected(self):
        self.data['files']['../outside.o'] = 'bad'
        self.sidecar()
        with self.assertRaises(BuildError):
            artifact.read_manifest(self.root, self.project)

    def test_snapshot_cannot_overwrite_unapproved_sdk_source(self):
        self.data['sdk_snapshots']['qos_tools/python/unirtos_make.py'] = 'bad'
        self.sidecar()
        with self.assertRaises(BuildError):
            artifact.read_manifest(self.root, self.project)

    def test_all_build_is_rejected_before_toolchain_or_compiler(self):
        platform = self.root / 'qos_components/system/platform/platform.json'
        platform.parent.mkdir(parents=True)
        platform.write_text(json.dumps({'eigen_718': [self.project]}), encoding='utf-8')
        with patch.object(unirtos_make, 'sdk_root', return_value=self.root), \
             patch.object(unirtos_make, 'run_cmake_and_ninja', side_effect=AssertionError('compiler called')):
            with self.assertRaises(BuildError):
                unirtos_make.handle_make([self.project, 'A04_TEST', 'all', 'new'])


if __name__ == '__main__':
    unittest.main(verbosity=2)