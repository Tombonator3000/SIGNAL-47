# Chapter 09 — Den andre eksponeringen

**Levert: spillbar alpha med hele første kapittels forløp.** De åtte avtalte kapittelkriteriene er PASS på den angitte Kubuntu-maskinen og utgaven. Dette er ikke hele det planlagte SIGNAL / 47-spillet eller ferdigpolert sluttgrafikk. GPU-tid, faktisk lytting og ekstern førstegangstesting er uttrykkelig uverifisert.

## Spill og identitet

Kjør `./Spill-SIGNAL47.sh` fra prosjektroten, eller pakk ut `Artifacts/Releases/SIGNAL47-Chapter09-86a28b30ad94-Linux.tar.gz` og kjør den utpakkede `./Start-SIGNAL47.sh`. Den faste lokale pakken er `Artifacts/Releases/Chapter09-86a28b30ad94/`. [README](../README.md) beskriver kontroller, lagringssted og gjenbygging.

| Identitet | Verdi |
| --- | --- |
| Kildecommit for spillinnhold | `9329a2d819108796c6f731cc305f16dc3250c651` |
| Unity-kilder SHA-256 | `86a28b30ad94fc9e67aa0bf0558f0a51f511adadc76323fcf69eca7c72d8c8b9` |
| Målt bygginnhold SHA-256 | `6b0c99dc455267875ac836594e6f1478fb58515d337313032604325c2de6056b` |
| Pakket innhold SHA-256 | `1a68b4d210a7a470e411105ca7e1dbbe5033d58bf0d5092657d7f43796b5dcdf` |
| Komprimert arkiv SHA-256 | `bce4c4c7423a4e7f42792991553fb1ee3386d0f3a9918a2cdb14aee8ab9a1696` |

Byggmanifestet var rent ved stempling. Senere endringer gjelder testverktøy, launcher, dokumentasjon og bevis; spillets Unity-kilder og byggfiler er uendret og kontrollert på nytt ved pakking. Den lille Linux-startbinærens hash identifiserer ikke alene spillinnholdet. Pakkingens tillegg, filmodi og alle 173 filer er dokumentert i pakkemanifestet.

## Valgt design og faktisk innhold

Spilleren oppdager et avvik, undersøker det med fysiske verktøy, velger en vanlig forklaring og tester den. Et tett SARO-kapittel ble valgt: dette gjorde fungerende mottaker, telefon, servicegård og feltkamera til en sammenhengende etterforskning. Bil/motell ville krevd nye kjøretøy/steder før mysteriet fungerte; bare mer terminalanalyse ville gitt mindre fysisk og visuell variasjon. Instrumentanalysen støtter derfor fotoarbeidet.

Åpningen beholder nattevaktens kalibrering, returlinje, fysiske utskrift, telefon og antennebevegelse. Det gamle sluttkortet fører videre til servicegården. Spilleren leser S-03s motorlogg og tar første bilde, går gjennom norddøren til en ny fotolab, laster forseglet film, overfører og samler kontaktkopien. Lupen og referansekartet lar spilleren finne den doble lyse stripen og identifisere B-12. S-07s trestripesystem og en ny antennekommando er prøvbare feilspor med nyttig tilbakemelding.

Ved B-12 velger spilleren en passiv skjerming/folding med isolert motor, eller en aktiv lokal 042-graders referanseprøve. Lys og referansens stilling endres forskjellig; den direkte observasjonen har én fysisk stripe. Spillerens andre eksponering gjengir en ekstra stripe som beholder tidligere retning. Tilbake i laben framkalles og sammenlignes begge opptak, uriktige konklusjoner kan avvises, og spilleren leverer en lokal rapport ved arkivbordet. Rapporten avgrenser hva akkurat forsøket viser, mens det større spørsmålet om −39 LY står åpent.

Dette er nytt kapitteldesign innenfor bestillingen. 1419.900/1420.110/1420.405 MHz, 4/7, −39 LY og hendelsen 47 spillsekunder etter død telefonlinje er bevart. Motellutkastet er fortsatt inaktivt; bil, kamp, crafting og økonomi er ikke brukt som fyll.

## Åtte bindende kriterier

Kriteriene ble registrert før sluttkontroll i kildehistorikken. Bevisene ligger samlet i [final-9329a2d](Evidence/Chapter09/final-9329a2d/); de gjelder dette kapitlet, ikke gamle pass08-rapporter.

| Kriterium | Status | Konkret bevis |
| --- | --- | --- |
| 1 Hel reise | PASS | Passiv normalstart gjennom alle deler i tre prosesser, 54 kontrollpunkter. Aktiv normalstart til ny avslutning i én sammenhengende kjøring, 52 kontrollpunkter. Ingen teleportering, direkte løsningskall eller redigerte hovedreiselagringer. |
| 2 Spillbar romlig utvidelse | PASS | Kontrollrom, servicegård, nordlig fotolab og B-12 nås og forlates gjennom faktisk kollisjon og WASD/mus. Begge ruter og retur til lab/arkiv er gjennomført. |
| 3 Etterforskning med konsekvens | PASS | Motorlogg, fotografisk analyse og to ulike kontrollmetoder; synlig endret lys/referanse og ulik observasjon/rapport. Feil kart, hypotese og konklusjoner er prøvd. For tidlig B-12-forsøk avvises uten lås. |
| 4 Fotografisk beviskjede | PASS | To virkelige sceneopptak, fysisk tretrinns framkalling, markering, lupe/panorering og sammenligning. Uavhengig kritiker kan peke ut stripene uten metadata. SHA/ID/opptaksdata bevares ved gjenstart og gjentatt åpning. |
| 5 Signatur og avslutning | PASS | Normal observasjon og framkalte kontrollbilder viser forskjellig antall referanser. Spillerens markering/sammenligning åpner en begrunnet lokal rapport og nytt kapittelutfall for begge metoder. Eksisterende tids-/signalkontrakter er regresjonskontrollert. |
| 6 Varig og robust tilstand | PASS | Faktisk QUIT og ny prosess før første framkalling og etter kontrollfoto. Pause fryser framkalling. Backup, ugyldig/duplisert data, manglende foto med ny eksponering, skrivefeil og eksplisitt avslutning uten siste endringer er prøvd. Ny sak bevarer eksportarkivet. |
| 7 Presentasjon og kontroll | PASS | Begge ruters originalbilder og lupe er inspisert; tidligere mørke rekvisitter, uleselig kart/kort og takglød er korrigert. Nye hendelser har målt lydutgang. Mus/volum/fullskjerm er prøvd og gjenlastet; vanlig Alt+Tab og retur virker i pakket spiller. |
| 8 Verifisert leveranse | PASS | Utviklingsregresjon, begge native ruter, gjenopptakelse/feilprofiler, visuell kontroll og sluttbyggmåling. 173 filer/hash/størrelse/modus kontrollert etter ny utpakking. Vanlig start→spill→pause→QUIT med originalbilder og exit 0, uten observasjons- eller smoke-flagg. |

Native automatisering bruker bare tastatur/mus til handlinger. Observasjonsdata hjelper rute-/tilstandskontroll og plassering av fotomarkører. Dette dokumenterer betjening og sammenheng; det er ikke blind spilleroppdagelse. Direkte smoke-/kontrakttester og redigerte feilprofiler er merket separat. [Kjøringsoversikt](Evidence/Chapter09/final-9329a2d/run-summary.json), [visuell kritikk](Evidence/Chapter09/visual-final-review.md) og [lagrings-/lydgjennomgang](Evidence/Chapter09/passive-persistence-audio-review.md) beskriver avgrensningene.

Aktiv skriptet tur var 311,02 sekunder mellom første/siste kontrollpunkt, inklusive negative handlinger og gjentatt lupe-/notatbokbruk. Passiv tur var 317,23 sekunder summert over tre prosesser, med rominspeksjon og uten pausen mellom prosessene. Dette er kjent-løsning-automatisering. Ambisjonen 20–30 minutter for en førstegangsspiller er ikke målt eller dokumentert; ingen venting eller redusert gangfart er lagt til for å fylle tid.

## Faktisk ytelse og lyd

Release-bygg, Intel Core Ultra 5 225U, Intel ARL/Mesa 26.0.8, Kubuntu/Ubuntu 26.04.1, Linux 7.0.0-31, OpenGLCore, 1280×800 Ultra. Primærskjerm DP-3 var 75 Hz, strøm tilkoblet, `targetFrameRate=75`, `vSyncCount=0`. Ingen editor-/Blender-bygg kjørte parallelt. Spillet beholdt fokus hele målevinduet.

Etter normal prolog og første eksponering ble det ventet 20 ekstra sekunder før 121,397 sekunder med lab, referanse/hypotese, feltprøve, reell kontrollfoto-rendering/eksport, retur, andre framkalling, sammenligning og avslutning. QA-bilder og lydstikkprøver ble tatt i egne kjøringer. Samtlige 9104 bildeintervaller er beholdt i [frames.csv](Evidence/Chapter09/final-9329a2d/Final-Performance/frames.csv); tallene er regnet på nytt fra hele filen.

| Måling | Resultat | Avtalt grense |
| --- | ---: | ---: |
| Gjennomsnitt | 74,993 fps | minst 59 fps |
| p95 bildetid | 15,897 ms | høyst 17,2 ms |
| p99 bildetid | 16,187 ms | under 20 ms |
| Lengste intervall | 20,087 ms | ingen intervaller over 50 ms |
| Intervaller over 50 ms | 0 | 0 |

Percentiler bruker nærmeste rang i sorterte monotone Unity Update-intervaller. Målt hovedtrådarbeid var 1,388 ms i snitt; presentasjonsventing 11,940 ms. GPU-tid var 0 fra verktøyet og er **UNVERIFIED**, ikke null GPU-belastning. Unitys målte minnetopp var 99,73 MiB; prosessens VmHWM var 314,41 MiB, målt separat over hele prosesslivet inklusive last. Dette er forskjellige mål. [Full rapport](Evidence/Chapter09/final-9329a2d/Final-Performance/performance.json) og [uavhengig CSV-kontroll](Evidence/Chapter09/final-9329a2d/Final-Performance/independent-csv-check.json) inneholder råbetingelser og terskler.

Eksplisitt Fortsett ligger utenfor dette kontinuerlige målevinduet. Senere native feil-/gjenstartskontroller observerte spilleklar tilstand innen 0,421 sekunder etter klikk; dette inkluderer 0,42 sekunders inputstabilisering og opptil 0,1 sekunders tilstandsutvalg, og er en øvre observasjonsgrense, ikke presis isolert disk-/lastetid. De to første passive gjenstartene hadde ikke dette tidsmålet og er ikke gitt et etterkonstruert tall.

Lydutgang er målt ved 48 kHz i relevante prolog-, kamera-, lab-, kontroll-, sammenlignings- og sluttfaser. Passive stikkprøver hadde høyeste peak 0,222733 (−13,04 dBFS) og ingen sampleverdier over full skala i det målte utvalget på kanal 0. Det beviser ikke kontinuerlig klippefrihet eller andre kanaler. Faktisk lytting, høyttalerlyd og subjektiv miks er **UNVERIFIED**. Eksakt stillhet under pause er heller ikke fastslått av de aggregerte overgangsvinduene. Mus-/voluminnstillinger er kontrollert gjennom faktisk UI og prosessgjenstart.

## Ressurser, metode og vesentlige rettelser

Nye forstørrer-, kar-, vask-, flaske-, stol- og B-12-modeller er faktisk produsert i Blender 4.5.13 med redigerbare `.blend`, eksport og Unity-import. Eksisterende originale modeller og lisensressurser er bevart; [lisensoversikten](THIRD_PARTY_NOTICES.md) inkluderer kapittelressursene, VT323 og Scott Buckley. Ingen nye kjøp eller fakturerbar bilde-til-3D-tjeneste ble brukt.

[Fotolabmålet](VisualTargets/Chapter09/photolab-target-v1.png) og [feltmålet](VisualTargets/Chapter09/field-target-v1.png) er **genererte, foreløpige målvisninger**, ikke implementasjonsbevis eller tidligere brukergodkjenning. Sammenligning og uavhengig kritikk førte til korrigert praktisk belysning, tydeligere kort/kart, leselig filmstripe, mindre takglød og sannferdig planlagt spor på CRT-en. Materialrikdom, kontaktlys og enkelte kantede silhuetter når fortsatt ikke hele målbildenes ambisjon. Nødvendige spor, rutemerker og nærinteraksjoner er lesbare i runtime; dette er alpha-grafikk.

Kameraet lagrer en 960×600-render fra spillerens virkelige kamera. En bevisst, foto-eksklusiv geometrisk respons gjør at filmen registrerer den ekstra referansen; normalvisningen gjør det ikke. Kamera/world-metode, tid, bildehash og stabile ID-er fryses ved eksponering. Framkalling endrer tilgjengelighet; den tar aldri et nytt bilde. Opprinnelig sidecar beskriver eksponeringen, mens framkalt/undersøkt tilstand ligger i sakslagringen.

Vesentlige feil som ble funnet og rettet: køet checkpoint ved Quit, receiver-LED etter Continue, eksportmetadata skrevet før ferdig SHA, klikk gjennom innstillingspanelet, og Kubuntu-fullskjerm med feilskalerte IMGUI-koordinater. Native fullskjerm bruker nå skjermens opprinnelige oppløsning og gjenoppretter tidligere vindusstørrelse. En eldre scene-lagringshook overskrev lablyset; rekkefølgen er nå eksplisitt og idempotent. Berørte kontroller ble kjørt på nytt før sluttbygget ble godkjent.

Testverktøyene krevde også rettelser, uten endring av spillet: feilprofilens konvolutthash måtte ha samme store heksadesimale bokstaver som formatet; retur fra telefonbordet måtte følge gangveien fremfor en diagonal gjennom dørkarmen. XWayland root-bildefangst og root-fokusforespørsel virket ikke. Installert KDE Spectacle fanget det faktiske aktive spillvinduet, og vanlig Alt+Tab bekreftet fokusbytte. Tidligere mislykkede verktøyforsøk er beholdt separat i `VerificationCorrections/`; de brukes ikke som PASS-bevis.

## Skills og videreføring

| Skill/metode | Kilde og faktisk bruk |
| --- | --- |
| gauntlet-loop | Installert lokal SKILL.md og visuell-/ytelsesreferanse lest. Definer→inspiser→bygg→kjør→vurder→rett→checkpoint anvendt gjennom hele kapitlet. |
| Brainstorming | [Original obra/superpowers](https://github.com/obra/superpowers/blob/b36e0829c6d0140e93cfef2ca599b1b07d4a7797/skills/brainstorming/SKILL.md) commit `b36e0829c6d0140e93cfef2ca599b1b07d4a7797`, gjennomgått og installert gratis gjennom skill-installer i `~/.codex/skills/brainstorming`. Original MIT-lisens og låst kildeinfo er lagret lokalt. Valgt SARO-design fulgt med brukerens delegerte rutinevalg. Dette er metodebruk, ikke en ny motorintegrasjon. |
| Dream Loop / imagegen | [Original achimala/dream-loop](https://github.com/achimala/dream-loop/blob/9bddb901f7d071cfefdd21e264267c757177a9df/SKILL.md) commit `9bddb901f7d071cfefdd21e264267c757177a9df` lest. Inkludert bildegenerering, faktiske runtime-bilder og separat kritiker brukt. Ingen betalt 3D-API. |
| skill-creator | Lest før én prøvd arbeidsmetode ble lagt til eksisterende lokale Gauntlet som `references/unity-linux-chapter-validation.md`. Identiteten er bevart. Filplasseringer og SHA er i `skill-learning.json`; lokal lagring innebærer ikke synkronisering til andre maskiner. |

Gameplay, lagring, miljø og uavhengig kritikk hadde avklart filansvar. Hovedagenten integrerte, kjørte editor/bygg og alle eksklusive native inputtester. Subagentenes ferdigmeldinger ble kontrollert mot faktisk kjøring.

Forrige FieldCamera08-pakke og originale 169 innholdsfiler matcher fortsatt payload `672a8a768f0069dc1e06e561c50ed496cb1bca7d8ca3772374aeea88bdb8a020`; den eksisterende ekstra LES_MEG-filen er skilt fra originalpayloaden. Synkroniserte `sources/` og andre prosjekter er urørt. Arbeidet ligger på `gauntlet/chapter-one-09`; ingen merge eller offentlig publisering er utført.
