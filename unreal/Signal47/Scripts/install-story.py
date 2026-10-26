#!/usr/bin/env python3
"""Valider og installer en hashbundet, flettet SC-leveranse. Standard er tørrkjøring.

Ingen bygg, oppstart, prosessavslutning eller Git-skriving utføres. Bruk
--template SC01 for bevisskjemaet og --self-test for isolerte filsystemprøver.
Saved er alltid utelatt ved installasjon og rollback. En egen sikkerhetskopi
av Saved beholdes, men skriptet skriver aldri denne tilbake til spillet.
"""
from __future__ import annotations

import argparse
from contextlib import contextmanager
from datetime import datetime, timezone
import fcntl
import hashlib
import types
import json
import os
from pathlib import Path, PurePosixPath
import re
import stat
import subprocess
import sys
import tempfile
import uuid

sys.dont_write_bytecode = True
REPOSITORY = 'Tombonator3000/SIGNAL-47'
BASE = 'unreal-port-core'
BINARY = 'Signal47/Binaries/Linux/Signal47'
TARGET = Path.home() / 'Games/Signal47/Linux'
SCRATCH = Path.home() / 'scratch-bm'
REPO = Path(__file__).resolve().parents[3]
REQUIREMENTS = {'SC01': {'selftest', 'scenario_high', 'scenario_low', 'legacy_routes', 'continue', 'visuals', 'media'}}
ERRORS = re.compile(r'Critical error:|Fatal error:|Assertion failed:|VK_ERROR_DEVICE_LOST|Signal \d+ caught|Ensure condition failed|Horror contract FAIL|T02 contract FAIL|T02 native route FAIL|T02 unresolved contract FAIL|T01 contract FAIL|T01 native route FAIL|T01 drive FAIL|Dale documents FAIL|line contract FAIL|sky contract FAIL|takeaway FAIL|self test FAILED|smoke test FAILED')


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def digest(value):
    require(isinstance(value, str) and re.fullmatch(r'[a-f0-9]{64}', value), 'Ugyldig SHA256')
    return value


def revision(value):
    require(isinstance(value, str) and re.fullmatch(r'[a-f0-9]{40}', value), 'Ugyldig Git-revisjon')
    return value


def relative(value):
    require(isinstance(value, str) and value, 'Relativ filsti mangler')
    path = PurePosixPath(value)
    require(not path.is_absolute() and '..' not in path.parts and str(path) == value and value != '.', 'Utrygg relativ filsti: ' + repr(value))
    return value


def checked_path(value):
    """Reject every symlink component before returning an absolute, uncollapsed path."""
    path = Path(value).expanduser()
    require(path.is_absolute() and '..' not in path.parts, 'Absolutt filsti uten .. kreves')
    current = Path(path.anchor)
    for name in path.parts[1:]:
        current /= name
        require(not stat.S_ISLNK(current.lstat().st_mode), 'Symbolsk lenke avvist: ' + str(current))
    return path


def identity(info):
    return (info.st_dev, info.st_ino, info.st_mode, info.st_size, info.st_mtime_ns, info.st_ctime_ns)


@contextmanager
def root_fd(root):
    path = checked_path(root)
    fd = os.open(path, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW)
    try:
        checked_path(path)
        require(identity(os.fstat(fd)) == identity(path.lstat()), 'Mappe ble byttet under åpning')
        yield fd
    finally:
        os.close(fd)


def file_bytes(name, parent=None):
    flags = os.O_RDONLY | os.O_NOFOLLOW
    fd = os.open(name, flags, dir_fd=parent)
    try:
        before = os.fstat(fd)
        require(stat.S_ISREG(before.st_mode), 'Kun vanlige filer kan leses')
        chunks = []
        while True:
            chunk = os.read(fd, 1024 * 1024)
            if not chunk:
                break
            chunks.append(chunk)
        require(identity(before) == identity(os.fstat(fd)), 'Fil endret under lesing')
        return b''.join(chunks)
    finally:
        os.close(fd)


def sha_file(name, parent=None):
    fd = os.open(name, os.O_RDONLY | os.O_NOFOLLOW, dir_fd=parent)
    try:
        before = os.fstat(fd)
        require(stat.S_ISREG(before.st_mode), 'Kun vanlige filer kan hashes')
        result = hashlib.sha256()
        while True:
            chunk = os.read(fd, 4 * 1024 * 1024)
            if not chunk:
                break
            result.update(chunk)
        require(identity(before) == identity(os.fstat(fd)), 'Fil endret under hashing')
        return result.hexdigest(), before
    finally:
        os.close(fd)


def sha(path):
    path = checked_path(path)
    with root_fd(path.parent) as fd:
        result, before = sha_file(path.name, fd)
        require(identity(before) == identity(os.stat(path.name, dir_fd=fd, follow_symlinks=False)), 'Hashfil ble byttet')
        return result


def snapshot(root, saved=False):
    """Hash regular files/modes and all directories through no-follow directory fds."""
    result = {}
    with root_fd(root) as anchor:
        if not saved:
            result['.'] = {'directory': True, 'mode': stat.S_IMODE(os.fstat(anchor).st_mode)}

        def walk(parent, prefix=(), under_saved=False):
            before = os.fstat(parent)
            for name in sorted(os.listdir(parent)):
                rel = prefix + (name,)
                info = os.stat(name, dir_fd=parent, follow_symlinks=False)
                require(not stat.S_ISLNK(info.st_mode), 'Symbolsk lenke i leveransen/lagringene: ' + '/'.join(rel))
                is_saved = under_saved or name == 'Saved'
                key = '/'.join(rel)
                if stat.S_ISDIR(info.st_mode):
                    if is_saved == saved:
                        result[key] = {'directory': True, 'mode': stat.S_IMODE(info.st_mode)}
                        if saved:
                            result[key]['mtime_ns'] = info.st_mtime_ns
                    if is_saved and not saved:
                        continue
                    child = os.open(name, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW, dir_fd=parent)
                    try:
                        require(identity(info) == identity(os.fstat(child)), 'Mappe byttet før gjennomgang: ' + key)
                        walk(child, rel, is_saved)
                    finally:
                        os.close(child)
                else:
                    require(stat.S_ISREG(info.st_mode), 'Spesialfil avvist: ' + key)
                    if is_saved == saved:
                        value, opened = sha_file(name, parent)
                        require(identity(info) == identity(opened), 'Fil byttet før hashing: ' + key)
                        result[key] = {'sha256': value, 'bytes': info.st_size, 'mode': stat.S_IMODE(info.st_mode)}
                        if saved:
                            result[key]['mtime_ns'] = info.st_mtime_ns
                require(identity(info) == identity(os.stat(name, dir_fd=parent, follow_symlinks=False)), 'Fil/mappe endret under gjennomgang: ' + key)
            require(identity(before) == identity(os.fstat(parent)), 'Mappe endret under gjennomgang')
        walk(anchor)
    return dict(sorted(result.items()))


def write_json(path, data):
    temporary = path.with_name(path.name + '.tmp-' + uuid.uuid4().hex)
    fd = os.open(temporary, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 0o600)
    try:
        with os.fdopen(fd, 'w') as stream:
            json.dump(data, stream, indent=2, ensure_ascii=False)
            stream.write('\n')
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)


def git(repo, *args):
    return subprocess.check_output(['git', *args], cwd=repo, text=True).strip()


def documentation_only(path):
    # JSON, CSV, generated .inl and every other non-Markdown file remain runtime inputs.
    return path.lower().endswith('.md')


def clean_runtime(repo):
    tracked = subprocess.check_output(['git', 'diff', '--name-only', '--no-renames', '-z', 'HEAD'], cwd=repo)
    loose = subprocess.check_output(['git', 'ls-files', '--others', '--exclude-standard', '-z'], cwd=repo)
    paths = sorted({value.decode() for value in (tracked + loose).split(b'\0') if value})
    require(all(documentation_only(path) for path in paths), 'Ulagrede runtimefiler: ' + repr(paths))
    return paths


def ref(proofs, row, base, candidate, target):
    require(isinstance(row, dict) and set(row) == {'path', 'sha256'}, 'Bevis krever {path, sha256}')
    expected = digest(row['sha256'])
    path = Path(row['path'])
    path = checked_path(path if path.is_absolute() else base / path)
    require(not path.is_relative_to(candidate) and not path.is_relative_to(target), 'Bevis må ligge utenfor kilde og installasjon')
    with root_fd(path.parent) as parent:
        raw = file_bytes(path.name, parent)
    require(hashlib.sha256(raw).hexdigest() == expected, 'Bevishash stemmer ikke: ' + str(path))
    proofs[str(path)] = expected
    return path, raw


def bound_json(proofs, row, base, candidate, target):
    path, raw = ref(proofs, row, base, candidate, target)
    return path, json.loads(raw)


def timestamp(value):
    require(isinstance(value,str), 'Tidsstempel må være tekst')
    parsed=datetime.fromisoformat(value.replace('Z','+00:00'))
    require(parsed.tzinfo is not None and parsed.utcoffset() is not None, 'Tidsstempel må ha tidssone')
    return parsed.astimezone(timezone.utc)


def input_identity(files):
    raw=json.dumps(files,sort_keys=True,separators=(',',':'),ensure_ascii=True).encode('ascii')
    return hashlib.sha256(raw).hexdigest()


def content_inventory(repo):
    # Content is a packaging boundary: include ignored/licensed/raw data and even
    # Markdown files there. Hash working LFS bytes, never Git pointer blobs.
    root=repo/'unreal/Signal47/Content'
    nodes=snapshot(root)
    nodes.update(snapshot(root,saved=True))
    return {'unreal/Signal47/Content/'+name:row['sha256']
            for name,row in nodes.items() if 'sha256' in row}


def verify_frozen_inventory(repo, frozen, tested, tracked):
    require(frozen.get('schema')=='s47-story-inputs-v1' and frozen.get('tested_commit')==tested,'Feil kildefrysing')
    timestamp(frozen.get('frozen_at'))
    files=frozen.get('files')
    require(isinstance(files,dict) and files,'Tom kildefrysing')
    content=content_inventory(repo)
    required={p for p in tracked if not documentation_only(p)} | set(content)
    require(required <= files.keys(),'Kildefrysing utelater runtime-/pakkefiler: '+repr(sorted(required-files.keys())[:10]))
    require(any(p.startswith('unreal/Signal47/Source/') for p in files) and content and
        'unreal/Narrative/HorrorStory/generated/HorrorPackData.inl' in files,'Kildefrysing mangler kode, assets eller HorrorStory-runtime')
    for name,expected in files.items():
        relative(name);digest(expected)
        if name.startswith('unreal/Signal47/Content/'):
            require(name in content and content[name]==expected,'Fryst Content-input endret eller fjernet: '+name)
        elif not documentation_only(name):
            require(sha(repo/name)==expected,'Fryst runtimeinput endret: '+name)
    return files


def verify_build_receipt(proofs, proof, frozen, base, candidate, target, repo):
    require('package_build' in proof,'Hashbundet pakkebyggkvittering mangler')
    receipt_path,build=bound_json(proofs,proof['package_build'],base,candidate,target)
    scenario=proof['scenario'];tested=proof['tested_commit'];frozen_sha=proof['frozen_inputs']['sha256']
    require(build.get('schema')=='s47-story-package-build-v1' and build.get('status')=='PASS' and
        build.get('scenario')==scenario and build.get('tested_commit')==tested and build.get('candidate')==str(candidate) and
        build.get('frozen_inputs_sha256')==frozen_sha and build.get('payload_snapshot_sha256')==proof['payload_snapshot']['sha256'] and
        build.get('binary_sha256')==proof['binary_sha256'],'Pakkebyggkvitteringen gjelder en annen frysing, kilde eller pakke')
    require(type(build.get('returncode')) is int and build['returncode']==0,'Pakkebygg fullførte ikke med kode null')
    start,finish=timestamp(build.get('started_at')),timestamp(build.get('finished_at'))
    require(start<finish,'Ugyldig faktisk byggevindu')
    command=build.get('command')
    require(isinstance(command,list) and command and all(isinstance(v,str) and v and '\n' not in v for v in command),'Faktisk pakkekommando mangler')
    require(Path(command[0]).name=='RunUAT.sh' and Path(command[0]).is_absolute() and 'BuildCookRun' in command,'Kvitteringen krever faktisk RunUAT BuildCookRun-kommando')
    lowered={value.lower() for value in command}
    require({'-build','-cook','-stage','-pak','-archive'} <= lowered,'Pakkekommando mangler nødvendige bygg-/cook-/arkivtrinn')
    options={part.split('=',1)[0].lower():part.split('=',1)[1] for part in command if part.startswith('-') and '=' in part}
    require(options.get('-project')==str(repo/'unreal/Signal47/Signal47.uproject') and
        options.get('-archivedirectory')==str(candidate.parent) and options.get('-platform','').lower()=='linux','Pakkekommando peker til feil prosjekt, plattform eller kandidat')
    checks={}
    for phase,key in [('before','pre_inputs'),('after','post_inputs')]:
        _,check=bound_json(proofs,build[key],receipt_path.parent,candidate,target)
        require(check.get('schema')=='s47-story-package-input-check-v1' and check.get('status')=='PASS' and
            check.get('phase')==phase and check.get('tested_commit')==tested and
            check.get('frozen_inputs_sha256')==frozen_sha and check.get('files')==frozen['files'],
            'Faktisk '+phase+'-kontroll stemmer ikke med full fryst inputidentitet')
        checks[phase]=check
    require(timestamp(frozen['frozen_at']) <= timestamp(checks['before'].get('checked_at')) <= start < finish <= timestamp(checks['after'].get('checked_at')),
        'Frysing og faktiske før-/etterkontroller omslutter ikke pakkebyggets veggklokketid')
    _,raw=ref(proofs,build['log'],receipt_path.parent,candidate,target)
    log=raw.decode('utf-16') if raw[:2] in (b'\xff\xfe',b'\xfe\xff') else raw.decode('utf-8-sig',errors='replace')
    identity_sha=input_identity(frozen['files'])
    markers=[
        'S47_PACKAGE_INPUTS_BEFORE '+frozen_sha+' '+identity_sha+' '+checks['before']['checked_at'],
        'S47_PACKAGE_BUILD_STARTED '+build['started_at'],
        'BUILD SUCCESSFUL',
        'S47_PACKAGE_BUILD_FINISHED '+build['finished_at']+' 0',
        'S47_PACKAGE_INPUTS_AFTER '+frozen_sha+' '+identity_sha+' '+checks['after']['checked_at'],
        'S47_PACKAGE_OUTPUT '+str(candidate)+' '+proof['payload_snapshot']['sha256']+' '+proof['binary_sha256']]
    positions=[log.find(marker) for marker in markers]
    command_at=log.find('S47_PACKAGE_COMMAND '+json.dumps(command,separators=(',',':'),ensure_ascii=True))
    require(all(position>=0 for position in positions) and positions==sorted(positions) and 0<=command_at<positions[1],
        'Pakkeloggen mangler faktisk kommando eller ordnede før-/etter-/outputmarkører')
    require(not re.search(r'Fatal error:|Critical error:|Failed to compile Material|Default Material will be used in game|AutomationTool exiting with ExitCode=[1-9]',log),
        'Pakkeloggen inneholder en bygg-/cookfeil')
    return {'receipt':str(receipt_path),'started_at':build['started_at'],'finished_at':build['finished_at'],
            'frozen_input_identity_sha256':identity_sha,'input_files':len(frozen['files'])}

def repository_proof(repo, proof, frozen):
    tested = revision(proof['tested_commit'])
    merged = revision(proof['merged_core'])
    pr = proof['pr']
    require(type(pr.get('number')) is int and pr['number'] > 0 and pr.get('base') == BASE, 'Uventet scenario-PR')
    head, merge = revision(pr['head_sha']), revision(pr['merge_commit_sha'])
    require(isinstance(pr.get('head'), str) and pr['head'].startswith('codex/'), 'PR-gren mangler')
    actual = json.loads(subprocess.check_output(['gh', 'pr', 'view', str(pr['number']), '--repo', REPOSITORY,
        '--json', 'number,url,state,baseRefName,headRefName,headRefOid,mergeCommit'], text=True))
    require(actual.get('state') == 'MERGED' and actual.get('number') == pr['number'] and actual.get('url') == pr.get('url') and
        actual.get('baseRefName') == BASE and actual.get('headRefName') == pr['head'] and actual.get('headRefOid') == head and
        actual.get('mergeCommit', {}).get('oid') == merge, 'Faktisk GitHub-fletting stemmer ikke med beviset')
    require(git(repo, 'rev-parse', 'origin/' + BASE) == merged, 'Lokal origin/core stemmer ikke med levert fletting; hent status separat')
    descendants = {}
    for label, commit in [('pr_head', head), ('merged_core', merged), ('checkout', revision(git(repo, 'rev-parse', 'HEAD')))]:
        require(subprocess.run(['git', 'merge-base', '--is-ancestor', tested, commit], cwd=repo,
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0, 'Testet kilde er ikke stamfar: ' + label)
        paths = git(repo, 'diff', '--name-only', '--no-renames', tested, commit).splitlines()
        require(all(documentation_only(path) for path in paths), 'Runtime endret etter prøve: ' + label + repr(paths))
        descendants[label] = paths
    require(subprocess.run(['git', 'merge-base', '--is-ancestor', merge, merged], cwd=repo,
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0, 'Scenariofletting finnes ikke i levert core')
    tracked = set(git(repo, 'ls-tree', '-r', '--name-only', tested, '--', 'unreal/').splitlines())
    files=verify_frozen_inventory(repo,frozen,tested,tracked)
    return {'actual_pr': actual, 'descendants': descendants, 'allowed_dirty_markdown': clean_runtime(repo), 'runtime_inputs': len(files),
        'idle_guard_sha256': digest(files['unreal/Signal47/Scripts/unreal_guard.py'])}


def runtime_requirements(requirements, runtime):
    """Bind each required journey to its actual arguments and completion log.

    Self-test may share a complete SC01 run. High, Low, both arrival routes and
    ordinary Continue must be separate executions with separate reports/logs.
    """
    def option(row, name):
        values = [part.split('=', 1)[1].lower() for part in row['command'][1:]
                  if part.lower().startswith(name.lower() + '=')]
        require(len(values) <= 1, 'Dobbelt kommandovalg i aksept: ' + name)
        return values[0] if values else None

    def flag(row, name):
        return name.lower() in {part.lower() for part in row['command'][1:]}

    def choose(label, predicate, message):
        matches = [cid for cid in requirements[label] if cid in runtime and predicate(runtime[cid])]
        require(matches, message)
        return matches

    choose('selftest', lambda row: flag(row, '-S47SelfTest') and 'self test OK' in row['text'],
           'Selvtestkravet mangler faktisk -S47SelfTest og sluttmarkør')
    journeys = {}
    for quality in ('high', 'low'):
        journeys[quality] = choose('scenario_' + quality, lambda row, q=quality:
            option(row, '-S47HorrorContract') == 'sc01' and option(row, '-S47Quality') == q and
            'Horror SC01 PASS COMPLETE' in row['text'],
            'SC01 ' + quality + ' mangler faktisk kontrakt, kvalitetsvalg eller sluttmarkør')
    for route in ('direct', 'burger'):
        journeys[route] = choose('legacy_routes', lambda row, r=route:
            flag(row, '-S47HorrorArrival') and option(row, '-S47EveningRoute') == r and
            'horror arrival ' + r + ' PASS COMPLETE' in row['text'],
            'SC01 mangler fullført faktisk ankomstrute: ' + route)
    journeys['continue'] = choose('continue', lambda row:
        flag(row, '-S47Title') and option(row, '-S47Open') == 'continue' and
        all(option(row, name) is None for name in ('-S47HorrorContract', '-S47EveningRoute', '-S47Jump', '-S47Case')) and
        'title shown' in row['text'] and re.search(r'night started, case [1-9][0-9]*,', row['text']) and
        'horror route revision 1 restored' in row['text'],
        'Fersk Continue mangler tittelhandlingen, vanlig lagret SC01-sak eller faktisk restore')
    # Choose a genuinely separate report and log for every required journey.
    # Reusing case IDs under different labels cannot manufacture test coverage.
    selected = {}
    def select(labels, used_reports, used_logs):
        if not labels:
            return True
        label = labels[0]
        for cid in journeys[label]:
            row = runtime[cid]
            if row['report'] in used_reports or row['log'] in used_logs:
                continue
            selected[label] = cid
            if select(labels[1:], used_reports | {row['report']}, used_logs | {row['log']}):
                return True
        selected.pop(label, None)
        return False
    require(select(list(journeys), set(), set()),
            'High/Low, begge ankomster og Continue må ha separate faktiske rapporter og logger')
    return selected


def validate(proof_path, proof_sha, candidate, repo=REPO):
    candidate, repo, target = checked_path(candidate), checked_path(repo), checked_path(TARGET)
    require(candidate.is_relative_to(checked_path(SCRATCH)) and candidate != SCRATCH and candidate.name=='Linux' and candidate.is_dir(), 'Kandidaten må være en egen mappe under ~/scratch-bm')
    require(not candidate.is_relative_to(target) and not target.is_relative_to(candidate), 'Kandidat og installasjon overlapper')
    path = checked_path(proof_path)
    proofs = {}
    _, proof = bound_json(proofs, {'path': str(path), 'sha256': proof_sha}, path.parent, candidate, target)
    scenario = proof.get('scenario')
    require(scenario in REQUIREMENTS and proof.get('schema') == 's47-story-install-proof-v1' and proof.get('status') == 'PASS_READY_TO_INSTALL', 'Scenarioet mangler godkjent leveringsskjema')
    require(proof.get('candidate') == str(candidate), 'Beviset gjelder en annen kandidatmappe')
    require(isinstance(proof.get('package_build'),dict),'Hashbundet pakkebyggkvittering mangler')
    binary_sha = digest(proof['binary_sha256'])
    payload_path, expected = bound_json(proofs, proof['payload_snapshot'], path.parent, candidate, target)
    _, frozen = bound_json(proofs, proof['frozen_inputs'], path.parent, candidate, target)
    acceptance_path, accept = bound_json(proofs, proof['acceptance'], path.parent, candidate, target)
    require(accept.get('schema') == 's47-story-acceptance-v1' and accept.get('status') == 'PASS_FOCUSED_' + scenario + '_SCOPE' and
        accept.get('scenario') == scenario and accept.get('tested_commit') == proof.get('tested_commit') and
        accept.get('binary_sha256') == binary_sha and accept.get('payload_snapshot_sha256') == proof['payload_snapshot']['sha256'] and
        accept.get('pull_request_number') == proof['pr']['number'], 'Aksept, kilde, pakke og scenario-PR er ikke bundet sammen')
    repository = repository_proof(repo, proof, frozen)
    package_build=verify_build_receipt(proofs,proof,frozen,path.parent,candidate,target,repo)
    actual = snapshot(candidate)
    require(isinstance(expected, dict) and actual == expected and actual.get(BINARY, {}).get('sha256') == binary_sha, 'Full payload eller binær avviker fra prøvd pakke')
    require(sum('sha256' in row for row in actual.values()) == 40, 'SC01 krever de 40 komplette pakkefilene')
    require(actual[BINARY]['mode'] & 0o111 and not snapshot(candidate, saved=True), 'Kandidaten mangler kjørbar binær eller inneholder Saved')
    cases, requirements = accept.get('cases'), accept.get('requirements')
    require(isinstance(cases, list) and cases and isinstance(requirements, dict) and set(requirements) == REQUIREMENTS[scenario], 'Ufullstendig scenarioaksept')
    ids, kinds, runtime = set(), {}, {}
    for case in cases:
        require(isinstance(case,dict), 'Aksepttilfelle må være et objekt')
        cid, kind = case.get('id'), case.get('kind')
        require(isinstance(cid, str) and cid and cid not in ids and kind in {'runtime', 'visual', 'media'}, 'Ugyldig eller dobbelt aksepttilfelle')
        ids.add(cid); kinds[cid] = kind
        report_path, report = bound_json(proofs, case['report'], acceptance_path.parent, candidate, target)
        status = case.get('expected_status')
        require(isinstance(status, str) and status.startswith('PASS') and report.get('status') == status and
            case.get('binary_sha256') == binary_sha, 'Aksepttilfellet mangler korrekt PASS/binærbinding: ' + cid)
        for key in ('binary_sha256', 'package_sha256', 'candidate_package_sha256'):
            if key in report:
                require(report[key] == binary_sha, 'Rapporten gjelder en annen binær: ' + cid)
        if kind == 'runtime':
            require(type(report.get('returncode')) is int and report['returncode'] == 0 and not any(report.get(k) for k in ('stop_reason','contention','fatal','missing','file_missing')), 'Kjøringen ble ikke fullført rent: ' + cid)
            command = report.get('command')
            require(isinstance(command, list) and command and checked_path(command[0]) == candidate / BINARY, 'Kjøringen brukte en annen binær')
            log_path, raw = ref(proofs, case['log'], acceptance_path.parent, candidate, target)
            text = raw.decode('utf-16') if raw[:2] in (b'\xff\xfe', b'\xfe\xff') else raw.decode('utf-8-sig', errors='replace')
            markers = case.get('required_markers')
            require(isinstance(markers, list) and markers and all(isinstance(m,str) and m and m in text for m in markers) and not ERRORS.search(text), 'Kjøringslogg mangler sluttmarkører eller inneholder feil: ' + cid)
            runtime[cid] = {'command': command, 'text': text, 'report': str(report_path), 'log': str(log_path)}
        else:
            evidence = case.get('evidence')
            require(isinstance(evidence, list) and evidence, 'Bilde-/medieaksept mangler hashbundne originaler')
            for item in evidence:
                source, _ = ref(proofs, item, report_path.parent, candidate, target)
                if kind == 'visual':
                    require(source.suffix.lower() in {'.png','.jpg','.jpeg','.webp'}, 'Bildeaksept peker ikke til originalbilde')
    for label, refs in requirements.items():
        require(isinstance(refs, list) and refs and len(set(refs)) == len(refs) and set(refs) <= ids, 'Krav mangler aksepttilfeller: ' + label)
        needed = 'visual' if label == 'visuals' else 'media' if label == 'media' else 'runtime'
        require(any(kinds[cid] == needed for cid in refs), 'Krav mangler riktig bevisart: ' + label)
    runtime_requirements(requirements, runtime)
    return {'proof': proof, 'proof_hashes': proofs, 'expected': expected, 'repository': repository, 'cases': len(cases), 'candidate': candidate, 'target': target, 'package_build':package_build}


def idle(repo, expected_sha):
    guard_path = checked_path(repo / 'unreal/Signal47/Scripts/unreal_guard.py')
    with root_fd(guard_path.parent) as parent:
        source = file_bytes(guard_path.name, parent)
    require(hashlib.sha256(source).hexdigest() == digest(expected_sha), 'Prøvd inaktivitetsvakt er endret')
    # Execute only the already-hashed bytes; a second import read cannot race them.
    guard = types.ModuleType('s47_story_idle_guard'); guard.__file__ = str(guard_path)
    exec(compile(source, str(guard_path), 'exec'), guard.__dict__)
    grace = os.environ.pop('S47_INSTALLED_GAME_GRACE', None)
    try:
        result = guard.require_idle()
        require(not result.get('heavy_jobs') and not result.get('exempted_installed_game'), 'Streng inaktivitet kreves før installasjon')
        return result
    finally:
        if grace is not None:
            os.environ['S47_INSTALLED_GAME_GRACE'] = grace


def rsync(source, target, saved_copy=False):
    # File descriptors anchor both roots even if a pathname is renamed concurrently.
    with root_fd(source) as source_fd, root_fd(target) as target_fd:
        argv = ['rsync', '-a', '--checksum', '--modify-window=-1', '--delete', '--safe-links']
        argv += ['--include=*/', '--include=**/Saved/***', '--exclude=*'] if saved_copy else ['--exclude=Saved']
        argv += [f'/proc/self/fd/{source_fd}/', f'/proc/self/fd/{target_fd}/']
        subprocess.run(argv, pass_fds=(source_fd,target_fd), check=True)
        require(os.fstat(source_fd).st_ino == checked_path(source).stat().st_ino and
            os.fstat(target_fd).st_ino == checked_path(target).stat().st_ino, 'Kilderot eller målrot ble byttet under kopiering')


def transaction(candidate, target, expected, evidence, prewrite, rollback_guard=lambda: None):
    """Install one already-validated payload; Saved is never a rollback source."""
    evidence.mkdir(mode=0o700)
    backup = evidence / 'payload-backup'; backup.mkdir()
    saved_backup = evidence / 'Saved-backup'; saved_backup.mkdir()
    old, saved = snapshot(target), snapshot(target, saved=True)
    for name in saved:
        for parent in PurePosixPath(name).parents:
            if str(parent) == '.':
                continue
            if 'Saved' not in parent.parts:
                require(expected.get(str(parent), {}).get('directory') is True, 'Ny payload ville fjerne en overmappe til Saved')
    write_json(evidence / 'before-payload.json', old); write_json(evidence / 'before-Saved.json', saved)
    result = {'status':'RUNNING', 'candidate':str(candidate), 'target':str(target), 'backup':str(backup),
        'Saved_backup':str(saved_backup), 'expected_payload':expected, 'before_payload':old, 'before_saved':saved,
        'method':'Full payload backup; separate read-only Saved copy; install/rollback exclude Saved; no Saved restoration'}
    write_json(evidence / 'result.json', result)
    try:
        rsync(target, backup)
        require(snapshot(backup) == old and not snapshot(backup,saved=True), 'Payloadbackup stemmer ikke')
        rsync(target, saved_backup, saved_copy=True)
        require(snapshot(saved_backup,saved=True) == saved, 'Saved-backup stemmer ikke')
        require(snapshot(target) == old and snapshot(target,saved=True) == saved, 'Installert tilstand endret under backup')
        result['prewrite'] = prewrite()
        require(snapshot(candidate) == expected and not snapshot(candidate,saved=True), 'Kandidat endret rett før skriving')
        require(snapshot(target) == old and snapshot(target,saved=True) == saved, 'Installert tilstand endret rett før skriving')
        try:
            rsync(candidate, target)
            require(snapshot(target) == expected and snapshot(target,saved=True) == saved, 'Payload eller Saved avviker etter installasjon')
            require(snapshot(saved_backup,saved=True) == saved and snapshot(candidate) == expected, 'Backup eller kandidat endret under installasjon')
        except BaseException as error:
            result['installation_error'] = repr(error)
            try:
                # No Saved bytes are copied here, even on rollback failure.
                rollback_guard()
                rsync(backup, target)
                require(snapshot(target) == old and snapshot(target,saved=True) == saved, 'Rollback eller Saved-kontroll feilet')
                result['rollback'] = 'PASS'
            except BaseException as rollback_error:
                result['rollback'] = 'FAIL'; result['rollback_error'] = repr(rollback_error)
            raise
        after_saved = snapshot(target,saved=True)
        require(after_saved == saved, 'Saved endret før kvittering')
        write_json(evidence / 'after-Saved.json', after_saved)
        result.update(status='PASS', payload_files=sum('sha256' in row for row in expected.values()),
            saved_nodes_unchanged=len(saved), installed_at=datetime.now(timezone.utc).isoformat())
    except BaseException as error:
        result.update(status='FAIL', error=repr(error)); write_json(evidence / 'result.json', result); raise
    write_json(evidence / 'result.json', result)
    return evidence / 'result.json'


def template(scenario):
    require(scenario in REQUIREMENTS, 'Kun SC01 har definert aksept nå')
    ref_template = {'path':'ABSOLUTE_OR_RELATIVE_PATH', 'sha256':'SHA256_64_HEX'}
    return {'proof':{'schema':'s47-story-install-proof-v1','scenario':scenario,'status':'PASS_READY_TO_INSTALL',
        'candidate':'/home/tom/scratch-bm/Signal47-SC01/candidate/Linux','tested_commit':'GIT_SHA40','merged_core':'GIT_SHA40',
        'binary_sha256':'SHA256_64_HEX','pr':{'number':0,'url':'https://github.com/'+REPOSITORY+'/pull/N',
        'base':BASE,'head':'codex/sc01','head_sha':'GIT_SHA40','merge_commit_sha':'GIT_SHA40'},
        'acceptance':ref_template,'frozen_inputs':ref_template,'payload_snapshot':ref_template,'package_build':ref_template},
        'acceptance':{'schema':'s47-story-acceptance-v1','scenario':scenario,'status':'PASS_FOCUSED_'+scenario+'_SCOPE',
        'tested_commit':'GIT_SHA40','binary_sha256':'SHA256_64_HEX','payload_snapshot_sha256':'SHA256_64_HEX','pull_request_number':0,
        'requirements':{label:['CASE_ID'] for label in sorted(REQUIREMENTS[scenario])},
        'cases':[{'id':'CASE_ID','kind':'runtime','expected_status':'PASS','binary_sha256':'SHA256_64_HEX',
        'report':ref_template,'log':ref_template,'required_markers':['ACTUAL FINISH MARKER']},
        {'id':'VISUAL_OR_MEDIA_ID','kind':'visual','expected_status':'PASS','binary_sha256':'SHA256_64_HEX',
        'report':ref_template,'evidence':[ref_template]}]},
        'frozen_inputs':{'schema':'s47-story-inputs-v1','tested_commit':'GIT_SHA40','frozen_at':'2026-10-26T00:00:00Z',
        'files':{'unreal/PATH':'SHA256_64_HEX','unreal/Signal47/Content/ALL_ACTUAL_FILES_INCLUDING_IGNORED_AUDIO':'SHA256_64_HEX'}},
        'package_build':{'schema':'s47-story-package-build-v1','status':'PASS','scenario':scenario,'tested_commit':'GIT_SHA40',
        'candidate':'/home/tom/scratch-bm/Signal47-SC01/candidate/Linux','frozen_inputs_sha256':'SHA256_64_HEX',
        'payload_snapshot_sha256':'SHA256_64_HEX','binary_sha256':'SHA256_64_HEX','returncode':0,
        'command':['/home/tom/.local/opt/UnrealEngine-5.8.4/Engine/Build/BatchFiles/RunUAT.sh','BuildCookRun',
        '-project=/ABSOLUTE_REPO/unreal/Signal47/Signal47.uproject','-platform=Linux','-build','-cook','-stage','-pak','-archive',
        '-archivedirectory=/home/tom/scratch-bm/Signal47-SC01/candidate'],
        'started_at':'2026-10-26T00:00:02Z','finished_at':'2026-10-26T00:04:00Z','log':ref_template,'pre_inputs':ref_template,'post_inputs':ref_template},
        'package_input_check':{'schema':'s47-story-package-input-check-v1','status':'PASS','phase':'before_OR_after',
        'tested_commit':'GIT_SHA40','frozen_inputs_sha256':'SHA256_64_HEX','checked_at':'2026-10-26T00:00:01Z',
        'files':{'note_nb':'Eksakt samme fullstendige fil/hash-kart som frozen_inputs.files, beregnet fra faktiske bytes før og etter UAT.'}},
        'package_log_markers':[
        'S47_PACKAGE_COMMAND <json.dumps(actual_argv,separators=(comma,colon),ensure_ascii=True)>',
        'S47_PACKAGE_INPUTS_BEFORE <frozen_inputs_sha256> <input_identity_sha256> <before.checked_at>',
        'S47_PACKAGE_BUILD_STARTED <started_at>','BUILD SUCCESSFUL','S47_PACKAGE_BUILD_FINISHED <finished_at> 0',
        'S47_PACKAGE_INPUTS_AFTER <frozen_inputs_sha256> <input_identity_sha256> <after.checked_at>',
        'S47_PACKAGE_OUTPUT <absolute_candidate> <payload_snapshot_sha256> <binary_sha256>'],
        'input_identity_note_nb':'Bruk input_identity(frozen.files): SHA256 av json.dumps(files,sort_keys=True,separators=(comma,colon),ensure_ascii=True). Før- og etterkontrollene må beregne alle faktiske filhashene på nytt.',
        'payload_snapshot':{'note_nb':'Bruk snapshot(candidate) fra denne modulen; alle 40 filer, mapper og moduser kreves.'}}


def self_test():
    """Only temporary synthetic trees; never validate or write the real installation."""
    import unittest

    class FilesystemTests(unittest.TestCase):
        def setUp(self):
            self.temp = tempfile.TemporaryDirectory(prefix='s47-story-installer-')
            self.root = Path(self.temp.name)
            self.source, self.target = self.root/'source', self.root/'target'
            for path in (self.source,self.target):
                (path/'Signal47').mkdir(parents=True)
                (path/'Signal47/game').write_bytes(b'new' if path==self.source else b'old')
            (self.target/'Signal47/Saved/Empty').mkdir(parents=True)
            (self.target/'Signal47/Saved/case.json').write_bytes(b'personal-bytes')
        def tearDown(self):
            self.temp.cleanup()
        def test_complete_install_and_saved_backup(self):
            saved = snapshot(self.target,True)
            receipt = transaction(self.source,self.target,snapshot(self.source),self.root/'receipt',lambda:{'isolated_test':True})
            self.assertEqual(json.loads(receipt.read_text())['status'],'PASS')
            self.assertEqual(snapshot(self.target,True),saved)
            self.assertEqual(snapshot(self.root/'receipt/Saved-backup',True),saved)
        def test_symlink_is_refused_before_copy(self):
            (self.source/'Signal47/link').symlink_to(self.target/'Signal47/game')
            with self.assertRaises(RuntimeError):snapshot(self.source)
            self.assertEqual((self.target/'Signal47/game').read_bytes(),b'old')
        def test_payload_hash_failure_rolls_back_without_saved_write(self):
            original, saved = snapshot(self.target), snapshot(self.target,True)
            expected = snapshot(self.source); expected['Signal47/game']['sha256']='0'*64
            # The prewrite candidate mismatch is caught before installation.
            with self.assertRaises(RuntimeError):transaction(self.source,self.target,expected,self.root/'receipt',lambda:{})
            self.assertEqual(snapshot(self.target),original);self.assertEqual(snapshot(self.target,True),saved)
        def test_actual_write_failure_rolls_back(self):
            original, saved = snapshot(self.target), snapshot(self.target,True)
            original_rsync=globals()['rsync']; writes=[]
            def damaged(source,target,saved_copy=False):
                original_rsync(source,target,saved_copy)
                if source==self.source:
                    writes.append(True);(target/'Signal47/game').write_bytes(b'corrupted')
            globals()['rsync']=damaged
            try:
                with self.assertRaises(RuntimeError):transaction(self.source,self.target,snapshot(self.source),self.root/'receipt',lambda:{})
            finally:globals()['rsync']=original_rsync
            self.assertTrue(writes);self.assertEqual(snapshot(self.target),original);self.assertEqual(snapshot(self.target,True),saved)
            self.assertEqual(json.loads((self.root/'receipt/result.json').read_text())['rollback'],'PASS')
        def test_hash_bound_evidence_rejects_modified_bytes(self):
            proof=self.root/'proof.json';proof.write_text('{}')
            with self.assertRaises(RuntimeError):
                bound_json({}, {'path':str(proof),'sha256':'0'*64}, self.root,self.source,self.target)
        def test_open_pr_is_not_a_merged_delivery(self):
            proof={'tested_commit':'a'*40,'merged_core':'b'*40,'pr':{'number':1,'base':BASE,'head':'codex/sc01','head_sha':'c'*40,'merge_commit_sha':'d'*40,'url':'https://github.com/'+REPOSITORY+'/pull/1'}}
            original=subprocess.check_output
            subprocess.check_output=lambda *args,**kwargs:json.dumps({'state':'OPEN'})
            try:
                with self.assertRaises(RuntimeError):repository_proof(self.root,proof,{})
            finally:subprocess.check_output=original
        def test_file_change_between_hash_and_inventory_is_rejected(self):
            original=globals()['sha_file']
            def racing(name,parent=None):
                result=original(name,parent)
                if name=='game' and parent is not None:
                    fd=os.open(name,os.O_WRONLY|os.O_TRUNC|os.O_NOFOLLOW,dir_fd=parent)
                    try:os.write(fd,b'changed-during-inventory')
                    finally:os.close(fd)
                return result
            globals()['sha_file']=racing
            try:
                with self.assertRaises(RuntimeError):snapshot(self.source)
            finally:globals()['sha_file']=original
        def test_ignored_raw_audio_and_catalog_are_required_inputs(self):
            repo=self.root/'repo';tracked={'unreal/Signal47/Source/example.cpp','unreal/Signal47/Scripts/unreal_guard.py',
                'unreal/Narrative/HorrorStory/generated/HorrorPackData.inl'}
            for name in tracked:
                path=repo/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(b'actual-source')
            raw=repo/'unreal/Signal47/Content/Audio/Raw/horror'
            raw.mkdir(parents=True);(raw/'cue.wav').write_bytes(b'actual-raw-wave');(raw/'voices.json').write_text('{}');(raw/'pack.md').write_text('packaged-content')
            files={name:sha(repo/name) for name in tracked};files.update(content_inventory(repo))
            frozen={'schema':'s47-story-inputs-v1','tested_commit':'a'*40,'frozen_at':'2026-10-26T00:00:00Z','files':dict(files)}
            self.assertEqual(verify_frozen_inventory(repo,frozen,'a'*40,tracked),files)
            del frozen['files']['unreal/Signal47/Content/Audio/Raw/horror/cue.wav']
            with self.assertRaises(RuntimeError):verify_frozen_inventory(repo,frozen,'a'*40,tracked)
            frozen['files']=dict(files);(raw/'pack.md').unlink()
            with self.assertRaises(RuntimeError):verify_frozen_inventory(repo,frozen,'a'*40,tracked)
            (raw/'pack.md').write_text('packaged-content');(raw/'voices.json').write_text('{"changed":true}')
            with self.assertRaises(RuntimeError):verify_frozen_inventory(repo,frozen,'a'*40,tracked)
        def test_package_receipt_requires_real_pre_and_post_freeze_binding(self):
            frozen={'schema':'s47-story-inputs-v1','tested_commit':'a'*40,'frozen_at':'2026-10-26T00:00:00Z','files':{'some-input':'e'*64}}
            candidate=self.root/'built/Linux';candidate.mkdir(parents=True)
            def save(name,value):
                path=self.root/name;path.write_text(json.dumps(value));return {'path':str(path),'sha256':sha(path)}
            proof={'scenario':'SC01','tested_commit':'a'*40,'frozen_inputs':{'sha256':'f'*64},
                'payload_snapshot':{'sha256':'d'*64},'binary_sha256':'b'*64}
            command=['/synthetic/RunUAT.sh','BuildCookRun','-project='+str(self.root/'unreal/Signal47/Signal47.uproject'),
                '-platform=Linux','-build','-cook','-stage','-pak','-archive','-archivedirectory='+str(candidate.parent)]
            before={'schema':'s47-story-package-input-check-v1','status':'PASS','phase':'before','tested_commit':'a'*40,
                'frozen_inputs_sha256':'f'*64,'checked_at':'2026-10-26T00:00:01Z','files':dict(frozen['files'])}
            after=dict(before,phase='after',checked_at='2026-10-26T00:04:01Z')
            build={'schema':'s47-story-package-build-v1','status':'PASS','scenario':'SC01','tested_commit':'a'*40,
                'candidate':str(candidate),'frozen_inputs_sha256':'f'*64,'payload_snapshot_sha256':'d'*64,'binary_sha256':'b'*64,
                'returncode':0,'command':command,'started_at':'2026-10-26T00:00:02Z','finished_at':'2026-10-26T00:04:00Z',
                'pre_inputs':save('before.json',before),'post_inputs':save('after.json',after)}
            identity_sha=input_identity(frozen['files'])
            markers=['S47_PACKAGE_COMMAND '+json.dumps(command,separators=(',',':'),ensure_ascii=True),
                'S47_PACKAGE_INPUTS_BEFORE '+'f'*64+' '+identity_sha+' '+before['checked_at'],
                'S47_PACKAGE_BUILD_STARTED '+build['started_at'],'BUILD SUCCESSFUL',
                'S47_PACKAGE_BUILD_FINISHED '+build['finished_at']+' 0',
                'S47_PACKAGE_INPUTS_AFTER '+'f'*64+' '+identity_sha+' '+after['checked_at'],
                'S47_PACKAGE_OUTPUT '+str(candidate)+' '+'d'*64+' '+'b'*64]
            log=self.root/'build.log';log.write_text('\n'.join(markers)+'\n');build['log']={'path':str(log),'sha256':sha(log)}
            with self.assertRaises(RuntimeError):verify_build_receipt({},proof,frozen,self.root,candidate,self.target,self.root)
            proof['package_build']=save('build.json',build)
            self.assertEqual(verify_build_receipt({},proof,frozen,self.root,candidate,self.target,self.root)['input_files'],1)
            old=dict(build,tested_commit='c'*40);proof['package_build']=save('build.json',old)
            with self.assertRaises(RuntimeError):verify_build_receipt({},proof,frozen,self.root,candidate,self.target,self.root)
            proof['package_build']=save('build.json',build)
            after['files']={'changed-input':'0'*64};build['post_inputs']=save('after.json',after);proof['package_build']=save('build.json',build)
            with self.assertRaises(RuntimeError):verify_build_receipt({},proof,frozen,self.root,candidate,self.target,self.root)
            # A receipt preceding the actual freeze is also rejected even when the
            # source/hash strings were otherwise reused verbatim.
            after['files']=dict(frozen['files']);build['post_inputs']=save('after.json',after);proof['package_build']=save('build.json',build)
            newly_frozen=dict(frozen,frozen_at='2026-10-26T00:05:00Z')
            with self.assertRaises(RuntimeError):verify_build_receipt({},proof,newly_frozen,self.root,candidate,self.target,self.root)
        def test_runtime_whitelist_is_markdown_only(self):
            self.assertTrue(documentation_only('unreal/todo.md'))
            for path in ('unreal/Narrative/HorrorStory/pack.json','unreal/Narrative/HorrorStory/generated/HorrorPackData.inl','unreal/Narrative/HorrorStory/generated/subtitles.json'):
                self.assertFalse(documentation_only(path))
        def journey_fixture(self):
            def row(name, args, text):
                return {'command':['/candidate/Signal47', *args], 'text':text,
                        'report':name+'.json', 'log':name+'.log'}
            runtime={
                'high':row('high',['-S47SelfTest','-S47HorrorContract=SC01','-S47Quality=high'],
                           'self test OK\nHorror SC01 PASS COMPLETE'),
                'low':row('low',['-S47HorrorContract=SC01','-S47Quality=low'],'Horror SC01 PASS COMPLETE'),
                'direct':row('direct',['-S47HorrorArrival','-S47EveningRoute=direct'],'horror arrival direct PASS COMPLETE'),
                'burger':row('burger',['-S47HorrorArrival','-S47EveningRoute=burger'],'horror arrival burger PASS COMPLETE'),
                'continue':row('continue',['-S47Title','-S47Open=continue'],
                               'title shown\nhorror route revision 1 restored, SC01 complete\nnight started, case 1, evening')}
            requirements={'selftest':['high'],'scenario_high':['high'],'scenario_low':['low'],
                          'legacy_routes':['direct','burger'],'continue':['continue']}
            return requirements,runtime
        def test_required_journeys_accept_distinct_real_profiles(self):
            requirements,runtime=self.journey_fixture()
            selected=runtime_requirements(requirements,runtime)
            self.assertEqual(selected,{'high':'high','low':'low','direct':'direct','burger':'burger','continue':'continue'})
        def test_one_smoke_run_cannot_cover_all_required_journeys(self):
            requirements,runtime=self.journey_fixture()
            for label in requirements:
                requirements[label]=['high']
            with self.assertRaises(RuntimeError):runtime_requirements(requirements,runtime)
        def test_quality_alias_and_shared_logs_are_rejected(self):
            requirements,runtime=self.journey_fixture()
            runtime['low']['command'].append('-S47Quality=high')
            with self.assertRaises(RuntimeError):runtime_requirements(requirements,runtime)
            requirements,runtime=self.journey_fixture()
            runtime['low']['log']=runtime['high']['log']
            with self.assertRaises(RuntimeError):runtime_requirements(requirements,runtime)
        def test_continue_requires_real_saved_cohort_without_fixture_flags(self):
            requirements,runtime=self.journey_fixture()
            runtime['continue']['text']='title shown\nnight started, case 1, evening'
            with self.assertRaises(RuntimeError):runtime_requirements(requirements,runtime)
            requirements,runtime=self.journey_fixture()
            runtime['continue']['command'].append('-S47Jump=chapter1')
            with self.assertRaises(RuntimeError):runtime_requirements(requirements,runtime)
    suite=unittest.defaultTestLoader.loadTestsFromTestCase(FilesystemTests)
    return 0 if unittest.TextTestRunner(verbosity=2).run(suite).wasSuccessful() else 1


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--candidate',type=Path,help='Eksplisitt kandidat/Linux under ~/scratch-bm')
    parser.add_argument('--proof',type=Path,help='Endelig SHA-bundet leveringsbevis')
    parser.add_argument('--proof-sha256',help='Forventet SHA256 oppgitt separat fra bevisfilen')
    parser.add_argument('--repo',type=Path,default=REPO,help='Kildecheckout som inneholder prøvde inputs')
    parser.add_argument('--receipt-root',type=Path,help='Eksisterende mappe under ~/scratch-bm for ny kvittering/backup')
    parser.add_argument('--apply',action='store_true',help='Installer først etter full bevis- og flettekontroll')
    parser.add_argument('--template',choices=sorted(REQUIREMENTS),help='Vis konkret JSON-skjema uten filskriving')
    parser.add_argument('--self-test',action='store_true',help='Kjør kun syntetiske filsystemprøver i /tmp')
    args=parser.parse_args()
    if args.template:
        require(not args.apply,'Skjema kan ikke kombineres med installasjon');print(json.dumps(template(args.template),indent=2,ensure_ascii=False));return 0
    if args.self_test:
        require(not args.apply,'Selvprøve kan ikke installere spillet');return self_test()
    require(args.candidate and args.proof and args.proof_sha256,'--candidate, --proof og --proof-sha256 kreves; standard er tørrkjøring')
    value=validate(args.proof,args.proof_sha256,args.candidate,args.repo)
    if not args.apply:
        print(json.dumps({'status':'VALIDATED_NOT_INSTALLED','scenario':value['proof']['scenario'],
            'payload_files':sum('sha256' in row for row in value['expected'].values()),'cases':value['cases'],
            'repository':value['repository']},indent=2));return 0
    receipt_root=checked_path(args.receipt_root or args.proof.parent)
    require(receipt_root.is_relative_to(checked_path(SCRATCH)) and not receipt_root.is_relative_to(value['candidate']), 'Kvitteringsmappe må ligge utenfor kandidat, under ~/scratch-bm')
    idle(args.repo,value['repository']['idle_guard_sha256'])
    lock_path=TARGET.parent/'.story-install.lock'
    lock_fd=os.open(lock_path,os.O_CREAT|os.O_RDWR|os.O_NOFOLLOW,0o600)
    try:
        fcntl.flock(lock_fd,fcntl.LOCK_EX|fcntl.LOCK_NB)
        stamp=datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')+'-'+uuid.uuid4().hex[:8]
        evidence=receipt_root/('installation-'+value['proof']['scenario'].lower()+'-'+stamp)
        def prewrite():
            guard=idle(args.repo,value['repository']['idle_guard_sha256'])
            current=validate(args.proof,args.proof_sha256,args.candidate,args.repo)
            require(current['expected']==value['expected'] and current['proof_hashes']==value['proof_hashes'],'Bevis eller pakke endret under forberedelse')
            return {'idle':guard,'proof_path':str(args.proof),'proof_sha256':args.proof_sha256,
                'proof':current['proof'],'proof_hashes':current['proof_hashes'],'repository':current['repository'],'package_build':current['package_build']}
        result=transaction(value['candidate'],value['target'],value['expected'],evidence,prewrite,lambda:idle(args.repo,value['repository']['idle_guard_sha256']))
        print('STORY_INSTALLATION_PASS',result)
    finally:
        os.close(lock_fd)
    return 0


if __name__=='__main__':
    try:raise SystemExit(main())
    except (RuntimeError,OSError,ValueError,KeyError,subprocess.CalledProcessError) as error:
        print('STORY_INSTALLATION_REFUSED:',error,file=sys.stderr);raise SystemExit(1)
