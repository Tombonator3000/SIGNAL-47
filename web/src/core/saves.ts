// Saved games, after the save contract in the design bible (part 13), sized for a browser.
//
// Three cases. Each case keeps three autosaves that take turns and three manual saves.
// A save is the game state as JSON plus a little card for the menus (chapter, place,
// clock, play time, a small picture). Photographs are kept once in IndexedDB under a key
// made from their content, and saves point at them with 'idb:<key>', so an autosave never
// copies a 200 kB print. Without IndexedDB (some private windows) everything goes to
// localStorage instead, with the photographs inline; the menus say so when a save does
// not fit.
//
// A photograph that cannot be read shows a stand-in print. The original may still be in
// IndexedDB, so a later save writes the old reference back and never the stand-in (Codex
// found this in the first version; see evidence/claude-handoff-2026-10-04).

export const CASES = 3, AUTO = 3, MANUAL = 3;
export type SaveKind = 'auto' | 'manual';

export interface SaveMeta {
  id: string;          // `${caseId}:${kind}:${slot}`
  caseId: number;      // 1..CASES
  kind: SaveKind;
  slot: number;        // 0..AUTO-1 or 0..MANUAL-1
  savedAt: number;     // real time, ms
  playtime: number;    // seconds of active play in this case
  chapter: string;     // "Chapter 2: The Amended Record"
  place: string;       // "Records room"
  clock: string;       // the night's clock, "03:12"
  thumb: string | null;
  photos: string[];    // photograph keys this save points at
  version: 2;
}
interface SaveRecord<S> extends SaveMeta { state: S }
export type SaveCard = Pick<SaveMeta, 'chapter' | 'place' | 'clock' | 'playtime' | 'thumb'>;

const DB_NAME = 's47', DB_VERSION = 2, PHOTOS = 'photos', SAVES = 'saves';
const REF = 'idb:';
const LS = 's47.save.', LS_INDEX = 's47.saves', LS_ACTIVE = 's47.activeCase', LS_MIGRATED = 's47.migrated';
const OLD_CASE = 's47.case', OLD_CHECKPOINT = 's47.checkpoint';

type Photoish = { id: string; url: string };
const isPhoto = (v: unknown): v is Photoish =>
  !!v && typeof v === 'object' && typeof (v as Photoish).id === 'string' && typeof (v as Photoish).url === 'string';

// Every photograph in a state, wherever it sits (chapter one's prints, later chapters).
function photos(v: unknown, out: Photoish[] = [], depth = 0): Photoish[] {
  if (!v || typeof v !== 'object' || depth > 6) return out;
  if (isPhoto(v) && (v.url.startsWith('data:') || v.url.startsWith(REF))) { out.push(v); return out; }
  for (const x of Object.values(v as Record<string, unknown>)) photos(x, out, depth + 1);
  return out;
}

// cyrb53: a quick 53-bit string hash, plenty to tell photographs apart
function hash(s: string) {
  let h1 = 0xdeadbeef, h2 = 0x41c6ce57;
  for (let i = 0; i < s.length; i++) {
    const c = s.charCodeAt(i);
    h1 = Math.imul(h1 ^ c, 2654435761); h2 = Math.imul(h2 ^ c, 1597334677);
  }
  h1 = Math.imul(h1 ^ (h1 >>> 16), 2246822507) ^ Math.imul(h2 ^ (h2 >>> 13), 3266489909);
  h2 = Math.imul(h2 ^ (h2 >>> 16), 2246822507) ^ Math.imul(h1 ^ (h1 >>> 13), 3266489909);
  return (4294967296 * (2097151 & h2) + (h1 >>> 0)).toString(36);
}
export const photoKey = (url: string) => 'p' + hash(url);

// No timer here: building the world can block the main thread for seconds on a slow
// device, and a timer would win that race even when IndexedDB works fine.
function openDb(): Promise<IDBDatabase | null> {
  return new Promise((resolve) => {
    let done = false;
    const finish = (db: IDBDatabase | null) => { if (!done) { done = true; resolve(db); } };
    try {
      const req = indexedDB.open(DB_NAME, DB_VERSION);
      req.onupgradeneeded = () => {
        const db = req.result;
        if (!db.objectStoreNames.contains(PHOTOS)) db.createObjectStore(PHOTOS);
        if (!db.objectStoreNames.contains(SAVES)) db.createObjectStore(SAVES);
      };
      req.onsuccess = () => finish(req.result);
      req.onerror = () => finish(null);
      req.onblocked = () => finish(null);
    } catch { finish(null); }
  });
}

function idb<T>(db: IDBDatabase, store: string, mode: IDBTransactionMode, fn: (s: IDBObjectStore) => IDBRequest<T>): Promise<T | undefined> {
  return new Promise((resolve) => {
    try {
      const tx = db.transaction(store, mode);
      const req = fn(tx.objectStore(store));
      tx.oncomplete = () => resolve(req.result);
      tx.onerror = tx.onabort = () => resolve(undefined);
    } catch { resolve(undefined); }
  });
}
// a write that must succeed: resolves true only when the transaction completed
function idbWrite(db: IDBDatabase, store: string, fn: (s: IDBObjectStore) => void): Promise<boolean> {
  return new Promise((resolve) => {
    try {
      const tx = db.transaction(store, 'readwrite');
      fn(tx.objectStore(store));
      tx.oncomplete = () => resolve(true);
      tx.onerror = tx.onabort = () => resolve(false);
    } catch { resolve(false); }
  });
}

// A stand-in print when a photograph has gone missing (site data cleared in between).
function missingPrint() {
  const c = document.createElement('canvas');
  c.width = 508; c.height = 360;
  const g = c.getContext('2d')!;
  g.fillStyle = '#ebe5d3'; g.fillRect(0, 0, c.width, c.height);
  g.fillStyle = '#1b1a18'; g.fillRect(14, 14, 480, 300);
  g.fillStyle = '#d8d0bc'; g.font = '22px "Special Elite", serif'; g.textAlign = 'center';
  g.fillText('PRINT NOT AVAILABLE', 254, 170);
  return c.toDataURL('image/jpeg', 0.8);
}

const within = <V>(p: Promise<V>, ms: number, fallback: V) =>
  Promise.race([p, new Promise<V>((r) => setTimeout(() => r(fallback), ms))]);
const lsGet = <V>(k: string, d: V): V => { try { const v = localStorage.getItem(k); return v === null ? d : JSON.parse(v) as V; } catch { return d; } };
const lsSet = (k: string, v: unknown) => { try { localStorage.setItem(k, JSON.stringify(v)); return true; } catch { return false; } };
const lsDel = (k: string) => { try { localStorage.removeItem(k); } catch { /* blocked */ } };

export const saveId = (caseId: number, kind: SaveKind, slot: number) => `${caseId}:${kind}:${slot}`;

export class SaveStore<S extends object> {
  ready: Promise<void>;
  /** true when IndexedDB is not available and saves live in localStorage */
  fallback = false;
  private db: IDBDatabase | null = null;
  private metas = new Map<string, SaveMeta>();
  private stored = new Set<string>();                                  // photograph keys known to be in IndexedDB
  private unread = new Map<string, { shown: string; ref: string }>();  // photo id -> stand-in shown, reference kept
  private queue: Promise<unknown> = Promise.resolve();                 // writes run one at a time

  constructor(private migrateOld?: (checkpoint: string, oldCase: unknown) => { state: S; card: SaveCard } | null) {
    this.ready = this.init();
  }

  private async init() {
    // a browser that never answers must not hold up the title screen for ever
    this.db = typeof indexedDB === 'undefined' ? null : await within(openDb(), 10000, null);
    this.fallback = !this.db;
    if (this.db) {
      const all = await idb<SaveRecord<S>[]>(this.db, SAVES, 'readonly', (s) => s.getAll() as IDBRequest<SaveRecord<S>[]>) ?? [];
      for (const r of all) { const { state: _state, ...meta } = r; this.metas.set(r.id, meta); }
      const keys = await idb<IDBValidKey[]>(this.db, PHOTOS, 'readonly', (s) => s.getAllKeys()) ?? [];
      for (const k of keys) this.stored.add(String(k));
    } else {
      for (const id of lsGet<string[]>(LS_INDEX, [])) {
        const r = lsGet<SaveRecord<S> | null>(LS + id, null);
        if (r) { const { state: _state, ...meta } = r; this.metas.set(id, meta); }
      }
    }
    await this.migrate();
  }

  // The first web version kept one case (s47.case) and a checkpoint name (s47.checkpoint).
  // It becomes the newest autosave of case 1. The old keys stay where they are.
  private async migrate() {
    if (lsGet<number>(LS_MIGRATED, 0) >= 2 || this.metas.size || !this.migrateOld) { lsSet(LS_MIGRATED, 2); return; }
    const checkpoint = lsGet<string | null>(OLD_CHECKPOINT, null);
    if (!checkpoint) { lsSet(LS_MIGRATED, 2); return; }
    const old = this.migrateOld(checkpoint, lsGet<unknown>(OLD_CASE, null));
    if (old) {
      // the old prints were stored under 'frame01' and 'frame02'; keep pointing at them
      try { await this.write(1, 'auto', 0, old.state, old.card); this.setActive(1); }
      catch { return; } // try again next time
    }
    lsSet(LS_MIGRATED, 2);
  }

  // ---------- reading ----------
  list(caseId?: number): SaveMeta[] {
    return [...this.metas.values()].filter((m) => caseId === undefined || m.caseId === caseId).sort((a, b) => b.savedAt - a.savedAt);
  }
  latest(caseId?: number): SaveMeta | null { return this.list(caseId)[0] ?? null; }
  usedCases(): number[] { return [...new Set([...this.metas.values()].map((m) => m.caseId))].sort(); }
  freeCase(): number | null { for (let c = 1; c <= CASES; c++) if (!this.usedCases().includes(c)) return c; return null; }
  get active(): number | null { const c = lsGet<number | null>(LS_ACTIVE, null); return c && c >= 1 && c <= CASES ? c : null; }
  setActive(caseId: number) { lsSet(LS_ACTIVE, caseId); }
  /** the save Continue opens: the newest in the case played last, else the newest of all */
  continueSave(): SaveMeta | null { const a = this.active; return (a ? this.latest(a) : null) ?? this.latest(); }

  async load(id: string): Promise<S | null> {
    await this.ready;
    const r = this.db
      ? await idb<SaveRecord<S>>(this.db, SAVES, 'readonly', (s) => s.get(id) as IDBRequest<SaveRecord<S>>)
      : lsGet<SaveRecord<S> | null>(LS + id, null);
    if (!r) return null;
    const state = r.state;
    for (const p of photos(state)) {
      if (!p.url.startsWith(REF)) continue;
      const key = p.url.slice(REF.length);
      const read = () => this.db ? idb<string>(this.db, PHOTOS, 'readonly', (s) => s.get(key)) : Promise.resolve(undefined);
      const url = (await read()) ?? (await read()); // one more try: a failed transaction is often a one-off
      if (url) { this.stored.add(key); p.url = url; this.unread.delete(p.id); }
      else { const shown = missingPrint(); this.unread.set(p.id, { shown, ref: p.url }); p.url = shown; }
    }
    this.setActive(r.caseId);
    return state;
  }

  // ---------- writing ----------
  /** Write a save. `slot` 'rotate' picks the autosave to replace. Resolves with the new card when it is safely stored. */
  save(caseId: number, kind: SaveKind, slot: number | 'rotate', state: S, card: SaveCard): Promise<SaveMeta> {
    // copied now: the game goes on (or is reset) while the write waits its turn
    const copy = JSON.parse(JSON.stringify(state)) as S;
    const run = async () => {
      await this.ready;
      const s = slot === 'rotate' ? this.nextAuto(caseId) : slot;
      return this.write(caseId, kind, s, copy, card);
    };
    const p = this.queue.then(run, run);
    this.queue = p.catch(() => undefined);
    return p;
  }

  /** Autosaves take turns; one written less than a minute and a half ago is updated in place. */
  private nextAuto(caseId: number): number {
    const autos = this.list(caseId).filter((m) => m.kind === 'auto');
    const fresh = !this.generation.delete(caseId);
    if (fresh && autos.length && Date.now() - autos[0].savedAt < 90_000) return autos[0].slot;
    for (let i = 0; i < AUTO; i++) if (!autos.some((m) => m.slot === i)) return i;
    return autos[autos.length - 1].slot; // the oldest
  }
  /** The next autosave of this case starts a new generation (a chapter or an area begins). */
  newGeneration(caseId: number) { this.generation.add(caseId); }
  private generation = new Set<number>();

  private async write(caseId: number, kind: SaveKind, slot: number, state: S, card: SaveCard): Promise<SaveMeta> {
    const slim = JSON.parse(JSON.stringify(state)) as S;
    const keys: string[] = [];
    const toStore: { key: string; url: string }[] = [];
    for (const p of photos(slim)) {
      const lost = this.unread.get(p.id);
      if (lost && lost.shown === p.url) p.url = lost.ref;              // still the stand-in: keep pointing at the original
      if (p.url.startsWith(REF)) { keys.push(p.url.slice(REF.length)); continue; }
      if (!this.db) continue;                                          // no IndexedDB: the photograph stays inline
      const key = photoKey(p.url);
      if (!this.stored.has(key)) toStore.push({ key, url: p.url });
      p.url = REF + key; keys.push(key);
    }
    const meta: SaveMeta = {
      id: saveId(caseId, kind, slot), caseId, kind, slot, savedAt: Date.now(),
      playtime: Math.round(card.playtime), chapter: card.chapter, place: card.place, clock: card.clock, thumb: card.thumb,
      photos: [...new Set(keys)], version: 2,
    };
    const record: SaveRecord<S> = { ...meta, state: slim };
    if (this.db) {
      // photographs first, so a save never points at a print that is not there
      if (toStore.length && !(await idbWrite(this.db, PHOTOS, (s) => { for (const p of toStore) s.put(p.url, p.key); })))
        throw new Error('The photographs could not be stored.');
      for (const p of toStore) this.stored.add(p.key);
      if (!(await idbWrite(this.db, SAVES, (s) => { s.put(record, record.id); }))) throw new Error('The save could not be written.');
    } else {
      if (!lsSet(LS + meta.id, record)) throw new Error('Not enough room in this browser to save. Delete an older save.');
      lsSet(LS_INDEX, [...new Set([...this.metas.keys(), meta.id])]);
    }
    this.metas.set(meta.id, meta);
    // a new photograph under the same id has replaced the one that could not be read
    for (const p of photos(state)) if (p.url.startsWith('data:') && this.unread.get(p.id)?.shown !== p.url) this.unread.delete(p.id);
    void this.sweep();
    return meta;
  }

  async remove(id: string) {
    const run = async () => {
      if (this.db) await idbWrite(this.db, SAVES, (s) => { s.delete(id); });
      else { lsDel(LS + id); }
      this.metas.delete(id);
      if (!this.db) lsSet(LS_INDEX, [...this.metas.keys()]);
      await this.sweep();
    };
    const p = this.queue.then(run, run);
    this.queue = p.catch(() => undefined);
    return p;
  }
  async removeCase(caseId: number) { for (const m of this.list(caseId)) await this.remove(m.id); if (this.active === caseId) lsDel(LS_ACTIVE); }

  // Photographs no save points at any more are deleted.
  private async sweep() {
    if (!this.db) return;
    const used = new Set<string>();
    for (const m of this.metas.values()) for (const k of m.photos) used.add(k);
    const stale = [...this.stored].filter((k) => !used.has(k));
    if (!stale.length) return;
    if (await idbWrite(this.db, PHOTOS, (s) => { for (const k of stale) s.delete(k); })) for (const k of stale) this.stored.delete(k);
  }
}
