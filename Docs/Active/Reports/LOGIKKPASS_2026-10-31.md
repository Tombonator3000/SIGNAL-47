# Logikkpass gjennom SC01 til SC13

**Status:** Referanse. **Gjelder:** Kildegjennomgang av Unreal-spillet og oppfølging i STORY-05. **Sist kontrollert:** 31. oktober 2026.

## Konklusjon og avgrensning

Historien har en forståelig hovedkjede: rutine, strømfeil, fysisk møte, savnetsak, sikring av originaler, kontrollerte observasjoner, gamle opptegnelser, fjernkilder og transport av bevis. Returen til SARO og siste tur har dermed en begrunnelse. To kildefeil kan likevel stoppe normal progresjon, og flere mål og bevistekster følger eldre regi.

Gjennomgått spillkode tilsvarer main etter PR263, med dokumentgrunnlag fra PR264 (`758bd31e`). Installasjonen er fortsatt `a202d88a`. Dette er en statisk gjennomgang av produksjonskode, tilstandsoverganger, lagringskroker, historie og spilltekst, fordelt mellom tre avgrensede lesende hjelpere og hovedagenten. Ingen Unreal-, Blender-, bygg-, spill- eller ytelsesprøve er kjørt mens Tom spiller. Ingen personlige lagringer eller installerte spillfiler er lest eller endret. Kildebekreftet betyr at kontrollflyten viser problemet; det betyr ikke at det er gjenskapt med ekte input.

Arbeidslisten står i [TODO](../TODO.md). Alle funn nedenfor er åpne, ingen rettelse eller ny spillpakke inngår i dette passet. P1 betyr mulig progresjonsstopp; P2 betyr funksjons-/veiledningsfeil. Funnene gjelder nye evening-/Spine-saker der det er angitt. Gamle saker skal beholde sin avtalte oppførsel.

## Prioriterte kodefunn

### LOGIC-01 / P1: Continue under SC11-telexen kan låse reisen

Kilde: [S47SC11.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47SC11.cpp), linje 91–101, 140–158, 190–194 og 618–620. [S47Flow.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47Flow.cpp), `GameSaveBlock`.

Et lagret `Telex` blir ved `Begin` gjort om til `Collect`, samtidig som begge samleflaggene settes til true. Samlehandlingene skjules nettopp når flaggene er true; telexhandlingen krever `Telex`. Den eneste overgangen tilbake til `Telex` skjer etter en gyldig samlehandling. Dermed finnes ingen tilgjengelig handling etter innlasting. `SetStep(Telex)` bestiller autosave og gjeldende lagringsvakt sperrer ikke dette vinduet. Eksisterende selvtest kontrollerer den strandede tilstanden uten å prøve videre progresjon.

**Utløser:** Ta begge originalene, la autosave skrives mens telexen går, avslutt og velg Continue. **Følge:** Ingen telex, beleiring eller SC12. **Rettelse:** Bevar/restart `Telex` med nullstilte transienter, eller la `Collect` gjenkjenne at begge originalene allerede er sikret. Bevar originalene og øvrige dokumenter.

**Målrettet senere kontroll:** Lagre i telexfasen, last gjennom vanlig Flow/Continue, vent til avrivning faktisk tilbys og fullfør videre til beleiring og utgang. Kontroller mer enn flaggenes likhet.

### LOGIC-02 / P1: Usett avreise fra SC02 har ingen fortsettelse

Kilde: [S47Horror.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47Horror.cpp), linje 608–619 og 701–707; [S47HorrorThreat.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47HorrorThreat.cpp), rundt linje 529; [S47Spine.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47Spine.cpp), rundt linje 293.

Spilleren kan kjøre klar uten at kroppen har blitt visuelt registrert. SC02 går da til `Unwitnessed`. Bare `Escaped`, med presentert flukt, åpner SC03. Ingen produksjonsgren gjenopptar `Unwitnessed`, og SC04 venter på fullført SC03. Tilstanden lagres.

**Utløser:** Unngå minimumsblikket og kjør bort, eller last i førerhuset før kroppen har vist seg og kjør ut. **Følge:** Ortiz og resten av den nye hovedreisen starter ikke. **Rettelse:** Lag en eksplisitt sannferdig fortsettelse eller mulighet til å gjenoppta møtet. Ikke gi spilleren et falskt vitneflagg.

**Målrettet senere kontroll:** Både sett og usett avreise, inkludert Continue i bilen, må ha en nåbar vei videre uten oppdiktet observasjon.

### LOGIC-03 / P2: Egen bil er ikke tilgjengelig etter introen

Kilde: [S47Evening.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47Evening.cpp), linje 283; [S47EveningVisual.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47EveningVisual.cpp), linje 285; [S47World.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47World.cpp), linje 175–203 og 1946–1983.

Godkjent [tidslinje](../../Design/Story/TIDSLINJE_SC01_SC13_2026-10-29.md) tillater egen bil fra SC03. Eneste ordinære `BeginCommute`-inngang er åpningen. Etterpå er bilen et parkert synlig objekt med kollisjonsboks, uten registrert innstigningshandling. Servicebilen er fortsatt standardreisen; dette er et manglende godkjent valg, ikke et påvist stopp i servicebilruten.

**Rettelse:** Koble egen bil til ordinær transport fra riktig fase, med parkering, retur og lagring. Finalens servicebilregel må avklares i samme flyt, slik at et fritt valg ikke strander spilleren senere. **Senere kontroll:** Begge biler på sammenhengende SC03–SC12-rute, bytte og Continue, uten at målene krever en utilgjengelig bil.

### LOGIC-04 / P2: Feil mål mens Ortiz løftes

Kilde: [S47HorrorSC03.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47HorrorSC03.cpp), linje 596–611, 640 og 683.

Etter første kontakt viser `Lift` fortsatt `TELL ORTIZ WHAT YOU SAW`. Samhandling med Ortiz er bare gyldig i `Meet`. Videre progresjon krever at spilleren ser den svevende Ortiz lenge nok. En spiller som følger målet forsøker en utilgjengelig samtale; hvis spilleren ser bort, venter scenen ubegrenset.

**Rettelse:** Bytt til et situasjonsriktig mål når løftet starter, og bruk stedlig lys/lyd samt et forsinket hint ved behov. Bevar fri kameraorientering. **Senere kontroll:** En spiller som følger HUD uten fasit finner riktig handling; å se bort må ikke gi et selvmotsigende mål.

### LOGIC-05 / P2: Dineren sender spilleren til bilen før obligatorisk telefon

Kilde: [S47Chapter5.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47Chapter5.cpp), linje 336, 351 og 369–377.

P13 løst gir `road` og mål om C og bilen utenfor. Avreise nekter likevel inntil Ward er oppringt. Notatboken nevner samtalen, men HUD gir ikke den neste nødvendige handlingen. Dette er samme type frustrasjon som Toms tidligere bilrapport.

**Rettelse:** I Spine-saker med P13 ferdig og Ward uavklart skal HUD peke til betalingstelefonen og de tre kildene. Etter samtalen byttes det til avreise. **Senere kontroll:** Løs P13 før Ward, følg HUD, ring og kjør uten misvisende mellomsteg.

### LOGIC-06 / P2: Gamle C-instruksjoner motsier kurerturen

Kilde: [S47Chapter5.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47Chapter5.cpp), linje 336–337; [S47Chapter6.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47Chapter6.cpp), linje 242–249 og 269–273.

Ny SC13 sier kureren ved mile 10.4 og passér C uten stopp. Eldre mål ber fortsatt spilleren følge C til bolten ved mile 8, og ved snuing sier spillet `Ward can wait. The bolt is ahead.` Stillstand nær C gir også gammelt hint om linjen foran. Kartlegging av C er nødvendig for å unngå stedet; den skal ikke presenteres som dagens reisemål.

**Rettelse:** Samordne HUD, oppgaver, avreise og kjørehint til kureren og passering av C for nye saker. Behold legacy-tekst i legacy-grenen. **Senere kontroll:** Les alle mål og snu-/stillstandshint fra dineren til motorstoppet, ikke bare kapittelstartens hovedmål.

### LOGIC-07 / P2: Beviset i SC04 har faste, mulige fremtidige tider

Kilde: [S47SC04.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47SC04.cpp), linje 158–163, 192 og 277–281.

Stripen sier alltid strøm tilbake 01:52 og avrivning 02:58. SC04 starter ved 02:55 og lar spilleren ta stripen straks. Avrivningen kan dermed beskrive fremtiden; ved langsommere spilling beskriver den et tidspunkt som allerede er passert. Faktisk T01-fullføringsstempel finnes. Ortiz-stempelet brukes allerede dynamisk.

**Rettelse:** Bruk lagret faktisk T01-tid og aktuell avrivningstid i tittel, side og transkripsjon. Bevar historiske dokumenter uendret ved lasting. **Senere kontroll:** Tidlig og sen innsamling og forskjellig T01-varighet må gi sammenhengende kronologi.

### LOGIC-08 / P2: SC08 tilskriver spilleren en uprøvd spoleobservasjon

Kilde: [S47SC08.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47SC08.cpp), linje 114–118 og 137–149.

Bruk av lobbydøren setter spolen som ferdig og skriver at spilleren har hørt samme svar uansett eget spørsmål. Den faktiske handlingen spiller bare en klikklyd og åpner Earls dokument. Notatet forteller hva Earl gjorde; journalen gjør det til spillerens egen gjennomførte kontroll. Dette er et bevis-/presentasjonsavvik, ikke en progresjonslås.

**Rettelse:** La spilleren faktisk spille et kort fragment og prøve gjentakelsen, eller skriv ærlig at dette er Earls andrehåndsrapport. Behold muligheten til å lese transkripsjon. Ikke krev en ny lang lydproduksjon bare for å rette feil attribusjon.

### LOGIC-09 / P2: Tilfeldig bortføring bruker gammel hendelsestidslinje

Kilde: [S47Away.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47Away.cpp), linje 82–89, 509–510 og 536; [S47Abduction.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47Abduction.cpp), linje 30 og 275; [S47Timeline.h](../../../unreal/Signal47/Source/Signal47/Public/S47Timeline.h), linje 34–36.

`Fixed()` bruker fortsatt gamle ch2/ch3/ch4-grenser 03:40, 04:35 og 05:00; ch1 bruker den tidlige hendelsen pluss 40 minutter. Tilfeldig bortføring krever minst 18 minutter til neste faste punkt, også i nye saker som ikke skal miste tid. Nye kapitler begynner 04:25, 04:50 og 05:12, og ch1 begynner først etter SC04 rundt 03:15. Dermed blir mange av de bevarte tilfeldige hendelsenes vinduer stengt for tidlig eller helt. Den særskilte STATION01-hendelsen har en egen nyere korreksjon, som ikke retter `Fixed()`.

**Rettelse:** Samordne neste faste punkt og manglende tidstap med faktisk nye scenevinduer og beskyttede jakter. Behold gamle saker. **Senere kontroll:** Prøv tillatt rolig vindu og sperret manussekvens på begge tidslinjer. Dette er en timing-/funksjonsfeil, ikke påvist krasj eller progresjonslås.

## Dekning gjennom hele reisen

| Del | Lest sammenheng og vurdering |
| --- | --- |
| Åpning og SC01 | Direkte ankomst/valgfri diner, Dales overlevering, dører, kalibrering, fire observasjoner, generator, arkiv, port, vindu/merker og nattens strømbrudd. Krav til referansefilm er vernet; ikke registrert som ny filmforbruksfeil. |
| SC02 | T01-resultat, strømkrav, første kropp, faktisk sikt/lyd, lokal fangst og bilflukt. LOGIC-02 må rettes. |
| SC03 | Første melding, hjelp, Ortiz, bortføring, dekning, separat savnetmelding, spor og sheriff. Spor er valgfrie. LOGIC-04 og egen bil krever oppfølging. |
| SC04 | Sikring, plansperre, permen som jaktutløser, telefonlokking/dekning og lukket kontrollrom. Resterende papirer kan hentes etter jakt. LOGIC-07 svekker bevisenes tider. |
| SC05 | Residual holdes til originalene er sikre; retning, utskrift, telefon og hendelsen etter 47 sekunder leder til fotoarbeidet. Ingen ny sikker kodefeil funnet her. |
| SC06 | To eksponeringer, aktiv/passiv kontroll, fremkalling, sammenligning og rapport. Negativ bevares ved tåket kopi; valgt kontrollmetode videreføres til Nora. |
| SC07 | Endret/original arkivpost, feltmappen, valgfri VHS, nøkkelholder og overgang til SC08. Fullført kapittel har eksplisitt overgang ved lasting. |
| SC08 | Lobbyspole, serviceark, A/B/retur, følgemekanikk, jakt og flukt. Lasting under felt/jakt starter feltprøven på nytt med dokumenter bevart. LOGIC-08 gjelder bevisopplevelsen. |
| SC09 | Referanse, kabel, feltbevis, telefon, begge fremkalte kopier og SARO-samtalen før Nora. Endepunkt-fixtures i eldre tester erstatter ikke vanlig kjøring. |
| SC10 | Nora, dokumentkrav, valgt kontrollmetode, P11/P12 og avreise mot SARO. Den nye motiveringen om å bevare originalene er tydeligere enn den eldre kontrollturen. |
| SC11 | Originaler, telex, avsøkning, lokking, flukt og Continue. LOGIC-01 er viktigste innlastingsfeil. |
| SC12 | P13, tre kilder til Ward, patruljearket og avreise. LOGIC-05/06 gjelder handlingsveiledning og motivasjon. |
| SC13 | Kurerreise, CB, ufrivillig motorstopp, EVENT 47 sekunder og uttrekk 34 sekunder. Fri leting tidligere bruker ikke automatisk opp finalen; avstand/tilstand styrer innkjøringen. Gamle kjørehint må samordnes. |

## Gameplay som bør forbedres

1. **Ett gjennomførbart neste mål.** Bruk samme verdensnavn på HUD, etikett og notatbok. Etter hvert funn bør neste sted/handling komme først; E-/P-/FRAME-koder er støtte. Særlig viktig ved retur etter pause.
2. **Blikkøyeblikk skal kunne oppdages.** Spilleren skal kunne se bort uten at historien lyver om hva som ble sett. Bruk stedlig lyd/lys og gradvis hint, ikke tvungen kameraorientering eller skjult vinkelgjetting.
3. **Førstehåndsbevis der det betyr noe.** Spolen i SC08 og patruljearket i SC12 er sterke muligheter for korte faktiske handlinger. Patruljearket legges nå automatisk i dokumentene mens teksten sier det ligger på disken. En fysisk ta-/les-handling ville gjøre kilderekken tydeligere.
4. **Bevar variasjonen.** Fagarbeid, undersøkelser, ro og fire korte farer gir bedre kontrast enn flere gjentatte sjekklister eller skremmehendelser. SC04s lokking og lengre skjulte vei er et godt grunnlag. Subjektivt tempo må vurderes fra faktisk spilling.
5. **Ventingen på Ortiz trenger synlig respons.** Ankomstlyd/lykter eller et kort statusspor kan vise at hjelpen er på vei. Nå er nærheten til vinduet den praktiske porten; målets ord om synlighet bør passe det som faktisk måles.

Dette er forslag, ikke ferdig bestilte nye mekanikker eller bevis på at den subjektive opplevelsen er dårlig.

## Uavklarte feilrisikoer og senere kontroller

- `Main::OneAtATime`, `Night::OneAtATime` og `Horror::Busy` inkluderer ikke eksplisitt Spine SC04/08/11. Flere ambientfamilier bygger på disse vaktene. Faktisk konkurranse med jakt/lyd må prøves, siden indirekte vern finnes. Ikke påstå at tilfeldig bortføring er sett midt i SC08/11: LOGIC-09 stenger slike spontane starter på de aktuelle klokkene.
- Dørendringer rutes fra Web til Flow, kapitler og Horror, og global dørstatus lagres. Terskelstenging har vern. Dette lukker ikke Toms nye spillerfunn om en bestemt dør, men gir ikke grunnlag for å erklære alle dører feil.
- Gamle saker aktiverer ikke automatisk Spine. Kjøring lagres tilsiktet ved avreisestedet, og finalen bruker et trygt punkt før siste tur. Dette er ikke i seg selv strandingsfeil.
- Vanlig fysisk gjennomspilling, mus/gamepad, lesbarhet, lytting, bevegelseskomfort og ytelse er fortsatt ikke attestert. Historiske Vulkan-feil står separat i FIX-01; statisk historiepass avklarer ikke GPU-feil.

## Oppfølging

**31. oktober, senere beskjed fra Tom:** Vent med retting til hans spilltest er ferdig. Feilene står åpne. [Forslag til gameplay, skrekk og uro per del](../../Design/Story/FORBEDRINGER_OG_URO_SC01_SC13.md) er laget separat som designforslag. Påbegynte, ubygde kildeutkast er lagt til side og inngår ikke i denne dokumentleveransen.

Rett først LOGIC-01 og LOGIC-02, deretter LOGIC-04/05/06 og transportvalget. Samordne bevis og tilfeldige tidsvinduer etterpå. En samlet målrettet kontroll av de endrede rutene kan kjøres når Tom ikke spiller. Ny installasjon krever pakking, selvtest og kort kontroll etter prosjektets vanlige regler. En ny fullmatrise per funn trengs ikke.

Prosjektbiblioteket ble søkt med oppgave/teknologi og deretter bredere. Ingen direkte løsning for gameplay-/historielogikk ble funnet. [EngineWire-profilen](https://github.com/Tombonator3000/prosjektbibliotek/blob/main/repoer/prosjekter/ChiR24--Unreal_mcp.md) ble lest som verktøyreferanse: oppgitt UE5.0–5.8, men bibliotekets runtime-status er UNVERIFIED. Ingen ny avhengighet eller kildekode ble gjenbrukt. Gjeldende prosjektkode og godkjent historie er grunnlaget for funnene.

## Oppfølging

LOGIC-01 til09 er rettet og lokalt levert gjennom [PR272](https://github.com/Tombonator3000/SIGNAL-47/pull/272). Rapporten over beskriver funnene før retting; [gjeldende TODO](../TODO.md) og [leveringsrapporten](../../Evidence/UnrealDeliveries/PLAYTEST_FIXES_2026-10-31.md) fører status og kontrollgrenser.
