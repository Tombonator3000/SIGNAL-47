# Linux-distribusjon 11. november 2026

**Status:** Gjeldende. **Gjelder:** Linux x86_64, UE 5.8.4, offentlig testutgave 0.9.58. **Sist kontrollert:** 11. november 2026.

## Bygg

Kilde `f16655f0` (versjon 0.9.58, samme som installert d58), kokt og pakket for seg med `Scripts/package-linux.sh`. Tre lokale tredjepartsklipp ble lagt midlertidig til side under kokingen og lagt tilbake etterpå: Unarius-TV-programmet (bilde og lyd; rettighetene er ikke avklart) og `alien_interference.wav`. Pakkens manifester er kontrollert uten dem; `Scripts/distribute-linux.py` nekter nå å pakke hvis de står i manifestet. Utgivelsesverktøy og starter fra `80afab19`.

## Kontroller

- Pakket scenariomatrise (`Scripts/screenshots.sh`): 112 av 112. `starhole` feilet først fordi forventningen bare godtok et hull på vei opp (0,1–0,9) mens hullet stod helt åpent (1,000). Forventningen er utvidet, og tilfellet ble kjørt alene på nytt: PASS.
- Freeform-reisen (`-S47FreeformJourneyTest`, editor i spillmodus, isolert profil): PASS, 59 kontroller, 4 Flow-gjenopprettinger. Den erstatter Story-nattkontrakten som utgivelseskrav; den krever en fullført SC01-lagring som fixture, og Story bygges om på Freeform (DECISIONS 11. november).
- Selvtest 4764 og regresjon 251 på samme kilde.
- ZIP: 49 oppføringer lest tilbake med SHA256 og kjørebiter; delt UCAS settes sammen til testet hash. Alle tre arkivene er pakket ut i en tom mappe og startet gjennom `Play.sh`: sammensetting, selvtest 4764 og røykprøve OK, tittelbilde med versjon 0.9.58.
- Arkiver: 1,317 / 1,781 / 1,807 GB, under GitHubs grense per fil. SHA256 i `SHA256SUMS.txt` på releasen.

## Avgrensning

Ikke prøvd på en annen PC eller GPU. Nattkontrakten for Story er ikke kjørt. Menneskelig spilling og lyd er ikke godkjent; se TODO «Ferdig, venter på Toms test». Bensinstasjonens pumpetekstur og skråninger er åpne.
