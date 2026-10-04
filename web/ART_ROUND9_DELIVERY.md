# Runde 9: datakart for Ultra

Ferdig assetleveranse etter runde 9 i `ART_BRIEF.md`: 19 normal-/ruhetspar, 38 kart. Claude eier innkoblingen og samlet spilltest. Fargebildene er låst mot `7e0aad8`; leveransegrenen bygger på Ultra i main `f458943`, PR #50. Ingen av de 19 eksisterende fargebildene er endret.

## Kontrakt

Alle kart er 512×512. Normalene er 8-bit RGB PNG uten alfa eller fargeprofil, tangentrom med OpenGL +Y. Ruhet er JPEG kvalitet 90, én gråtonekanal, lineære verdier. Filer ligger ved siden av fargebildet med `_n.png` og `_r.jpg` som ending.

Innebygd ChatGPT-bildegenerator lager kildeforankrede høydeskisser. Eksakt modellversjon er ikke eksponert av verktøyet. Skissene er støtte til materialtolkning, og kan regularisere geometri; derfor hentes synlige fuger, sprekker og sømmer fra de uendrede albedo-pikslene i den deterministiske datakartberegningen. Pigment, trykk og flekker skal ikke bli til høyde. Det brukes ingen nedlastede bibliotekteksturer eller ekstern betalt tjeneste.

## Integrasjon hos Claude

Last bare i Ultra. Datakart skal bruke `THREE.NoColorSpace`, mens fargebildene beholder sRGB. Ruhetsverdiene er absolutte; vurder `material.roughness = 1` når `roughnessMap` brukes, fordi three.js multipliserer kartet med materialfaktoren. Three.js leser ruhet fra grønn kanal; gråtonefilene gir samme verdi i RGB etter dekoding. Se [MeshStandardMaterial-dokumentasjonen](https://threejs.org/docs/pages/MeshStandardMaterial.html). Kartene må ha identisk repeat, offset, rotation og orientering som albedo. Asfalt og grus som tegnes i egne veicanvas, krever samme sammensetting av datakartene som fargebildet, inkludert underlag, spor og veimerking. Et kvadratisk datakart direkte på den ferdige veiteksturen blir feil.

Ved overgang til High/Low fjernes kartene. Delte kart skal følge loaderens eierskap og ikke disponeres av et enkelt område. `tools/ultra.py`, Pages fra undermappe og faktisk PC-ytelse eies av integrasjonen.

## Kontrollstatus

`src/assets/art/production/round9_manifest.json` inneholder alle 19 kildeidentiteter, 38 karthasher, fullstendige genereringsprompts, verktøyidentitet, visuelle vurderinger og lenker til kontrollbevis. De 19 uendrede albedo-hashene er verifisert mot `round9_sources/albedo_baseline.json`.

- Format, normalvektorer, helningsgrenser, JPEG90, materialkontrakt og analytisk +Y: 19/19 PASS. Vektorlenge 0,99409–1,00584, maksimal helning 39,694°, minste blå kanal 226.
- Frosset konverter etter P2-rettingen nedenfor: alle 19 par reprodusert; 38/38 utdata byteidentiske. Verktøyet er `tools/round9_maps.py`, SHA256 `be4c93adb4e542887ca0d74fa4237d65af8a8b008321333cee7c0ad5b89d9fe6`, med Pillow 12.1.1. Kontrollverktøyets hash er i manifestet.
- Visuell kontroll av kilde, maske, 50 % overlegg, 2×2, høyde, ruhet og plan med lys ovenfra: 19/19 PASS innenfor begrensningene nedenfor. Grus har to hovedspor; gulvbord har ti horisontale bord; stolknapper er senket; svart og krem på sjakkgulvet har samme høyde.
- Privat three.js-visning: alle 19 materialer lastet, tre teksturer per materiale, ingen fangede advarsler eller konsollfeil. 2×2 bekreftet i grensesnittet og inspisert for heksagulv, asfalt og kunstskinn. Bevis: `round9_qa/browser_loads.json` og tre nettleserbilder. Dette er materialkontroll, ikke spillbilder.
- Manifest og alle tre delmanifest: 430 gjeldende fil-/SHA-referanser kontrollert etter P2-rettingen. Kartene er også kontrollert av en annen Codex-agent uten å endre filer.

Spillintegrasjon, High/Low-skifte, sammensatte veiteksturer, Pages fra undermappe, PC-fps og mobilinput er **UNVERIFIED** i denne assetleveransen. Claudes uendrede grønne spilltester fra PR #50 er gjenbrukt som tidligere dokumentert grunnlag, ikke kjørt på nytt eller påstått som test av de nye kartene.

## Materialvisning og reproduksjon

Fra `web/`, med eksisterende `node_modules`, start en lokal filserver og åpne `src/assets/art/production/round9_qa/index.html`. Visningen bruker prosjektets lokale three.js, ingen CDN, og lar deg sammenligne fargekart med farge/normal/ruhet under samme lys. Den er en privat produksjonsfil og importeres ikke av spillet.

Eksempel fra `web/` for å reprodusere ett kartpar til en midlertidig mappe:

```sh
python3 tools/round9_maps.py --albedo src/assets/art/room/tex_floor_hextile.jpg --height-guide src/assets/art/production/round9_sources/saro/tex_floor_hextile_height.png --profile hextile --output-prefix /tmp/s47-hextile --qa-dir /tmp/s47-hextile-qa --report /tmp/s47-hextile-report.json
```

De øvrige profilene og bevarte generatororiginalene står i manifestet. Genererte høydeskisser er støttebilder, ikke nye spillalbedoer. Eksakt bildemodellversjon er ikke eksponert.

## Sømbegrensning

De eksisterende albedoene er laget som repeterende bilder, men noen har synlige eller målbare avvik over kanten, blant annet heksagulvets tidligere registreringsfeil og enkelte fiber-/sanddetaljer. Datakart kan kondisjoneres periodisk og vises i 2×2, men dette reparerer ikke originalbildet. Kart-/albedosamsvar ved en slik original søm må oppgis presist, uten å kalle fargebildet feilfritt sømløst.

Normalkartene har identiske motsatte kantpiksler, med en 8-pikslers utjevningssone og to flate kantpiksler. Dette gir et lokalt avvik fra relieffet helt ved kanten. Ruhet har hele 8-pikslers DCT-kantblokker som er konstante langs kantnormalen, fulgt av åtte piksler med myk overgang. Stående vann på asfalt og sammenhengende gangsti på gulvene er ikke sikkert dokumentert i fargebildene: asfaltens blankere felt følger en myk mørkhetsmaske, gulvbord har lokal slitasjevariasjon, og disse tolkningene er UNVERIFIED. Ingen nye dammer eller gangstriper er lagt i albedo.

## P2-review: kontroller ferdigkodet JPEG

Reviewen av assetcommit `187af49` fant at den tidligere kontrollen bare registrerte dekodet ruhet, uten å avvise kompresjonsoversving eller ulikhet over motsatte JPEG-kanter. Reproduksjon bekreftet kantavvik i sju kart og verdioversving i tre. Den tidligere numeriske PASS-statusen dekket derfor ikke disse JPEG-kravene.

Dette er rettet i samme PR: 19/19 ferdigdekodede JPEG-er har null ulike kantpiksler og null piksler utenfor profilbåndet. Profilbåndet tillater bare ±0,5/255 for representasjon i 8 bit, for eksempel 229 eller 230 som nærmeste verdi for nominell 0,9. Kantkragen overlever JPEG90; lokal kodingsfeedback korrigerer oversving, og eksporten avvises hvis kontrollen ikke konvergerer. Heksagulv er nå 117–230, skap 140–230 og gulvbord 153–188 byte. Åtte ruhetsfiler er endret; alle 19 normalkart og 19 albedoer er byteidentiske med før rettingen.

Fem målrettede regresjoner PASS: avvis ferdig oversving, avvis kantavvik, godta korrigert JPEG, godta bare nærmeste konstantrepresentasjon og avvis et skarpt testfelt som ikke konvergerer. Kjør `python3 tools/round9_regression.py`. Selvstendig filkontroll kan kjøres med `python3 tools/round9_check.py --normal <fil_n.png> --roughness <fil_r.jpg> --profile <profil>`; uten profil er områdekontrollen uttrykkelig UNVERIFIED. Før/etter, forskjell og dekodet 2×2 står i `round9_qa/p2_roughness_comparison.png`; hvert materiale har også et eget ruhetsbilde i 2×2. En annen agent har kontrollert de faktiske JPEG-pikslene mot profilene uten å stole på rapportens PASS-felt.
