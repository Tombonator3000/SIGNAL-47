# SIGNAL / 47 — aktuell overlevering

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
