#!/usr/bin/env python3
"""A sketch of the rhythm, not proof of it: throws the Director's budget, ticks and cooldowns from director.json over many days and says
how many small things come and when. The world is not modelled: sight, being at a cabinet, and what a player does are not here.
  python3 simulate.py [--days 30] [--runs 200] [--without-hints] [--no-pace] [--only id,id]
--only keeps to what can reach a player who stays in one place, e.g. the farmhouse:
  --only roof_tick,service_door_open,roof_steps,empty_car,flyover"""
import argparse, random, sys, zlib
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import director_rules
ROOT = Path(__file__).resolve().parents[2]
HINTS = ['phone_one_ring', 'printer_line', 'rack_click', 'radio_crackle', 'service_door_open', 'antenna_creak', 'fence_tap', 'night_hush', 'roof_tick']
# Rates from S47Freeform.cpp: 11 night hours in 45 active minutes, 13 day hours in 30.
NIGHT_RATE, DAY_RATE = 11 * 3600 / 2700, 13 * 3600 / 1800
def cool_days(i, last, lo=2, hi=4):
    # FS47Rhythm::CoolDays, from the event's cooldown_days. The CRC is not UE's, so the spread is what is alike.
    v = (zlib.crc32(i.encode()) ^ (last * 2654435761)) & 0xffffffff
    hi = max(lo, hi)
    return lo + ((v >> 7) ^ v) % (hi - lo + 1)
def pace(spent, budget, hour, jitter):
    if budget <= 0 or spent >= budget: return False
    night = hour - 19 if hour >= 19 else hour + 5 if hour < 6 else 0
    import math
    return spent < math.ceil(budget * min(1, max(0, night / 11)) + jitter * .75)
def night_of(clock): return max(1, int((clock - 8 * 3600) // 86400) + 1)
def run(doc, days, rnd, hints, use_pace, only=None):
    g = doc['globals']
    minor = [e for e in doc['events'] if e.get('budget') == 'minor' and (hints or e['id'] not in HINTS) and (not only or e['id'] in only)]
    results = {}
    clock = 8 * 3600. + 86400  # the second day, past the first shift
    last, seen, left, spent, budget, jitter = {}, 0, 0, 0, 0, 0
    next_at, active, rest = 60., 0., 0.
    while night_of(clock) <= days:
        hour = (clock % 86400) / 3600
        is_night = hour >= 19 or hour < 6
        rate = NIGHT_RATE if is_night else DAY_RATE
        step = 5.
        clock += step * rate; active += step
        day = night_of(clock)
        if day != seen:
            seen = day; budget = left = rnd.randint(*g['budget_per_day']['minor']); spent = 0; next_at = max(active + 60, rest)
        if active < next_at or active < rest: continue
        lo, hi = g['tick_seconds']
        next_at = active + rnd.uniform(lo, hi)
        ready = []
        for e in minor:
            if day < e['from_day']: continue
            if e['hours'] == 'night' and not is_night: continue
            if e['id'] == 'roomlight_return' and rnd.random() > .05: continue  # a player has to have switched one off and left
            if e['id'] == 's03_failure' and is_night: continue
            if e['id'] in last and day - last[e['id']] < cool_days(e['id'], last[e['id']], *e.get('cooldown_days', [2, 4])): continue
            ready.append(e)
        if left <= 0 or not ready: continue
        # A new draw at each night; the small things keep to the night (the fault is the day's).
        if use_pace and is_night and not pace(spent, budget, hour, rnd.random()): continue
        pick = rnd.choice(ready)
        last[pick['id']] = day; left -= 1; spent += 1
        rest = active + g['rest_after']['other']
        results.setdefault(day, []).append((pick['id'], hour))
    return results, days - 1
def main():
    a = argparse.ArgumentParser()
    a.add_argument('--days', type=int, default=30); a.add_argument('--runs', type=int, default=200)
    a.add_argument('--without-hints', action='store_true'); a.add_argument('--no-pace', action='store_true'); a.add_argument('--seed', type=int, default=47)
    a.add_argument('--only', default='')
    o = a.parse_args()
    doc = director_rules.load(ROOT / 'Content/Director/director.json')
    errors, warnings = director_rules.check(doc)
    for e in errors: print('error:', e)
    rnd = random.Random(o.seed)
    total, days = 0, 0
    by_hour = [0] * 24
    empty_late = 0; nights = 0; empty = 0; early = 0
    for _ in range(o.runs):
        res, d = run(doc, o.days, rnd, not o.without_hints, not o.no_pace, set(filter(None, o.only.split(','))))
        days += d; total += sum(len(v) for v in res.values())
        for day, row in res.items():
            for _, h in row: by_hour[int(h)] += 1
        for day in range(2, o.days + 1):
            nights += 1
            if not any(h < 6 for _, h in res.get(day, [])): empty_late += 1
            if not any(h >= 19 or h < 6 for _, h in res.get(day, [])): empty += 1
            if any(h >= 19 for _, h in res.get(day, [])): early += 1
    print(f'small events per day: {total / days:.2f}  (budget {doc["globals"]["budget_per_day"]["minor"]})')
    print(f'nights with nothing small after midnight: {100 * empty_late / nights:.0f}%')
    print(f'nights with nothing small at all: {100 * empty / nights:.0f}%  with one before midnight: {100 * early / nights:.0f}%')
    row = lambda r: ' '.join(f'{h:02d}:{100 * by_hour[h] / max(1, total):3.0f}%' for h in r)
    print('share by hour:', row(list(range(19, 24)) + list(range(0, 7))))
    print('sketch only: no world, no player; sight and place are not modelled.')
if __name__ == '__main__':
    main()
