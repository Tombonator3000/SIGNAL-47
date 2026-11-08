#!/usr/bin/env python3
"""The one check of Content/Director/director.json: used by generate.py now, and by the editor when it comes.
Keep NEEDS and the reasons in step with S47Rhythm.cpp; S47Rhythm's self-test reads the same file."""
import json
SYSTEMS = {'director', 'indoor', 'site', 'taken', 'highway', 'sky', 'night'}
SLOTS = {'ground', 'background'}
LEVELS = {'hint', 'observation', 'presence', 'danger'}
BUDGETS = {'minor', 'invest', 'sky'}
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
        cool = e.get('cooldown_days')
        if cool is not None and (len(cool) != 2 or not 0 <= cool[0] <= cool[1]): bad(i + ': cooldown_days')
        for n in e.get('needs', []):
            if n not in NEEDS: bad(i + ': needs ' + n)
        if not isinstance(e.get('enforce'), bool): bad(i + ': enforce')
        # Tom, 7 November: the evening is a normal job. It says so; it does not stop anything, since two o'clock is no longer absolute.
        if e.get('level') == 'danger' and isinstance(h, list) and (h[0] >= 18 or h[0] < 1): warnings.append(i + ': a danger in the evening goes against Tom\'s rule that the evening is work')
    for i in SAVED_IDS - seen:
        if i not in seen: bad(i + ': a save can name this event, so it stays (mark it retired)')
    return errors, warnings

def load(path):
    with open(path, encoding='utf-8') as f:
        return json.load(f)
