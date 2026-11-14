# Plan: harness og agenter fra modell til ferdig produkt (ART-HARNESS-20261114)

**Status:** Plan, ikke startet · **Gjelder:** utseendet i Freeform (M6) fram til Freeform 1.0 (M7) · **Sist kontrollert:** 14. november 2026

Tom 14. november: «Lag en plan, harness pluss subagenter for planlegging, modellering, rendering og visuelle sjekker, til ferdig produkt.» Planen bygger på det som allerede virker: vindmølla (`Tools/landplaces`, `S47LandWindmill`, `-S47LandPlaceShotTest`), [arbeidsmalen for Blender og Unreal](BLENDER_BUILDING_WORKFLOW.md), grafikkprøven (`Scripts/graphics-review.py`, 20 kameraer) og nattskiftreglene (`q.py`, én motorjobb om gangen). Reglene i `AGENTS.md` gjelder: referansepakke først, én gjenstand om gangen, bakte PBR-kart, kritikk i spillet, høyst fire runder.

## Hva «ferdig produkt» betyr her

En gjenstand eller et sted er ferdig når alt dette holder, og Tom har sett bildene:

1. **Mål:** hovedmål innen 2 cm av spesifikasjonen, satt på tegnet bakke innen 2 cm (som vindmølla).
2. **Geometri:** trekantbudsjett etter bruk (5 000–20 000 for heltegjenstand), LOD for 40, 120 og 350 m, kollisjon som følger det som er tegnet (gangbokser, bilhindre).
3. **Flater:** bakte kart (farge, normal, ORM) i 4K, 2K for små ting; etiketter som egen tekstur; bare oppdiktede merker.
4. **Spillet:** alle kroker bevart (interaksjoner, lyd, spor, hendelser), det gamle som reserve med eget av-flagg, selvtest og regresjon grønne.
5. **Bilder i spillet:** 6 m, 40 m, 300 m og 1 km, dag, skumring og natt, side om side med referansen; to uavhengige kritikere godkjenner (eller fire runder er brukt og restfeil står i TODO).
6. **Ingen skade andre steder:** grafikkprøvens 20 kameraer natt og daggry uten nye feil.

## Køen (rekkefølge, milepæl)

| # | Arbeid | Grunnlag | Milepæl |
|---|---|---|---|
| 1 | Linjehytta | referansepakke og `GAMEPLAY.md` i `~/scratch-bm/land-places-20261114/line_shack` | M6, M1 |
| 2 | Esperanza-gruva | samme, `mine` | M6, M1 |
| 3 | Cairnet (varden) | samme, `cairn` | M6, M1 |
| 4 | JEEP-FORM | TODO JEEP-FORM-20261108; trenger referansepakke (ChatGPT) | M6 |
| 5 | MOTEL-REAL (kontoret først) | TODO MOTEL-REAL-20261108 | M6 |
| 6 | CALICHE-REAL | TODO CALICHE-REAL-20261108; ett bygg om gangen | M6 |
| 7 | Gatelys, gjerder og trafo, DALE-WINDOW, SARO-CLIP, rest fra T16-15 | TODO M6 | M6 |
| (8) | Båndstasjonen ved mottakeren (variant A) | bare hvis Tom vil etter REEL-DIRECT | M3 |

Punkt 1–3 kan starte straks (pakkene finnes). Punkt 4–7 trenger referansepakke først; den bestilles parallelt så køen aldri står.

## Rollene

Hovedøkten leder, fletter, bygger og avgjør. Agentene får koden og bildene, ikke hovedøktens konklusjon, og svarer med fil:linje og skille mellom sett og antatt.

| Rolle | Modell | Får | Leverer | Kan ikke |
|---|---|---|---|---|
| **Planlegger** | sonnet | referansepakke, `GAMEPLAY.md`, kildens kroker (C++ og props) | `spec.md`: mål i meter, deler og origo, hva som drives av kode, trekantbudsjett, kroker som må bevares, målbar akseptliste | modellere |
| **Modellør** | opus, egen worktree | `spec.md`, `lpkit.py`/`lpbake.py`, vindmølla som mønster | modellskript (`Tools/landplaces/<sted>/build_<sted>.py`), etiketter, `--dry`-tall | starte Blender eller Unreal selv (alt går gjennom renderer og `q.py`) |
| **Renderer** | haiku | kommandoene under | grå leire (fire rette og tre skrå vinkler), bake, eksport, `contract.json`, kontaktark ref mot render | vurdere |
| **Leire-kritiker A og B** | sonnet, uavhengige | referanse, kontaktark, `spec.md` | avvik med mål (proporsjon, silhuett, mangler) og om leira kan gå videre | se hverandres svar |
| **Integrator** | hovedøkten | godkjent eksport | import, C++-kroker, av-flagg, bygg, selvtest | |
| **Spillfotograf** | haiku | byggeriet | bilder i spillet (avstander, tider, nærbilder) og graphics-review-utsnitt | vurdere |
| **Spill-kritiker A og B** | sonnet, uavhengige | referanse, spillbilder, før-bilder | poeng per punkt i akseptlista, konkrete rettelser, «ferdig» eller ny runde | se hverandres svar |
| **Leser** | sonnet | diffen etter fletting | feil i kroker, tekst, Story-risiko | |

## Harnessen

En liten løkke over verktøyene som finnes, ikke et nytt system. Bygges som første steg (omtrent en halv dag):

- **`Tools/artloop/loop.py <sted> <steg>`**: steg `spec`, `clay`, `bake`, `export`, `import`, `shots`, `review`. Hvert steg kjører gjennom `q.py`, skriver bildene til `~/scratch-bm/artloop/<sted>/runde-<n>/` og en `status.json` (steg, runde, hva som feilet, hvilke bilder). Bruker `Tools/landplaces/run.py` og importoppskriften slik de er.
- **Kontaktark:** referansen og renderen i samme vinkel side om side, med målestokk, ett PNG-ark per runde. Det er det kritikerne og Tom ser.
- **Generell bildeprøve i spillet:** `-S47LandPlaceShotTest=<sted>` (finnes for vindmølla) utvides til alle steder via en tabell med kamerapunkter, så ingen ny prøve skrives per sted.
- **Kritikkskjema:** kritikerne svarer i JSON (`{punkt, ok, avvik_cm, rettelse}`), så hovedøkten kan slå dem sammen og se om runden er bestått uten å lese prosa.
- **Stoppregler:** høyst fire runder per fase; to kritikere må si «ferdig»; ellers står restfeilene i TODO, og køen går videre.

## Én runde, steg for steg

1. Planlegger skriver `spec.md`. Hovedøkten godkjenner (endrer mål hvis kilden sier noe annet).
2. Modellør skriver skriptet i worktree og kjører `--dry` (mål og trekanter, uten Blender).
3. Renderer kjører grå leire gjennom `q.py` og lager kontaktark.
4. Leire-kritikerne A og B svarer. Rettelser tilbake til modelløren. Høyst fire runder.
5. Renderer baker og eksporterer; kontrakten leses tilbake (mål, trekanter, LOD).
6. Hovedøkten importerer, kobler kroker, bygger, kjører selvtest.
7. Spillfotografen tar bildene; spill-kritikerne svarer. Rettelser i modell eller materialer. Høyst fire runder.
8. Leseren går gjennom diffen. Hovedøkten fletter, kjører regresjonen og graphics-review for de berørte kameraene.
9. Bilder til Tom.

Mens ett sted er i steg 5–8 (motoren), er det neste i steg 1–4 (ingen motor, bare Blender i kø). Slik står maskinen aldri stille, og det går aldri to tunge jobber samtidig.

## Rammer

- **Arbeidskopier:** høyst to modellør-worktrees samtidig (hovedkopi + to = tre, under grensen på fire). Avvikles når stedet er flettet.
- **Én tung jobb om gangen:** Blender og Unreal går gjennom `q.py`; ingen kø mens en vaktet prøve kjører. Spillet til Tom stoppes aldri (`pidof Signal47`).
- **Disk:** minst 100 GB ledig; arbeidsfilene (blend, høyoppløste bakekilder) i `~/scratch-bm/land-places-*`, utenfor Git; bare FBX/uasset i LFS.
- **Lange skift:** `systemd-inhibit` mot hvile; ingen ventesløyfer med `pgrep -f`.
- **Lisens:** bare egne modeller og CC0/CC-BY med kreditering; aldri Cosmos- eller rippede modeller som referanse.
- **Ærlighet:** hver leveranse sier skrevet, bygget, prøvd i spill, installert og sett av Tom hver for seg.

## Levering

- Installasjon etter hvert andre eller tredje sted: pakking, selvtest, røyktest, graphics-review natt og daggry for kameraene i nærheten, STATUS og logg. Behold gjeldende og to eldre pakker.
- Bilder til Tom omtrent hver time og ved hvert ferdige sted.

## Anslag

| Del | Tid |
|---|---|
| Harnessen (`loop.py`, kontaktark, kritikkskjema, generell bildeprøve) | ca. en halv dag |
| Hvert landsted (linjehytta, gruva, cairnet) | 4–8 timer hver, 1–2 parallelt |
| Jeep, motellkontor | 6–10 timer hver, etter referansepakke |
| Caliche-bygg | 3–6 timer per bygg |

## Tom avgjør

1. Rekkefølgen i køen (forslag: de tre landstedene først, så jeepen).
2. Om referansepakker for jeep, motell og Caliche skal bestilles nå (ChatGPT, Toms konto).
3. Om båndstasjonen ved mottakeren (variant A) skal inn i køen.
4. Om installasjon etter hvert andre eller hvert tredje sted.
