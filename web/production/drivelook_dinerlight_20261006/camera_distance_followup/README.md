# Oppfølging: kamera som lysreferanse under kjøring

P2 fra [review av PR #82](https://github.com/Tombonator3000/SIGNAL-47/pull/82#discussion_r4196819016), 6. oktober 2026. Runtime-rettelsen `51d45fa` lar Ultra velge lamper rundt kameraet mens bilen kjører. Opptaksverktøyet målte fortsatt mot spilleren som står igjen der hen satte seg inn. Nye opptak kunne derfor rapportere falske kandidater for fjerne spotlys.

`tools/drivelook.py` registrerer nå både `world.driving` og kameraets XYZ i nye lysledger. Kjøring bruker kameraet; fot- og parkeringsdiagnoser bruker spilleren, også når kameraet er midlertidig flyttet eller ennå ikke er synkronisert etter utstigning. Grensen er fortsatt over 45 meter i XZ. Kandidater viser hvilken referanse og pose som er brukt, og er fortsatt ikke en visuell dom.

Gamle ledger mangler begge feltene. De beholder nøyaktig den tidligere spillerbaserte klassifikasjonen og resultatformatet. Hvis bare ett nytt felt finnes, eller kjørefeltet/kameraposen er ugyldig, blokkerer kontrollen. Ingen historiske manifest, funn, kontrollresultater eller bilder er omskrevet.

## Kontrollbevis

- `regression_checks.json`: 11/11 målrettede regresjoner PASS, inkludert faktisk JavaScript-observasjon mot API-dobler (2/2 tilfeller), og nøyaktig reproduksjon av alle 220 historiske bildestat-/fottrace-ledger. Et faktisk lagret fottilfelle med over 307 meter mellom kamera og spiller inngår.
- `existing_contract_checks.json`: uendret `tool_check.py`, 32/32 kontrakter PASS med rettet verktøy.
- `negative_control.json`: forventet FAIL, exit 1, mot verktøyet fra main `bb2c2cd` før rettelsen. Dette viser at regresjonene fanger den gamle feilen. Historisk replay er fortsatt 220/220 i denne negative kontrollen.
- `scope_checks.json`: Python-syntaks og diffkontroll PASS. Alle 236 eksisterende produksjonsfiler er byteidentiske med `bb2c2cd`; bare opptaksverktøyet og dette nye oppfølgingsmaterialet endres, i tillegg til eget loggavsnitt.

Ingen nye spillbilder eller tunge nettlesertester er tatt. Claudes tidligere grønne runtime-kontroll på `51d45fa` (Ultra 8/8 og kjøring 23/23) gjenbrukes fordi runtime er uendret. Den historiske pakken med 156 PNG-er viser fortsatt bygget før `51d45fa`. Faktisk ny opptakskjøring, visuell lysvurdering, ekte GPU/fps og lyd er UNVERIFIED. Claude eier eventuell senere samlet integrasjonsrunde.

## Gjenta den målrettede kontrollen

Med prosjektets Python-avhengigheter og Node på PATH:

```sh
python3 light_distance_check.py --result /tmp/dinerlight-distance-new.json
```

Resultatfila må være ny. Skriptet starter ingen nettleser. `ledger_observation_check.cjs` mottar det faktiske `DINERLIGHT_EXTRA`-uttrykket på stdin fra Python-kontrollen.
