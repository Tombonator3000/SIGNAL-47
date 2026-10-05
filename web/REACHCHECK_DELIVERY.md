# Reachcheck som vakt

Vaktene for SARO, STATION 01, rom 6 og dineren gir **PASS, exit 0** på det faktisk kjørte bygget fra Claude-commit `c96e082`, med motellrettingen `b4d20cb`. Nye lommer, manglende startforbindelser og målte gulvhopp gir FAIL. Ufullstendig kontroll eller ukjent interaksjonsdekning gir UNVERIFIED, med exit 2 når ingen målt feil allerede krever exit 1.

Oppfølgingen er bestilt i `todo.md` på `c96e082`, kl. 00.52 UTC 5. oktober, og avtalt i SIGNAL / 47-samtalen. Gren: `codex/reachcheck-guard-20261005`. Codex eier bare `tools/reachcheck.py`, dette notatet, `production/reachcheck/` og eget loggavsnitt. Claude beholder spillrettinger og én samlet integrasjonstest.

## Kontrollregler

Gyldige punkter følger motorens faktiske firkantklarering, identisk med `Player.resolve`, og `Player.walkable`. Rutenettet har 0,15 m avstand. Rettede nabokanter testes med ekte `Player.update` i steg på høyst 0,04 m; endepunktavvik må være høyst 0,0001 m. Gulvhopp over 0,12 m rapporteres separat og fjernes ikke fra ganggrafen. Aktive interaksjoner prøves med den faktiske `Interaction.pick`, hele registeret og identiteten til det registrerte målet.

Tre dokumenterte, ubrukte lommer er EXPECTED. Hele komponenten må bestå av de opprinnelige, faste rutenettpunktene, med høyst det opprinnelige antallet. Boksene er ikke en generell tillatelse til nye punkter: også et nytt punkt inne i samme boks gir FAIL. Koordinattoleransen på 0,000001 m absorberer bare flyttallsstøy.

| Lomme | Verdenskoordinater, x / z i meter | Maksimalt antall |
|---|---|---:|
| Bak B-12-skiltet | x 8,4–10,65 / z -19,2–-17,55 | 56 |
| Hjørnet ved stasjonshytta | x -12,6–-12,3 / z 8002,65–8003,1 | 5 |
| Dinerens nordhjørne | x -8000,4–-7998,6 / z -8,4–-7,65 | 72 |

Dinerboksen er lokalt x -0,4–1,4, med verdensforskyvning x -8000. Nøyaktige punktrader, koordinater og begrunnelser ligger i verktøyet og komponentfunnene.

En smal, positiv råoverlapp gir PASS bare når begge sonene har dokumenterte, rettede bevegelsesbaner fra det eksakte startpunktet. Kompakte polylinjer og seed-vitner følger rapporten. Direkte toveis gange inne på en isolert øy kan ikke sertifisere adgang. Råoverlapp og rett gjennomgang rapporteres separat; en alternativ rute kan forbinde sonene.

**PASS gjelder vakten, ikke full interaksjonsdekning.** SARO og rom 6 har henholdsvis ni og tre inaktive kapittelobjekter, fortsatt UNVERIFIED. Dineren har ingen registrerte kapittelinteraksjoner: dekningen er UNVERIFIED. Den avtalte vaktforventningen krever både null registrerte interaksjoner og det eksakte settet med 13 navngitte treffflater. Ukjent label, ny tom interaksjonsliste eller endret proxiesett slipper ikke gjennom dette unntaket.

## Faktisk kjøring

Endelig kjøring startet **5. oktober 2026 kl. 01.26.57 UTC**, fullførte alle fire områder på **37,864 sekunder**, og returnerte **exit 0**. Ingen fangede `pageerror`, ingen uventede lommer og ingen passérbare gulvhopp over 0,12 m. Alle fire kompakte JSON-filer og PNG-kart er skrevet og hashkontrollert.

| Område | Nådde / gyldige punkter | Forventet ikke nådd | Aktive faktiske treff | Forbundne smale skjøter | Interaksjonsdekning |
|---|---:|---:|---:|---:|---|
| SARO | 208 161 / 208 217 | 56 | 34 / 34 | 3 | UNVERIFIED, ni inaktive |
| STATION 01 | 128 241 / 128 246 | 5 | 14 / 14 | 0 | PASS |
| Rom 6 | 705 / 705 | 0 | 11 / 11 | 0 | UNVERIFIED, tre inaktive |
| Dineren | 19 324 / 19 396 | 72 | Ingen registrerte | 3 | UNVERIFIED, 13 ubundne |

Enkeltfilbygget ble laget fra en isolert `git archive` av `c96e082`, uten kildeendringer. Vite-bygg: PASS, 192 moduler, 2,57 s. HTML: 18 556 096 byte, SHA-256 `c66c2af86d6a49b1b4c3eb393fccd986293a197ed68ff9f3fece0b7ed560a7aa`. Frosset verktøy-SHA: `cc5a2da23a9a5750fea144036f5400734405909503661ca9576531908ad6b2fa`.

Rapportens `git_base` er kontrollverktøyets checkout ved kjøring (`9a6270d`), ikke byggkildens commit. `validation.json` knytter den faktisk kjørte HTML-hashen til full Claude-commit. På den eldre, urettede `6097012` gir vakten fortsatt **FAIL, exit 1**, med alle 12 gulvhopp ved motellet. Det gamle bygget er ikke erklært grønt. PR #57 og #58 er nå flettet. Main `9fc1ab8` har byteidentiske runtimekilder og bygginnstillinger med det faktisk kjørte `c96e082`-snapshotet; den uendrede grønne kontrollen gjenbrukes. Et nytt enkeltfilbygg etter rebase på main er også byteidentisk med den allerede kjørte HTML-fila, PASS på 2,54 s. Sammenligningen er dokumentert i `validation.json`.

## Målrettet verifikasjon

- 40 JavaScript- og 28 Python-fixtures på endelig verktøy: PASS. Dekker nye punkter innenfor kjent boks, utvidede komponenter, isolerte skjøter, proxyidentitet, ukjente labels, statuspresedens og skriveavbrudd. Dette er syntetiske prøver, ikke kapitteltester.
- 38 kontroller av registrerte runtimebevis: PASS. Omfatter filhash, fullføring, tellinger, ekstern fullgrid, kompaktformat og kartdekoding. Separat lesekontroll bekreftet 14 hasher, alle 356 427 foreldrekanter, de 133 eksakte lommepunktene, 59 treffbevis og alle 12 rekonstruerte startpolylinjer.
- 12 skjøteruter fulgt på nytt fra eksakt start gjennom ekte `Player.update`, med bare én plassering per rute: PASS. Ingen `place` underveis; største endepunktavvik 5,37e-11 m. Bevis: `guard_join_replays.json`.
- Faktiske, midlertidige nettleserprøver: ny sperre i rom 6 gir FAIL; kastende telefonlabel og endret dinerproxy gir UNVERIFIED. Alle tre negative prøver: PASS. Dette er kontroll av motorens rapportstatus, ikke CLI-exitprøver. Bevis: `guard_runtime_qa.json`.
- Faktiske CLI-prøver: gammelt motelbygg gir exit 1; punktgrense 1 avviser hele rom 6 med exit 2. Disse ble kjørt før siste, rene legend-layoutrettelse; uendret sampling/status gjenbrukes. Direkte og symlinket fullgrid-mål under `production` avvises med exit 2 på endelig verktøy.
- Python-syntaks, diffkontroll og separat kodegjennomgang: PASS. Kartene er visuelt kontrollert; rom 6 har nå plass til hele tegnforklaringen.

Uendrede grønne spilltester gjenbrukes. Codex kjører ingen parallell full kapitteltestrekke; Claude eier den avtalte samlede integrasjonen. Programvare-rendering verifiserer ikke PC-/mobil-fps, ekte tastatur/berøring, manuell spillerforståelse, lydmiks eller Pages-publisering. Det endelige rutenettet er fortsatt en endelig sampling, ikke et bevis for alle kontinuerlige eller diagonale baner.

## Bruk og små rapporter

Fra `web/`, med Python, Playwright Chromium og Pillow >= 10.1 tilgjengelig:

```sh
python3 tools/reachcheck.py
python3 tools/reachcheck.py --url http://localhost:5173/SIGNAL-47/ --areas room6
python3 tools/reachcheck.py --self-test
```

`S47_URL` kan velge byggadresse. `--self-test` trenger også Node og verifiserer bare fixtures. Avvikende spacing er UNVERIFIED, men skjuler aldri målt FAIL.

Vanlige JSON-rapporter utelater `valid_indices`, `reached_indices`, `parents` og komponentenes indekslister. De beholder tellinger, funn, koordinater, forventninger, faktiske vitner og opphav. Kartene tegnes fra hele rutenettet før komprimering. Karttitlene sier «sampling» og henviser til `summary.json` for endelig leveransestatus.

Detaljer skrives bare ved uttrykkelig flagg, utenfor `web/production`:

```sh
python3 tools/reachcheck.py --full-grid --full-grid-dir /tmp/s47-reachcheck-detail
```

Uten katalogvalg brukes en midlertidig `s47-reachcheck-full`-mappe. Direkte og symlinkede stier inn i `production` avvises. `.full.json` er sampledokumentasjon og har ingen egen samlet PASS-status; den kompakte rapporten er autoritativ for vakt og skrivefullføring. Endelige detaljer fra denne kjøringen ligger lokalt i `/tmp/s47-reach-guard-full-c96e082`, og kan forsvinne ved tempopprydding.

De fire områdenes JSON er samlet rundt 185 kB. Hele `production/reachcheck/`, inkludert kart og aktuelle QA-notater, er rundt 412 kB. Tidligere fullgrid og daterte prøvebevis kan hentes fra commit `77b751f`; P2-reviewbevis fra `9a6270d`. Git-historikken er beholdt. Rapportene i arbeidstreet viser nå den aktuelle vakten.
