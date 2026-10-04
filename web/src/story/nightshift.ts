import type { DocSpec } from '../ui/UI';
import { clockText, pad } from './time';

// What Night Shift leaves in the control room besides the receiver (KAPITLER.md, Night
// Shift): the exceptions binder above the printer, the service record on the rack, the
// telex roll, Dale's interference card, the Halley's comet poster, and the operations
// terminal on the west desk with the survey files, the solver exceptions, the tape
// catalogue and the console log. Nothing here explains anything. Lines that start with
// "~ " are pencil on the paper (UI.document draws them by hand).

export const BINDER: DocSpec = {
  id: 'binder', title: 'Solver exceptions binder', kind: 'typed', stamp: 'EXCEPTIONS',
  page: `SARO RX/DSP
DIRECTION SOLVE - EXCEPTIONS
FILE COPIES. DO NOT REMOVE.

FROM SYSTEMS, 03/84:
NEGATIVE RANGES CANNOT BE GENERATED
BY THE SOLVER. RANGE IS COMPUTED FROM
DELAY AND IS UNSIGNED. ANY NEGATIVE
RANGE IS AN INPUT ERROR.
RECLASSIFY AND FILE HERE.
~ Then why does it keep doing it?

- - - - - - - - - - - - - - - - - -
DATE    09/22/81   01:52:07
FREQ    1420.405 MHz
PATTERN PULSE GROUP 4 / 7
SOURCE DISTANCE:  -34 LY
** CHECK SOLVE INPUTS **
RECLASSIFIED: INPUT ERROR
~ clock?

- - - - - - - - - - - - - - - - - -
DATE    03/02/83   03:10:44
FREQ    1420.405 MHz
PATTERN PULSE GROUP 4 / 7
SOURCE DISTANCE:  -36 LY
** CHECK SOLVE INPUTS **
RECLASSIFIED: INPUT ERROR
~ baseline file reloaded

- - - - - - - - - - - - - - - - - -
DATE    10/19/85   23:58:31
FREQ    1420.405 MHz
PATTERN PULSE GROUP 4 / 7
SOURCE DISTANCE:  -38 LY
** CHECK SOLVE INPUTS **
RECLASSIFIED: INPUT ERROR
~ third time. Ask Systems.

- - - - - - - - - - - - - - - - - -
DATE    07/--/47   02:17:00
FREQ    1420.405 MHz
PATTERN PULSE GROUP 4 / 7
SOURCE DISTANCE:  -00 LY`,
  transcript: `A ring binder from the shelf above the printer. The note from Systems is typed. The pencil under it looks like the hand in the shift log.

Three printouts in the same format as tonight's, from September 1981, March 1983 and October 1985. Each one is stamped as an input error.

The last sheet is on older, yellowed paper, with no stamp and no pencil. The date field says 1947. SARO's solver did not exist in 1947.`,
};

export const SERVICE: DocSpec = {
  id: 'service', title: 'RX bank 3 / Service record', kind: 'typed', stamp: 'MAINTENANCE',
  page: `SARO / MAINTENANCE
RX BANK 3 - SERVICE RECORD

09/23/81  K3 RELAY REPLACED. TRIPPED
          APPROX 1 MIN BEFORE LINE POWER
          FAILURE 09/22. NO FAULT FOUND.

03/03/83  K3 REPLACED. TRIPPED BEFORE
          POWER DIP 03/02. RELAY TESTS
          GOOD. REPLACED ANYWAY.

10/21/85  K3 REPLACED (3RD). TRIPPED
          40 SEC BEFORE OUTAGE 10/19.
          RELAY IS NOT FAULTY.
          IT TRIPS BEFORE, NOT AFTER.
                              M.O.`,
  transcript: `A clipboard on the rack, under the status lights. Maintenance has replaced relay K3 three times since 1981.

Each entry is the day after the trip. Each time, the relay tripped first and the power went after.`,
};

export function interferenceCard(locked: boolean): DocSpec {
  return {
    id: 'rfiCard', title: "Dale's card on the console", kind: 'typed', stamp: 'RX 3',
    page: `CALIBRATION
1419.900 MHZ  GAIN 45-65  BW 34-62

KNOWN INTERFERENCE
1420.110  HIGHWAY MICROWAVE RELAY.
          NOTCH.
1420.6    SATELLITE DOWNLINK,
          PASSES ONLY.

ANYTHING ELSE: LOG IT.`,
    transcript: `An index card, taped to the console so long ago the tape has gone brown.${locked ? '\n\nA pulse pattern of four and seven is not on it.' : ''}`,
  };
}

export function halleyPoster(image?: string): DocSpec {
  return {
    id: 'halley', title: "Halley's comet poster", kind: 'typed', stamp: '1986', image,
    page: `HALLEY'S COMET
APRIL 1986

CLOSEST TO EARTH APRIL 11
LOOK LOW IN THE SOUTH
EARLY APRIL: BEFORE DAWN
AFTER THE 12TH: AROUND MIDNIGHT
GET AWAY FROM TOWN LIGHTS
~ Public line: comet calls go to the planetarium in town. Not us. Not at 3 a.m.  E.W.`,
    transcript: `A printed poster from the spring, with a little chart of the southern sky and the comet's place for the 5th, 10th, 14th and 20th of April.

A note is taped in the corner, in the supervisor's hand.`,
  };
}

// The telex roll. Night Shift only has last evening's traffic on it; later chapters add
// their messages to the same roll (HISTORIE.md, threads 9 and 11).
export const TELEX_EVENING = [
  `ZCZC WX2250
SARO OPS
WX ADVISORY 2250 MST 13 APR 86
TSTMS W OF MAGDALENA MTNS MOVG NE 15 KT.
DRY LIGHTNING. NO PRECIP E OF RIO GRANDE.
NNNN`,
  `ZCZC NET2300
ALL SITES
NET STATUS 2300 MST
SITE 03 - NORMAL
SITE 11 - NORMAL
SARO - NORMAL
ALL OTHER SITES NORMAL
NNNN`,
];
export function telex(messages: string[]): DocSpec {
  return {
    id: 'telex', title: 'Telex roll', kind: 'typed', stamp: 'TELEX', page: messages.join('\n\n'),
    transcript: 'The roll on the teleprinter in the corner, torn off below the last message. A weather advisory at 22:50, and the network status at 23:00.',
  };
}

// ---------- the operations terminal ----------

export type LogLine = [number, string];   // clock (seconds after midnight), text

// Last evening, before the shift: already in the console log when the night begins.
const SEEDS: LogLine[] = [
  [22 * 3600 + 50 * 60, 'TELEX RECEIVED  WX'],
  [23 * 3600, 'TELEX RECEIVED  NET STATUS'],
  [23 * 3600 + 14 * 60, 'RX BANK 3  K3 OPEN'],
  [23 * 3600 + 15 * 60, 'LINE POWER  DIP 0.4 S'],
  [23 * 3600 + 41 * 60, 'SURVEY RUN OPENED  RUN860413_2341'],
];

export interface OpsView {
  clock: number;          // now
  solveAt: number | null; // when tonight's range came out negative, if it has
  log: LogLine[];         // tonight's lines after the seeds, in order
  future: boolean;        // RUN860414_0529.DAT is in the directory (All Night)
}

const MON = ['JAN', 'FEB', 'MAR', 'APR', 'MAY', 'JUN', 'JUL', 'AUG', 'SEP', 'OCT', 'NOV', 'DEC'];
const day = (clock: number) => (((clock % 86400) + 86400) % 86400) >= 12 * 3600 ? 13 : 14;
const stamp = (clock: number, secs = false) => `${day(clock)}-APR ${clockText(clock, secs)}`;
const START = 23 * 3600 + 41 * 60;

function runBlocks(clock: number) {
  const s = ((clock % 86400) + 86400) % 86400;
  const since = s >= 12 * 3600 ? s - START : s + 86400 - START;
  return Math.max(1, Math.floor(since / 9.6));
}

// Reel dates: the commissioning series was one reel a night from 25 June 1979, then the
// numbering runs on evenly to reel 1311, mounted the evening of 13 April 1986.
function reelDate(n: number) {
  const base = Date.UTC(1979, 5, 25);
  const late = Date.UTC(1986, 3, 13);
  const t = n <= 100 ? base + (n - 1) * 864e5 : base + 99 * 864e5 + (n - 100) * ((late - base - 99 * 864e5) / 1211);
  const d = new Date(t);
  return `${pad(d.getUTCDate())}-${MON[d.getUTCMonth()]}-${String(d.getUTCFullYear()).slice(2)}`;
}

export class OpsTerminal {
  private extra = new Set<string>();
  constructor(private view: () => OpsView) {}

  header() {
    const v = this.view();
    return `SARO OPERATIONS SYSTEM   V4.2        ${day(v.clock)}-APR-1986 ${clockText(v.clock, false)}\nTERMINAL 2   OPERATOR REYES`;
  }
  /** The commands offered as buttons; a few appear once the screen has shown why. */
  buttons() {
    return ['DIR SURVEY.RAW', 'SHOW EXCEPTIONS', 'SHOW TAPES', 'SHOW LOG', 'HELP', ...this.extra];
  }

  run(input: string): string {
    const v = this.view();
    const cmd = input.trim().toUpperCase().replace(/\s+/g, ' ');
    if (!cmd) return '';
    if (cmd === 'HELP') return 'COMMANDS\n  DIR SURVEY.RAW     SURVEY FILES\n  TYPE <FILE>        FILE HEADER\n  SHOW EXCEPTIONS    SOLVER EXCEPTIONS\n  SHOW TAPES [A-B]   RAW TAPE CATALOGUE\n  SHOW LOG           CONSOLE LOG\n  LOGOUT';
    if (cmd === 'DIR' || cmd === 'DIR SURVEY.RAW' || cmd === 'DIRECTORY' || cmd === 'DIRECTORY SURVEY.RAW') return this.dir(v);
    if (cmd.startsWith('TYPE ')) return this.type(v, cmd.slice(5).trim());
    if (cmd === 'SHOW EXCEPTIONS' || cmd === 'SHOW EXCEPTIONS/SOLVER') return this.exceptions(v);
    if (cmd === 'SHOW LOG' || cmd === 'SHOW LOG/TODAY') return this.log(v);
    if (cmd === 'SHOW TAPES') { this.extra.add('SHOW TAPES 040-050'); return 'RAW TAPE CATALOGUE   SARO   1979-1986\nREELS ON FILE: 001-046, 048-1311'; }
    const m = cmd.match(/^SHOW TAPES (\d{1,4})\s*-\s*(\d{1,4})$/);
    if (m) return this.tapes(+m[1], +m[2]);
    return `%OPS-W-UNKNOWN, unrecognized command\n \\${input.trim().split(' ')[0].toUpperCase()}\\`;
  }

  private files(v: OpsView) {
    const f: [string, number, string][] = [
      ['RUN860411_2340.DAT', 2210, '11-APR-1986 23:40'],
      ['RUN860412_2338.DAT', 2214, '12-APR-1986 23:38'],
      ['RUN860413_2341.DAT', runBlocks(v.clock), '13-APR-1986 23:41'],
    ];
    if (v.future) f.push(['RUN860414_0529.DAT', 0, '14-APR-1986 05:29']);
    return f;
  }
  private dir(v: OpsView) {
    const f = this.files(v);
    for (const [name] of f) this.extra.add('TYPE ' + name);
    const rows = f.map(([n, b, d]) => `${n.padEnd(20)}${String(b).padStart(6)}   ${d}`);
    const total = f.reduce((a, [, b]) => a + b, 0);
    return `Directory DUA1:[SURVEY.RAW]\n\n${rows.join('\n')}\n\nTotal of ${f.length} files, ${total} blocks.`;
  }
  private type(v: OpsView, name: string) {
    const f = this.files(v).find(([n]) => n === name || n === name + '.DAT');
    if (!f) return `%TYPE-E-OPENIN, error opening ${name}\n-RMS-E-FNF, file not found`;
    const [n, blocks, made] = f;
    const id = n.replace('.DAT', '');
    const open = n === 'RUN860413_2341.DAT';
    if (n === 'RUN860414_0529.DAT') return `SARO SURVEY RUN  ${id}\nOPENED    ${made}:00\nOPERATOR  REYES\nRECORDS   0\n\nFILE INCOMPLETE. RECORD NOT CLOSED.`;
    return `SARO SURVEY RUN  ${id}\nOPENED    ${made}:00\nOPERATOR  ${open ? 'REYES' : 'DALE'}\nRECORDS   ${blocks}\nSTATUS    ${open ? 'OPEN' : 'CLOSED 06:00'}`;
  }
  private exceptions(v: OpsView) {
    const rows = [
      '22-SEP-81  01:52:07  -34 LY   INPUT ERROR',
      '02-MAR-83  03:10:44  -36 LY   INPUT ERROR',
      '19-OCT-85  23:58:31  -38 LY   INPUT ERROR',
    ];
    if (v.solveAt !== null) rows.push(`14-APR-86  ${clockText(v.solveAt)}  -39 LY   UNCLASSIFIED`);
    return `DATE       TIME      RANGE    CLASS\n${rows.join('\n')}\n\n${rows.length} entries.`;
  }
  private tapes(a: number, b: number) {
    if (b < a) [a, b] = [b, a];
    a = Math.max(1, a); b = Math.min(1311, b);
    if (a > 1311) return 'NO REELS IN RANGE.';
    if (b - a > 19) b = a + 19;
    const rows: string[] = [];
    for (let n = a; n <= b; n++) if (n !== 47) rows.push(`${pad(n, 3)}   ${reelDate(n)}  ${n <= 60 ? 'SURVEY / COMMISSIONING' : 'SURVEY'}`);
    return `REEL  DATE       CONTENT\n${rows.join('\n')}`;
  }
  private log(v: OpsView) {
    const rows = [...SEEDS, ...v.log].map(([c, t], i) => `${stamp(c, i >= SEEDS.length).padEnd(17)}${t}`);
    return rows.join('\n');
  }
}
