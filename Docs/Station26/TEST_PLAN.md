# STATION 01 / P06–P09 — testdekning og åpne porter

## Utført i faktisk spiller

`Automation/run-station26-checks.sh` oppretter en disponibel profil og krever `--signal47-station26-checks`, profilnavnet `station26-test-*` og `ALLOW_STATION26_TEST`. Ingen vanlig brukersak brukes som testmål. En historisk fullført v1-sak kopieres; bare fotostier flyttes og lagringskonvoluttens digest beregnes på nytt. Originale JPEG-bytes bevares.

162 API-kontroller består både direkte ved 1280×800 og gjennom den nyutpakkede starteren ved 1600×900. Rapportene under `Evidence/Direct1280` og `Evidence/Packaged1600` inneholder hver enkelt kontroll og registrerte runtime-feil (ingen).

- Startmeny og ufullført P05 blokkerer avreise. Den fysiske folioen er tilgjengelig fra arkivbenken, og avreise kan avbrytes.
- Interaksjonsstråler fra spillerhøyde treffer transit, lampe, kabel og tidslogg. CharacterController går ni etapper rundt området, gjennom hytta og tilbake. Dette bruker bevegelses-API, ikke ekte tastetrykk.
- P07 før P06 og P06 før P07 prøves med en eksplisitt tilbakestilt testtilstand. Pause, åpen lampe, feil merke, feil kabelslutning og feil tidsvalg avvises.
- `FieldCamera.TryExpose()` bruker faktisk kameraposisjon, gyldig innramming og kameraets render-/eksportløp. Begge nye JPEG-er har metadata og innholdshash; ingen oppdiktet bildefil brukes til å passere fotokravene.
- Framkalling bruker den eksisterende våtbenkens handlinger etter retur. Notatboken gjenåpner begge originalene. Lagring og faktisk scenegjenstart bevarer område, kilder, funn, lampetilstand og alle fire framkalte/undersøkte foto.
- Skrivefeil simuleres ved å erstatte kun testprofilens checkpoint-sti med en mappe. Mislykket avreise bevarer SARO, feltframdrift og forrige varige checkpoint; rettet teststi tillater reise igjen.
- En faktisk kamerafangst kjøres med blokkert fotomappe. Returfolioen treffes fysisk og tilbyr lokal eksportretry. Gjentatt eksportfeil bevarer samme frame, filsti og pikselbuffer; etter reparert testmappe lagres originalen uten ny eksponering eller reise. Dette lukker en P1-feil funnet av den uavhengige kodegjennomgangen.
- Manglende grunnfoto fører til trygg retur til SARO og sperret avreise inntil reparasjon, uten tap av feltfoto. Manglende merkefoto nullstiller bare avhengige P06/P08/P09-funn og bevarer P07. Eksakte testfiler gjenopprettes og saken lastes på nytt.

## Regresjoner fra samme utpakkede kandidat

| Suite | Kontroller | Dekning |
| --- | --- | --- |
| WorldCase22 | 118 PASS | P04/P05, arkivframdrift, historiske foto, gjenoppretting og scenegeometri |
| Resources19 | 42 PASS | Eksisterende signaler, spillkonstanter, telefon, antenner og tidsstyrt hendelse |
| Recovery15 | 50 PASS | Start-/pausemeny, feil, avbryt, Previous Shifts og historisk v1-sak |

Regresjonene bruker isolerte profiler og gameplay-/meny-API. Recovery15 er kontroll av historiske v1-saker; en manuell Previous Shifts-runde med fullført feltsekvens er fortsatt en del av inputprøven nedenfor. Samlet [verifikasjon](Evidence/verification.json) kobler hver rapport til kilde, bygg, pakke, oppløsning og testprofil. Fem historiske kildefiler, åtte fotoposter fra de to feltkjøringene og alle 173 pakkefiler er kontrollert på nytt etter kjøring.

## Gjenstår før kandidaten kan bli standard

1. **Native input — UNVERIFIED:** vanlig gange, kamera, E, knapper, Tab/Escape, fokus, avreise/retur og Previous Shifts med en fullført feltprofil. Stråler og bevegelseskall erstatter ikke denne prøven.
2. **Blind forståelse og tid — UNVERIFIED:** en ny spiller begrunner markerings-, lampe-, kabel- og tidsfunn uten fasit; mål tid og noter steder vedkommende står fast. 5–6 timer er fortsatt et designmål for hele spillet.
3. **Ytelse — UNVERIFIED:** separat måling etter oppvarming, med bevart frametidsserie rundt STATION 01 og gjennom områdeskifte. Tidligere Visual10-/splatmålinger gjelder ikke Station26.
4. **Endelig historie, lyd og miljø — UNVERIFIED:** historisk originalnegativ, innspilt mottaksfragment, subjektiv lydmiks, videre SARO-/motellforløp og ferdig miljøkunst. Diagram og teksttranskript er eksplisitte prototyper.

Standardstarteren forblir Visual10. Native inngrep eller skjermlåsomgåelse brukes ikke for å lukke en testport.
