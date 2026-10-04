# Rom 6 til kapittel 4

4. oktober 2026. Selvstendig område etter Claudes kontrakt i `todo.md` på `1a08434`. Nye kodefiler: `src/world/Room6.ts` og `tools/room6preview.py`. Claude eier kapittel 4, dialog, lagring, `World.ts` og samlet spilltest.

Rommet er 4,5 × 6 m med dobbeltseng, to stoler ved bordet, bordlampe, TV og innrammet foto på kommode, panelovn, lukket baderomsdør, nattbordtelefon, papirer/skoeske og askebeger. Nora sitter ved vinduet som en stilisert figur uten ansiktsdetaljer. Sørdøra er åpen mot en overbygd gangvei; nummer 6 tegnes i kode. Alt er bygget lokalt under `group`.

## Integrasjonskontrakt

```ts
import { Room6 } from './Room6';
const room6 = new Room6(new THREE.Vector3(0, 0, -8000));
scene.add(room6.group);
```

- `zones` og `colliders` er i verdenskoordinater. Rom, dør og gangvei overlapper med 0,95/0,70 m. `anchors.arrive`, `.talk` og `.exit` gir verdens-X/Z og yaw. Gulvet er lokalt y = 0.
- `proxies`: `door`, `nora`, `table`, `fieldCard`, `letter`, `correction`, `photo`, `window`, `phone`, `tv`, `bed`, `lamp`, `bathroom`.
- `objs.nora` og `objs.noraHead` er separate grupper. Hodegruppen kan roteres med `rotation.y`. De tre bevisene ligger separat i `objs.fieldCard`, `.letter` og `.correction`; kapittel 4 kan styre synlighet uten å bygge møblene på nytt.
- `interior` inneholder rommets innside og møbler. Vis/skjul hele `group` ved bytte mellom områder, også egne Hemisphere-/månelys. HemisphereLight er globalt virkende mens gruppa er synlig.
- `setLamp(on)` oppdaterer både High/Low-materialet og lampens flomlys. `update(dt, t)` bruker tid i sekunder til et svakt neonskjær. `dispose()` er idempotent og frigjør bare rommets egne geometrier, materialvarianter og CanvasTexture-ressurser.
- `export const motelFlood = floodSet(6, 'motel', 0.2)`; fire plasser er i bruk. Settets eierskap forutsetter ett aktivt Room6-objekt. Kall `dispose()` før en erstatning opprettes.
- Papirenes 3D-tekst er bare nøytrale titler. De endelige bevistekstene og samtalen skal fortsatt komme fra kapittel 4s UI/story.

To vesentlige feil ble rettet ved review: bordets brede proxy lå først foran papirene i raycast; toppen ligger nå under papirproxyene. Lampen blokkerte Noras hode fra samtaleposisjonen; den er flyttet til bordets andre hjørne med samsvarende lys og proxy.

## Selvstendig forhåndsvisning

```sh
cd web
python3 tools/room6preview.py --port 8482
```

Åpne `http://127.0.0.1:8482/`. Knapper: Door, Table, Nora, Window, High, Low, Lamp og Recreate. Scriptet skriver en egen entry og Vite-konfigurasjon under `/tmp`. Det endrer ikke `main.ts` eller prosjektets `vite.config.ts`, og åpner ikke nettleser eller kjører gamle spilltester. `--write-only` lager bare inngangsfilene.

## Verifisering

TypeScript-kontroll av hele `src`: PASS etter siste endring. Python-syntaks og Vite-transpilering av forhåndsvisningen: PASS. Ordinært produksjonsbygg: PASS, 34 filer og de samme 22 importerte rasterbildene. Forhåndsvisningen og nye grafikkfiler er holdt ute av spillpakken; Room6 er ennå ikke importert av spillinngangen.

CUA-kontroll av den isolerte romforhåndsvisningen: fire faste utsnitt i 1280 × 800 og de samme fire i faktisk bekreftet nettleser-/canvasstørrelse 844 × 390. Alle er uredigerte skjermbilder. To Low-utsnitt følger i tillegg. 33–52 draw calls i de målte utsnittene, mot budsjettet 120. Ingen konsolladvarsler eller feil fanget i kontrollen.

Alle 17 kontraktkontroller besto, inkludert virkelige `Interaction.pick`-kall på hvert papir med et inert bord registrert først, radiusbasert gangrute og frie ankere. Low, lampen og tre Recreate-runder besto. Ved samme utsnitt var GPU-tellerne stabile på 41 geometrier og åtte teksturer etter hver opprettelse, med én romgruppe og fire av seks flomlysplasser.

Bevis: `evidence/support-2026-10-04/room6/browser-verification.json`, faste JPG-er og `contracts-mobile.jpg`. Filhash, eksportmål og byggkontroll står i `evidence/support-2026-10-04/support-build-verification.json`.

Dette er ikke en kapittel-4-spilltest. Dialog, Save/Load, overgang fra kapittel 3, retur til SARO, fotoaksept, ekte mobil/fps og Safari krever Claudes integrasjon og samlede kontroll. Tidligere beståtte kapitteltester er ikke gjentatt her.
