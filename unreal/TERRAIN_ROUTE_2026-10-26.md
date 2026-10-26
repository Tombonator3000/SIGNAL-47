# Terreng og natur langs ruten

Tom bestilte punkt 3 i den visuelle prioriteringen og ba uttrykkelig om flere agenter. Arbeidet omfatter landskapet langs SARO-innkjøringen, utvalgte formasjoner langs hovedveien og forgrunnen sør for Caliche. Original dag-/nattehimmel, besøksparkering, inngang, kontor, lamper, historie og personlige lagringer videreføres.

## Utgangspunkt og retning

Installert referanse er nærdetalj-R4, binær `26673e1b033ca1defa3cc2a2b7a4736028fe20af3893616535fffad3729e2819`. Før produksjonsendringer ble 1 527 relevante filer kopiert og verifisert. Fire faktiske førbilder ved 17:20 viser SARO fra parkeringen, innkjøringen fra 160 meter, hovedveien ved beitet og Caliche sør. Alle er åpnet og vurdert. Bevisrot: `/home/tom/scratch-bm/Signal47-terrain-20261026`.

Førbildene viser brede trinn og plane flater i fjellene, jevne veikantbelter og spredte enkeltplanter. Den eksisterende delte geologien er gjeldende utgangspunkt; arbeidet erstatter ikke verdenen med en leverandørs demobane. Kjøreflate, gangruter, historiepunkter og områdenes felles transform beholdes.

- Den originale Blender-geologien får sammenhengende erosjon gjennom skråningsfot, bergvegg og platå, med tettere tverrprofil. De dominerende nordlige fjernryggene får én erodert bergvegg i stedet for gjentatte hyller. Eksisterende beskyttelse av hovedvei/OldRoad beholdes.
- Nære formasjoner settes sammen av et lite utvalg eide sandsteinsklipper, steiner, småstein og tørre planter. Større formasjoner får nedfall og spredt vegetasjon rundt foten, med fri vei og lesbare siktlinjer.
- En målt kant på omtrent 70 cm ved SARO z=100 ligger i overgangen mellom to eksisterende dekorgrunnmesher, utenfor selve kjøreflaten. En smal lokal overgang skal dekke kanten innen det eksisterende beltet som er unntatt fri gange; ingen generell høydeendring innføres.

## Kilder og avgrensning

Utvalget kommer fra de allerede nedlastede pakkene Old West Learning og JustB Studios Modular Post Apocalyptic Desert Environment. Åtte meshfamilier er valgt. Originalpakkene forblir uendret. Leverandørfiler og bearbeidede native kopier ligger i ignorerte lokale Content-mapper; oppskrift, pakkenavn og proveniens ligger i `Signal47/Tools/routenature`. De er ikke merket som CC0 eller lagt i det offentlige repoet.

Old Wests opprinnelige steinmateriale har RVT-/wetness-avhengigheter fra demoverdenen. De lokale kopiene får derfor direkte PBR-materialer fra sine egne faktiske teksturkart. Kanalvalg kontrolleres i originalenes native materialgraf. Ingen ny RVT-verden eller motorutvidelse innføres.

Prosjektbiblioteket ble søkt med `Unreal terrain landscape rocks foliage`, deretter bredere med `Unreal terrain --match any`. Den pinnefestede [world-building-referansen](https://github.com/scenario-labs/skills/blob/f0adb5b5ebc1fc8f7e333554d2aeb767c5558d6b/skills/game-engines/unreal/scenario-unreal-world-building/SKILL.md) ble lest som arbeidsflytreferanse. Dens native skript er ikke prøvd i Unreal, og ingen slik kode er tatt inn. VibeUE-profilen er også UNVERIFIED og brukes ikke som ny avhengighet. Prosjektets egne import-, modell-, høyde- og områdeløsninger er grunnlaget.

## Gjeldende produksjonsstatus

RN2-geologien er generert i Blender og gjenåpnet/verifisert: 179 476 trekanter, seks materialplasser og gyldige teksturkoordinater. Ingen polygoner bryter veimaskene. Native import består med alle synlige kildetrekanter bevart og en separat 45 % representasjon for ray tracing. Ingen global grafikkgrense er endret. Dette er kilde-/importbevis; endelig bilde- og minnekontroll føres etter faktisk pakket resultat.

Runtime plasserer sju sammensatte grupper, med nedgravd steinfot fra faktiske meshmål, grus og to vegetasjonslommer rundt hver formasjon. Fire grupper ligger langs SARO-innkjøringen, to ved hovedveien og én sør for Caliche. Hele fotavtrykk holdes unna veier, gjerder, master og historiepunkter. Store steiner publiseres i både gang- og kjørekolliderne; smågrus og planter har ingen egen kollisjon, skygge eller RT. Eksisterende natur fjernes bare innen de erstattede gruppene. En original overgang på 552 trekanter dekker den målte terrengskjøten.

Lesende kontroll bekrefter at de brede terrassene i førbildene faktisk er den delte `SM_Caliche_Mesas`, og at gamle alternative mesa-eksporter er skjult. Overgangens sampler og runtime bruker samme fysiske X/Y i både SARO- og veikoordinater. Spillbare høydefunksjoner er uendret.

Native naturimport består med åtte modeller og en lukket avhengighetskjede på 40 private pakker. Alle 70 stagingpakker og de originale kildefilene er byteidentiske med før importen. Detaljert kildegeometri, tilgjengelige MeshDescription-nivåer, Nanite-/bygg-/LOD-innstillinger og mål kontrolleres mot originalene. Ingen full vertexbufferhash hevdes. Ferdig import er idempotent, slik at korrekte modeller gjenbrukes ved gjenopptakelse.

Importavbruddene er bevart som feilbevis: første forsøk manglet kommandolinjens subsystem-fallback; andre forsøk brukte shaderressursenes `GetUsedTextures` i NullRHI. Den rettede naturimporten verifiserer den faktiske materialgrafen, alle forbindelser og det eksakte private tekstursettet. Senere avbrudd avdekket at `GetNumTriangles(i)` teller regenerert RenderData, også for automatisk reduserte LOD-er, og ikke den originale MeshDescription-kilden. Klippens genererte fallback endret seg fra 15 442 til 16 525 trekanter, mens kilden fortsatt har 1 998 242. Slike bygde nivåer rapporteres separat med avgrenset toleranse; eksakte kildedata og innstillinger kontrolleres fortsatt. Ingen ugyldig import er kalt bestått.

## Første pakkede vurdering og retting

R1 ble avvist og er ikke installert. Tre faktiske bilder viste en detaljert klippe med god bakkekontakt, men de dominerende fjelltrinnene var nesten uendret og smågruset for lyst og jevnt. Skjermstråler mot kildegeometrien viste at de sentrale og høyre trinnene var `far_northern_ridges`, som første endring hadde latt stå urørt. RN2 endrer denne profilen, øker den sammenhengende erosjonen og glatter fjernnormalene. Stor stein og lav klippefot bindes til faktiske plasserte grenser; gruslinjene erstattes av spredte flekker med varm støvfarge og lange opphold.

R1 avdekket også en uinitialisert UV-tetthet i det nye terrengskjørtet og en endret radkontrakt i sluttkrediteringen. Skjørtet har nå eksplisitt 500 cm UV-tetthet. Alle Fab-/Pure Sky-krediteringer beholdes innen de eksisterende fire genererte radene; den opprinnelige testen er uendret. R1s avgrensede RT-prøve var under 80 MiB, men dette overstyrer ikke de påviste feilene eller behovet for nytt faktisk bildebevis.

RN2 Blender-/nativeimport og minimalt editorbygg består. De 40 ferdige private naturpakkene gjenbrukes byteidentisk. Bygg og grafiske jobber kjøres sekvensielt selv om kildearbeidet fordeles. Sluttkontrollen skal dekke de endrede utsnittene og én sammenhengende eksisterende kjøre-/gangreise fra Caliche til SARO. Den erstatter ikke menneskelig fullnatt eller en generell FPS-måling.

## R2, faktisk pakket bildeaksept

R2 har binær `6d0ae58f452afcf45b459198588723676ad36bae8355787f4f83ac60537cf901`. Faktisk IoStore-kontroll finner alle 40 private naturpakker, åtte modeller, geologi, seks himmelassets og tre armaturmodeller. Originale Northwood-/JustBStudios-kildepakker er ikke med. De seks himmelassetene og øvrige beskyttede modeller er byteidentiske med utgangspunktet.

Seks faktiske sluttbilder er åpnet av både integrator og QA: hovedvei ved beitet, SARO-innkjøring, klippefot på nært hold, parkering, Caliche sør og parkering klokken 03:00. De fire daglige oversiktene har samme plassering, retning, klokkeslett 17:20 og High/Clean-oppsett som førbildene. De dominerende fjelltrappene er erstattet av sammenhengende skråninger og eroderte skuldre. Nærklippen har tydelige overflatedetaljer og bakkekontakt, med en lavere steinmasse mot veien. Gruset er varmere og mer uregelmessig. Natthimmelen og armaturene er beholdt.

Samme første bildeprosess består de eksisterende 1 581 selvtestene, uten UV-ensure eller kredittfeil. Den avgrensede hardware-RT-prøven har 598 aktive CSV-prøver og maksimum 75,8328 MiB `TotalAlwaysResidentSizeMB`, under den uendrede 80 MiB-grensen. Dette er ikke total VRAM eller en måling av FPS over hele ruten.

Runtime bekrefter sju klippegrupper, 84 solide steinfotavtrykk, 727 smågrusinstanser, 184 gress- og 278 buskinstanser. Fem erstattede gamle naturinstanser er fjernet selektivt. `runtime-r2/primary` og `runtime-r2/remaining` inneholder originalbilder, kommandoer og resultater.

Den eksisterende `S47EveningRoute=direct` består på samme R2. Faktiske kjøre-/gangkontrollere kjørte 2 622,6 meter fra Caliche til SARO, registrerte ankomst innen første sekund av 18:00 og fullførte gang til telefon, D01 og isolert disk-/verdenslasting. Ingen ny enginefeil eller RT-budsjettadvarsel. Dette er skriptede fysiske kontrollere og API-dialog, ikke menneskelig tastatur-/Xbox-prøving. Kollisjonspubliseringen er kildegjennomgått; separat HighwayAudit er ikke kjørt. Alle 70 personlige Saved-noder er bevart gjennom prøvene.

Integratorens `runtime-r2/visual-acceptance.json` binder alle seks faktisk åpnede bilder til eksakt pakke og kildefrys. QA fører en uavhengig gjennomgang. Ingen hel screenshot-matrise, ny fullnatt eller generell ytelsespåstand inngår.

## Lokal levering

Samme R2 er installert under `/home/tom/Games/Signal47/Linux`. `installation-r2.json` er `INSTALLED_VERIFIED`: alle 68 payloadnoder er identiske med prøvd kandidat, og alle 70 personlige Saved-noder, inkludert 50 regulære filer, er bevart. En full verifisert kopi av den tidligere nærdetalj-R4-installasjonen ligger i `previous-installed-r2/Linux`.

Faktisk standardstarter består tolv kontroller i `installed-smoke-r2/result.json`. Integrator og QA har åpnet det installerte innkjøringsbildet. De nye modellene, opprinnelig nattmateriale og detaljerte armaturer er aktive; normal avslutning, identisk payload og bevarte Saved er bekreftet.

Canonical prune er anvendt etter levering: tre gamle nærdetalj-payloadsett med 21 genererte filer er pensjonert, 11 610 438 194 logiske bytes. Dette er ikke en påstand om fysisk frigjort plass på reflink-lagringen. Gjeldende terreng-R2, eldre terreng-R1 og full nærdetalj-R4-rollback beholdes. Aktiv `/home/tom/scratch-bm/Signal47-sc01-20261026/candidate-r4/Linux` er særskilt beskyttet. Kilder, Blender-filer, assets, personlige lagringer, bilder og øvrige bevis er bevart.

Ingen ny PR, merge, push eller ekstern spillpublisering inngår. Experimental Mesh Terrain, MetaHuman, Lumen Lite og FSSS er separate vurderinger/arbeider. Dette passet oppgraderer den valgte ruten og fjellutsiktene; det er ingen påstand om ferdig ny kunst for hele spillverdenen.
