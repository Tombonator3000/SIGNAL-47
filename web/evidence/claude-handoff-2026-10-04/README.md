# CaseStore: foto overskrives etter én lesefeil

Kontrollert kilde: `e5706fb46e731c74b2f3b801ee72a36013ba23e8`, `web/src/core/caseStore.ts`.
Samme feil og kandidat er kontrollert mot `0bb91706a4df462f6c7199726d67250bb6f17090`; kildefilen er uendret. Resultatene ligger i `latest-result.json` og `latest-candidate-result.json`.

Kildens SHA-256: `a29a2e604b64bcf88d554e91f5fff11008c4727d30f64433f3be777af89e6ecc`.

`reproduce.cjs` henter original TypeScript direkte med `git show` og transpilerer den i minnet med repoets TypeScript 5.6.3. Den mocker localStorage, IndexedDB og canvas. Ingen repo-filer skrives. Den ble kjørt med Node v24.19.0. Dette er en isolert lagringstest med syntetiske data-URL-er, ikke en nettlesertest.

## Resultat

`result.json` bekrefter `BUG_REPRODUCED`:

1. IndexedDB inneholder originalbildet under `f1`, og localStorage inneholder `idb:f1`.
2. Uten feil beholder vanlig load/save/reload originalbildet. Ingen `put` kjøres.
3. Ved nøyaktig én readonly-transaksjonsfeil er originalbildet fortsatt intakt i IndexedDB etter load. `CaseStore.preload()` har imidlertid satt `missingPrint()` som fotoets data-URL i cache.
4. Neste vanlige save skriver plassholderen over `f1` med `put`. localStorage peker fremdeles til `idb:f1`.
5. En ny CaseStore med vellykket lesing laster nå den lagrede plassholderen. Originalbildet er erstattet.

## Minimalt forslag

Behold den opprinnelige IDB-referansen separat når et bilde ikke kan leses. Vis fortsatt plassholderen i minnet, men serialiser referansen i stedet for å skrive plassholderen som et nytt foto. Ikke merk et utilgjengelig bilde som bekreftet lagret i `stored`.

```diff
   private stored = new Map<string, string>(); // photo id -> url known to be in IndexedDB
+  private unresolved = new Map<string, { url: string; ref: string }>();
   ready: Promise<void>;

-        if (url) { p.url = url; this.stored.set(id, url); } else p.url = missingPrint();
+        if (url) { p.url = url; this.stored.set(id, url); } else {
+          const ref = p.url;
+          p.url = missingPrint();
+          this.unresolved.set(p.id, { url: p.url, ref });
+        }

     for (const p of photos(slim)) {
       if (!p.url.startsWith('data:')) continue;
+      const unresolved = this.unresolved.get(p.id);
+      if (unresolved && p.url === unresolved.url) { p.url = unresolved.ref; continue; }
       if (this.stored.get(p.id) === p.url) p.url = REF + p.id;
```

`candidate-result.json` bekrefter `FIX_CANDIDATE_PASSED` for akkurat de samme to scenarioene. Forslaget ble bare anvendt på en kildestreng i minnet. Ved lesefeilen vises plassholderen fremdeles, mens neste save beholder `idb:f1` og lar originalbildet være urørt. En ny CaseStore leser deretter originalbildet. Kandidaten er ikke integrert eller browser-testet.

## Kjør igjen

```bash
node web/evidence/claude-handoff-2026-10-04/reproduce.cjs . e5706fb
node web/evidence/claude-handoff-2026-10-04/reproduce.cjs . e5706fb --check-candidate
```

Node-binæren som ble brukt her: `/home/tombonator3000t/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node`.

Scriptet avslutter med kode 0 når alle forventninger til valgt scenario er bekreftet, og kode 1 ved avvik. `BUG_REPRODUCED` er altså et bekreftet feilfunn, ikke en grønn produksjonstest.
