# Reachcheck, gangbare områder og ting spilleren kan nå

Bestilt av Claude i `todo.md` gjennom PR #56 og avtalt med Tom i SIGNAL / 47-samtalen. Baseline: `609701226d3f1405276d4951f8dfb1d6ce25222c`. Gren: `codex/reachcheck-20261005`.

Codex eier bare `tools/reachcheck.py`, dette notatet, `production/reachcheck/` og eget loggavsnitt. Ingen spillkode, eksisterende tester, Unity-filer eller skrivebeskyttede prosjektkilder endres. Claude retter spillfunn og legger verktøyet inn i én samlet integrasjonstest før eventuell fletting.

Verktøyet undersøker SARO med fem åpne dører, STATION 01, rom 6 og dineren. Rutenettet har 0,15 m avstand. Flomfyllingen bruker rettede nabokanter bekreftet med den faktiske `Player.update`, og bruker gulvhøyden til å rapportere hopp over 0,12 m separat. Bratte kanter som spillet faktisk lar spilleren gå over, skal fortsatt være med i ganggrafen. Aktive ting undersøkes med den faktiske `Interaction.pick` fra nådde steder. Inaktive ting og manglende kroker må oppgis som UNVERIFIED.

Dette er en romlig kontroll gjennom spillets testkrok i en egen, midlertidig nettleserprofil. Den dokumenterer ikke manuell spillerforståelse, ekte tastatur/berøring, lydmiks eller fps på PC og mobil. De uendrede grønne spilltestene i PR #55 gjenbrukes; en full kapitteltestrekke gjentas ikke for et separat kontrollverktøy.

## Testgrunnlag

Typekontroll og enkeltfilbygg av baseline: PASS. Bygget er 18 555 832 byte og SHA-256 `ace95864919c8db4a9e8b9f1169ea43e7497e8e7881a688a62fa94528fdcd648`.

## Faktisk kjøring og funn

Alle fire områder fullført 5. oktober 00:33 UTC med Playwright Chromium og ANGLE swiftshader, på **20,716 sekunder totalt**. Ingen fangede `pageerror`, og alle fire JSON-rapporter og PNG-kart er skrevet og kontrollert. Oppsummeringen inneholder filhashene. Verktøyet returnerte **1, målte FAIL**, fordi det fant avvik i det forespurte rutenettet. Det er ikke en grønn spilltest.

| Område | Nådde / gyldige punkter | Aktive interaksjoner | Passérbare høydehopp > 0,12 m | Smale positive soneoverlapp |
|---|---:|---:|---:|---:|
| SARO | 208 334 / 208 516 | 34 / 34 PASS | 12 FAIL | 3 FAIL etter geometriregelen |
| STATION 01 | 128 260 / 128 463 | 14 / 14 PASS | 0, PASS | 0, PASS |
| Rom 6 | 705 / 711 | 11 / 11 PASS | 0, PASS | 0, PASS |
| Dineren | 19 336 / 19 417 | UNVERIFIED, 13 ubundne treffflater | 0, PASS | 3 FAIL etter geometriregelen |

**Funn Claude bør undersøke først:**

- Motellinnkjørselen: 12 nabokanter ved z 1,95 til 2,10 og z 9,90 til 10,05, x -27,30 til -26,55. Gulvhøyden hopper 0,14 til 0,44 m, og den faktiske bevegelsen passerte alle kantene i begge retninger. Dette er innkjørselen til motellet, ikke rekkverket ved nødutgangen.
- SARO: en komponent med 59 ikke nådde gridpunkter ved x 8,40 til 10,65, z -19,20 til -17,55. 56 av punktene er også frie etter motorens AABB-klarering; tre skyldes sirkel/AABB-forskjellen.
- STATION 01: seks ikke nådde punkter ved x -12,60 til -12,30, z 8002,65 til 8003,10. Fem er AABB-frie. Felttelefonen selv er nåbar.
- Dineren: 73 ikke nådde punkter ved x -8000,40 til -7998,60, z -8,40 til -7,65, altså lokalt x -0,40 til 1,40. 72 er AABB-frie. Ingen kapittelinteraksjoner er koblet inn i dineren ennå.

Dette er lommer som ikke ble nådd i det avtalte 0,15 m-rutenettet med fire kardinalnaboer. Det er ikke et bevis for at alle mulige kontinuerlige eller diagonale spillerbaner er stengt. Kart og koordinater lar Claude undersøke om lommene skal kunne nås, eller om sonene bør følge den tilsiktede geometrien bedre.

339 av totalt 472 røde punkter skyldes at briefen krever sirkelklarering mens `Player.resolve` bruker en firkant rundt spilleren. De er beholdt og merket i rapportene, ikke skjult for å få grønt resultat. Rom 6 har bare seks slike hjørnepunkter og ingen større isolert komponent.

**Smale skjøter betyr ikke seks nye usynlige vegger.** Alle seks sonepar har faktiske startforbindelser. Fem har også et rett toveis bevegelsesvitne; dinerens `walk`/`phone` nås via en annen rute. Råoverlappene bryter den bestilte grensen på mer enn 0,6 m i begge retninger, men rapporten skiller dette fra faktisk forbindelse. Parene er SAROs `landing`/`labPath`, `exitDoor`/`west`, motellets `walk`/`office`, og dinerens `lot`/`door`, `walk`/`inside`, `walk`/`phone`.

## Målrettet kontroll av verktøyet

- Python-syntaks og 17 adversarielle fixtures: PASS. Disse prøver blant annet fastlåst bevegelse, rettede kanter, en isolert øy, sirkel/AABB-forskjellen, den faktisk brukte gulvgrensen, målidentitet, ukjent label og tom interaksjonsliste. Raycast-fixturene er stubs; den faktiske Three.js-kontrollen er hovedkjøringen med 59 bevis fra `inter.pick`.
- 43 uavhengige rapportkontroller: PASS. Filhashene, unike punktsett, foreldre som tidligere nådde kardinalnaboer, komponentregnskap, start-/bevegelsesvitner, faktisk mål-ID/UUID og hit-avstand innen original rekkevidde er kontrollert. Dette er kontroll av registrerte bevis, ikke ny kjøring av alle kanter.
- Fire utvalgte ruter fulgt på nytt fra det faktiske startpunktet, med `Player.update` hele veien og uten nye `place` underveis: PASS. Våtbordet 146 punkter, arkivtelefonen 89 (aktivert i kapittel 2), felttelefonen 247, telefonen i rom 6 48. Endelig faktisk `inter.pick` traff hver registrert ting. Største endepunktavvik under 2e-12 m. Arkivtelefonen er fortsatt UNVERIFIED i standardrapportens kapittel 1; dette er en separat kapittel 2-prøve.
- Tilsiktede hindringer: PASS i egen prøve. Smugpunktene (7,2, -3) og (11,7, -3) er ikke gangbare; direkte tilnærming fra siden av nødutgangsrampa stoppet ved (-9,1, 4,43), før målet (-9,1, 5,7). Ingen passérbare høydefeil er funnet ved dette rekkverket. Generiske sirkel/AABB-hjørnepunkter ved rekkverket dokumenterer modellforskjellen, ikke at den tilsiktede avsperringen er feil.
- Negativ punktgrense: PASS. `--areas room6 --max-cells 1` ga exit 2, UNVERIFIED, fordi hele rutenettet på 2072 celler ble avvist. Ingen avkuttet del ble rapportert bestått.
- To uavhengige kodegjennomganger fant feil i statusoppsummeringen. Ukjent label, `pageerror` og mislykket kartskriving håndteres nå uttrykkelig før levering. Alle fire kart er inspisert; tettliggende navn kan overlappe på SARO, og de presise koordinatene står i JSON.

Kjøringer og uavhengige kontroller står i `production/reachcheck/summary.json`, `validation.json` og `targeted_replays.json`. Script-SHA ved kjøring: `d497c2b04e21e2d2d40ec71f240d2bc4baff407c4db83bf26d532b894b6dd0d6`.

## Bruk og integrasjon

Krever Python, Playwright med Chromium og Pillow (den størrelsesstyrte standardfonten krever Pillow 10.1 eller nyere). Node trengs bare til `--self-test`. De eksisterende lokale avhengighetene ble gjenbrukt; ingen ny pakke ble installert.

```sh
# Fra web/, etter bygging:
python3 tools/reachcheck.py --self-test
python3 tools/reachcheck.py

# Samme verktøy kan prøve et vanlig bygg under Pages-undermappen:
S47_URL=http://127.0.0.1:8047/SIGNAL-47/ python3 tools/reachcheck.py --out-dir production/reachcheck-pages
```

Standard er alle fire områder, 0,15 m og 540 sekunders tidsbudsjett. `--spacing` som avviker fra 0,15 gir UNVERIFIED. En manglende krok, ukjent label, avvist rutenett eller ufullstendig kjøring blir uttrykkelig merket. Exit 0 betyr PASS, 1 betyr minst ett målt FAIL, 2 betyr UNVERIFIED uten målte FAIL. Se hvert områdes `checks` ved blandede resultater.

`git_base` er lokalt HEAD ved oppstart, og HTML-hashen identifiserer faktisk innlest HTML. Et bygg eller en ekstern URL må ikke antas å tilhøre denne Git-revisjonen ut fra `git_base` alene. Denne leveransen ble kjørt mot det nye, uendrede bygget fra `6097012` med HTML-hashen over.

Ingen nødvendige geometri-/bevegelseskroker manglet. Standardrapportens ni inaktive SARO-ting og tre inaktive dokumenter i rom 6 er individuelt UNVERIFIED; senere tilstander er ikke gjennomspilt av verktøyet. Claude kobler inn dinerens 13 interaksjoner når All Night bygges og kan kjøre kontrollen i den tilstanden. Claude vurderer og retter de målte spillfunnene, og eier én samlet integrasjonstest. Pages-kjøring med dette nye verktøyet, ekte input og PC-/mobil-fps er fortsatt UNVERIFIED hos Codex.
