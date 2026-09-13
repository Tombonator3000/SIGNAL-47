# SIGNAL / 47 — aktuell overlevering

## Station26 — sammenhengende feltreise i hovedspillet

13. september 2026: [Station26](Station26/README.md) er nyeste hovedspillkandidat, bygget fra flettet Scan25/PR21 (`db36ff92e5cd7cf7be83c2c9bf7ab6634b3b315a`). Faktisk GitHub-status viser fortsatt PUBLIC; synligheten er ikke endret. Arbeidet samler avreise etter P04/P05, et fysisk STATION 01-område, P06–P09, to nye kameraeksponeringer, retur til våtbenken og varig lagring. Feltområdet er en avgrenset, aktivert del av samme Unity-scene; ingen bilkjøring eller separat scenelasting er innført.

**Prøv lokalt:** `~/Nedlastinger/Station26-8b181fb4830e/Start-SIGNAL47.sh`, også under `Artifacts/Releases/`. Bruk **FIELD TRAVEL**-folioen ved arkivbenken etter B-12 og arkivfunnene. Begge feltfilmene kan framkalles etter samme retur. WorldCase22-pakken og brukerlagring er bevart. **Visual10 er fortsatt standardstarter; Station26 er en testkandidat.** Kilde-SHA `8b181fb4830ee7956234a8f24b077dfe92457cb6c6049ae74c6592553df527d4`; arkiv-SHA `b9c72948896c1d18777206fb2311e3177cdbce7da6ba1863bc9d416842e0d3c3`.

162 feltkontroller består direkte ved 1280×800 og fra nyutpakket starter ved 1600×900. Samme utpakkede starter består 118 arkiv-, 42 eksisterende spill- og 50 meny-/gjenopprettingskontroller. Begge oppgaverekkefølger, fysisk gangrute gjennom hytta, faktiske eksporterte foto, scenegjenstart og feil ved lagring/manglende bilder er prøvd via API. 173 pakkefiler, fem historiske kildefiler og fire foto i begge feltkjøringer er hashkontrollert. To lightmaps og opprinnelige kollidere beholdes; 28 feltkollidere tilkommer. Se [fullt bevis](Station26/Evidence/verification.json) og [presise testgrenser](Station26/TEST_PLAN.md).

Teksturert Hybrid23-bygning brukes som vanlig mesh sammen med originale Archive17-detaljer. Ingen splatpakke er innført i hovedspillet. Faktiske runtime-bilder viser prototypeterreng og enkle instrumenter. Et historisk diagram og teksttranskript er tilgjengelig; originalnegativ og ny innspilt stemme er ikke produsert. Native input, blind forståelse/tidsbruk, subjektiv lyd og separat releaseytelse er fortsatt UNVERIFIED. Hele K3 eller 5–6-timersspillet er ikke ferdig.

En P1-feil fra uavhengig kodegjennomgang er rettet før levering: mislykket fotoeksport kan prøves igjen ved feltets returfolio. Reell eksportfeil, bevart originalbuffer og vellykket lokal gjenoppretting er med i begge sluttkjøringer. Tidligere Station26-pakke `ef2a95b4a65b` er en bevart mellomkandidat, ikke den anbefalte prøvepakken.

**Neste samlede produksjon:** følg opp feltbevisene på SARO og bygg den avgrensede overgangen mot SIERRA MOTOR COURT fra gjeldende verdensdesign, med nødvendige historieassets, lagring og verifikasjon. Lukk kandidatens åpne spiller-/ytelsesprøver når verktøy og spiller er tilgjengelige. Ikke erstatt dette med flere isolerte splat-/verktøyforsøk uten konkret behov. Eldre «neste P06–P09» og «WorldCase22 siste kandidat» nedenfor er historikk.

## Scan25 — detaljert, trent steinskann faktisk prøvd i Unity

13. september 2026: [Scan25](Research/Scan25/README.md) bruker Loop CEs fotograferte og trente «rock», CC BY 4.0. 493 869 Gaussians er beskåret/renset fra originalen; alle beholdte felt er kontrollert bit for bit, med SH3 bevart. PlayCanvas og Unity viser tre tilsvarende vinkler. En faktisk fargeromsfeil er rettet: Gamma-prosjektet skal ikke bruke splatkomponentens ubetingede GammaToLinear-konvertering. Dette oppdaterer forståelsen av mørke splatbilder; historiske Hybrid23-resultater er ikke kjørt om.

Lokal prøvepakke: `Artifacts/Releases/Scan25-8892097d2889/Start-Scan25.sh`. Kildehash `8892097d28891cf9074f9b8ec011d76913e7822bd6bf7d60e7719a17b568f1e9`; spillerhash `10867df793932c39c4591acd66eaec0c00304239b1bea90132230731deb28074`. Fra utpakket starter består 19 API-kontroller, 188 pakkefiler og åtte separate kameraeksponeringer; 21 bilder dekodes. Detalj og form ligger visuelt nær kildens referanse-render. Opprinnelig lys og litt lodne ytterkanter er beholdt. Kollisjon er en konservativ, håndlaget boks, ikke automatisk rekonstruksjon.

**Åpne porter:** cyan-null-overlapp feiler med 29 endrede av 24 544 innvendige piksler. Separat 60-sekunders kamerarunde gir 45,80 FPS, p95 27,76 ms og p99 30,86 ms på Intel ARL / Vulkan / 1280×800 / Gamma / HDR / 1×MSAA; stabile 60 FPS er ikke bestått. Native input, hovedspillets fotoapparat, 2×MSAA og nattbelysning av skannen er uverifisert. Ingen produksjonspromotering. Hele prøvepakken og krediteringen er levert lokalt; store kildefiler kan hentes på nytt med den pinnede oppskriften.

WorldCase22 er fortsatt produksjonskandidat og Visual10 standard. Neste ordinære innhold er fortsatt P06–P09. Videre splat-integrasjon krever lettere detaljnivå med bevart utseende, riktige overganger og passende innbakt lys. SplatQuality24/PR20 er bekreftet MERGED på `bb52eada117a6d40c8427ec37b4eb6a0f83be7e4`.

## SplatQuality24 — ny vurdering av den grove miljøprøven

13. september 2026: [PlayCanvas-/Gabor-vurderingen](Research/SplatQuality24/README.md) presiserer Hybrid23-resultatet: den genererte modellen var skarpere enn vår **utrente** overflatesampling; dette avviser ikke fotorealisme fra godt rekonstruerte splats. To lette subagenter analyserte primærkilder. SplatTransform 3.4.2 leste prøvefilen med 150 000 Gaussians uten NaN/Inf, under minne-/tidsgrense. Ingen ny Unity-kjøring eller spillendring er utført.

Anbefalt neste splat-prøve er én rettighetsavklart, trent skann, sammenlignet i kildens visning og Unity før større miljøer eller komprimering. PlayCanvas-verktøy for kollisjon og kildebehandling er relevante; strømmet SOG må verifiseres separat i Unity. Gabor krever egen visningskode, CUDA i forskningsimplementasjonen og særskilt kommersiell tillatelse. Den tidligere anbefalingen om vanlig mesh gjelder foreløpig den konkrete Hybrid23-bygningen. WorldCase22 er fortsatt produksjonskandidat, Visual10 standard, og neste ordinære feltsekvens er P06–P09. Hybrid23/PR19 er bekreftet MERGED på `cc522940929be56cbe43d789acf0cb070f7f3bf8`.

## Hybrid23 — faktisk bilde/modell/splat-prøve for STATION 01

Kilde, oppskrift og bevis leveres gjennom [PR19](https://github.com/Tombonator3000/SIGNAL-47/pull/19). GitHub viser faktisk flettestatus.

13. september 2026: [Hybrid23](Research/Hybrid23/README.md) undersøker Arloopa UnitySplats **1.2.0**, låst revisjon `6c0258189a2b124af1282fa9236fd9b6637f1a1a`. MIT tillater gratis kommersiell bruk med bevarte notiser; de konkrete avhengighetenes MIT/BSD-tekster følger kilde og prøvepakke. **Anbefalingen for selve feltbygningen er den genererte teksturerte modellen som vanlig Unity-3D.** Splats er et separat, faktisk testet alternativ, uten hovedspillintegrasjon.

GPT 2.5-forlegg → Magnific/Tripo v3.1 → Blender-kontroll → samme klippede mesh og 150 000 overflatesamplede Gaussians. Dette er ikke en trent Gaussian-rekonstruksjon eller Marble-verden. Original GLB, forlegg, Blender-render, opphav, import-/konverteringskode og reelle Unity-bilder er bevart. Vanlig geometri gir gangflate, innvendige vegger, roterende dør og kollisjon. Matchet lysproxy gir synlig lampesvar og samme-poses separate fotoeksponeringer. Flekkete detaljer og enkelte lyse interiørskjøter gjenstår; mesh-versjonen er skarpere og har lavere diagnostisk bildetid.

Syntetisk prøve består **13**, generert uten proxy **16**, og sluttvarianten **22 API-kontroller**, alle med null registrerte runtime-feil. Sluttpakken består de samme **22** etter utpakking og start via egen launcher, med **186** hashkontrollerte filer og fem nye separate JPEG-eksponeringer. Generert cyanreferanse: 2414 innvendige piksler / null endrede ved A/B. Syntetisk fotreferanse har lokal splatoverlapp og består ikke null-overlappskontrollen; generell overdekking og Splat21s identiske feilscene er ikke godkjent av dette. [Samlet bevis og presise avgrensninger](Research/Hybrid23/Evidence/verification.json).

Lokal prøve: `Artifacts/Releases/Hybrid23-b37b22e6eca6/Start-Hybrid23.sh`. Kildehash `b37b22e6eca619242ff23de7143d98536a4aa663e51bd26eea2a3900ec96f0d2`; spillerhash `9e411c618c7b64305f89d17361422612ca46138ead87609d5221f5c5ffe32b88`. Starteren viser mesh først; G bytter representasjon, F lampen, E døren, C diagnostisk foto. Kun Linux/Vulkan, 1280×800, HDR/1×MSAA er kjørt. Hovedprosjektets 2×MSAA, kanonisk fotoapparat, native input og lengre GPU-/ytelsesprøver er uverifisert. Ingen testprosess står igjen etter levering.

**Marble ba fortsatt om innlogging.** Ingen konto eller betaling er opprettet, og bilde → Marble-verden er uutført. Innloggingsfanen er beholdt for eventuell brukerhandling. Ingen ventende bakgrunnsgenerering eller automatisk oppfølging er startet. Unity-/Blender-arbeidet og lokal prøvelevering er ferdig uavhengig av denne tilgangen.

WorldCase22/PR18 er faktisk flettet på `ea0ec2438e31c1fffb6f9c70cd9600791e90c366`. WorldCase22 er fortsatt siste produksjonskandidat og Visual10 standard; hovedspillets `Unity/` og `Spill-SIGNAL47.sh` er uendret i Hybrid23. Neste ordinære produksjon er fortsatt hele STATION 01-feltsekvensen P06–P09, med den kontrollerbare mesh-/Unity-arbeidsflyten. Hybrid23 er miljøprøve, ikke ferdig historiekapittel.

## WorldCase22 — hovedspillets arkivsak og hele områdekartet

Kilde, kart og bevis leveres gjennom [PR18](https://github.com/Tombonator3000/SIGNAL-47/pull/18). GitHub viser faktisk flettestatus.

13. september 2026: [WorldCase22](WorldCase22/README.md) samler P04 og P05 i hovedspillets fotolab etter den opprinnelige toeksponeringssaken. Fire kildekort, rapport­sammenligning, begrunnet STATION 01-spor og varig framdrift er integrert ved den eksisterende arkivbenken. Tre subagenter bidro til implementering, verifikasjon og [samlet verdensdesign](WorldCase22/WORLD_DESIGN.md). [Illustrert områdekart](WorldCase22/Visuals/world-map-final.png) viser SARO, STATION 01 og SIERRA MOTOR COURT som en avgrenset produksjon med 330 minutters designmål. Kartet er konsept, ikke faktisk spillbilde eller målt nivågeografi.

**Siste produksjonskandidat:** `Artifacts/Releases/WorldCase22-5c453a898fce/Start-SIGNAL47.sh`. Kildehash `5c453a898fce7d670d866204ef2102231acf4d8096b1860e24b43903dde68a16`. Visual10 er fortsatt standard. 118 arkivkontroller består direkte ved 1280×800 og fra utpakket starter ved 1600×900; samme utpakkede starter består 42 spillkontroller og 50 meny-/gjenopprettingskontroller. 173 pakkefiler, begge originale foto og uendret standardstarter er kontrollert. 26 originale runtime-bilder er gjennomgått. [Bevis og identitet](WorldCase22/Evidence/verification.json).

Lagring dekker delvis lesing, begge funn, Previous Shifts og eksplisitt foto­reparasjon uten tap av arkivframdrift. Ulovlig kombinasjon av ufullført originalsak og arkivframdrift avvises. 154 opprinnelige kollidere og to lightmaps beholdes, én arkivkollider tilkommer. Folioens bordkontakt, etikett, HUD-overlapp og et feiltegnet indeksmerke ved større oppløsning ble rettet før sluttpakken.

**Neste samlede leveranse:** hele feltreisen til STATION 01, K3/P06–P09: avreise/retur, feltbygning, historisk oppstilling, lampetest, kabelsløyfe, ekte feltfoto og lagring på tvers av området. Ikke gå tilbake til små, urelaterte modell-/verktøypass. Resten av K2, STATION 01, motellet og senere kapitler er ennå ikke ferdige. Ekte input, blind leseforståelse/tidsbruk, subjektiv lyd og separat ytelsesmåling er fortsatt åpne. Splat21 forblir separat, og ingen splatpakke er innført i hovedspillet.

Eldre «neste P04» og «Resources19 siste kandidat» nedenfor er historikk og erstattes av denne statusen.

## Splat21 — gjenopprettet etter systemkrasj

13. september 2026: [Unity-splatprøven og krasjrapporten](Research/UnitySplat21/README.md) er fullført som separat eksperiment. PR16 er kontrollert MERGED på `e976fa7f9b46225ed37873131e878bfd87c40dec`. Alt lokalt Splat21-arbeid ble bevart etter omstart; kerneljournalen bekrefter global minnemangel og at ChatGPT ble drept, men ikke hele årsakskjeden.

Vulkan fullfører fire faser med 784/50 176 originale splats, både før og etter gjenoppretting, med null registrerte runtime-feil. OpenGL feilet med 398 feil og ufullført måling; denne kombinasjonen avvises nå før start. Ny starter har kontrollerte minne-/tidsgrenser og seks beståtte grensetester. Begge Vulkan-nærbilder har synlig blandingsfeil mot et vanlig objekt: splats er **ikke godkjent for hovedspillet**. Resultater, originale bilder og spiller-/kildehash er bevart. Ingen testprosess skal stå igjen etter levering.

Resources19 er fortsatt siste produksjonskandidat, Visual10 standard; `Unity/` og `Spill-SIGNAL47.sh` er uendret i Splat21. Neste ordinære produksjonssteg er dokumentlesbarhet/P04; videre splat-arbeid er et separat mulig eksperiment og skal ikke forsinke dette. Kilde og bevis er levert i [PR17](https://github.com/Tombonator3000/SIGNAL-47/pull/17), opprinnelig kildecommit `55320c13378d06e4a5041d3af8d3812e10df677d`; GitHub viser faktisk flettestatus.

## Nyeste verktøy- og instruksjonspass: ProductionTools20

Leveringssporet er [PR16](https://github.com/Tombonator3000/SIGNAL-47/pull/16); GitHub viser faktisk flettestatus. Dette sporet inneholder verktøybevis og instruksjoner, ingen ny spillpakke.

13. september 2026: [PlayCanvas/Tripo-vurdering og testbevis](Research/ProductionTools20/README.md), med [gjennomgang av stoppunkter](Research/ProductionTools20/SKILL_AUDIT.md). Fem personlige skills er rettet etter gjennomgang av 47 lokale skill-filer. Brainstorming krever ikke lenger ny godkjenning for allerede autorisert implementering eller en manglende `writing-plans`-skill. Delsteg er checkpoints; hele bestillingen og relevant autorisert levering styrer ferdigstatus. Repoets `AGENTS.md` og personlig `~/.codex/AGENTS.md` gjør arbeidsformen varig. Reelle tilgangs-/kvalitetsgrenser og uutførte tester er beholdt.

PlayCanvas SplatTransform 3.4.2 er installert separat og faktisk kjørt: original prøve med 784 splats, CPU-komprimering/tur-retur, GPU-render, separat kollisjonsmesh og import i Blender 4.5.13. Testgeneratorens aksefeil er funnet i bilde og rettet; 515 kollisjonstrekanter og strålekontroller består. Proxyen er grov og ikke godkjent som presis spillkollisjon. Gaussian splatting anbefales som avgrenset miljøeksperiment; ingen Unity-integrasjon, rendererendring eller ny personmodell er innført. Gjenbrukbar kunnskap ligger i game-production-skillen.

PR15 er kontrollert MERGED på `524c56d0f446cdc757cf541c390d24963c3698f7`. Resources19 er fortsatt siste produksjonskandidat, Visual10 fortsatt standard. Nye verktøyprøver lukker ingen av spillets åpne input-/lyd-/ytelsestester. Neste tidligere avtalte spillsteg er dokumentlesbarhet/P04; det trenger ikke vente på splat-eksperimentet.

## Nyeste produksjon: Resources19

Kilde og bevis er publisert gjennom [PR15](https://github.com/Tombonator3000/SIGNAL-47/pull/15). GitHub viser gjeldende flettestatus; dette endrer ikke kandidatens åpne tester.

13. september 2026: [Resources19](Resources19/README.md) svarer på ønsket om gratis lyd/musikk, mer detaljerte antenner og riktig telefonskala. Telefonhuset er korrigert fra 68 til 20,4 cm, med bevart rørløft, synlig ledning og tilpasset treffboks. Den overstore nabokoppen er også korrigert. Elleve antenner har original Blender-geometri for paneler, avstivere, motorhus, stige og lager. Gjennomskjæring i sluttstillingen er målt og korrigert; geometrikontroll i elleve posisjoner per antenne er innført. 154 kollisjonskomponenter og to lightmaps er beholdt.

15 gratis lydkandidater, kreditering, kilde-/filhash og [lydprøve](Resources19/lydprove.html) følger repoet. Tre CC0-musikkspor og tolv CC BY 3.0-maskinlyder er konvertert og målt, men ikke tildelt den aktive spillmiksen. NASA-modellen er bare lokal vurderingskandidat; ingen Sonniss-filer er hentet.

Kandidat: `Artifacts/Releases/Resources19-bea744ade4d9/Start-SIGNAL47.sh`. Kildehash `bea744ade4d9d86adf4b91429dad0b057e20ef6d871bb5246438525eec4b5caa`. 42 spillkontroller og 50 lagringskontroller består fra samme utpakkede starter, med 173 pakkefiler og begge originalfoto kontrollert. Seks originale runtime-bilder er inspisert. [Bevis og identitet](Resources19/Evidence/verification.json). Dette er siste kandidat, ikke ny standard: Visual10 er uendret. Native input, subjektiv lydmiks, ytelse og andre oppløsninger er fortsatt UNVERIFIED. Neste tidligere avtalte innholdssteg er dokumentlesbarhet/P04; neste lydsteg er lytting, valg og miks av kandidatene.

## Nyeste verktøypass: Blender MCP-skill

12. september 2026: Brukeren ba om analyse av bpy-dev/blender-mcp og en gjenbrukbar skill. [Vurdering og testresultater](Research/BlenderMCP/README.md) dokumenterer faktisk MCP/stdio-kjøring med eksisterende Blender 4.5.13. `$blender-mcp` er lagret lokalt og [sikkerhetskopiert i repoet](../Automation/Skills/blender-mcp/SKILL.md). Serveren er installert i et eget miljø; den medfølgende klienten gjør den brukbar uten global registrering eller skjermopplåsing.

Modellinspeksjon, testlagring/gjenåpning, CPU-render og FBX tur-retur består på kopier av Workstation18. To upstream-API-begrensninger på 4.5.13 har testede alternativer; den utvalgte upstream-testpakken har én feil og sju hoppede tester. Se rapporten før bruk. Dette er et verktøypass, ingen endring av spillinnhold. Dokumentlesbarhet/P04-kobling er fortsatt neste avgrensede spillsteg.

Skill, vurdering og bevis er publisert gjennom [PR14](https://github.com/Tombonator3000/SIGNAL-47/pull/14); GitHub viser gjeldende flettestatus.

PR13 er nå bekreftet MERGED på `af3aba5daf878595acfdff8e8db5f8262c2cef34`. Eldre tekst om å slå opp flettestatus nedenfor er historikk; standardstarter og åpne tester er uendret.

## Nyeste produksjon: Workstation18

12. september 2026: Brukeren godkjente anbefalingene fra VotV-vurderingen og ba om videreføring. [Workstation18](Workstation18/README.md) erstatter synlig CRT-/tastatur-/stolgeometri på de tre arbeidsplassene i hovedscenen med tre originale Blender-modeller. Levende skjermer, signalprofiler, 154 eksisterende kollisjonskomponenter, to lightmaps og spilloppførsel er bevart. Ni modellinstanser er kontrollert etter Unity-import. Integrasjonscommit `f92db83`; kildehash `ae3fe61bf1903bec8c2978cf6b6a413b3eb82b54bb15eae1c5798dacf60d5e0c`.

19 API-kontroller består direkte og 19 gjennom nyutpakket starter, inkludert den faktiske 47-sekunderssekvensen. 50 lagrings-/gjenopprettingskontroller består gjennom samme utpakkede starter; begge originalfoto dekodes med uendrede bytes. Seks originale runtime-bilder er inspisert. Pakken har 173 verifiserte filer. Se [identitet og bevis](Workstation18/Evidence/verification.json).

Kandidat: `Artifacts/Releases/Workstation18-ae3fe61bf190/Start-SIGNAL47.sh`. Visual10 er fortsatt standard og `Spill-SIGNAL47.sh` er byteidentisk med før passet. Native brukerreise, ytelse, andre oppløsninger og subjektiv lyd er fortsatt UNVERIFIED. Ingen nye forsøk på native input er gjort mens skrivebordet er utilgjengelig. Arkivprøven er fortsatt separat; neste avgrensede steg er dokumentlesbarhet og avklart kobling fra instrumentruten til P04, med P04/P05-blindtesten fortsatt åpen.

Kilde, VotV-rapport og verifikasjon er publisert gjennom [PR13](https://github.com/Tombonator3000/SIGNAL-47/pull/13). Se GitHub for gjeldende flettestatus; standardstarter og åpne tester er uavhengige av dette.

## Nyeste status: VotV-referansevurdering

12. september 2026: GitHub bekrefter at [PR12](https://github.com/Tombonator3000/SIGNAL-47/pull/12) nå er MERGED, mergecommit `f476b1ec884030e67971948121eb32ef05ec399d`. Eldre omtale av PR12 som åpen nedenfor er historisk. Dette endrer ikke kandidatens åpne tester eller standardstarter.

Brukeren ba om bred undersøkelse av Voices of the Void, inkludert kode. [Referanserapporten](Research/VOTV/README.md) dekker offisielle nettsider, tolv galleribilder, indeks/emnesøk i 31 utviklerposter og vurdering av offentlige mod-/kodeprosjekter. Den inneholder prioriterte forslag, konkrete koblinger til eksisterende Unity-kode, lisensfunn og tolv foreslåtte regresjonstester. VotvIO er en MIT-lisensiert Blender-referanse; den tilgjengelige OFL-fontens Regular-fil mangler norske bokstaver. Ingen verifisert offentlig kildekode til selve VotV ble funnet.

Leveransen er kun dokumentasjon på `research/votv-reference-audit`, basert på den oppdaterte hovedgrenen. Ingen eksterne spillassets, font eller kode er innlemmet; ingen nye gameplay-funksjoner eller spilltester er utført. JSON-registre og lokale dokumentlenker er kontrollert. Neste foreslåtte produksjonspass er CRT/tastatur/stol og dokumentlesbarhet, fulgt av avklart integrasjon av eksisterende P04. Rapporten erstatter ikke designbibelen og lukker ikke blindtest, native brukerreise eller ytelsesportene.

Oppdatert 12. september 2026 fra faktisk lokal Git-status, bygg og pakkekontroll. GitHub-hovedgrenen ble kontrollert på nytt; PR11 er flettet på `f2d92a7a8cc770849346221751a752831b6b2637`. Kanonisk mappe: `/home/tombonator3000t/.codex/.chatgpt-projects/g-p-6aa05604a4708191a65139c1d5f89363/SIGNAL-47`.

GitHub API bekrefter nå `private: false`, `visibility: public` for Tombonator3000/SIGNAL-47. Den eldre instruksens «privat repo» er dermed utdatert. Brukeren har uttrykkelig godkjent publisering av NightSky12-grenen og opprettelse av PR uten sin egen gjennomgang først. Brukeren har deretter selv flettet [PR8](https://github.com/Tombonator3000/SIGNAL-47/pull/8); merge er kontrollert via GitHub, commit `f156a9f2f656d168356145dabbe0bc8fb4ef0ec9`, 11. september 2026 kl. 10:32:08 UTC. Dette godkjenner ikke uutførte tester. Repoets synlighet er ikke endret av agenten.

## Gjeldende spill og ny kandidat

**Visual10 er den siste verifiserte spillutgaven.** PR7 ble merget 11. september; PR8 og PR9 er også flettet; PR9s mergecommit er `52c23abf28b3bf74adccb99d0f392fb8640892fb`. `./Spill-SIGNAL47.sh` starter fortsatt den faste pakken `Artifacts/Releases/Visual10-eb0748660027/`. Første kapittel har nattevakt, S-03, fysisk framkalling, bildeundersøkelse, aktiv/passiv B-12-prøve, to faktiske eksponeringer, lokal avslutning og lagring. Se [Visual10](VISUAL_10.md) og [Chapter09](CHAPTER_09.md).

**NightSky12 er en ny testkandidat**, med 8K katalogbasert stjernehimmel og dempet Melkevei, integrert i hovedgrenen gjennom PR8. Kildecommit `94811347a00811648f937400a1522c7d2cdd1d2b`. Unity er faktisk bygget; originalbilder fra faste inspeksjonskameraer er tatt i den utpakkede spilleren. Full brukerreise, lagring/fotobevis, bevegelse og ny ytelsesmåling er fortsatt UNVERIFIED fordi Kubuntu-skjermen var låst. Native tester oppdager nå låsen før de starter. Brukeren er bedt om å låse opp; låsen er ikke omgått.

Kandidaten åpnes med `Artifacts/Releases/NightSky12-79cafef6f5a5/Start-SIGNAL47.sh`. Flyttbar pakke: `Artifacts/Releases/SIGNAL47-NightSky12-79cafef6f5a5-Linux.tar.gz`. Den er merket TESTKANDIDAT. Kilde, dokumentasjon og bevisbilder er offentlig publisert og flettet gjennom PR8 av brukeren. Begge spillpakkene er lokale leveranser og følger ikke et rent Git-klon.

[NightSky12-rapporten](NIGHT_SKY_12.md) beskriver endringen, faktisk kjørte kontroller, skillet mellom bilder og spillerreise, og vurderingen av X-bildene / Magnific / Grok / Claude. Konsept11 «Den slettede natten» er bevart på `concept/story-sky-11` med åtte genererte bilder. Ny lore, himmelhendelser, motell og bil er fortsatt forslag eller senere innhold.

## Kildeidentitet og målinger

NightSky12: Unity SHA `79cafef6f5a59d042d101652cb5fb55942de4b13af91e8949a080d4caf580512`, byggpayload `c04306ebfdfe5046170ff419acb4281dfb5fcdc90137d4a853dfadb453c8b51a`, arkiv SHA `fc49644ab2a853d4afb2cf1e1bdc45425e3e3fa9d27039ff0260534cb379545d`. Senere dokumentasjonscommits endrer ikke denne Unity-kilden. [Innsjekket bevissett](Evidence/NightSky12/final-9481134/README.md) dokumenterer ni inspiserte sluttbilder, førbilde, korrigert polfeil, bygging, pakkeinnhold og den negative skjermlåstesten. Full arbeidslogg ligger lokalt i `Artifacts/Sky12/`.

Visual10: kildecommit `d026cc3`, Unity SHA `eb0748660027744a8ad717c16a51a1e8801f4908b758e4ca191e24108c9a2c01`. Historisk måling på Intel Core Ultra 5 225U / Mesa Intel ARL / OpenGLCore / 1280×800 Ultra / 75 Hz: 73,412 fps over 121,533 sekunder etter 20 sekunders oppvarming; p95 15,368 ms, p99 19,364 ms, maksimum 30,718 ms, ingen intervaller over 50 ms. Dette gjelder Visual10, ikke den nye himmelen. GPU-tid, subjektiv lydmiks og ekstern førstegangsspilltesting er fortsatt uverifisert.

## Designbibel 13 - nytt arbeidsgrunnlag

Brukeren ønsker nå en komplett historie og en tydelig ramme for et gjennomførbart spill på 5–6 timer. [Designbibel v0.1](DesignBible13/README.md) er et sammenhengende forslag: tre steder, seks kapitler og epilog, 18 hovedoppgaver, to avslutninger, 330 minutters førstegangsmål, kildetekster, fire kart, tre genererte konseptbilder og seks UI-skisser. PDF og redigerbare kilder er nå offentlig publisert og flettet gjennom PR9 etter brukerens «Fortsett, merge». Dokumentenes opprinnelige v0.1-status er historikk; selve historieforslaget innfører ingen nye Unity-områder.

Eksisterende kanon er skilt fra nye forslag. Nora/Tomás-forløpet, fenomenets presise lokale regler og sluttvalget må gjennomgås før ny historie implementeres. Tidsbudsjettet er ikke målt spilletid. De gamle punktene om en enkelt himmelhendelse er nå underlagt den samlede foreslåtte beviskjeden og roadmappen; ikke bygg en løs hendelse som motsier denne rammen.

## Menu14 — første implementeringssteg etter bibelen

Brukeren ba 12. september om å fortsette. Designbibelens trestedsramme brukes som arbeidsretning; dette passet innfører ingen ny lore i Unity. [Menu14](Menu14/README.md) retter eksisterende start-/checkpoint-flyt: Fortsett først når en sak finnes, eksplisitt avbrytbar ny-sak-bekreftelse, bevaring av begge checkpoint-filer i et unikt PreviousCases-arkiv og ingen overgang ved kopieringsfeil. Originalfoto bevares. Ingen tre-saks-/flerplassmeny eller nye historieområder er implementert.

[P04/P05-papirprøven](Menu14/PAPER_CASE.md) har kildekort, diagram, fasit og dokumentert egenkontroll. Vedlikeholdskortets for tidlige avsløring er rettet i prøveteksten. Dette er ikke en bestått blindtest; M1 og 5–6-timersmålet er fortsatt åpne. Den eldre designbibel-PDF-en er bevart som v0.1.

28 API-styrte kontroller består i faktisk Unity-spiller og gjennom launcher etter utpakking. De inkluderer feil, avbryt, faktisk scenelasting av en tidligere fullført v1-sak og dekoding av to originale foto med identiske bytes. Originale menyskjermer er inspisert. Ingen nye native tastatur-/musforsøk eller ytelsesmålinger er utført. Menu14 er en separat testkandidat; Visual10 er fortsatt standardstarteren. Se [verifikasjon og pakkeidentitet](Menu14/Evidence/verification.json). Arbeidet ble flettet som [PR9](https://github.com/Tombonator3000/SIGNAL-47/pull/9), mergecommit `52c23abf28b3bf74adccb99d0f392fb8640892fb`, 12. september kl. 18:24:43 UTC. GitHub bekreftet MERGED; vanlig merge uten bypass. Dette endrer ikke testkandidatstatus eller standardstarter.

## Recovery15 — videreført tilgang til tidligere saker

[Recovery15](Recovery15/README.md) bygger videre på PR9: Previous Shifts i startmenyen, liste med lokal dato og backup-status, avbrytbar forhåndsvisning, ny validering før bytte og bevaring av gjeldende sak før gjenoppretting. Samme v1-format og fotofiler brukes. Dette er en avgrenset del av M2, ikke ferdig treplasslagring, områdeovergang eller ny historie. Unity-bygg og 50 API-kontroller består både direkte og gjennom launcher fra utpakket kandidat. Begge originale foto og kildeprofilen er uendret; seks originale menystater er visuelt inspisert. Kandidat: `Recovery15-bda088dfe638`. Native input og ytelse er fortsatt UNVERIFIED; Visual10 er standard. Bygg-, pakke- og testidentitet følger [verifikasjonsrapporten](Recovery15/Evidence/verification.json). Kilde og bevis ble flettet som [PR10](https://github.com/Tombonator3000/SIGNAL-47/pull/10), commit `51752367a591ba6a900b7006e6efa8a1af5813e4`, 12. september kl. 18:45:30 UTC; MERGED er kontrollert via GitHub. Brukerens «Fortsett, merge» autoriserer vanlig fletting av denne avgrensede videreføringen; testportene over gjelder uavhengig av flettestatus.

## Archive16 — selvstendig arkivprøve

Brukeren ba deretter om å fortsette med gauntlet-loop og game-production, etter forslaget om en liten visuell arkivprøve. [Archive16](Archive16/README.md) prøver P04 ved et fysisk bord: åpne mappe, les original og korrigert protokoll, sammenlign og velg en støttet slutning. Egen Unity-scene med to faste inspeksjonskameraer; ingen ny gangrute, kapittelovergang eller lagring. Dette er en separat produksjonsprøve og lukker ikke M1/M2 eller K2s spilletidsbudsjett.

Blender-oppskrift, redigerbar mappe og FBX er bevart; Unity-bounds er kontrollert. Første visuelle inspeksjon fant for stor tekst, etiketter gjennom bordet og for sterkt lys; disse er korrigert. Alle 18 API-kontroller består både direkte og gjennom en nyutpakket launcher; åtte originale runtime-bilder er inspisert. Kandidat `Archive16-4748e9d2fc14` og endelig testidentitet følger [verifikasjonsrapporten](Archive16/Evidence/verification.json). Kilde og bevis er publisert i [PR11](https://github.com/Tombonator3000/SIGNAL-47/pull/11) på `gauntlet/archive-16`, nå flettet; MERGED og mergecommit `f2d92a7a8cc770849346221751a752831b6b2637` er kontrollert via GitHub. `Artifacts/GauntletLinux` inneholder nå arkivprøven; navngitte hovedspillpakker og `Spill-SIGNAL47.sh` er bevart.

## Archive17 — Blender-pass

Brukerens neste bestilling var bedre Blender-modeller. [Archive17](Archive17/README.md) erstatter bord, skap, lampe, mappe og papirbunke i den selvstendige arkivprøven med fem originale FBX-modeller og redigerbar `.blend`-kilde. Samme P04-handling, kameraer og kildetekster. Sammenfallende bordflater og synlig lysrør fra siden ble oppdaget og rettet i faktisk visuell kontroll. UV-er, normaler, dimensjoner og geometribudsjett er kontrollert etter Unity-import. Ingen Magnific-generering var nødvendig; bilde-til-3D-verktøyet finnes i den tilkoblede katalogen.

Ny lokal kandidat: `Artifacts/Releases/Archive16-74eb85f963c7/Start-SIGNAL47.sh` (Archive16-familie, Archive17-modeller). Kilde-SHA `74eb85f963c76cbb6f5277e3605ce68d1c843da97e7f1d74460c857c21d89082`. Blender- og Unity-bilder har separate merkinger. Se rapporten for tester, pakkeidentitet og gjenværende porter. Visual10 er fortsatt standard. Hovedspillets scene og lagring er bevart. Kilde og bevis er pushet som [PR12](https://github.com/Tombonator3000/SIGNAL-47/pull/12) på `gauntlet/archive-models-17`; åpen ved denne overleveringen. 18 API-kontroller består direkte og fra utpakket launcher; åtte sluttbilder er inspisert. Native input og releaseytelse er fortsatt UNVERIFIED.

## Fortsett her

Brukeren opplyste 11. september at vedkommende er borte hjemmefra og ikke kan låse opp skjermen nå. Ikke gjenta opplåsingsforespørselen eller start flere native inputforsøk mens dette gjelder. Bygging, statisk runtime-inspeksjon og pakkekontroll er allerede utført; manglende interaktive kontroller utsettes til skrivebordet er tilgjengelig. Ingen overvåking eller bakgrunnsjobb er startet.

1. Prøv Archive16 med ekte input og en ny leser når dette er mulig; registrer begrunnelsen før fasit vises. Bruk også P04/P05-spillerarket i Menu14 til den komplette papirprøven. Dokumenter begrunnelser og tidsbruk; egenkontrollen er allerede gjort. Før større historieproduksjon må åpne kanonvalg og innholdstak fryses.
2. Når brukeren har tilgang til et opplåst Kubuntu-skrivebord: kontroller Recovery15s faktiske knapper, Escape og tastaturfokus, listesider og valg av tidligere sak, avbryt fra meny/pause, Fortsett etter prosessgjenstart og begge forsøksveier. Recovery15 inkluderer Menu14 og NightSky12; en full ny tur på samme pakke kan dekke begge. Bruk nye, isolerte testprofiler.
3. Kontroller de virkelige fotografiene, inkludert gjenåpning, og himmelen under vanlig gange og raske vendinger. Kjør separat releaseytelse med alle frameintervaller beholdt.
4. Korriger eventuelle funn før kandidaten kan erstatte Visual10. Følg deretter M1–M3 i designbibelens roadmap: papirprøve, trygg lagring/områdeovergang og kort spillbar arkivsekvens. Ikke start alle nye systemer og miljøer samtidig.

Bygg himmelkandidaten med `bash Automation/build-night-sky12-linux.sh`; dette bevarer de to eksisterende lablysatlasene. `build-visual10-linux.sh` regenererer og baker scenen på nytt, nå også med himmelpasset. Eldre genereringsskript er ikke oppskriften for et identisk bakt releasebygg.

Behold Unity 6000.3.22f1 / URP 17.3.0 / Input System 1.20.0 / Blender 4.5.13, originalmodeller og metadata, NASA-kildespor og øvrige lisenser, VT323/Scott Buckley og 1419.900 / 1420.110 / 1420.405 MHz, 4/7, −39 LY og 47 spillsekunder. Ingen brukerlagring, synkronisert `sources/` eller andre prosjekter er endret. Ingen ny bakgrunnsjobb er planlagt etter leveringen.
