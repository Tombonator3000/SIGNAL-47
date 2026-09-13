# Scan25 — trent steinskann ved STATION 01

13. september 2026. Isolert Unity-prøve av en ferdig rekonstruert, lisensiert stein. Dette er én steinblokk ved en enkel ruinbase utenfor prøvebygningen, ikke en ferdig ruin, steinvegg eller implementert STATION 01-historie.

## Kilde og faktisk bevaring

Loop CEs [«rock»-datasett](https://loopce.com/gaussian-splats) er publisert under **CC BY 4.0**, med eksplisitt rett til kommersiell bruk og bearbeiding. [Datasettkortet](https://huggingface.co/datasets/loopce/gaussiansplats) bekrefter lisensen. [Kreditering og endringer](License/ATTRIBUTION.md) følger både kilde og lokal prøvepakke.

Den originale, trente PLY-filen har 7 339 238 Gaussians, SH3 og 1 820 132 556 bytes. Kilden har også fotografier og COLMAP-data; **439 registrerte kamerastillinger** ble lest. Originalfilene er låst til revisjon `f8c5eeae5ff5410dc537a27762c8927ba2fd29a2` og kontrollert mot utgiverens SHA256. [Opphav, URL-er, hash, transform og utsnitt](asset-provenance.json).

Utsnittet beholder **493 869 trente Gaussians**. Hagen rundt steinen og brede, løse splats ble fjernet. Alle 62 float32-felt i hver beholdte rad er kontrollert bit for bit mot originalen: posisjon, orientering, størrelse, opasitet, grunnfarge og samtlige SH3-koeffisienter. Ingen overflatesampling, ny trening, nedskalering av detaljnivå eller fargeendring er brukt i datafilen. Skala er foreløpig valgt for scenen; fotografiene beviser ikke meterpresisjon. Orientering er utledet fra kameradata og visuelt kontrollert.

## Referanse og faktisk Unity-visning

[Originalt kildefoto](Reference/original-photo-0001.png) viser steinen i opptaksmiljøet. [Front](Reference/front.webp), [skråvinkel](Reference/oblique.webp) og [nærbilde](Reference/detail.webp) er nye, uredigerte gjengivelser av det trente utsnittet med PlayCanvas SplatTransform 3.4.2. De er referanse-render, ikke spillbilder. Kameraposisjon, retning, oppvektor, 70° vertikalt synsfelt og 1280×800-oppløsning er matchet i Unity gjennom en eksplisitt aksekonvertering.

Første Unity-kjøring bevarte formen, men gjorde fargene for mørke og oransje. Årsaken var at komponentens `GammaToLinear=true` ble brukt i prosjektets **Gamma**-fargerom. Prøven setter nå konverteringen etter prosjektets faktiske fargerom og kontrollerer innstillingen i spilleren. Prosjektets fargerom og upstream-pluginen er beholdt.

Etter rettingen sank gjennomsnittlig absolutt RGB-avvik inne i steinens silhuett fra omtrent **62–65 til 11–13 av 255** over de tre kameravinklene. Dette er sammenligning mellom to rasterizere, ikke en fotorealismescore. Fargene og detaljene ligger visuelt mye nærmere referansen, men rasterkantene er fremdeles litt lodne. Dette finnes også i kildens referanse-render.

## Samspill og avgrensninger

- Vanlig Unity-geometri gir gulv, ruinbase, bygning, roterende dør og kollisjon. En enkel, konservativ boks inne i steinen stopper spilleren; den er håndlaget og er ikke en automatisk rekonstruert overflate. Nærkontakt fra alle sider er ikke godkjent som presis kollisjon.
- Steinen beholder den fotograferte belysningen. F styrer lys på vanlig geometri; prøven hevder ikke dynamisk belysning av den uendrede splatten. En solbelyst skann er derfor ikke ferdig tilpasset SIGNAL / 47s nattmiljø.
- C tar et separat diagnostisk kamerabilde gjennom URP. Det er ikke en skjermkopi eller et nytt historisk bevis i hovedspillet. Eksponeringer og skjermbilder sammenlignes og hashes separat.
- Det ugjennomsiktige cyan-kontrollobjektet har en liten gjenværende blandingsfeil ved nedre kant. Denne kontrollen er ikke bestått som null-overlapp. Objektet er skjult i den rene visningen og ved manuell start; original test og feil beholdes i bevisene.
- Vulkan, Unity 6000.3.22f1, URP 17.3.0, Arloopa UnitySplats 1.2.0, Intel ARL, Gamma, HDR og 1×MSAA er den testede kombinasjonen. Ingen OpenGL-prøve er gjentatt. Native tastatur/mus, hovedprosjektets fotoapparat og 2×MSAA er fortsatt uverifisert.

## Åpne og gjenskape prøven

Lokal pakke: `Artifacts/Releases/Scan25-8892097d2889/Start-Scan25.sh`. WASD/mus beveger spilleren, E styrer døren, F lampen, G viser/skjuler skannen, C tar diagnostisk foto og Escape frigjør musen. Manuell start viser steinen først. Spilloppførsel og standardstarter i hovedprosjektet er uendret.

Oppskriften ligger i `Automation/Scan25/`: `source.py` henter ved behov de låste originalene, kontrollerer hash og lager utsnittet; `reference.py` lager referansebilder; `prepare.py` gjenbruker den avgrensede Hybrid23-pakkeoppsettingen; `build.sh`, `run.sh`, `verify_evidence.py` og `package.py` bygger og verifiserer prøven. Den ferdige pakken har `--checks NY_MAPPE` og `--benchmark NY_MAPPE`. Eksperimentelle grafikkprosesser kjøres sekvensielt under den felles låsen, med minne- og tidsgrenser.

De store originalene og spillerpakken er lokale, ikke innebygd i Git-repoet. En ny klone trenger originalnedlasting på omtrent 1,95 GB, de pinnede UnitySplats/Unity.WebP-checkoutene fra Hybrid23, Unity og den oppgitte SplatTransform-versjonen. Bruk en ny outputmappe for hver prøve. Bildene og bevisene i denne mappen er tilgjengelige i repoet.

Prøven undersøker den fotorealistiske arbeidsflyten. WorldCase22 er fortsatt siste produksjonskandidat og Visual10 standard. Feltsekvensen P06–P09 og resten av spillet blir ikke ferdige som følge av denne miljøprøven.

## Leveringsbevis

Sluttkildehash: `8892097d28891cf9074f9b8ec011d76913e7822bd6bf7d60e7719a17b568f1e9`. Spillerhash: `10867df793932c39c4591acd66eaec0c00304239b1bea90132230731deb28074`. Arkivhash: `5ecf490aa82cb6fffb25bf4b4fe714ba72157db4137b4bc348d0608f96cb3959`.

Fra den nyutpakkede pakken består **19 API-kontroller**, med null registrerte runtime-feil. **188 pakkefiler** er hashkontrollert. **21 originale bilder**, inkludert åtte separate kameraeksponeringer, dekodes og kontrolleres. [Samlet datakontroll](Evidence/verification.json) holder dette atskilt fra bildekvalitet og de åpne portene. Den strenge cyan-kontrollen feiler: 29 av 24 544 utvalgte innvendige piksler endret seg ved splat av/på. Vi har ikke godkjent generell overdekking.

[Faktisk Unity-nærbilde](Evidence/reference-detail.png) og [ren visning ved prøvebygningen](Evidence/scene-preview.png) er fra sluttpakken. Se også de separate `hybridprobe-exposure-*.jpg`-filene. Ingen av disse bildene er retusjert eller generert med en bildemodell.

Den samme utpakkede spilleren gjennomførte en separat kamerarunde: 10 sekunder oppvarming, deretter **60,00 sekunder / 2 748 bilder**, uten skjermbilde-/fotoarbeid i måleperioden. Gjennomsnitt **45,80 FPS**, p95 **27,76 ms**, p99 **30,86 ms** ved 1280×800 med hele utsnittet og SH3. [Rå måling og sammendrag](Evidence/Benchmark/summary.json). Målet om stabile 60 FPS er **ikke bestått**. Dette er en liten miljøprøve, ikke en måling av hele spillet eller langvarig varmebelastning.

**Videre beslutning:** Den trente kilden og importveien er klart mer lovende enn Hybrid23s utrente sampling. Før bruk i hovedspillet må tetthet/detaljnivå optimaliseres med nye nærbildesammenligninger, kontakt/overdekking rettes og belysningen passe nattscenen. Ikke fyll hele spillområdet med slike fulloppløselige utsnitt på grunnlag av denne prøven.
