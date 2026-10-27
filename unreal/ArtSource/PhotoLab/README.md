# Fotolab, detaljmodell

Status: Blender 5.2.2 har generert ti redigerbare modelldeler, 23 616 trekanter og portable teksturer/FBX. Tre faktiske Blender-bilder er vurdert, og fersk gjenåpning består geometrikontroll samt frie lys-/vaskåpninger. Native import og spilltilpasning pågår; bildene er forfatterforhåndsvisninger med studiolys.

`build_photolab.py` lager ti originale tilleggsgrupper rundt det eksisterende laboratoriet fra 1986. Ingen opprinnelig node skal skjules. Om passformen krever fjerning av gammel geometri, må dette avgrenses separat før import.

| Gruppe | Innhold |
|---|---|
| WetCabinetDetail | Seks innfelte dørfronter, hengsler, håndtak og rullet metallkant på nordbenken. |
| RinseSink | Åpen vask i kjemikroken, med avløp, vannlås, kran, ventiler og bein rundt eksisterende kanner. |
| EnlargerDetail | Ventilert hus, negativholder, fokushjul, skala og åpen linsering. |
| ProcessDetails | Analog timerfront, brettkanter og målesylinder. FIX-bildets flate er fri. |
| StorageDetail | Hylleknekter, brettekanter, filmkartonger og forseglede kjemiflasker. |
| ArchiveDetail | Understell, skuffer og negativlommer ved eksisterende arkivbenk. |
| RecordsDetail | Borddetaljer, føtter og en liten konvoluttholder uten å dekke rapporten. |
| SafelightHousing | Hus, avstandsfester og rør rundt de originale lysglassene. |
| Ventilation | Skrudd ventilasjonsrist, ledningsrør og bryter ved vasken. |
| UtilityDetail | Rørklammer, koblingsbokser, sokkellist og hylleender. |

Materialene er originale prosedyriske PBR-teksturer med moderate overflatevariasjoner: grågrønn emalje, krem, børstet rustfritt stål, bakelitt, gummi, papir og kjemiplast. Ingen nedlastet geometri eller lisensierte tredjepartsassets brukes. Detaljeringen følger arbeidsstasjonene og holder gulvgangen fri.

## Kjøring

Kjør gjennom prosjektets eksisterende guard, med høyst én tung Blender/Unreal-jobb på maskinen. Foreldertasken samordner dette. Selve kommandoen etter ledighetskontroll:

```sh
blender -b -t 2 --python unreal/ArtSource/PhotoLab/build_photolab.py -- --render
```

Utdata blir `Signal47_PhotoLab_Detail.blend`, ti FBX-filer i `Export/`, `materials.json`, `model_contract.json`, egne teksturer og tre Blender-forhåndsvisninger. Forfatterobjektene beholdes separat fra sammenslåtte eksportobjekter. En egen samling har enkle plassholdere for eksisterende møbler og studiolys. Disse eksporteres ikke og viser ikke spillets belysning.

Mål: under 80 000 trekanter i samlet LOD0. Skriptet avviser større resultat. Faktisk tall finnes først etter kjøring. LOD-plan er tre native nivåer, omtrent 100/50/20 prosent. Vaskens og linsens åpninger må overleve reduksjonen; reell RT-belastning må kontrolleres.

## Importkontrakt

- Helperkoordinater er Three `(x, høyde, z)` i meter. Blender lagrer `(x, -z, høyde)`. FBX bruker prosjektets målte Office-oppskrift: `FBX_SCALE_UNITS`, frem `-Y`, opp `Z`. Mål en kjent kant ved native import.
- Eksportene har identitetsorigin og absolutt spillplassering. Ikke flytt geometrien en gang til i native transform.
- Nye detaljer får ingen egen kollisjon. Eksisterende benk-/hylle-/arkiv-/rapport-/kjemikrokkollisjon brukes. Kontroller at vasken ligger innenfor `x17.35..19, z-5.85..-5.15`.
- Ingen eksporterte lamper eller nye emissivflater. `S47LabLighting.cpp` driver fortsatt normal/safelight/enlarger og blackout. Bevar alle `yard.redGlass` og den opprinnelige plastnoden som brukes som lysenes gate.
- Bevar våtbildet `(15.4,1.003,-5.4)`, tørkebildene `(15+0.6i,1.99,-3)`, referansearket, rapporten, døren og gameplayproxyene.
- Forstørrerlyskilden ved `(17,1.49,-5.45)` trenger fri kjegle til basen. Ny linsering har ikke bunnlokk. Gammel linse beholdes.
- Ventilasjonsgeometri alene endrer ingen lyd. Koble videre til eksisterende rom-/strømlyd ved integrasjon.

Før levering: Blender-kjøring, fersk gjenåpning og bildeinspeksjon, native måling/material-/LOD-kontroll og kort fotolabprøve med fremkalling, lysmodus, strømbrudd og adgang. Full testmatrise kjøres samlet før spillfletting, ikke for hvert objekt.

## Metodekilder

Prosjektets `ArtSource/Office/build_office.py` gir helper-/FBX/PBR-oppskriften. Prosjektbibliotekets «Rom fra Blender til Unreal» er vurdert som arbeidsflytreferanse, merket UNVERIFIED; ingen kode er hentet derfra.

[arjun988/blender-skills](https://github.com/arjun988/blender-skills/tree/8f778d2405a214b508d4c7d80742be8e43acdd52) er lest på festet revisjon, MIT. `prop-artist`, `set-dressing` og `unreal-export` brukes som metodeveiledning for skala, meningsfull plassering og målt eksport. Ingen kode fra pakken eller MCP-installasjon er brukt. Generiske FBX-akser erstatter ikke prosjektets målte løype.
