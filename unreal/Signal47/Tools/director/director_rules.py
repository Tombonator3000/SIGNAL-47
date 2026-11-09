#!/usr/bin/env python3
"""The one check of Content/Director/director.json: used by generate.py now, and by the editor when it comes.
Keep NEEDS and the reasons in step with S47Rhythm.cpp; S47Rhythm's self-test reads the same file."""
import json
SYSTEMS = {'director', 'indoor', 'site', 'taken', 'highway', 'sky', 'night'}
SLOTS = {'ground', 'background'}
LEVELS = {'hint', 'observation', 'presence', 'danger'}
BUDGETS = {'minor', 'invest', 'sky'}
TIERS = {'hint', 'glimpse', 'full'}  # TIERS-20261109, S47NightTiers.h
NEEDS = {'reference_filed', 'has_finding', 'no_power_fault', 'no_array_fault', 'roof_window_free', 'room_left_dark',
         'native_body', 'no_bodies_out', 'near_feeder', 'near_service', 'indoors', 'outdoors', 'door_found', 'peek_place',
         'flood_room', 'site_waiting', 'on_foot', 'not_driving', 'past_fence', 'driving'}
AREAS = {'saro', 'road', 'diner', 'station01', 'roswell'}
# Ids a save may name (S47FreeformDirector.cpp, Begin): one may be retired, never removed or renamed.
SAVED_IDS = {'extra_pulse', 'wrong_source', 'time_disagreement', 'duplicate_print', 'unsigned_telex', 'feeder_fault', 'landing_trace',
             's03_failure', 'roof_steps', 'empty_car', 'flyover', 'roomlight_return', 'response_lights', 'quiet_zone', 'observer',
             'service_presence', 'close_pass', 'feeder_presence', 'array_presence', 'farm_lights', 'town_lights', 'dusk_orb'}

def check(doc):
    """Errors stop the build; warnings say what Tom's rules would want to know."""
    errors, warnings = [], []
    def bad(text): errors.append(text)
    if doc.get('version') != 1: bad('version must be 1')
    g = doc.get('globals', {})
    hours = g.get('their_hours', {})
    if not (0 <= hours.get('earliest', -1) <= hours.get('latest', -1) <= hours.get('end', -1) <= 12): bad('their_hours: earliest <= latest <= end, past midnight')
    # The night's tiers (TIERS-20261109, S47NightTiers.h): clock hours a glimpse and a full meeting may start from, and the same from
    # the fourth week's first night. Optional; S47Rhythm.cpp turns away the same.
    tiers = g.get('tiers')
    if tiers is not None:
        night = lambda h: h + 24 if isinstance(h, (int, float)) and 0 <= h < 12 else h if isinstance(h, (int, float)) and 12 <= h < 24 else -1
        if not (18 <= night(tiers.get('glimpse_from', -1)) <= night(tiers.get('full_from', -1)) and 18 <= night(tiers.get('late_glimpse_from', -1)) <= night(tiers.get('late_full_from', -1))):
            bad('tiers: glimpse_from before full_from, in the night, also late_')
        if not isinstance(tiers.get('late_from_night'), int) or tiers['late_from_night'] < 1: bad('tiers: late_from_night a night')
    tick = g.get('tick_seconds', [])
    if len(tick) != 2 or not 0 < tick[0] <= tick[1]: bad('tick_seconds: [low, high]')
    rest = g.get('rest_after', {})
    for k in ('danger', 'other', 'taken'):
        if not isinstance(rest.get(k), (int, float)) or rest[k] < 0: bad('rest_after.' + k)
    budget = g.get('budget_per_day', {})
    for k in BUDGETS:
        r = budget.get(k, [])
        if len(r) != 2 or not 0 <= r[0] <= r[1]: bad('budget_per_day.' + k)
    limit = g.get('danger_per_night')
    if limit is not None and (not isinstance(limit, int) or limit < 0): bad('danger_per_night: null or a whole number')
    # LOOP-1-20261109: how many times a night SARO points the player out at the land (the bearing slip); S47Rhythm.cpp the same.
    point = g.get('point_out_per_night', 1)
    if not isinstance(point, int) or isinstance(point, bool) or not 0 <= point <= 3: bad('point_out_per_night: a whole number 0 to 3')
    # The build-up (Tom's critics, 8 November): one row a week from day 1, in order; S47Rhythm.cpp turns away the same.
    weeks = g.get('curve', {}).get('weeks', [])
    last = 0
    for n, w in enumerate(weeks):
        day = w.get('from_day')
        if not isinstance(day, int) or (n == 0 and day != 1) or (n > 0 and day <= last): bad('curve: weeks start on day 1, in order')
        last = day if isinstance(day, int) else last
        for k in ('hint', 'observation', 'presence'):
            if not 0 <= w.get(k, 1) <= 3: bad(f'curve week {n + 1}: {k} 0 to 3')
        if not 0 <= w.get('danger', -1) <= 1: bad(f'curve week {n + 1}: danger 0 to 1')
        if not .1 < w.get('near', 1) <= 2: bad(f'curve week {n + 1}: near above 0.1')
    if weeks:
        if weeks[0].get('danger', 0) > .1: warnings.append('curve: the first week is meant for small things and things seen, its danger above 0.1')
        if any(b.get('danger', 0) < a.get('danger', 0) for a, b in zip(weeks, weeks[1:])): warnings.append('curve: the danger falls from one week to the next')
        if any(b.get('near', 1) > a.get('near', 1) for a, b in zip(weeks, weeks[1:])): warnings.append('curve: things go further off from one week to the next')
    # The moon (MOON-20261108): what a night's levels and danger are worth at a full and a new moon; S47Rhythm.cpp turns away the same.
    moon = g.get('moon')
    if moon is not None:
        for k in ('full', 'new'):
            v = moon.get(k, 1)
            if not isinstance(v, (int, float)) or not .3 <= v <= 2: bad(f'moon: {k} 0.3 to 2')
        if moon.get('full', 1) > moon.get('new', 1): warnings.append('moon: a full moon is meant to be the quieter night')
    seen = set()
    for e in doc.get('events', []):
        i = e.get('id', '?')
        if i in seen: bad(i + ': twice')
        seen.add(i)
        if e.get('system') not in SYSTEMS: bad(i + ': system')
        if e.get('slot') not in SLOTS: bad(i + ': slot')
        if e.get('level') not in LEVELS: bad(i + ': level')
        if 'budget' in e and e['budget'] not in BUDGETS: bad(i + ': budget')
        if not isinstance(e.get('from_day'), int) or e['from_day'] < 1: bad(i + ': from_day')
        h = e.get('hours')
        if not (h in ('any', 'night', 'their_hours') or (isinstance(h, list) and len(h) == 2 and all(0 <= x <= 24 for x in h))): bad(i + ': hours')
        for a in e.get('areas', []):
            if a not in AREAS: bad(i + ': area ' + a)
        draw = e.get('draw', {})
        if 'per_night' in draw and not 0 <= draw['per_night'] <= 1: bad(i + ': draw.per_night')
        if 'at' in draw:
            at = draw['at']
            night = [x + 24 if isinstance(x, (int, float)) and x < 12 else x for x in at] if isinstance(at, list) else []
            if len(night) != 2 or not all(isinstance(x, (int, float)) for x in night) or not 18 <= night[0] < night[1] <= 30: bad(i + ': draw.at, clock hours in the night, from before to')
        cool = e.get('cooldown_days')
        if cool is not None and (len(cool) != 2 or not 0 <= cool[0] <= cool[1]): bad(i + ': cooldown_days')
        for n in e.get('needs', []):
            if n not in NEEDS: bad(i + ': needs ' + n)
        if not isinstance(e.get('enforce'), bool): bad(i + ': enforce')
        if 'tier' in e and e['tier'] not in TIERS: bad(i + ': tier is hint, glimpse or full')
        if 'early' in e and not isinstance(e['early'], bool): bad(i + ': early')
        if h == 'their_hours' and e.get('tier', 'full') != 'full': bad(i + ': what comes in their hours is full')
        # Tom, 9 November: the tiers hold every start; an hour window that opens before its tier says less than it seems.
        if e.get('tier') in ('glimpse', 'full') and isinstance(h, list) and not e.get('early'):
            start = h[0] + 24 if h[0] < 12 else h[0]
            if start < (23 if e['tier'] == 'glimpse' else 25): warnings.append(i + f": its hours open before its tier ({e['tier']}) in any week; the tier holds it back until then")
        # Tom, 7 November: the evening is a normal job. It says so; it does not stop anything, since two o'clock is no longer absolute.
        if e.get('level') == 'danger' and isinstance(h, list) and (h[0] >= 18 or h[0] < 1): warnings.append(i + ': a danger in the evening goes against Tom\'s rule that the evening is work')
    for i in SAVED_IDS - seen:
        if i not in seen: bad(i + ': a save can name this event, so it stays (mark it retired)')
    return errors, warnings

def load(path):
    with open(path, encoding='utf-8') as f:
        return json.load(f)
