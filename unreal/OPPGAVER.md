# Oppgaver og arbeidsdeling (Unreal)

Felles liste for Claude (integrator og hjelpere) og Codex (ChatGPT). Hver oppgave har eier, gren og status. Les denne før du starter, og oppdater status når du begynner og når du er ferdig.

## Arbeidsdeling

- Codex: rendering, lys, skygger, materialer, post-prosess, importløypa for materialer.
- Claude-hjelpere: spillogikk, grensesnitt, lyd, plassering av modeller i verden.
- Claude (integrator): fletter inn i `unreal-port-core`, kjører kort testutvalg, pakker og oppdaterer `~/Games/Signal47`, holder logg og denne lista.
- Gjennomgang: når en side leverer, ser den andre over koden og bildene før fletting. Codex' leveranser gjennomgås av Claude og rettes i samme Codex-økt (`codex exec resume`); Claude-hjelpernes leveranser kan sendes til `codex review`.
- Filansvar: rør bare filene oppgaven gjelder. Delte filer (`S47HUD.cpp`, `S47Main.*`, `S47Story.*`, `S47Web.*`, `S47World.*`) endres minimalt. Ta inn `unreal-port-core` i grenen din før du leverer.
- Tester: det som er endret og `-S47SelfTest`. Ett Unreal-program om gangen per tung jobb: sjekk `pgrep -fa "UnrealEditor|UnrealBuildTool|Signal47"` før start.

## Pågår

| Oppgave | Eier | Gren | Status |
|---|---|---|---|
| Unreal-lys og skygger erstatter det gamle systemet (taklys fra armaturene, grønt CRT-lys foran skjermene, opplyste paraboler, ytelse 1440p) | Codex | `port-light` | under arbeid, rettelser sendt 22.10 |

## Neste

| Oppgave | Eier | Merknad |
|---|---|---|
| Grafikkpass med de 70 gratismodellene (`~/Assets/signal47-free`, `MODELLER.md`): pickup, biler, kontor- og radioutstyr, diner og motell, ørken; logoer fjernet; CREDITS i rulleteksten | Claude-hjelpere (plassering), Codex (materialer og lys) | etter at lyset er flettet inn |
| Matriseskriver og motellseng (finnes ikke gratis) | åpen | lages i Blender eller hentes fra Fab |
| Lyttetest av lyd og hel gjennomspilling med ekte input | Tom | |

## Ferdig 22.10

Menyer, grensesnitt i spillet, pipetonen, møll, 02:15-kameraet, terminalene på skjermene, musepeker på brettet, ut av bilen overalt, hopp, fri ferdsel, Unarius-filmene, den kjøpte signalprosessoren, tonegåten (TAPE A / 1947).
