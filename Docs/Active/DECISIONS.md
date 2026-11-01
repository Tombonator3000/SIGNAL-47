# Gjeldende beslutninger

**Status:** Gjeldende. **Gjelder:** Retning og arbeidsregler for SIGNAL / 47. **Sist kontrollert:** 1. november 2026.

Dette dokumentet samler beslutninger som ellers ligger spredt i historiske notater. Det dokumenterer føringer, ikke at funksjonene er ferdig implementert. Se [STATUS](STATUS.md) for levering og [TODO](TODO.md) for arbeidet som gjenstår.

## Nattskift 1. til 2. november 2026

Tom bestilte1. november arbeid fra kl.22:00 til kl.08:00 neste morgen, Europe/Oslo, for å gjennomføre mest mulig av TODO. Prioriter felles fysisk PC-betjening og lysstyring, deretter tydelig utstyrsrespons, etterprøvbare arbeidsrelaterte hendelser og skrekk som reagerer på spillerens handlinger. Se [nattprioriteringene](TODO.md#nattskift-1-november-kl2200-til-2-november-kl0800). Aktiv trådautomasjon er `signal-47-nattskift-22-08`; dette er ett nattskift, ikke en stående daglig bestilling. Vanlige prosjektregler om kildevern, reduserte tester, én tung jobb og samlet opprydding gjelder.

## Freeform-verdensprøve 1. november

Tom bestiller Survey Station01 åpen og tilgjengelig i Freeform, med kontroll av opplevd grafisk regresjon og ruten til/fra stasjonen. Alderete-avkjøringen ved SARO skal brukes som gårdsadkomst og åpnes; kuene knyttes til gården nærmere SARO, på høyre side etter motellet slik Tom angir. Dette overstyrer eldre avgrensning av gården som stengt bakgrunnsdekor. Ved motellet fjernes skiltet nærmest veien og det gjenværende får mer neon. De fortsatt rapporterte kontorveggene og brå verdensovergangene er åpne P1-funn. [Oppgaver og originalbilder](TODO.md#nye-freeform-spillfunn-1-november-survey-station-rute-motell-og-gård).

## Pakkehåndtering og gjenbruk, oppfølging 1. november

Tom ønsker enkel billasting: Hold en pakkbar gjenstand og bruk egen parkerte bil for å legge den i lasterommet. Lasting prioriteres foran kjøring når hendene er opptatt, med tydelig kvittering og bevart cargo-tilstand. Den fremste gjennomtrengelige av to gjerderekker langs veien fjernes. Den gamle telefonkioskmodellen ved bensinstasjonen erstattes ved å gjenbruke den nyere modellen på andre siden av gaten. Se TRUCK-01, WORLD-03 og PHONE-01 i [TODO](TODO.md#nye-freeform-spillfunn-1-november-survey-station-rute-motell-og-gård).

## Prosjekt og kanon

| ID | Beslutning | Grunnlag |
| --- | --- | --- |
| D-01 | Videre spillarbeid gjelder Unreal-utgaven. Web og Unity bevares som historikk og referanse. | Toms beslutning 22. oktober, [rotens AGENTS](../../AGENTS.md) og [Unreal-reglene](../../unreal/AGENTS.md). |
| D-02 | Spillet er kosmisk etterforskning ved SARO i New Mexico, 1986. Bevar 1420.405 MHz, 47 sekunder, 27 antenner og azimut 026. Spilltekst er engelsk, prosjektdokumentasjon norsk. | Gjeldende AGENTS og historisk kanonreferanse. |
| D-03 | Faktisk kode og siste relevante test-/leveringskvittering avgjør hva som er implementert. Designbilder, manus og en main-merge er separate fra pakket, installert og publisert spill. | Gjeldende arbeidsregler og [STATUS](STATUS.md). |
| D-04 | Lisensierte MetaHuman-/Fab-kilder og avledede private assets holdes utenfor offentlig Git. Bevar egne redigerbare kilder, proveniens og kreditering. | [Unreal-reglene](../../unreal/AGENTS.md) og de enkelte assetrapportene. |

## Historie: hvilke planer styrer?

Nyere eksplisitte beslutninger går foran eldre planer for den delen de endrer. De erstatter ikke automatisk alle andre krav.

1. **Toms siste direkte instrukser** gjelder først.
2. **[Ny kveldsrekkefølge, 28. oktober](../Design/Story/KVELD_PLAN_2026-10-28.md)** styrer den endrede forbindelsen mellom SC01, signal/strøm, trafotur og SC02/SC03. Den senere mørke trafoturen og enklere SC01 overstyrer eldre rekkefølge der de er uenige.
3. **[SC02 til SC13-revisjonen](../Design/Story/HISTORIE_SC02_SC13_REVISJON.md)** styrer revidert dramaturgi og fire aktleveranser der den avviker fra den eldre planen. Ny talende sjåfør i SC12 og diktering av måletabeller er kuttet; H08/H09 er slått sammen. Dette er ikke en beskjed om å fjerne den eksisterende bakgrunnssjåføren i diner.
4. **[Implementeringsplanen](../Design/Story/HISTORIE_IMPLEMENTERINGSPLAN.md)** fører scenario-/verdenskontrakter og produksjonsgrunnlag under revisjonene ovenfor. Den gamle regelen om full separat levering per scenario er erstattet av fire sammenhengende akter og det reduserte testregimet.
5. **[MISSION_FLOW](../Design/Story/MISSION_FLOW_1800_ROSWELL.md)** og eldre detaljvedlegg brukes for kontrollverdier, bevis og delscener der nyere beslutninger ikke sier noe annet.

De engelske forfatterkildene i [Narrative/HorrorStory](../../unreal/Narrative/HorrorStory/README.md) er produksjonsgrunnlag. En skrevet scene er ikke en ferdig verdenshandling. Gamle lagrede saker skal ikke få retroaktive krav fra nye innholdsrevisjoner.

## Nye føringer fra spillprøven

Tom bestemte 31. oktober at **0.1, liten Roswell-alien**, kommer først: referansebilder, faktisk Tripo3D-modell, Blender-klargjøring, Unreal-import og bruk i spillets Glimpse-møter. Dette overstyrer arbeidsrekkefølgen, men lukker eller nedgraderer ikke de åpne krasj-/progresjonsfeilene. Den store jakt-alienen beholdes.

| ID | Beslutning | Konsekvens |
| --- | --- | --- |
| D-05 | Signalet rundt 00:13 skal utløse strømsvikten; trafoturen skjer senere i mørket, og første egentlige SC02-møte flyttes mot kl. 02. | Kvelds-/strøm-/reisekoordinatoren og relevante tekster må samordnes. Dette er pågående arbeid, ikke ferdig levering. |
| D-06 | SC01 skal bruke klask mot ruta og avtrykk, med enklere lampekontroll. | Ikke viderefør den tydelige hånden eller unødvendige turer inn/ut som fremtidig fasit. |
| D-07 | Rolig kveldsarbeid og uro skal fylle tiden før midnatt; nye hovedhendelser må passe den endrede rekkefølgen. | Kveldens oppgaver og SKY01 skal gi sammenheng uten tidlige alienmøter eller ventetid som innhold. |
| D-08 | Godkjent terreng/himmel, eksisterende interaksjoner og personlige lagringer bevares under visuelle forbedringer. Nye konkrete spillerfunn er åpne selv om en eldre leveranse bestod tester. | Følg [feilrapporten](Reports/PLAYTEST_TODO_2026-10-28.md); ikke lukk nye feil med gamle matriser. |
| D-09 | Første personpass er MetaHumans som stille stående/sittende mannekenger på de eksisterende stedene. | Radio-kvinne, røykende mann og diner-sjåfør får poserte statiske erstatninger. Bevar dagens servitrise og alle samhandlings-/tilstedeværelsesregler. [Produksjonskontrakt](../../unreal/Signal47/Tools/metahuman/mannequins-plan.json). |
| D-10 | Nanite utvides etter innhold og faktisk nytte, særlig tette solide miljømodeller. | Ingen global konvertering av vegetasjon, glass eller effekter; bevar RT-fallback og collision. [Kildegjennomgang](../Research/Unreal/UNREAL_SKILLS_2026-10-28.md). |
| D-11 | Den ønskede visuelle retningen er et troverdig retro New Mexico-motell og sammenhengende 1986-farger gjennom dag/natt. | Bevar keramikkmønster, mørkt trepanel, KELVORA-TV og de tre detaljerte bilene. Bare den dårlige blokkbilmodellen skulle bort. Fargeforsøk må passe dagens `M_WebTone`. |
| D-12 | Finalen skal vurderes med en stor, langsom UFO-avsløring bak to skybanker som velter fremover fra hver sin side. | SC13 er anbefalt hovedplassering. Konsepter er regi, ikke implementert effekt. [Finalenotat](../Design/Proposals/FINALE_CLOUD_TANK_UFO.md). |
| D-13 | Dales kontor skal være et personlig, rotete konspirasjonskontor fra 1986. Forbindelsesgangen skal få reell plass og bedre geometri; kontor/gang/dører prioriteres før arkiv/fotolab. | Toms føring 29. oktober. Bruk eide originalmodeller før ny modellering/Tripo, bevar fungerende dokumenter, nøkkel/skuff/dører, lysdrivere og Saved. Ingen kjøp eller automatisk fysisk Dale-scene. Tom godkjente den viste og installerte kontorpiloten fra PR251 30. oktober 2026; denne er visuell referanse for videre romarbeid. |
| D-14 | Tom 30. oktober ber om alle14 eksisterende Tripo-figurer importert og plassert som statiske mannekenger foreløpig. | Bruk eksisterende rolleankre og scenevilkår, bevar originalriggene og fungerende dialog. Tripo-servitrisen er reserve ved kjøkkenet; MetaHuman ved disken beholdes. Ingen ny animasjonsløype bestilles nå. |

| D-15 | SARO-gangen forbedres først, deretter kontrollrommet, med samme metode som visuelt godkjent Dales kontor. | Toms bestilling 30. oktober. Bruk eksisterende eide modeller, egne PBR-overflater og faktisk før/etter i Unreal; bevar fungerende spillankre og lagringer. Tom godkjente begge rommene for nå 30. oktober, for installert PR258. |
| D-16 | Troverdig 1980-tallsdesign er en kjerne i spillet. SARO skal ha gjenkjennelige instrumenter, møbler, telefoner, beholdere og tekstiler, med forskjellige materialer og funksjonelle detaljer. | Toms presisering og fire kontorreferanser 30. oktober. Nytt detaljpass prioriterer RX3 og gjenbrukbare Tripo-armaturer; plakater monteres foran maling og blokklær fjernes eller erstattes. [Analyse og føring](../Design/SARO_1986.md). |
| D-17 | Gjenbruk først ved 3D-arbeid. Søk i eksisterende prosjektassets, Toms Tripo-modeller og eide kilder før ny modellering; bruk egnede bibliotekmodeller når samlet kvalitet og tidsbruk blir bedre. | Toms bestilling 31. oktober etter gjennomgang av arbeidsprinsippet. [Konkret arbeidsflyt](../../unreal/AGENTS.md). Bevar plassering, retning, hengsler, gulv og interaksjoner ved teknisk utskifting; sluttkontroll i Unreal. Verktøyvalg er fleksibelt og innebærer ingen automatisk installasjon, betaling eller publisering av råassets. |
| D-18 | Prøv Toms nye Tripo-kjøretøy: truck ved dineren, politibil i politirollen og jeep som kjørbart SARO-kjøretøy. Den gamle SARO-trucken beholdes på sideparkeringen. | Toms bestilling 31. oktober under spilltest. Prøvebytte, ikke endelig kjøretøyvalg. Identifiser originalmodellene, klargjør i Blender og bevar progresjon, inn-/utstigning og lagring. PLAY-31-14 i [TODO](TODO.md) fører implementeringen. |
| D-19 | Bærbar SARO-walkietalkie på skrivebordet ved start, med pickup og uvanlig skurring nær aktive aliens. Alienmøter skal også ha et tydelig lydsignal eller en musikalsk markering, samordnet med radioen. | Toms bestilling 31. oktober. PLAY-31-20/21 i [TODO](TODO.md) fører implementering. Senere Dale-kontakt er foreløpig et forslag. Bevar eksisterende Rom6-radio og dens historie-/lydrolle. |
| D-20 | Stjernehullet skal være større og tydeligere; signalet klargjør meteorregnet, utgang fra bygget starter det, og nok meteorbaner skal gå bak og skjules av tomrommet. | Toms bestilling 31. oktober. PLAY-31-23 og gjenåpnet FX-01 i [TODO](TODO.md). Denne regien går foran tidligere hendelsesklokke alene og baner som bare unngår hullet. Bevar retning/antennekobling, lagring og fri kamerastyring. |
| D-21 | Dagens sammenhengende spill videreføres som Story med planlagt natt, personer, oppdrag og fast slutt. | Toms føring 31. oktober etter VotV-gjennomgangen. By- og motellreisene beholder sine begrunnede roller i historien. [Modusplan](../Design/Story/SARO_STORY_FREEFORM.md). |
| D-22 | Senere Freeform skal ha vedvarende dager og netter, SARO-arbeid og varierte tilfeldige hendelser, aliens og skrekk, uten Story-finalen. | Tom ba om separat modus. Freeform-grunnlaget er siden levert via PR297. Del SARO og relevante systemer, men gi modusene egne fremdriftsregler og lagringer. [TODO](TODO.md#story-og-freeform). |
| D-23 | Undersøkelsesløkken skal hovedsakelig foregå på SARO-basen, ved antennefeltet og i teknisk område. Første implementering styrker Story-kvelden før 00:00. | Toms bestilling 31. oktober: normalreferanse, avvik, fysisk kontroll, sikret bevis, sammenligning og forståelig neste handling. Gjenbruk Observing-kvelden, bevar den senere historieeskaleringen og gamle lagringer. VOTV-01 i [TODO](TODO.md) fører leveringen. |
| D-24 | SARO-utstyret får faste aktive roller, faktisk antennebevegelse og flere fysiske servicesteg i Story før midnatt. | Tom godkjente gjennomføringsplanen 31. oktober: Story først, S-03-service i flere steg, maskinregistrering og fysisk arkiv. Nabobygget føres til senere Freeform. [Plan](../Design/Story/SARO_STORY_FREEFORM.md#aktive-arbeidsstasjoner-og-antenneservice). |
| D-25 | Byen får en forsynings- og reparasjonsrolle i senere Freeform, med SARO som hovedarbeidssted. | Tom ba 31. oktober om å føre bensinstasjon, brukt/ødelagt utstyr, reservedeler og feeder-vedlikehold inn i planen. Første prøve er liten: sikringer, relé, aggregatdrivstoff, reservelager og enkelt døgnbudsjett. [Plan](../Design/Story/SARO_STORY_FREEFORM.md#byen-som-forsynings--og-reparasjonssted-i-freeform), MODE-03C i TODO. |
| D-26 | Freeform bruker vedvarende døgn, hendelsesregissør og fremgang gjennom mysterier og bedre base. | Tom godkjente gjennomføring: 75 aktive minutter per døgn innen valgt 60–90-minuttersramme, lokal recovery som standard og valgfri død/lasting per ny sak. Ingen våpen; bruk utstyr, dører, skjul og flukt. Fysiske besøk kombineres med tvetydige målefunn. [Gjennomføringsplan](../Design/Story/SARO_STORY_FREEFORM.md#godkjent-gjennomføring-vedvarende-døgn-og-hendelsesregissør). |

Tom utvidet 0.1 etter modellleveransen: Roswell-figurene skal ha nysgjerrig, sky AI med lusking/skjul, gode bevegelser og romlige småsteg/taklyder. Dev-menyen skal styre antall og velge stilleståing, aktiv lusking eller jakt. Dette er bestilt implementering; jakt blir et eksplisitt dev-valg, mens vanlige bakgrunnsmøter beholder den nysgjerrige rollen.

## Linux som distribusjonsmål, 31. oktober

Mottakerens Arch/niri/Nvidia-rapport understreker at Linux-distribusjonen skal beskrive faktiske arkitektur-, ABI-, driver- og øktkrav fremfor å kreve Ubuntu. Utviklingsmaskin og støttet spillermaskin holdes atskilt; kompatibilitet krever konkret grafisk prøve. [Linux-veiledningen](Guides/LINUX_RUNTIME.md) fører dagens krav og begrensninger. Bedre oppstartssjekker prioriteres i PLAY-31-38; Flatpak vurderes i SHIP-04 som tillegg, ikke som allerede vedtatt eller verifisert erstatning.

## Arbeid, testing og levering

- **Én aktiv status og todo:** Oppdater [STATUS](STATUS.md) og [TODO](TODO.md) ved relevante endringer. Nye leveringsnotater skal beskrive én leveranse og lenke hit for dagens status, fremfor å opprette konkurrerende «current»-dokumenter.
- **Arbeidsdeling:** [OPPGAVER](../../unreal/OPPGAVER.md) fører hvem som eier filene og den pågående jobben. `log.md` er append-only historikk, ikke en ekstra statusfasit. Bruk avgrensede subagenter aktivt med tydelige filansvar.
- **Test etter risiko:** Raske relevante kontroller og én målrettet prøve av det som er endret. Dokumentasjon trenger ikke motorbygg. Ikke fullmatrise per liten modell, menyfane eller PR. En nødvendig samlet kontroll før installasjon/offentlig release kjøres én gang; gjenta bare ved nye relevante endringer, feil eller konkret uavklart risiko.
- **Én tung jobb:** Blender, Unreal og pakking kjøres sekvensielt gjennom prosjektets vern. Ingen tester samtidig med Toms spilløkt. Ingen fremmed prosess stoppes for å skaffe testvindu.
- **Vanlig Git-levering:** Spill-PR-er går mot `main`; kontroller riktig head og relevante sjekker før merge og bekreft faktisk resultat. Gitt autorisasjon til vanlig PR/main-merge gir ikke automatisk autorisasjon til en ny offentlig spillrelease.
- **Lokal installasjon:** Bevar personlige Saved, testet payload og kontrollert tilbakekopi. Git-merge, lokal installasjon og offentlig nedlasting føres hver for seg.
- **Lagringsplass:** Behold gjeldende verifiserte testbygg og maksimalt to eldre komplette testbygg. Installerte standardfiler og aktive nødvendige kandidater vernes; kanonisk prune bevarer kilder, assets, Saved, logger og bevis.

Uavklarte eldre notater skal merkes og henvises til gjeldende beslutning. De slettes ikke og blir ikke automatisk gjort til nye oppgaver bare fordi et gammelt avkrysningsfelt står åpent.

## Fysisk betjening og oppgavevariasjon i Freeform, 1. november

Tom ber om at PC-handlinger utføres på selve maskinene i spillverdenen, med arbeidsdeling mellom status, mottaker, tracking og OPS. Lydsammenligning får fysiske kontroller ved telefonens spolebåndutstyr. Dette er aktivt arbeid: klokke, antenner og hendelser fortsetter ved utstyret; pausemenyen holder tiden. Fysiske dokumentoriginaler beholdes ved skriver og arkiv.

Bensin skal kunne kjøpes fra bensinstasjonens faktiske REGULAR-pumpe til den parkerte servicebilen. Levering og betaling følger samme måler; lasting skal ikke gjenta en betaling. Eksisterende aggregatkanne og nødvei beholdes.

Tilfeldig variasjon bygger på eksisterende kontekststyrte hendelsesregissør og en avgrenset dagskortgenerator. Generatoren varierer støttede kilder og faktiske måleinnstillinger, bevarer oppgaven gjennom lagring og lager ingen ubegrensede tekstoppdrag uten tilhørende spillhandling. Vedlikehold følger faktisk utstyrstilstand. Flere hendelsesfamilier og langsiktig balanse føres videre i TODO.
