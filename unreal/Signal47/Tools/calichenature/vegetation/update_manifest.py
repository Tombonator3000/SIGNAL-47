"""Hash the explicit current production kit. Never starts DCC or changes proof."""
from pathlib import Path, PurePosixPath
import hashlib
import json

ROOT = Path(__file__).resolve().parent
# Current editable source, deterministic recipes, contracts and actual proof.
# Pending revisions, old canopy diagnostics, raw job evidence/logs and caches
# deliberately stay outside the production inventory.
ROOT_FILES = (
    'README.md', 'alpha_texture_verification.json', 'botanical_preview.png',
    'build_vegetation.py', 'caliche_vegetation.blend', 'caliche_vegetation.glb',
    'geometry.py', 'make_textures.py', 'masked_atlas.py',
    'masked_atlas_stats.json', 'materials.json', 'model_contract.json',
    'preview_botanical.py', 'prototype_plan.json', 'run_guarded.py',
    'source_audit.json', 'update_manifest.py', 'verification.json',
    'verify_mask_textures.py', 'verify_vegetation.py',
)
TEXTURE_FILES = tuple(
    f'textures/{stem}_{role}.png'
    for stem in ('botanical', 'bark')
    for role in ('color', 'roughness', 'normal_dx', 'normal_gl')
)


def production_paths(contract):
    paths = set(ROOT_FILES) | set(TEXTURE_FILES)
    for part in contract['parts'].values():
        for lod in part['lods']:
            for field, suffix in (('fbx', '.fbx'), ('glb', '.glb')):
                name = lod[field]
                relative = PurePosixPath(name)
                if (relative.is_absolute() or '..' in relative.parts
                        or len(relative.parts) != 2 or relative.parts[0] != 'exports'
                        or relative.suffix != suffix or relative.name != lod['name'] + suffix):
                    raise ValueError('Unsafe/noncanonical contract export: ' + name)
                paths.add(name)
    return sorted(paths)


def update(previous_manifest=None):
    # Copies can reorder mtimes. Retain a previously completed preview only
    # while both original Blend and image bytes still match its inventory.
    previous_path = Path(previous_manifest) if previous_manifest is not None else ROOT / 'asset_manifest.json'
    previous = json.loads(previous_path.read_text()) if previous_path.is_file() else {}
    contract = json.loads((ROOT / 'model_contract.json').read_text())
    report = json.loads((ROOT / 'verification.json').read_text())
    verified = (report.get('source_revision') == contract.get('source_revision')
                and report.get('status') == 'PASS'
                and report.get('passed') == report.get('total')
                and report.get('total', 0) > 0)
    preview = ROOT / 'botanical_preview.png'
    blend = ROOT / 'caliche_vegetation.blend'
    guard = ROOT / 'verification' / 'preview-guard.json'
    preview_guard = json.loads(guard.read_text()) if guard.is_file() else {}
    rendered = (preview.is_file() and blend.is_file()
                and preview.stat().st_mtime > blend.stat().st_mtime
                and preview_guard.get('returncode') == 0
                and preview_guard.get('stop_reason') is None)
    files = {}
    for name in production_paths(contract):
        path = ROOT / name
        if not path.is_file() or path.is_symlink():
            raise ValueError('Missing/nonregular production kit file: ' + name)
        data = path.read_bytes()
        files[name] = {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
    prior_files = previous.get('files', {})
    same_preview_bytes = all(
        prior_files.get(name, {}).get('sha256') == files[name]['sha256']
        for name in ('caliche_vegetation.blend', 'botanical_preview.png')
    )
    rendered = rendered or (previous.get('preview_completed_for_current_blend') is True
                             and same_preview_bytes)
    binding = {
        'blend_sha256': files['caliche_vegetation.blend']['sha256'],
        'image_sha256': files['botanical_preview.png']['sha256'],
        'status': 'COMPLETED_GUARDED_RENDER' if rendered else 'RENDER_COMPLETION_UNVERIFIED',
        'scope': 'Previously completed guarded render retained only for exact matching Blend/image bytes, or fresh successful guard plus render timestamp; no new render or visual acceptance',
    }
    out = {
        'asset': 'Original Caliche vegetation prototype kit',
        'source_revision': contract.get('source_revision'),
        'inventory_scope': 'Explicit production kit: current source, contracts, proofs, textures and contract-named exports only; no pending/history/raw job evidence/log/cache',
        'technical_status': f"{report.get('passed')}/{report.get('total')} source/FBX/GLB checks PASS" if verified else 'CURRENT_SOURCE_NOT_YET_ROUNDTRIP_VERIFIED',
        'preview_completed_for_current_blend': rendered,
        'preview_byte_binding': binding,
        'visual_status': 'UNVERIFIED pending explicit review of actual botanical image and native game frames',
        'files': files,
    }
    (ROOT / 'asset_manifest.json').write_text(json.dumps(out, indent=2))
    print('ASSET_MANIFEST', len(files), 'production files', out['technical_status'],
          'preview_complete', rendered)


if __name__ == '__main__':
    update()
