#!/usr/bin/env python3
"""Bring the waitress MetaHuman into ~/SIGNAL-47 (dry run unless --apply).

Additive edits only, each skipped when already present, so Codex's uncommitted work in the same
files is preserved. Licensed content is copied into the ignored Content/MetaHumans folder only.
"""
import argparse
import json
import shutil
import subprocess
from pathlib import Path

B = Path.home() / 'scratch-bm/Signal47-metahuman-20261026'
REPO = Path.home() / 'SIGNAL-47'
U = REPO / 'unreal'
P = U / 'Signal47'
SRC = P / 'Source/Signal47'
PLUGINS = ['RigLogic', 'HairStrands', 'ControlRig', 'MetaHumanSDK']  # plus any found by --plugins-from

EDITS = [
    (SRC / 'Private/S47MesaDiner.cpp', '#include "S47Main.h"',
     '#include "S47Main.h"\n#include "S47Person.h"', 'S47Person.h'),
    (SRC / 'Private/S47MesaDiner.cpp', ' bMesaDinerReady=true;\n',
     ' bMesaDinerReady=true;\n S47Waitress::Attach(this); // MetaHuman waitress when the local assets exist\n', 'S47Waitress::Attach'),
    (SRC / 'Public/S47Evening.h', ' FS47EveningPack Pack;\n',
     ' FS47EveningPack Pack;\n // The diner waitress speaking now: take id and seconds into it on the audio clock.\n'
     ' bool PhysicalSpeech(FString& Clip, double& Seconds) const;\n', 'PhysicalSpeech'),
    (U / '.gitignore', 'Signal47/Content/Audio/\n',
     'Signal47/Content/Audio/\n# MetaHuman assets are licensed: rebuilt locally by Tools/metahuman, never committed.\n'
     'Signal47/Content/MetaHumans/\n', 'Content/MetaHumans'),
    (SRC / 'Signal47.Build.cs', '"MoviePlayer" });', '"MoviePlayer", "HairStrandsCore" });', 'HairStrandsCore'),
]
# Only what the game spawns: the assembled Blueprint, its Live Link graph and the Creator character
# are editor tooling that does not cook, so they stay in staging.
COOK_DIRS = ['/Game/MetaHumans/MHC_Waitress/Body', '/Game/MetaHumans/MHC_Waitress/Face', '/Game/MetaHumans/MHC_Waitress/Grooms',
             '/Game/MetaHumans/Waitress/Anims', '/Game/MetaHumans/Waitress/FaceAnims', '/Game/MetaHumans/Waitress/Uniform']
CONTENT_EXCLUDE = ['MHC_Waitress/BP_MHC_Waitress.uasset', 'Waitress/MHC_Waitress.uasset', 'Common/Animation/ABP_MH_LiveLink.uasset']
EVENING_IMPL = '''
bool FS47Evening::PhysicalSpeech(FString& Clip, double& Seconds) const
{
 const FS47EveningScene* S = bEnabled ? Pack.CurrentScene() : nullptr;
 if (!S || S->Channel != TEXT("physical") || !Voice || !Voice->Playing() || !M || !M->Audio || !M->Audio->Ctx) return false;
 const FS47DialogueStatus Status = Voice->Status();
 if (Status.Clips.IsEmpty()) return false;
 Clip = Status.Clips[0].Id;
 Seconds = M->Audio->Ctx->CurrentTime() - Voice->StartedAt();
 return true;
}
'''


def edit(apply, report):
    for path, anchor, replacement, marker in EDITS:
        text = path.read_text()
        if marker in text:
            report.append(f'present  {path.relative_to(REPO)}: {marker}')
            continue
        if anchor not in text:
            raise SystemExit(f'anchor missing in {path}: {anchor!r}')
        report.append(f'edit     {path.relative_to(REPO)}: {marker}')
        if apply:
            path.write_text(text.replace(anchor, replacement, 1))
    cpp = SRC / 'Private/S47Evening.cpp'
    if 'FS47Evening::PhysicalSpeech' in cpp.read_text():
        report.append('present  S47Evening.cpp: PhysicalSpeech')
    else:
        report.append('append   S47Evening.cpp: PhysicalSpeech')
        if apply:
            cpp.write_text(cpp.read_text().rstrip('\n') + '\n' + EVENING_IMPL)
    ini = P / 'Config/DefaultGame.ini'
    text = ini.read_text()
    head = '[/Script/UnrealEd.ProjectPackagingSettings]'
    if head not in text:
        raise SystemExit('no ProjectPackagingSettings section in DefaultGame.ini')
    broad = '+DirectoriesToAlwaysCook=(Path="/Game/MetaHumans")\n'
    if broad in text:
        report.append('remove   DefaultGame.ini: cook all of /Game/MetaHumans')
        text = text.replace(broad, '')
    for d in COOK_DIRS:
        line = f'+DirectoriesToAlwaysCook=(Path="{d}")'
        if line in text:
            report.append(f'present  DefaultGame.ini: {d}')
        else:
            report.append(f'edit     DefaultGame.ini: cook {d}')
            text = text.replace(head, head + '\n' + line, 1)
    if apply and text != ini.read_text():
        ini.write_text(text)


def plugins(apply, report, extra):
    path = P / 'Signal47.uproject'
    data = json.loads(path.read_text())
    have = {p['Name'] for p in data.get('Plugins', [])}
    for name in PLUGINS + extra:
        if name in have:
            report.append(f'present  uproject plugin {name}')
            continue
        report.append(f'add      uproject plugin {name}')
        data.setdefault('Plugins', []).append({'Name': name, 'Enabled': True})
    if apply and len(data.get('Plugins', [])) > len(have):
        path.write_text(json.dumps(data, indent='\t') + '\n')


def files(apply, report):
    pairs = [(B / 'runtime/Public/S47Person.h', SRC / 'Public/S47Person.h'),
             (B / 'runtime/Private/S47Person.cpp', SRC / 'Private/S47Person.cpp'),
             (B / 'runtime/Private/S47Waitress.cpp', SRC / 'Private/S47Waitress.cpp')]
    tools = P / 'Tools/metahuman'
    for f in ['MHStaging/Content/Python/s47mh.py', 'tools/mh_remote.py', 'tools/phase.py', 'tools/run_phase.sh',
              'tools/session_a.sh', 'tools/wait_then.sh', 'tools/integrate.py', 'blender/uniform.py']:
        pairs.append((B / f, tools / Path(f).name))
    pairs.append((B / 'METAHUMAN_WAITRESS.md', U / 'METAHUMAN_WAITRESS.md'))
    for a, b in pairs:
        same = b.exists() and b.read_bytes() == a.read_bytes()
        report.append(f"{'same    ' if same else 'copy    '} {b.relative_to(REPO)}")
        if apply and not same:
            b.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(a, b)


def content(apply, report):
    src, dst = B / 'MHStaging/Content/MetaHumans', P / 'Content/MetaHumans'
    if not src.exists():
        report.append('missing  staged Content/MetaHumans (build first)')
        return
    n = sum(1 for _ in src.rglob('*.uasset'))
    report.append(f'sync     Content/MetaHumans ({n} uasset, less {len(CONTENT_EXCLUDE)} editor-only) into the ignored game folder')
    if apply:
        subprocess.run(['rsync', '-a', '--delete', '--delete-excluded', *[f'--exclude=/{x}' for x in CONTENT_EXCLUDE], f'{src}/', f'{dst}/'], check=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--apply', action='store_true')
    ap.add_argument('--plugins-from', nargs='*', default=[], help='extra runtime plugins the assembled assets need')
    ap.add_argument('--no-content', action='store_true')
    a = ap.parse_args()
    if a.apply:
        raise SystemExit('Legacy bootstrap retired after Codex takeover: it would overwrite the repaired runtime, uniform and delivery report with old staging copies. Use the checked-in Source/Signal47 files and prepare_runtime.py; see unreal/METAHUMAN_WAITRESS.md.')
    report = []
    edit(a.apply, report)
    plugins(a.apply, report, a.plugins_from)
    files(a.apply, report)
    if not a.no_content:
        content(a.apply, report)
    print(('APPLIED' if a.apply else 'DRY RUN') + '\n' + '\n'.join(report))


if __name__ == '__main__':
    main()
