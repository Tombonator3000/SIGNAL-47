# Voices of the Void — lokalt analyseoppsett

Voices of the Void **0.9.0n** er klargjort for lokal undersøkelse i SIGNAL / 47-repoet. Spillkopien, Ghidra og et importert prosjekt finnes under `private/`. Oppsettet bruker Ghidra **12.1.4** og Temurin JDK **21.0.12.1+1**. Original Steam-installasjon, kompatibilitetsoppsett og lagringer er bevart.

**Status:** Alle 111 kopierte spillfiler matcher originalen med SHA-256. De tre PAK-indeksene har gyldig SHA-1; 42 941 filnavn er listet. Shipping.exe er importert og lagret i Ghidra uten automatisk analyse. Ingen spillkode er dekompilert eller integrert i SIGNAL / 47 i dette steget. Se [EVIDENCE.json](EVIDENCE.json) for kontroller og begrensninger.

## Åpne det ferdige oppsettet

Kjør fra roten av den lokale SIGNAL / 47-mappen:

```bash
python3 Docs/Research/VoicesOfTheVoid/scripts/ghidra.py status
python3 Docs/Research/VoicesOfTheVoid/scripts/ghidra.py gui
```

I Ghidra: **File → Open Project**, velg `Docs/Research/VoicesOfTheVoid/private/projects/VotV_0_9_0n.gpr`, og åpne `VotV-Win64-Shipping.exe`. Prosjektet er klart for manuell undersøkelse og senere avgrenset analyse. GUI-start og dekompilert kode er foreløpig ikke verifisert. Launcher begrenser Java-heap til 2 GB, annonserer 2 prosessorer til JVM og setter analyseparallellitet til 2; dette setter ikke fysisk CPU-affinitet. Stor analyse kan fortsatt kreve mer minne eller tid. Full automatisk analyse av Unreal-programmet er ikke kjørt.

Lokale mapper:

| Mappe/fil under denne katalogen | Innhold |
|---|---|
| `private/game/0.9.0n/WindowsNoEditor/` | Verifisert spillkopi; filene er skrivebeskyttet |
| `private/projects/VotV_0_9_0n.gpr` og `.rep/` | Importert Ghidra-prosjekt |
| `private/reports/preparation.json` | Hele kontrollresultatet og lokale kildestier |
| `private/reports/source-files.json` | SHA-256 og størrelse for hver av de 111 spillfilene |
| `private/reports/pak-files.txt` | Full, validert liste med 42 941 PAK-filnavn |
| `private/reports/ghidra-import.json` og `.log` | Importkvittering og faktisk Ghidra-logg |
| `private/tools/` | Portable verktøy, nedlastinger, lisenser og isolerte innstillinger |
| `private/toolchain.json` | Verifiserte verktøyversjoner, nedlastingshash og lokale stier |

På Toms PC ligger denne katalogen i `/home/tombonator3000t/Documents/Codex/signal-47-threejs/Docs/Research/VoicesOfTheVoid/`.

## Hva pakken faktisk inneholder

- `VotV-Win64-Shipping.exe` er **native Windows AMD64 / PE32+**, kompilert med Unreal **4.27.2.0**. CLR-directory er tom; dette er ikke en .NET-assembly for ILSpy.
- Programmet refererer til `VotV-Win64-Shipping.pdb`, men ingen PDB følger med. Ghidra kan vise maskinkode og rekonstruert pseudokode; det gjenoppretter ikke original C++ med kommentarer og opprinnelige variabelnavn.
- `VotV-WindowsNoEditor.pak` bruker **PAK v11**. Indeksen er ukryptert og kan leses uten nøkkel. Dette sier ikke om hvert enkelt asset-payload er kryptert eller støttet.
- Filnavnlisten inneholder blant annet 19 429 `.uasset`, 19 693 `.uexp`, 2 360 `.ubulk` og 264 `.umap`. Ingen `.cpp`, `.c`, `.h`, `.hpp`, `.cs`, `.sln`, `.map` eller `.pdb` ble funnet i de løse filene eller PAK-navnelisten.
- Én `.uproject` og 110 `.uplugin` er metadata; de dokumenterer ikke at et redigerbart kildeprosjekt er levert. Pakken inneholder cooked Unreal-innhold. Ghidra-import av EXE alene undersøker ikke Blueprint-bytecode, assets eller scenegrafer.

PAK-verktøyet vårt leser bare footer og indekser. Det ekstraherer ingen payloads og støtter ingen nøkkelhåndtering. Det stopper ved kryptert indeks, ukjent layout, ugyldig kontrollsum, størrelse eller filsti. Native import ga forventede meldinger om Windows- og spill-DLL-er som ikke er importert i prosjektet; det er ikke utført analyse av disse avhengighetene.

## Opprett samme oppsett i en annen klone

1. Last ned den offisielle releasen via [utviklerens itch.io-side](https://mrdrnose.itch.io/votv) og [utviklerens arkiv](https://archive.votv.dev/games/votv/). URL, størrelse og SHA-256 er låst i [release.json](release.json). Pakk ut `090n.7z` lokalt og bevar medfølgende lisenser.
2. Last ned verktøyene fra URL-ene i [toolchain-pins.json](toolchain-pins.json), kontroller SHA-256, og pakk dem ut i `private/tools/ghidra_12.1.4_PUBLIC/` og `private/tools/jdk-21.0.12.1+1/`. Bevar `LICENSE`, `licenses/` og JDKs `legal/`. Ingen global Java-installasjon trengs.
3. Kjør oppsett og import fra reporoten:

```bash
python3 Docs/Research/VoicesOfTheVoid/scripts/prepare.py \
  --source "/sti/til/a09n/WindowsNoEditor" \
  --archive "/sti/til/090n.7z"
python3 Docs/Research/VoicesOfTheVoid/scripts/ghidra.py import
```

`--archive` er valgfri når arkivet allerede er kontrollert. `prepare.py` kontrollerer alle løse filer og lager en separat skrivebeskyttet kopi. Eksisterende kopier verifiseres og overskrives aldri. Import bruker `-noanalysis -max-cpu 2` og nekter å overskrive et eksisterende prosjekt. Prosjekter, logger og innstillinger skrives under `private/`.

PAK-indeksen kan undersøkes separat:

```bash
python3 Docs/Research/VoicesOfTheVoid/scripts/pak_inspect.py \
  Docs/Research/VoicesOfTheVoid/private/game/0.9.0n/WindowsNoEditor/VotV/Content/Paks/VotV-WindowsNoEditor.pak
python3 -m unittest discover -s Docs/Research/VoicesOfTheVoid/scripts -p 'test_*.py' -v
```

Bruk `--list` for filnavn, `--files` for navn i JSON og `--sha256` for full pakkehash. Hold eventuelle nye utdata i `private/`.

## Læring som kan brukes i SIGNAL / 47

Undersøk ett avgrenset system om gangen, eksempelvis signalbearbeiding, interaksjonsrekkevidde eller lagring. Først observeres faktisk spilloppførsel; deretter brukes metadata og avgrenset kodeanalyse til å forklare den. Filnavn alene beviser ingen algoritme eller spillmekanikk. Unreal-motorkode må skilles fra spillets egen logikk, og native analyse må skilles fra cooked Blueprint-innhold.

Skriv funn med kildeversjon, adresse/asset-identifikator, observasjon og usikkerhet. Beskriv ønsket oppførsel før en selvstendig Three.js-implementasjon avtales med Claude. Test SIGNAL / 47-implementasjonen mot prosjektets egne krav og bevar spillkonstantene.

Ingen lisens for gjenbruk av VotVs egen kode er etablert i denne gjennomgangen. Medfølgende FFmpeg/SVT-AV1-lisenser gjelder sine komponenter. En gratis nedlasting eller rekonstruert pseudokode dokumenterer ikke tillatelse til å kopiere spillkode eller assets inn i SIGNAL / 47. Direkte kode-/asset-gjenbruk må derfor avklares før innlemming. Observasjoner, egne analyseverktøy og en selvstendig implementasjon er den planlagte arbeidsformen.

**Git-innhold:** Repoet får våre skript, dokumentasjon, verktøypinner og kompakte kontrollbevis. Spillbinærer, pakker, utpakkede assets, dekompilert kode, verktøynedlastinger og Ghidra-prosjekter holdes i `.gitignore`-dekkede `private/`. Spillet er fysisk lagt i den lokale repomappen, men er ikke lastet opp til det offentlige GitHub-repoet.

## Kilder og avgrensning

Kontrollene er gjort mot de faktiske lokale filene 5. oktober 2026. Spillet er levert av EternityDev/MrDrNose; analyseverktøyene i `scripts/` er laget for SIGNAL / 47. Full original kildekode, manuell Ghidra-GUI, Blueprint-dekompilering og gjenbruk av spillkode er ikke verifisert eller levert.

- [Ghidra 12.1.4 og pinnede JDK-krav](https://github.com/NationalSecurityAgency/ghidra/blob/Ghidra_12.1.4_build/GhidraDocs/GettingStarted.md)
- [Eclipse Temurin 21.0.12.1+1](https://github.com/adoptium/temurin21-binaries/releases/tag/jdk-21.0.12.1%2B1)
- [Microsoft: PE-format](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format)
- [Epic: FPakInfo](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/PakFile/FPakInfo)
- [Epic: cooked innhold](https://dev.epicgames.com/documentation/en-us/unreal-engine/working-with-cooked-content-in-the-unreal-engine)
