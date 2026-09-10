# SIGNAL / 47

Førstepersons kosmisk etterforskning ved SARO i New Mexico, 1986.

## Spill på Kubuntu

Start `./Spill-SIGNAL47.sh` i denne prosjektmappen. Startfilen peker fast på kapittelpakken `Artifacts/Releases/Chapter09-86a28b30ad94/`, slik at et senere utviklingsbygg ikke endrer hva du åpner.

Pakken kan også flyttes: pakk ut `Artifacts/Releases/SIGNAL47-Chapter09-86a28b30ad94-Linux.tar.gz` og kjør `./Start-SIGNAL47.sh` i den utpakkede mappen. Arkivet bevarer kjørerettigheter. Spillpakkene er lokale leveranser; de følger ikke med et rent Git-klon.

Velg **START NIGHT SHIFT** eller **CONTINUE CHECKPOINT**. WASD beveger, mus ser rundt, E bruker fysiske ting, Tab åpner notatboken og Escape lukker eller pauser. C åpner kamerasøkeren; Space eksponerer. Filmen må framkalles i den nordlige fotolaben før bildet kan undersøkes. Lydnivå, musefølsomhet og fullskjerm finnes i SETTINGS og lagres. Fullskjerm bruker skjermens opprinnelige oppløsning; vindusstørrelsen huskes.

## Første kapittel: Den andre eksponeringen

Den eksisterende nattevakten fortsetter gjennom S-03, en fysisk fotolab, bildeundersøkelse, referansekart, hypotese, et kontrollforsøk ved B-12 og en ny lokal avslutning. To forsøksmetoder gir forskjellig fysisk oppsett, observasjon og begrunnet konklusjon. Feil forklaringer gir en vei videre. De to fotografiene kommer fra dine faktiske eksponeringer i spillscenen og kan åpnes igjen, forstørres og sammenlignes.

Lagring blir tilgjengelig etter telefon-/antenneforløpet. Kapitlet lagrer ved sammenhengende kontrollpunkter; pausemenyen tilbyr SAVE CHECKPOINT og QUIT. Normal avslutning venter på lagringen. Saken ligger i `~/.config/unity3d/SARO/SIGNAL 47/Chapter09/`; det opprinnelige fotoarkivet ligger i `~/.config/unity3d/SARO/SIGNAL 47/FieldPhotos/`. Nytt spill nullstiller aktiv sak og bevarer eksportene. En skrivefeil viser status og lar deg velge å avslutte uten de siste endringene.

Dette er en spillbar alpha med første kapittels samlede forløp. Materialer og enkelte møbler har fortsatt et enkelt uttrykk; motell, bil og den større Roswell-historien er senere innhold. Førstegangsvarighet og subjektiv lydmiks er ikke bekreftet av ekstern spilltesting. Se [kapittelrapporten](Docs/CHAPTER_09.md) for bindende kontrollpunkter, faktiske målinger og begrensninger, og [overleveringen](Docs/CURRENT_HANDOFF.md) for kilde-/byggidentitet.

Forrige fungerende feltkamera-utgave er beholdt i `Artifacts/Releases/FieldCamera08-af5a85d/` og `SIGNAL47-FieldCamera08-Linux.tar.gz`. Historiske resultater står i [pass08](Docs/FIELD_CAMERA_08.md), [servicegård07](Docs/SERVICE_YARD_07.md), [kontrollrom06](Docs/CONTROL_ROOM_PASS_06.md) og [verden05](Docs/WORLD_AREA_PASS_05.md).

## Kilde, bygg og kontroll

Behold Unity 6000.3.22f1, URP 17.3.0, Input System 1.20.0 og Blender 4.5.13 LTS. Åpne `Unity/` i Unity Hub og scenen `Assets/Signal47/Scenes/Prototype/SARO_Prologue.unity`. Originale modeller, redigerbare Blender-kilder og Unity-metadata er bevart. Se [kreditering og lisenser](Docs/THIRD_PARTY_NOTICES.md), inklusive VT323 og Scott Buckley.

```sh
# Utviklingsbygg og eksisterende regresjon:
bash Automation/build-linux.sh
# Release-bygg og full kilde-/innholdsstempling:
bash Automation/build-gauntlet-linux.sh
# Eksempel: normal ny reise med virkelig tastatur/mus i isolert profil:
python3 Automation/run-chapter09.py --name MyChapterRun --profile Artifacts/Chapter09/Profiles/MyChapterRun -- --phase all --method active --quit
# Pakk bare kildene som er verifisert mot samme byggmanifest:
python3 Automation/package-chapter09.py --expect-source 86a28b30ad94fc9e67aa0bf0558f0a51f511adadc76323fcf69eca7c72d8c8b9
```

Bygging krever den installerte Unity-editoren og Linux-støtte; `UNITY_EDITOR` kan angi editorbanen. Native input krever grafisk sesjon og `/dev/uinput`. Testløperen låser skjermstyringen og avviser en annen kjørende spiller. Bruk et nytt navn og en isolert profil for hvert nytt forsøk. `--signal47-gauntlet` gir observasjon, skjermbilder og målinger; normal spilling bruker ikke dette flagget. Direkte smoke-/tilstandstester og feilinjeksjonskopier er separate fra beviset for brukerreisen.

`Artifacts/GauntletLinux/` er ordinær byggoutput. En ny endring krever berørt verifisering, ny kildeidentitet og ny entydig pakke. Eksisterende pakker skal bevares. Ingen ny merge eller offentlig publisering er gitt mandat i dette oppdraget.
