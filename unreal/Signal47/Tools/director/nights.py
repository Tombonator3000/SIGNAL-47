#!/usr/bin/env python3
"""A natural multi-night run, read night by night from its log (-S47NaturalNight, -S47NaturalStand): what came, how far from the
player, whether the player noticed it, and which dangers were met. Measures the game, not the dice (Tom's critics, 8 November).
  python3 nights.py Saved/Logs/Signal47.log [--near 400] [--quiet]
The night is the game's own day number: it turns at eight in the morning (FS47FreeformSession::NightOf), so 01:00 on day 4 is night 4.
Lines it reads (S47Happenings, S47Rhythm, S47Taken, S47FarmSite):
  S47: happening start <id> day <d> <hh:mm> in <area> at <place>, <n> m from the player
  S47: happening <id> noticed day <d> <hh:mm> at <n> m | at the player | far off by <how>
  S47: happening <id> met after <s> s, <n> m from the player: the night's danger
  S47: happening end <id> after <s> s (<m> min of the night) ..., noticed | not noticed, ...
  S47: happening <id> refused day <d> <hh:mm>: <why>
  S47: happenings night <n> summary: ...
  S47: rhythm danger <id>: <n> tonight      (every system's danger, as it is counted)
  S47: unease <n> <id> day <d> <hh:mm> in <area>
"""
import argparse, collections, re, sys
from pathlib import Path

START = re.compile(r'S47: happening start (?P<id>\w+)(?: \((?P<var>\w+)\))? day (?P<day>\d+) (?P<hm>\d\d:\d\d) in (?P<area>\w+) at (?P<at>.*?)(?:, (?:no distance|(?P<m>\d+) m from the player))?(?P<f10>, F10)?$')
NOTICED = re.compile(r'S47: happening (\w+) noticed day (\d+) (\d\d:\d\d) (at the player|far off|at (\d+) m) by (.*?)(, F10)?$')
MET = re.compile(r"S47: happening (\w+) met after ([\d.]+) s, (at the player|(\d+) m from the player): the night's danger")
END = re.compile(r'S47: happening end (\w+) after ([\d.]+) s \(([\d.-]+) min of the night\)(.*)$')
REFUSED = re.compile(r'S47: happening (\w+) refused day (\d+) (\d\d:\d\d): (.*)$')
SUMMARY = re.compile(r'S47: happenings night (\d+) summary: (.*)$')
DANGER = re.compile(r'S47: rhythm danger (\w+): (\d+) tonight')
UNEASE = re.compile(r'S47: unease (\d+) (\w+) day (\d+) (\d\d:\d\d) in (\w+)(, F10)?')
TAKEN = re.compile(r'S47: taken[: ](.*)$')
DAY = re.compile(r'\bday (\d+)\b')

def hour_of_night(hm):
    h, m = map(int, hm.split(':'))
    x = h + m / 60.
    return x + 24 if x < 8 else x

def main():
    a = argparse.ArgumentParser()
    a.add_argument('log'); a.add_argument('--near', type=float, default=400.); a.add_argument('--quiet', action='store_true')
    o = a.parse_args()
    lines = Path(o.log).read_text(errors='replace').splitlines()
    nights = collections.defaultdict(lambda: {'starts': [], 'noticed': [], 'met': [], 'dangers': [], 'unease': [], 'taken': [], 'refused': collections.Counter(), 'long': [], 'summary': ''})
    day = 0
    for line in lines:
        if m := START.search(line):
            if m.group('f10'): continue
            day = int(m.group('day'))
            nights[day]['starts'].append((m.group('hm'), m.group('id') + (f"({m.group('var')})" if m.group('var') else ''), m.group('area'), m.group('at')[:44], float(m.group('m')) if m.group('m') else None))
            continue
        if m := NOTICED.search(line):
            if m.group(7): continue
            day = int(m.group(2))
            dist = 0. if m.group(4) == 'at the player' else None if m.group(4) == 'far off' else float(m.group(5))
            nights[day]['noticed'].append((m.group(3), m.group(1), dist, m.group(6)))
            continue
        if m := MET.search(line):
            nights[day]['met'].append((m.group(1), 0. if m.group(3) == 'at the player' else float(m.group(4))))
            continue
        if m := END.search(line):
            if float(m.group(3)) > 90: nights[day]['long'].append((m.group(1), float(m.group(3))))
            continue
        if m := REFUSED.search(line):
            day = int(m.group(2)); nights[day]['refused'][f'{m.group(1)}: {m.group(4)[:60]}'] += 1
            continue
        if m := SUMMARY.search(line):
            nights[int(m.group(1))]['summary'] = m.group(2)
            continue
        if m := DANGER.search(line):
            nights[day]['dangers'].append(m.group(1))
            continue
        if m := UNEASE.search(line):
            if m.group(6): continue
            day = int(m.group(3)); nights[day]['unease'].append((m.group(4), m.group(2), m.group(5)))
            continue
        if m := TAKEN.search(line):
            nights[day]['taken'].append(m.group(1)[:80])
            continue
        if m := DAY.search(line):
            if 'S47:' in line: day = int(m.group(1))
    if not nights:
        print('no night events in this log'); return 1
    near_nights = danger_nights = late_nights = 0
    places = collections.Counter(); held_long = []
    for n in sorted(k for k in nights if k > 0):
        N = nights[n]
        seen_near = [x for x in N['noticed'] if x[2] is not None and x[2] < o.near]
        late = [x for x in seen_near if 23 <= hour_of_night(x[0]) < 25] + [u for u in N['unease'] if 23 <= hour_of_night(u[0]) < 25]
        near_nights += 1 if seen_near or N['unease'] else 0
        danger_nights += 1 if N['dangers'] else 0
        late_nights += 1 if late else 0
        for s in N['starts']: places[s[3]] += 1
        held_long += [(n, *x) for x in N['long']]
        if o.quiet: continue
        print(f'\n=== night {n}: {len(N["starts"])} started, {len(N["noticed"])} noticed ({len(seen_near)} under {o.near:.0f} m), {len(N["unease"])} unease, dangers: {", ".join(N["dangers"]) or "none"}')
        noticed = {x[1] for x in N['noticed']}
        for hm, i, area, where, dist in sorted(N['starts'], key=lambda s: hour_of_night(s[0])):
            base = i.split('(')[0]
            print(f'  {hm} {i:24s} {"noticed" if base in noticed else "-------"}  {"" if dist is None else f"{dist:5.0f} m"}  {area} {where}')
        for hm, i, dist, how in N['noticed']:
            print(f'    noticed {hm} {i} {"far off" if dist is None else f"{dist:.0f} m"} by {how}')
        for i, dist in N['met']: print(f'    MET {i} at {dist:.0f} m')
        for t in N['taken']: print(f'    taken: {t}')
        for hm, i, area in N['unease']: print(f'    unease {hm} {i} in {area}')
        print('  23-01 near the player:', ', '.join(f'{x[0]} {x[1]}' for x in late) or 'nothing')
        if N['refused']: print('  refused:', '; '.join(f'{k} x{v}' for k, v in N['refused'].most_common(6)))
        if N['summary']: print('  game says:', N['summary'])
    total = len([k for k in nights if k > 0])
    print(f'\nnights: {total}; something noticed near the player (under {o.near:.0f} m, or unease): {near_nights}; '
          f'near between 23 and 01: {late_nights}; a danger met (any system): {danger_nights}')
    print('places of night events:', dict(places.most_common(12)))
    if held_long: print('held the ground longer than 90 minutes of the night (a pace fault):', held_long[:10])
    return 0

if __name__ == '__main__':
    sys.exit(main())
