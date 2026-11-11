"""D45-20261110: check Tom's Cosmos VFX Smoke pack after Tools/cosmossmoke/import.sh has copied it to /Game/Rocket.

Runs through import.sh: bash unreal/Signal47/Tools/cosmossmoke/import.sh
Loads every asset of the pack, saves the chimney smoke system once in 5.8 (Niagara finishes compiling before it saves), and fails if
anything it needs lies outside /Game/Rocket and the engine. The pack keeps its own package paths, as the Fab packs do, so nothing is
renamed and no reference is rewritten. A few plain facts about the system go to the report (bounds, warmup), for the C++ size
(S47Hap::CosmosSmokeScale). No image is opened, exported or read here: the Cosmos licence allows no use as reference for AI tools.
"""
import json
import os
import traceback
from pathlib import Path

import unreal

E = unreal.EditorAssetLibrary
REPORT = Path(os.environ.get('S47_COSMOS_REPORT', '/tmp/s47-cosmos-smoke.json'))
ROOT = '/Game/Rocket'
SYSTEM = ROOT + '/NS1_Smoke/NS_NS1_Smoke_Chimney'
report = {'engine': unreal.SystemLibrary.get_engine_version(), 'checks': [], 'steps': [], 'facts': {}, 'status': 'FAIL'}


def save():
    REPORT.write_text(json.dumps(report, indent=1, default=str) + '\n')


def check(name, condition, detail=None):
    report['checks'].append({'name': name, 'pass': bool(condition), 'detail': detail})
    save()
    if not condition:
        raise AssertionError((name, detail))


def step(text, detail=None):
    report['steps'].append({'step': text, 'detail': detail})
    unreal.log('S47 COSMOS SMOKE: ' + text)
    save()


def dependencies(registry, package, options):
    got = registry.get_dependencies(package, options)
    if isinstance(got, tuple):
        got = got[-1]
    return [str(d) for d in (got or [])]


try:
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.scan_paths_synchronous([ROOT], True)
    assets = sorted({a.split('.')[0] for a in E.list_assets(ROOT, recursive=True)})
    check('the pack is in /Game/Rocket', len(assets) >= 8, assets)
    for path in assets:
        check('loads ' + path, E.load_asset(path) is not None)
    system = E.load_asset(SYSTEM)
    check('the chimney smoke is a Niagara system', isinstance(system, unreal.NiagaraSystem), str(type(system)))
    options = unreal.AssetRegistryDependencyOptions()
    for flag, value in (('include_soft_package_references', True), ('include_hard_package_references', True),
                        ('include_searchable_names', False), ('include_soft_management_references', False),
                        ('include_hard_management_references', False)):
        options.set_editor_property(flag, value)
    outside, seen, todo = set(), set(), [SYSTEM]
    while todo:
        package = todo.pop()
        if package in seen:
            continue
        seen.add(package)
        for dep in dependencies(registry, package, options):
            if dep.startswith(ROOT + '/'):
                todo.append(dep)
            elif dep.startswith('/Game/'):
                outside.add(dep)
    step('the chimney smoke and what it needs', sorted(seen))
    check('it needs nothing outside /Game/Rocket and the engine', not outside, sorted(outside))
    check('saved once in 5.8 (compiled)', E.save_loaded_asset(system, False))
    for name in ('fixed_bounds', 'warmup_time', 'b_fixed_bounds'):
        try:
            report['facts'][name] = str(system.get_editor_property(name))
        except Exception as error:  # a fact the Python API does not expose is not a failure
            report['facts'][name] = 'n/a: ' + str(error)
    report['status'] = 'PASS'
    step('PASS')
except Exception as error:
    report['error'] = repr(error)
    report['trace'] = traceback.format_exc()
    unreal.log_error('S47 COSMOS SMOKE FAIL ' + repr(error))
finally:
    save()
