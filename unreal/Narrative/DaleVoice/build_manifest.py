#!/usr/bin/env python3
"""Collect every line Dale speaks in the game into one manifest (manifest.json).

Sources, read as text (nothing is compiled or run):
  Story:    EveningShift and HorrorStory recording manifests (actor DALE), with the
            module JSON line that authors each cue.
  Freeform: C++ tables in unreal/Signal47/Source/Signal47/Private: the service line
            (S47PredecessorText People "dale"), his walkie barks and walkie lines,
            the big night's morning lines (S47BigNight), and the strangers' reports
            he passes on over the walkie (S47FieldRadio::Report -> RelayLine).
  Director: Content/Director (director.json and scripted/*.json, who "dale").

Each entry: id, text, source (repo path:line), channel (phone, walkie, radio),
pack, kind and status:
  voice     fixed text: generate.py makes one take for it.
  alias     the same words as an earlier entry, or a pack recording alias: reuses that take.
  template  text with runtime parts ({place}, %s ...): not generated; see variables.
  disabled  authored but switched off (enabled: false).

  python3 build_manifest.py           # write manifest.json and print the statistics
  python3 build_manifest.py --check   # fail if manifest.json is out of date
"""
from pathlib import Path
import argparse
import collections
import csv
import json
import re
import sys

HERE = Path(__file__).resolve().parent
NARRATIVE = HERE.parent
REPO = NARRATIVE.parent.parent
CPP = REPO / 'unreal/Signal47/Source/Signal47/Private'
DIRECTOR = REPO / 'unreal/Signal47/Content/Director'
LIT = r'"((?:[^"\\]|\\.)*)"'


def rel(path):
    return str(Path(path).resolve().relative_to(REPO))


def unescape(text):
    return re.sub(r'\\(.)', lambda m: {'n': '\n', 't': '\t'}.get(m.group(1), m.group(1)), text)


def lines_of(path):
    return path.read_text(encoding='utf-8').splitlines()


def block(lines, start_pattern, end_pattern, after=0):
    """Line numbers [first, last) of the block that starts at the first match of start_pattern."""
    first = next(i for i, l in enumerate(lines) if i >= after and re.search(start_pattern, l))
    last = next(i for i, l in enumerate(lines) if i > first and re.search(end_pattern, l))
    return first, last


def placeholders(text):
    return sorted(set(re.findall(r'\{(\w+)\}', text)) | ({'%s'} if '%s' in text else set()) | ({'%d'} if '%d' in text else set()))


# ---------------------------------------------------------------- Story packs
def story(pack, prefix):
    root = NARRATIVE / pack
    manifest = json.loads((root / 'generated/recording_manifest.json').read_text())
    texts = {r['line_id']: r for r in csv.DictReader((root / 'generated/voice_lines.csv').open(newline=''))}
    where = {}
    for module in sorted((root / 'modules').glob('*.json')):
        for n, line in enumerate(lines_of(module), 1):
            m = re.search(r'"id":\s*"([A-Z0-9_]+)"', line)
            if m and m.group(1) not in where:
                where[m.group(1)] = f'{rel(module)}:{n}'
    csv_rows = {row: n for n, row in enumerate([r['line_id'] for r in texts.values()], 2)}
    out = []
    for item in manifest['recordings']:
        if item['actor'] != 'DALE':
            continue
        line_id = item['line_id']
        row = texts[line_id]
        out.append({
            'id': line_id, 'text': row['text'], 'channel': item['channel'], 'pack': prefix,
            'source': where.get(line_id) or f"{rel(root / 'generated/voice_lines.csv')}:{csv_rows[line_id]}",
            'kind': 'story_cue', 'status': 'voice' if item['recording_required'] else 'alias',
            'alias_of': item.get('reuse_of') if not item['recording_required'] else None,
            'current_take': item.get('audio_file'), 'current_profile': item['voice_profile'],
            'delivery_nb': row.get('delivery_nb') or None})
    return out


# ---------------------------------------------------------------- Freeform C++
def predecessor_text():
    path = CPP / 'S47PredecessorText.cpp'
    lines = lines_of(path)
    out = []
    # The service line (telephone): his first words and every topic's answers.
    first, last = block(lines, r'\{TEXT\("dale"\),TEXT\("SERVICE LINE / DALE"\)', r'\{TEXT\("nora"\),')
    topic, count = 'first', collections.Counter()
    for i in range(first, last):
        m = re.match(r'\s*\{TEXT\("(\w+)"\),TEXT\("Ask|\s*\{TEXT\("(\w+)"\),TEXT\("Tell', lines[i])
        if m:
            topic = m.group(1) or m.group(2)
        for said in re.finditer(r'\{Dale,TEXT\(' + LIT + r'\)\}', lines[i]):
            count[topic] += 1
            out.append({'id': f'FF_PHONE_{topic}_{count[topic]}', 'text': unescape(said.group(1)), 'channel': 'phone',
                        'source': f'{rel(path)}:{i + 1}', 'kind': 'service_line', 'topic': topic})
    # Barks: unasked, over the walkie (or on the slip by the phone when there is no walkie).
    first, last = block(lines, r'FBark>& Barks\(\)', r'return Table;')
    for i in range(first, last):
        m = re.search(r'\{TEXT\("(\w+)"\),(?:TEXT\("[^"]*"\)|nullptr),(\d+),TEXT\(' + LIT + r'\)\}', lines[i])
        if m:
            out.append({'id': f'FF_BARK_{m.group(1)}', 'text': unescape(m.group(3)), 'channel': 'walkie',
                        'source': f'{rel(path)}:{i + 1}', 'kind': 'bark', 'from_day': int(m.group(2)),
                        'note': 'Also printed (upper case) on the slip by the phone when the walkie is not carried; that copy is paper.'})
    first, last = block(lines, r'FWalkieLine>& WalkieLines\(\)', r'return Table;')
    for i in range(first, last):
        m = re.search(r'\{TEXT\("(\w+)"\),TEXT\(' + LIT + r'\)\}', lines[i])
        if m:
            entry = {'id': f'FF_WALKIE_{m.group(1)}', 'text': unescape(m.group(2)), 'channel': 'walkie',
                     'source': f'{rel(path)}:{i + 1}', 'kind': 'walkie_line'}
            if m.group(1) == 'finally':
                entry['note'] = 'Said before a bark when an earlier hail was missed (S47Predecessor.cpp SayBark): two clips in a row.'
            if m.group(1) in ('relay_pre', 'relay_post'):
                entry['note'] = 'Frames a stranger\'s report (S47Predecessor.cpp RelayLine): relay_pre + report + relay_post.'
            out.append(entry)
    return out


def big_night():
    path = CPP / 'S47BigNight.cpp'
    lines = lines_of(path)
    out = []
    for i, line in enumerate(lines):
        # His morning (DaleFirst ...), his hail, and BIGNIGHT-SEEN-20261109's lines while it goes on and just after (DaleSee ...).
        m = re.search(r'const TCHAR\* const (Dale[A-Z]\w*|Hail[A-Z]\w*)=TEXT\(' + LIT + r'\);', line)
        if m:
            out.append({'id': f'FF_BIG_{m.group(1)}', 'text': unescape(m.group(2)), 'channel': 'walkie',
                        'source': f'{rel(path)}:{i + 1}', 'kind': 'big_night'})
    # FWords: Seen, DaleLine, Caller, Voice, Earl, Cb, Passed, Title, Mark, Lead, Found (11 per shape).
    first, last = block(lines, r'const FWords Words\[\]=\{', r'^\s*\};')
    literals = [(i, unescape(m.group(1))) for i in range(first, last) for m in re.finditer(r'TEXT\(' + LIT + r'\)', lines[i])]
    if len(literals) != 33:
        raise ValueError(f'S47BigNight Words: expected 3 x 11 literals, found {len(literals)}')
    shapes = ('column', 'furrow', 'stones')
    for s, shape in enumerate(shapes):
        n, text = literals[s * 11 + 1]
        out.append({'id': f'FF_BIG_{shape}_dale', 'text': text, 'channel': 'walkie', 'source': f'{rel(path)}:{n + 1}',
                    'kind': 'big_night'})
        n, text = literals[s * 11 + 6]
        out.append({'id': f'FF_RELAY_big_{shape}', 'text': text, 'channel': 'walkie', 'source': f'{rel(path)}:{n + 1}',
                    'kind': 'relay', 'note': 'A stranger\'s report as Dale passes it on (Happenings::Tip Passed): between relay_pre and relay_post, unless a CB is at hand.'})
    return out


def relays():
    out = []
    note = 'A stranger\'s report as Dale passes it on: between relay_pre and relay_post, unless a CB is at hand (S47FieldRadio::Report).'
    path = CPP / 'S47FreeformDirector.cpp'
    lines = lines_of(path)
    first, last = block(lines, r'FString LightsTip\(int32 Week', r'^\s*\}\s*$')
    week = 0
    for i in range(first, last):
        m = re.search(r'return FString::Printf\(TEXT\(' + LIT + r'\)', lines[i])
        if m:
            week += 1
            out.append({'id': f'FF_RELAY_lights_w{week}', 'text': unescape(m.group(1)).replace('%s', '{caller}', 1).replace('%s', '{place}', 1),
                        'channel': 'walkie', 'source': f'{rel(path)}:{i + 1}', 'kind': 'relay', 'note': note})
    for i, line in enumerate(lines):
        m = re.search(r'const FString Tip=FString::Printf\(TEXT\(' + LIT + r'\)', line)
        if m:
            out.append({'id': 'FF_RELAY_landing', 'text': unescape(m.group(1)).replace('%s', '{place}'), 'channel': 'walkie',
                        'source': f'{rel(path)}:{i + 1}', 'kind': 'relay', 'note': note})
    path = CPP / 'S47FarmSite.cpp'
    lines = lines_of(path)
    first, last = block(lines, r'^FString FS47FarmSite::SecondCallLine', r'^\}')
    case = 0
    for i in range(first, last):
        m = re.search(r'return FString::Printf\(TEXT\(' + LIT + r'\)', lines[i])
        if m:
            case += 1
            out.append({'id': f'FF_RELAY_second_{case}', 'text': unescape(m.group(1)).replace('%s', '{place}'), 'channel': 'walkie',
                        'source': f'{rel(path)}:{i + 1}', 'kind': 'relay', 'note': note})
    path = CPP / 'S47Happenings.cpp'
    lines = lines_of(path)
    count = 0
    for i, line in enumerate(lines):
        if not re.match(r'\s*Tip\(\{', line):
            continue
        # Passed is the call's last argument, on the line after the caller's own words.
        m = re.match(r'\s*FString::Printf\(TEXT\(' + LIT + r'\)', lines[i + 1])
        if m:
            count += 1
            out.append({'id': f'FF_RELAY_hap_{count}', 'text': unescape(m.group(1)).replace('%s', '{place}'), 'channel': 'walkie',
                        'source': f'{rel(path)}:{i + 2}', 'kind': 'relay', 'note': note})
    return out


def director():
    out = []
    files = [DIRECTOR / 'director.json'] + sorted((DIRECTOR / 'scripted').glob('*.json'))
    for path in files:
        data = json.loads(path.read_text())
        lines = lines_of(path)
        enabled = data.get('enabled', True) if isinstance(data, dict) else True
        for n, line in enumerate(lines, 1):
            m = re.search(r'"who":\s*"dale".*"text":\s*' + LIT, line)
            if m:
                out.append({'id': f"FF_SCRIPT_{data.get('id', path.stem)}_{n}", 'text': unescape(m.group(1)), 'channel': 'walkie',
                            'source': f'{rel(path)}:{n}', 'kind': 'director_script',
                            'status': None if enabled else 'disabled'})
    return out


def finish(entries):
    seen = {}
    for e in entries:
        e.setdefault('pack', 'freeform')
        holes = placeholders(e['text'])
        if holes:
            e['variables'] = holes
        if e.get('status') is None:
            e['status'] = 'template' if holes else 'voice'
        key = re.sub(r'\s+', ' ', e['text']).strip().lower()
        if e['status'] == 'voice':
            if key in seen:
                e['status'] = 'alias'
                e['alias_of'] = seen[key]
            else:
                seen[key] = e['id']
    ids = [e['id'] for e in entries]
    dupes = [k for k, v in collections.Counter(ids).items() if v > 1]
    if dupes:
        raise ValueError(f'duplicate ids: {dupes}')
    return entries


def template_domains():
    """What the runtime parts can be, so the number of spoken variants is known before anyone records them."""
    lines = lines_of(CPP / 'S47EventPlaces.cpp')
    first, last = block(lines, r'struct FSeed', r'^\s*\};')
    places = [m.group(1) for i in range(first, last + 40) for m in re.finditer(r'\{TEXT\("[a-z0-9_]+"\),TEXT\(' + LIT + r'\)', lines[i])]
    places = [p for p in places if p and p != 'STATION 01'] + ['the remote feeder on the county road', 'the lot']
    caller_line = next(l for l in lines_of(CPP / 'S47FreeformDirector.cpp') if 'const FString Caller=' in l)
    callers = [unescape(m.group(1)) for m in re.finditer(r'TEXT\(' + LIT + r'\)', caller_line)
               if m.group(1)[:1].isupper()]
    return {'place': {'count': len(places), 'values': places, 'source': rel(CPP / 'S47EventPlaces.cpp')},
            'caller': {'count': len(dict.fromkeys(callers)), 'values': list(dict.fromkeys(callers)), 'source': rel(CPP / 'S47FreeformDirector.cpp')},
            'az': {'count': 360, 'values': '000-359 (degrees, three digits)', 'source': rel(CPP / 'S47BigNight.cpp')},
            'el': {'count': 90, 'values': '0-89 (degrees)', 'source': rel(CPP / 'S47BigNight.cpp')},
            'azwords': {'count': 360, 'values': 'the bearing digit by digit: zero-zero-zero to three-five-nine', 'source': rel(CPP / 'S47BigNight.cpp')},
            'way': {'count': 8, 'values': ['north', 'north-east', 'east', 'south-east', 'south', 'south-west', 'west', 'north-west'], 'source': rel(CPP / 'S47Happenings.cpp')},
            'miles': {'count': 9, 'values': ['under a mile', 'about a mile', 'a mile and a half', 'about two miles', 'about three miles', 'about four miles', 'about five miles', 'about six miles', 'a good way'], 'source': rel(CPP / 'S47BigNight.cpp')}}


def build():
    entries = finish(story('EveningShift', 'evening') + story('HorrorStory', 'horror') + predecessor_text() + big_night()
                     + relays() + director())
    stats = collections.OrderedDict()
    for e in entries:
        group = f"{e['pack']}/{e['kind']}"
        row = stats.setdefault(group, collections.Counter())
        row[e['status']] += 1
        row['total'] += 1
    totals = collections.Counter(e['status'] for e in entries)
    channels = collections.Counter(e['channel'] for e in entries if e['status'] == 'voice')
    words = sum(len(e['text'].split()) for e in entries if e['status'] == 'voice')
    return {
        'version': 1,
        'note_nb': ('Alle replikker Dale sier i spillet, samlet av build_manifest.py fra kildene (ikke håndført). status voice: '
                    'én take per linje; alias: samme ord som en tidligere linje eller pakkens recording_alias; template: tekst med '
                    'kjøretidsdeler, genereres ikke; disabled: skrevet, men slått av. Spillteksten er engelsk og uendret.'),
        'sources': {'story': ['unreal/Narrative/EveningShift/generated/recording_manifest.json',
                              'unreal/Narrative/HorrorStory/generated/recording_manifest.json'],
                    'freeform_cpp': sorted({e['source'].rsplit(':', 1)[0] for e in entries if e['source'].endswith('.cpp', 0, e['source'].rfind(':'))}),
                    'director': ['unreal/Signal47/Content/Director/director.json', 'unreal/Signal47/Content/Director/scripted/*.json'],
                    'searched_without_dale_lines': ['S47FieldRadio.cpp (only the player\'s own key-up lines and self-test strings)',
                                                    'S47Mail*.cpp', 'S47Phone*.cpp', 'S47Walkie.cpp', 'S47Freeform*.cpp',
                                                    'S47PredecessorLate.cpp', 'Content/Director/director.json']},
        'excluded_nb': ('Papir og tekst som ikke er tale: Dales clipboard-liste og lapper (S47PredecessorText Docs, S47BigNight Slip), '
                        'utskrift av barks på lappen ved telefonen, Wards svar om Dale, spillerens egne walkie-linjer («SARO night to Dale. '
                        'Come in.», «Go ahead, Dale.»), selvtest-strenger i S47FieldRadio.cpp/S47GearFlicker.cpp og dev-rapporten i S47DevFreeform.cpp.'),
        'statistics': {'by_source': {k: dict(v) for k, v in stats.items()}, 'by_status': dict(totals),
                       'voice_by_channel': dict(channels), 'voice_words': words},
        'template_domains': template_domains(),
        'lines': entries,
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    value = build()
    text = json.dumps(value, ensure_ascii=False, indent=1) + '\n'
    target = HERE / 'manifest.json'
    if args.check:
        if not target.exists() or target.read_text() != text:
            print('manifest.json is out of date: run build_manifest.py', file=sys.stderr)
            return 1
    else:
        target.write_text(text)
    print(json.dumps(value['statistics'], ensure_ascii=False, indent=1))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
