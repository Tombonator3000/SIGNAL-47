// The case (stage, notes, findings) is small JSON in localStorage. The photographs are
// JPEG data URLs of about 200 kB each, so they live in IndexedDB instead: localStorage
// holds about 5 MB per origin, and every Pages site under tombonator3000.github.io shares
// that origin. Where IndexedDB is missing or blocked (some private windows, sandboxed
// frames), the photographs stay inline in localStorage as before.
//
// Saving is synchronous for the caller. The IndexedDB write runs behind it, and
// localStorage only points at a photograph once that write has succeeded.

const KEY = 's47.case';
const DB_NAME = 's47', STORE = 'photos';
const REF = 'idb:';

type Photoish = { id: string; url: string };
const isPhoto = (v: unknown): v is Photoish =>
  !!v && typeof v === 'object' && typeof (v as Photoish).id === 'string' && typeof (v as Photoish).url === 'string';

// Every photograph in a case, wherever it sits (f1, f2, later chapters).
function photos(v: unknown, out: Photoish[] = [], depth = 0): Photoish[] {
  if (!v || typeof v !== 'object' || depth > 4) return out;
  if (isPhoto(v) && (v.url.startsWith('data:') || v.url.startsWith(REF))) { out.push(v); return out; }
  for (const x of Object.values(v as Record<string, unknown>)) photos(x, out, depth + 1);
  return out;
}

// No timer here: building the world blocks the main thread for seconds on a slow
// device, and a timer would win that race even when IndexedDB works fine.
function openDb(): Promise<IDBDatabase | null> {
  return new Promise((resolve) => {
    let done = false;
    const finish = (db: IDBDatabase | null) => { if (!done) { done = true; resolve(db); } };
    try {
      const req = indexedDB.open(DB_NAME, 1);
      req.onupgradeneeded = () => { req.result.createObjectStore(STORE); };
      req.onsuccess = () => finish(req.result);
      req.onerror = () => finish(null);
      req.onblocked = () => finish(null);
    } catch { finish(null); }
  });
}

function idb<T>(db: IDBDatabase, mode: IDBTransactionMode, fn: (s: IDBObjectStore) => IDBRequest<T>): Promise<T | undefined> {
  return new Promise((resolve) => {
    try {
      const tx = db.transaction(STORE, mode);
      const req = fn(tx.objectStore(STORE));
      tx.oncomplete = () => resolve(req.result);
      tx.onerror = tx.onabort = () => resolve(undefined);
    } catch { resolve(undefined); }
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

export class CaseStore<T extends object> {
  private cache: T | null = null;
  private db: Promise<IDBDatabase | null>;
  private stored = new Map<string, string>(); // photo id -> url known to be in IndexedDB
  // photo id -> the stand-in shown and the reference it replaced, for photographs that could
  // not be read. The original may still be in IndexedDB, so a save writes the reference back,
  // never the stand-in (found by Codex, see evidence/claude-handoff-2026-10-04).
  private unread = new Map<string, { shown: string; ref: string }>();
  ready: Promise<void>;

  constructor() {
    this.db = typeof indexedDB === 'undefined' ? Promise.resolve(null) : openDb();
    this.ready = this.preload();
  }

  // Read the case and its photographs before anyone presses Continue.
  private async preload() {
    let raw: T | null = null;
    try { const v = localStorage.getItem(KEY); raw = v ? JSON.parse(v) as T : null; } catch { raw = null; }
    if (!raw) return;
    let migrate = false;
    for (const p of photos(raw)) {
      if (p.url.startsWith(REF)) {
        // a browser that never answers must not hold up Continue for ever
        const db = await within(this.db, 10000, null);
        const id = p.url.slice(REF.length);
        const read = () => db ? idb<string>(db, 'readonly', (s) => s.get(id)) : Promise.resolve(undefined);
        const url = (await read()) ?? (await read()); // one more try: a failed transaction is often a one-off
        if (url) { p.url = url; this.stored.set(id, url); }
        else { this.unread.set(p.id, { shown: missingPrint(), ref: p.url }); p.url = this.unread.get(p.id)!.shown; }
      } else migrate = true; // an older save with the photograph inline
    }
    this.cache = raw;
    if (migrate) this.save(raw);
  }

  load(): T | null { return this.cache; }

  save(c: T | null) {
    this.cache = c;
    if (!c) { try { localStorage.removeItem(KEY); } catch { /* blocked */ } return; }
    const slim = JSON.parse(JSON.stringify(c)) as T;
    const pending: Photoish[] = [];
    for (const p of photos(slim)) {
      if (!p.url.startsWith('data:')) continue;
      const lost = this.unread.get(p.id);
      if (lost && lost.shown === p.url) p.url = lost.ref;           // still the stand-in: keep pointing at the original
      else if (this.stored.get(p.id) === p.url) p.url = REF + p.id; // already safe in IndexedDB
      else pending.push({ id: p.id, url: p.url });                  // inline for now, a reference once written
    }
    this.write(slim);
    if (!pending.length) return;
    this.db.then((db) => {
      if (!db) return; // no IndexedDB: the photographs stay inline
      return Promise.all(pending.map((p) => idb(db, 'readwrite', (s) => s.put(p.url, p.id)).then((r) => r !== undefined ? p : null)))
        .then((done) => {
          let changed = false;
          // a new photograph under the same id has replaced the one that could not be read
          for (const p of done) if (p) { this.stored.set(p.id, p.url); this.unread.delete(p.id); changed = true; }
          // write the slim version again, unless a newer save has replaced the case meanwhile
          if (changed && this.cache === c) this.save(c);
        });
    });
  }

  private write(v: T) { try { localStorage.setItem(KEY, JSON.stringify(v)); } catch { /* full or blocked: keep playing */ } }
}
