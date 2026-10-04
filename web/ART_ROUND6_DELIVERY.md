# Runde 6: dineren og Roswell-veien

4. oktober 2026. Seks bildefiler etter den avtalte runde 6-briefen, klare for Claudes integrasjon etter rom 6. Leveransen omfatter grafikk og kontrollbevis. Den inneholder ikke nye spillområder, kapittellogikk eller endringer i bildeinnlasting.

## Grunnlag og filansvar

- Grafikkbrief: `web/ART_BRIEF.md` på Claude-grenen `17810930a941f574f5cfcb6f0ec373c26292007b`, SHA-256 `37ea02c74790b9815b0ae5f35ae9043d21207f927ddabf8d50bf4e1f6aa92bbe`.
- Historie: `web/HISTORIE.md` på samme ref. Avisbildet er oppdiktet og viser tre ranchere og ett rolig lys over en mesa.
- Egen gren `codex/diner-road-art-20261004`, startet fra main `3264fabc84ec36b5fa2b165a41b2b4f6b8f1e6dc`. Egen checkout `signal-47-diner-road` bevarer PR #34 og forhåndsvisningen i `signal-47-threejs`.
- Claude har mottatt PR #34 og bekreftet innkobling av seks arkiv-/stasjonsbilder. Runde 6 er neste dokumenterte grafikkoppgave. Ingen ny prioritetsforespørsel er sendt mens han arbeider.
- Filer fra Claudes aktive filansvarsliste, Unity-arkivet og historiske prosjektkilder er ikke endret. Claude eier `art.ts`, områdegeometri og den samlede spilltesten.

## Bildefiler

Stier er relative til `src/assets/art/`. De seks originaleksportene er til sammen 2 017 152 byte.

| Fil | Piksler / format | Mål per bilde i spillet | Bruk |
| --- | --- | --- | --- |
| `diner/sign_diner_blank.png` | 1024 × 512, RGBA PNG | 4,0 × 2,0 m | Buet, tomt dinerskilt med to stolper og tent rød/cyan neon |
| `diner/tex_counter_laminate.jpg` | 512 × 512, RGB JPEG 85 | 0,6 × 0,6 m | Mintgrønn benkelaminat med små grå stjerner |
| `diner/tex_floor_checker.jpg` | 1024 × 1024, RGB JPEG 85 | 2,4 × 2,4 m | 8 × 8 svarte og lyse ruter, nominelt 30 cm |
| `diner/menu_board_blank.png` | 1024 × 512, RGB PNG | 1,6 × 0,8 m | Svart tavle med hvite rammelister og fem tomme linjer |
| `diner/clipping_photo_1947.jpg` | 1024 × 768, RGB JPEG 85 | Ikke fastsatt | Tekstfritt avisfoto med grovt raster, laget for fortellingen |
| `road/tex_gravel_track.jpg` | 1024 × 1024, RGB JPEG 85 | 4,0 × 4,0 m | Grusflate med to hjulspor langs bildets Y-akse |

Alle seks har innebygd sRGB-profil. Navn, retter, priser, avisnavn og overskrifter legges i kode. Alle generatororiginaler er bevart lokalt under `.codex/generated_images/`; stier, mål, hash, prompts og vanlig skalering/eksport står i [DINER_SURFACES.json](src/assets/art/diner/DINER_SURFACES.json) og [DINER_ROAD_PROPS.json](src/assets/art/diner/DINER_ROAD_PROPS.json). Sju innebygde imagegen-kall: seks nye bilder og én målrettet paddingretting av skiltet. Eksakt modellversjon er ikke eksponert. Ingen ekstern betalt assettjeneste eller programmatisk retusjering er brukt.

## Integrasjon

Skiltets foreslåtte tekstfelt er `[225,169,799,246]` i 1024 × 512-kilden, origo øverst til venstre. Forslag: sentrert baseline `(512,230)`, høyst 50 px skrift og 574 px maksbredde. Endelig navn/font må inspiseres i spillet. De oppgitte 4 × 2 m gjelder hele bildeplanet, inkludert transparent padding. Unngå å bygge et ekstra par synlige stolper oppå stolpene i bildet.

Menytavlens omtrentlige indre skrivefelt er `[35,25,989,489]`. De fem vannrette linjene ligger rundt y = 102, 172, 243, 313 og 384. Briefen fastsetter ikke antall linjer. Tekst bør få `fillText`-maksbredde og kontrolleres ved faktisk kameraavstand.

Benkeplate, rutegulv og grus bruker vanlig `RepeatWrapping` med mål fra tabellen. Skilt, meny og avisfoto bruker hele bildet uten repetisjon. Hold grusveien til ett 4 m bredt bilde på tvers: gjentakelse på tvers lager flere par hjulspor. Det er albedo, uten normal-/høyde-/roughnesskart.

De nye bildene importeres ikke i `art.ts` i denne leveransen. Bare nødvendige filer bør importeres når det nye området bygges. Bruk prosjektets eksisterende WebP-rute for de to PNG-bildene ved innkobling, bevar disse briefeksportene, og kontroller alfa og tekst etter konvertering. Skiltets tegnede glød er ikke en lyskilde i 3D-scenen; lokal lyssetting og emissive materialer må vurderes under integrasjon. Dette er forslag til Claude, ingen endring av hans filer.

## Kontrollbevis

- **PASS, teknisk:** [asset-verification.json](evidence/diner-road-2026-10-04/asset-verification.json) kontrollerer seks bildefiler, to manifester, 26 hashreferanser, tre ordinære 2 × 2-ark og 17 lokale gallerireferanser. Ingen feil eller advarsler. Fire JPEG-tabeller er forenlige med Pillow-kvalitet 85. Skiltets alfa går fra 0 til 255, og alle fire ytterkanter er helt transparente.
- **PASS, visuell:** Root og assetagentene har inspisert alle seks finaler, de tre repeat-arkene og skiltets transparens. Ingen vesentlig tonal skjøt. Gulvet har 8 × 8 vekslende ruter; motivet i avisen har tre ranchere og ett lys. Ingen påstand om matematisk pikselidentiske teksturkanter.
- **PASS, nettleser:** [gallery-browser.json](evidence/diner-road-2026-10-04/gallery-browser.json) viser seks av seks dekodede bilder i faktisk DOM-viewport 1280 × 720 og 390 × 844, uten horisontal overflow eller fangede konsollfeil/advarsler. Alle mobilkort står innenfor sidebredden. Skjermbildefilene er 1265 × 1513 (fullside) og 375 × 812 (viewportopptak); bildefilstørrelse og DOM-viewport er forskjellige målinger.
- **Gjenbrukt kontroll:** Spillkode, byggekonfigurasjon og runtimeimporter er uendret fra grenens main-base. [delivery-verification.json](evidence/diner-road-2026-10-04/delivery-verification.json) sammenligner Git-blobene for runtimekildene og binder kontrollrapportene til filhashene. Grønn typesjekk og vanlig bygg fra PR #34 (`web/evidence/support-2026-10-04/support-build-verification.json`, 11:13:57 UTC) er historisk bevis for samme base med den isolerte Room6-støtten, ikke en ny kjøring for runde 6. Uendrede kapitteltester er ikke repetert.
- **UNVERIFIED:** Tekstpålegging, belysning, ferdig diner-/veigeometri, lasting i spillpakken og fysisk mobil/FPS. Claude utfører én samlet integrasjonstest når dette området kobles inn.

Små dokumenterte forbehold: gulvfuger og slitasje varierer litt rundt de nominelle rutene; fine detaljer gjentas i materialflatene. Skiltets kremfelt har alfa 253–254, nesten opakt. Meget svake alfapiksler rundt gløden er bevart. Avisbildet er generert spillgrafikk, ikke et autentisk fotografi fra 1947.

## Åpne og kontrollere

Kjør fra `web/`: `python3 tools/verify_round6_art.py --no-write`. Kontroll av lokalt bevarte generatororiginaler krever de lokale kildebanene i manifestene; på en annen maskin kan de referansene være utilgjengelige uten at slutteksporten er feil.

Galleriet ligger i [round6.html](src/assets/art/production/round6.html). Lokalt: `python3 -m http.server 8479 --bind 127.0.0.1 --directory src/assets/art`, åpne `http://127.0.0.1:8479/production/round6.html`. Det er assetvisning og kontrollark, ikke en ny inngang i Pages-spillet.
