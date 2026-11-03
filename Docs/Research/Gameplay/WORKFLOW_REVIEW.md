# Gameplaygjennomgang og agent-harness

**Status:** Referanse. **Gjelder:** SARO-arbeidsflyt i Story og Freeform, forståelighet, progresjon og forslag. **Sist kontrollert:** 3. november 2026.

Denne rapporten bevarer funnene fra grunnrevisjonen før retting. Senere implementering og faktisk teststatus føres i [gjeldende status](../../Active/STATUS.md); gamle kildelinjer nedenfor er referanser til grunnrevisjonen.

## Konklusjon

Spillet har en sammenhengende arbeidsløkke, men flere grensesnitt beskriver en annen tilstand enn den som faktisk driver arbeidet. Printerrapporten er et konkret eksempel: originalen er allerede hentet, mens mottakeren fremdeles sender spilleren til printeren. Før flere systemer legges til bør spilleren kunne stole på instrumentene, vite hva som er ferdig, og se hva neste handling oppnår.

Denne gjennomgangen består av tre parallelle agenter og en koordinerende gjennomgang: workflow_audit følger rutine og anomali i begge moduser; progression_audit følger service, økonomi og hendelser; audit_harness bygger et gjenbrukbart analyseverktøy; koordinator kontrollerer funn mot kode og lagring, undersøker VotV og prioriterer forslagene. Agentenes første funn er kontrollert på tvers før anbefalingene er samlet.

Kildegrunnlaget er `c0711eb07974bf695a481f8decefa354d6e36e72`, i en arbeidskopi med pågående Jeep-lookdev. Gameplayfilene som omtales er lest, ikke endret. Personlig lagring er kun lest lokalt; ingen full lagring, bildeinnhold eller personlige spillhistorikker inngår i dette dokumentet. Dette er statisk analyse og en kjøring av analyseverktøyet, ikke ny Unreal-kjøring, menneskelig gjennomspilling eller en bekreftelse på at alle scenene er forståelige. [Harness og kjøring](../../../unreal/Signal47/Tools/gameplay_audit/README.md). [Maskinlesbare, koordinatornormaliserte agentfunn](findings.json) følger samme ID-er som rapporten. Aktiv oppfølging ligger bare i [TODO](../../Active/TODO.md).

## Arbeidsflyten spilleren skal forstå

| Reise | Handling og synlig resultat | Sluttpunkt |
| --- | --- | --- |
| Rutineobservasjon | OPS velger jobb; tracking retter antennene; receiver viser riktig innstilling og tar opp; printer gir original; OPS registrerer resultatet. | Jobben er fullført, $20 er kreditert, originalen finnes i notatboken. Ingen ekstra printertur. |
| Anomali | Samme opptak/original, deretter relevant uavhengig kontroll. Pulsavvik sammenlignes ved spolebåndet; retning, klokker og duplikat bruker sine egne målinger. | OPS registrerer en støttet konklusjon; merket pakke hentes og legges i den fysiske arkivpermen. |
| S-03-service | Velg lokal reservedel eller forsyningstur; diagnostiser; isoler; bytt relé; lukk; kjør faktisk bevegelsestest; signer i OPS. | S-03 er tilbake i fjernstyring, og serviceordren har en tydelig sluttkvittering. |
| Strømfeil | Mål feilen; velg sikring/reparasjon eller midlertidig aggregat; observer at instrumentene får strøm igjen. | Arbeidet kan fortsette uten at sikrede originaler går tapt. En nødvendig nødvei må fungere også med tom konto. |

På hver arbeidsstasjon bør tre spørsmål kunne besvares: **Hva arbeider jeg med? Hva er faktisk ferdig? Hva gjør jeg nå, og hvor?** Mystisk årsak kan være uavklart; betjening og ferdigstatus bør være entydige.

## Prioriterte funn

Prioritet gjelder spillerkonsekvens. «Kodebevist» betyr at grenen og dens vilkår finnes i den leste kilden; det betyr ikke ny runtimeprøve. Den mulige økonomiske blindveien er særskilt merket risiko.

| ID | Prioritet og grunnlag | Utløser og konsekvens | Konkret endring og aksept |
| --- | --- | --- | --- |
| GP-01 | P1, brukerbilde + lokal lagring + kode | En ordinær original er hentet. `ReceiverNextAction` gir fortsatt printerbeskjed; receiver står «NOT READY», selv om opptaket er ferdig. Spilleren går i sirkel mellom apparater. | Vis «ORIGINAL COLLECTED / Register routine result at OPS». La receiver, printer, HUD og arbeidskort bruke samme jobbsteg. Prøv alle åtte stadier, gjentatt printerbruk og Continue. Ingen ny utskrift eller betaling ved gjentakelse. |
| GP-02 | P2, kodebevist | En valgfri serviceordre har fokus. Valg av en observasjon i OPS endrer ikke servicefokuset; HUD fortsetter å vise reservedeler mens observasjonen er aktiv. | La eksplisitt jobbvalg prioritere den valgte jobben, eller vis et tydelig valg mellom «Observing» og «Service» på OPS-hovedsiden. Bevar den andre ordren. Prøv bytte begge veier og Continue. |
| GP-03 | P2, kodebevist | Etter opptaksstart kan aktive skyvere flytte frekvens/gain/båndbredde utenfor kravene. Integrasjon sjekker antennevilkår, men ikke fortsatt korrekt tuning. Både Story og Freeform kan dermed belønne en måling som bryter skjermens regel. | Anbefaling: pause innsamling av gyldige sekunder ved tapt mottak, vis årsaken og fortsett ved gjenopprettet signal. Bevar tidligere gyldig arbeid. Alternativt må innstillingene låses tydelig under opptaket. Prøv hver skyver separat. |
| GP-04 | P1, kodebevist | Freeform spiller bare regulære pulser under selve pulsaropptaket, men senere «original» legger til faste ekstraticks. Samme avspilling er også tilgjengelig for andre anomalifamilier. Spilleren får ikke et konsistent lydbevis. | Bruk ett lagret målespor per jobb til live lyd, originalavspilling og graf/papir. Tilby relevant kontroll for fenomenet. Sammenlign en normaljobb, pulsavvik, tidsavvik, retningsavvik og duplikat etter Continue. |
| GP-05 | P2, kodebevist | «Reference comparison / $80» trekker penger og setter et flagg uten noen gameplayforbruker i den leste koden. UI lover hjelp til ekstra ticks. | Skjul/deaktiver kjøpet inntil det gir synlig nytte, eller implementer en konkret, valgfri sammenligningshjelp. Grunnkontrollen må fortsatt være mulig uten kjøpet. Avklar kompensasjon for allerede betalte lagringer. Prøv før/etter på samme spor. |
| GP-06 | P1, kodeavledet softlock-risiko | Tom konto, eksisterende nødgjeld, tomt sikrings-/aggregatlager, strømfeil ingen ferdig ubetalt pakke, ingen allerede betalt sikring/drivstoff i pakke eller last, tom aggregattank og ennå ikke montert sikring. Nødkjøp avvises på grunn av gjeld, mens ny observasjonsinntekt krever strøm. | Prøv den sammensatte situasjonen isolert før den kalles gjenskapt. Gi en siste nødvei som kan gjenopprette basisdrift, for eksempel en lånt sikring knyttet direkte til aktiv feil. Ingen salgbare gratisvarer eller ubegrenset inntekt. |
| GP-07 | P2, kodebevist | Første kvalifiserte nattsjekk skjer utenfor SARO. Døgnets threat/major-roll markeres brukt før område-/ressursvilkårene kontrolleres. En hjemvendt spiller kan miste nattens threat-/major-mulighet uten å vite det. Minor-/investigation-hendelser kan fortsatt velges. | Skill «ikke valgt» fra «valgt, men venter på gyldig sted». Lagre utfallet; utsett gyldighetskontrollen eller planlegg én ventende hendelse. Unngå nye tilfeldige kast ved dør-/områdespam og ingen garantert skrekk ved hver hjemkomst. |

### Flere konkrete friksjonspunkter

| ID | Grunnlag | Funn og forslag | Aksept |
| --- | --- | --- | --- |
| GP-08 | P2, kodebevist | Printeren sjekker bare valgt jobb. En original for A kan vente mens B er valgt, men meldingen sier at ingen utskrift finnes. Vis ventende jobb-ID/kø eller fortell spilleren at A må velges. Jobbskifte som avvises under opptak må ha en synlig grunn. | Fullfør A til printer, velg B, hent A uten leting eller tap. Prøv også jobbskifte under opptak/kontroll. |
| GP-09 | P2, kodebevist | Går spilleren ut av lytteområdet, viser båndet STOP mens markøren fortsetter; lyttedekning stanser. Anbefalt første løsning er konsistent automatisk pause, med tydelig beskjed ved retur. | Forlat og kom tilbake midt i original og kontroll; visning, markør og faktisk review er enige. |
| GP-10 | P1, kodebevist | MARK IN/OUT virker også etter registrering/arkivering og setter reviewed=false. Registrert pakke kan da ha andre markører enn jobbens nåværende sammenligning. Lås registrert sammenligning, eller opprett en eksplisitt revisjon med eget spor. | Prøv markering ved Recorded, Registered, Packet og Filed, også etter Continue. Innlevert bevis endres ikke stille. |
| GP-11 | P2, kodebevist | Bånd/rack gir generell veiledning som ikke følger fenomen eller allerede utført kontroll. Rutiner og retningsavvik kan bli sendt til feil arbeidsstasjon. Bruk samme fenomen-/stegbaserte veiledning som GP-01. | Rutine og alle anomalifamilier før, under og etter kontroll gir riktig neste apparat og avvisningsgrunn. |
| GP-12 | P2, kodebevist port / UX-risiko | Instrumentelle avvik krever en registrert normalreferanse fra Source 0, mens kortene lover fri rekkefølge. Forklar verdien av første normalreferanse og tilby veiledning, uten å skjule valgfriheten. | En førstegangsspiller forstår hvorfor referansen bør gjøres, selv etter først å ha valgt HI/Virgo. |
| GP-13 | P2, kodebevist struktur / UX-risiko | Tre nye jobber per døgn legges til; gamle uferdige står i samme lille liste og fokus beholdes etter Rest. Grupper pågående, dagens og tidligere tilbud. | Etter ti døgn kan dagens jobb og en gammel uferdig jobb finnes raskt; ingen original slettes eller får ny utløpsstraff. |
| GP-14 | P2, designforslag på eksisterende visning | Kryssdagsarkivet viser tellere og tekniske funn-ID-er, men gir svak retning. Vis kjent observasjon, usikkerhet og én mulig kontroll per spor. | Etter ett/to funn kan spilleren forklare hva som er kjent og hva som er et rimelig neste undersøkelsesgrep. |

GP-08: Freeform.cpp 361–365, 431–438, 724 og 865. GP-09: Freeform.cpp 518–526 og FreeformAudioDesk.cpp 162–168; Story stanser markøren ved avstand i Observing.cpp 1009. GP-10: Freeform.cpp 494–501 og 830–835. GP-11: FreeformAudioDesk.cpp 163–164 og Freeform.cpp 863/883/904–906. GP-12: FreeformDirector.cpp 265–266 og FreeformJobs.cpp 66. GP-13: Freeform.cpp 185–188/660–667/720–726. GP-14: Freeform.cpp 614–648 og 825–836. Alle filene ligger i [Private-kildene](../../../unreal/Signal47/Source/Signal47/Private).

### Kildepekerne bak funnene

- GP-01: [ReceiverNextAction og CollectChart](../../../unreal/Signal47/Source/Signal47/Private/S47Freeform.cpp), linje 381–397 og 431–439; registrering linje 535–543; [felles mottakervisning](../../../unreal/Signal47/Source/Signal47/Private/S47ReceiverReadout.cpp), linje 85–94. Den undersøkte originale rutinejobben hadde `Recorded`, `printed=true`, `paid=false` og et originaldokument. Dette viser at registrering, ikke ny utskrift, var neste steg.
- GP-02: samme [Freeform-kilde](../../../unreal/Signal47/Source/Signal47/Private/S47Freeform.cpp), `SelectJob` linje 361–366 og `Objective` linje 892; [serviceguiden](../../../unreal/Signal47/Source/Signal47/Private/S47FreeformGuide.cpp), linje 89. Fokusbyttet finnes, men ligger i serviceundermenyen.
- GP-03: [Freeform StepWork](../../../unreal/Signal47/Source/Signal47/Private/S47Freeform.cpp), linje 294–315 og aktive skyvere 808–809; [Story-opptak](../../../unreal/Signal47/Source/Signal47/Private/S47Observing.cpp), linje 369–387. Strøm- og antennevakter finnes; funnet gjelder tuning under opptak.
- GP-04: [Freeform](../../../unreal/Signal47/Source/Signal47/Private/S47Freeform.cpp), live pulser 307–309, `AudioAction` 491–513 og `StepAudio` 515–526. [Story](../../../unreal/Signal47/Source/Signal47/Private/S47Observing.cpp) har eksplisitte ekstraticks under opptak på linje 377–383; de to modusene har ulik oppførsel.
- GP-05: [Upgrade og salgstekst](../../../unreal/Signal47/Source/Signal47/Private/S47Freeform.cpp), linje 652–658 og 753–756; eneste gameplaybruk av `HasUpgrade` er indeks 0 i [FreeformWorld](../../../unreal/Signal47/Source/Signal47/Private/S47FreeformWorld.cpp), linje 725.
- GP-06: [Spend/Earn](../../../unreal/Signal47/Source/Signal47/Private/S47Freeform.cpp), linje 568–583; [Purchase, GeneratorUse og FeederUse](../../../unreal/Signal47/Source/Signal47/Private/S47FreeformWorld.cpp), linje 471–477 og 617–632. En allerede registrert pakke kan gi en annen vei via arkivering; risikoens scenario utelukker derfor denne ressursen eksplisitt.
- GP-07: [Eligible og nattkast](../../../unreal/Signal47/Source/Signal47/Private/S47FreeformDirector.cpp), linje 258–280 og 449–452. Start avvises utenfor SARO, mens dagens kast allerede er markert.

### Tillegg 3. november: forskningsløkken i Freeform

Tom ba om en vurdering av hvordan Freeform-løkken kan forbedres mot reisen «jeg observerte noe i natt, velger et forsøk i morgen, setter ut utstyr og sammenligner neste natt». Grunnlaget er TODO (FF-RESEARCH-00–10, GAMEPLAY-01–06, PHOTO-04), denne rapporten og lest kilde på `3194bd16`. Dette er statisk analyse og designforslag; ingen runtime- eller menneskelig spilltest er kjørt. Hovedvurderingen: arbeidsløkken er solid som jobb, men mysteriet skjer *med* spilleren. Spilleren formulerer ikke et eget spørsmål og har ingen grunn til å velge avviket fremfor rutinen.

| ID | Prioritet og grunnlag | Funn og konsekvens | Forslag og aksept |
| --- | --- | --- | --- |
| GP-15 | P1, kodebevist | Rutine og registrert avvik betaler begge $20 (`S47Freeform.cpp` 572 og 593). Avviket krever ekstra kontroll, sammenligning og arkivering for samme betaling; en rasjonell spiller kan velge bort mysteriet. | Skill belønningene: rutinen gir fortsatt penger; et undersøkt avvik, også et godt dokumentert negativt resultat, gir kunnskap og avgrenset adgang (lånt modul, nytt feltsted, nytt oppsett). Kunnskap kan ikke kjøpes med flere rutinejobber. Aksept: spilleren opplever et reelt valg mellom deler/penger og undersøkelse. |
| GP-16 | P1, kodebevist struktur / designforslag | Funn med dagnummer og `MysterySteps` finnes (`S47Freeform.cpp` 646–647 og 682–683), men neste kontroll kommer som tildelt jobb, ikke som spillerens valg. | Etter registrert avvik: OPS-valget «Planlegg oppfølging» med to–tre forsøk: gjenta samme kilde og tidsvindu neste natt, lokal støykontroll (FF-RESEARCH-03) eller retningsmåling med annen antenne. Valget lagres som neste natts jobb med henvisning til originalkilden og sammenlignes på bånd/OPS. Gjenbruk jobbløpet, funnene og arkivet; ingen ny maskinvare. Dette er kjernen i FF-RESEARCH-02 og bør komme før FF-RESEARCH-00. Aksept: to netter, tre utfall (gjentatt avvik, påvist lokal støy, ingen funn); spilleren kan begrunne valget. |
| GP-17 | P2, designforslag | Dag (30 min) og natt (45 min) fylles med samme type arbeid; dagen mangler en egen rolle. | Gi dagen en planleggingsfase: OPS viser kommende natts plan (rutine, forsøk, nødvendige deler/film/drivstoff). Caliche-turen blir en tur for planen, ikke bare vedlikehold. Aksept: spilleren kan si hvorfor dagens innkjøp trengs i natt. |
| GP-18 | P2, designforslag | Hendelser (UFO, skikkelser, foto) gir observasjon og eventuelt $20-foto, men sjelden et nytt spørsmål eller valg. | La høye UFO-baner (FF-RESEARCH-01), PHOTO-04, etterspor (FF-RESEARCH-07) og sheriff-/radiotips legge dato, grov retning og tidsvindu i saksarket, slik at de begrunner et oppfølgingsvalg i GP-16. Et tips foreslår en kilde, ikke en ferdig sannhet. |
| GP-19 | P2, rekkefølgeforslag | Feltloggeren (FF-RESEARCH-04) er planlagt som grunnlag for den første forskningsreisen, men gir mest verdi som andre målested når oppfølgingsvalget allerede finnes. | Innfør loggeren som oppgradering/adgang fra GP-15: to samtidige målesteder skiller lokal støy fra ytre kilde. Materialene (FF-RESEARCH-05) får dermed konkret nytte i neste forsøk. Utsett fri bygging (FF-RESEARCH-06) og nærmøter (FF-RESEARCH-08) til GP-15/16 er prøvd. |
| GP-20 | P1, forutsetning | Sammenligning over flere netter er bare troverdig hvis lagret måling er konsistent. GAMEPLAY-01/02/04 er levert i PR352, men ikke samlet spillerprøvd. | Prøv særlig GP-04 (ett lagret målespor for live lyd, original og graf) og GP-07 før forskningsløkken bygges på dem. Første avgrensede leveranse: GP-20-prøve, GP-16 og en minimal GP-15 med én adgangsbelønning. |

## Bevar fungerende kontrakter

Faktiske antenneencodere, separat original/duplikat, engangsbetaling og fysisk arkivering er allerede implementert. Spoling alene gir ikke lyttedekning, og en ubegrunnet sky-kildekonklusjon avvises. Nødgjeld betales før kontanter, øvrige antenner kan brukes mens S-03 er ute, og veihjelp har en faktisk returvei. De nye forslagene bygger videre på dette.

Eksisterende `S47FreeformEvidenceTest.cpp` linje 300–423 dekker fysisk rutine-/pulsarreise, feil konklusjon, originalvern, innlevering, betaling og disk/title/Continue. Den riktige rekkefølgen er godt representert; de nye funnene gjelder særlig alternative spillerhandlinger, misvisende avslag og uenighet mellom visning og tilstand. En ny fullmatrise er derfor ikke første tiltak.

## Forbedringer som gir mer forståelig og interessant gameplay

### Først: én sann ferdigstatus

La arbeidskortet vise fem enkle ledd: **valgt → målt → original hentet → kontrollert ved behov → registrert/arkivert**. Vis samme jobb-ID ved alle instrumenter, men bruk handlingstekst foran tekniske ID-er. Det er allerede mange av disse dataene i koden; forbedringen er konsekvent presentasjon og riktige overganger.

Etter ferdig rutinejobb: «Routine result registered / $20 / Original retained in notebook». Etter anomali: «Comparison registered / Collect labelled packet / File in records». Printeren bør svare spesifikt «Original already collected» eller «No selected observation», fremfor en felles feilmelding som nevner flere alternative prosesser.

### Lær den første løkken gjennom vanlig arbeid

Tilby én kort, frivillig veiledet normaljobb på den faktiske arbeidsplassen. La spilleren velge jobb, peke antenner, stille mottaker, hente papir og få betaling. Hjelpeteksten peker til fysisk apparat og forklarer hvorfor neste ledd trengs. Vis tidligere og neste steg uten en lang modal tutorial eller blokkert rom. Oppgaven kan avbrytes, gjenopptas og hoppes over av erfarne spillere.

Første normale referanse er også grunnlaget for senere instrumentavvik i dagens Director. Denne rekkefølgen bør forklares som faglig arbeid, slik at spilleren skjønner verdien av den første jobben. Et mulig prøvemål er fullført førstegangsjobb på 5–8 minutter uten ekstern forklaring; dette er et designmål, ikke målt spilletid.

### Gi valg som endrer undersøkelsen

Et nytt dagskort med andre tall gir variasjon i betjening, men lite nytt resonnement alene. Bygg videre på eksisterende retnings-, timing- og duplikatkontroller: la en lesbar observasjon begrunne hvilken kontroll spilleren velger. En rimelig feil kontroll skal gi et forståelig negativt resultat og bevare originalen. Den skal ikke automatisk godkjenne konklusjonen eller kreve omstart.

La arkivet vise hva som faktisk er lært: «første observasjon», «uavhengig kontroll» og «gjentatt en annen dag», samt ett konkret åpent spørsmål. Dagens kryssdagsbevis finnes allerede; forslaget er å gjøre fremgangen lesbar, ikke lage et parallelt mysteriesystem. Rutinearbeid kan fortsatt betale likt, mens bedre dokumentasjon åpner en avgrenset oppfølging.

### Gjør vedlikehold til et valg med merkbar gevinst

Vis praktisk forskjell mellom å jobbe videre med redusert kapasitet, bruke reservelager og kjøre til Caliche. Etter reparasjon bør statuslampe, bevegelse og måling vise den samme forbedringen. Samle innkjøp og tilby et klart forsyningsmål før avreise. Bevar eksisterende last-/benk-/relétest og unngå å kreve ny tur når riktig del allerede finnes.

Vis dagsaktuelle oppgaver først, med separat «Unfinished previous work». Bevar eldre resultater og muligheten til å fullføre dem; ikke la en voksende liste se ut som stadig flere misligholdte frister når kortene uttrykkelig ikke utløper. Koble dette til MODE-05, uten å innføre ny tvungen økonomi eller sult/tørste.

### La uro bruke regler spilleren kjenner

Etter at normaljobben er forstått, kan samme printer, lys eller lyd gi et kontrollerbart avvik. Feil i veiledning og målelogikk må først rettes, ellers blir det umulig å skille tilsiktet mysterium fra programfeil. Bevar rolige perioder, mulighet til å sikre arbeid og lokal recovery. Hendelser bør gi et spørsmål eller et valg; flere tilfeldige innslag alene gjør ikke arbeidsløkken bedre.

## Voices of the Void: hva mer kan brukes?

Den tidligere [lokale analysen av 0.9.0n](../VoicesOfTheVoid/GAMEPLAY_LESSONS.md) er fortsatt relevant for normalreferanse, avgrenset vedlikehold, fysiske medier og vedvarende arbeidssted. Ingen ny dekompilering eller gjenbruk av spillkode/assets er gjort i denne runden. Prosjektbiblioteket er søkt på gameplay/workflow og Unreal; treffene var generelle motorskill-kataloger, ikke en ferdig kompatibel løsning på disse tilstandsproblemene. Eksisterende prosjektkode og tidligere VotV-undersøkelse er gjenbrukt som analysegrunnlag.

Ny kontroll av utviklerens sider 2. november gir disse tilleggene:

| Kildegrunnlag | Lærdom for SIGNAL / 47 |
| --- | --- |
| Utvikleren beskriver innsamling, analyse, bearbeiding og salg av signaler som grunnløkken. [Offisiell spillside](https://mrdrnose.itch.io/votv). | Hvert ledd må ha et synlig produkt og en begripelig grunn. Vis signal, sikret original, sammenligningsresultat og registrert inntekt som ulike stadier. |
| 0.9-loggen beskriver oppgraderinger som fysiske moduler. [0.9.0](https://mrdrnose.itch.io/votv/devlog/1474386/voices-of-the-void-alpha-090). | Gjør vår eventuelle $80-modul konkret: en synlig referanse-/differansevisning som hjelper sammenligningen, med sammenlignbart før/etter-resultat. Fysisk installasjon har verdi bare hvis den gir nytte. |
| 0.9.0n retter tutorialtekst, tutorial-softlock, drivehåndtering og en stol som sperret signalstasjonen; loggen oppgir også endringer i slitasjefrekvens. [0.9.0n](https://mrdrnose.itch.io/votv/devlog/1527871/voices-of-the-void-alpha-090-n). | Test praktisk rekkevidde, synlighet og forståelse sammen med logikk. Vår kontorstol-feil og printerveiledning er relevante egne problemer. Vedlikehold må balanseres som del av hele døgnet. |
| Den tidlige 0.10-forhåndsvisningen fra 29. oktober endrer inventar og lagringsformat. Utvikleren betegner hovedspillet som lite testet/ustabilt og sier at 0.9-lagringer ikke virker direkte i 0.10. [0.10-preview](https://mrdrnose.itch.io/votv/devlog/1738259/voices-of-the-void-alpha-0100-halloween-2026-early-010-preview). | Bevar spillerens arbeid når arbeidsflyten forbedres. Bruk små migreringer og stabile bevis-ID-er fremfor å kreve ny sak for en UI-/progresjonsretting. Dette er vår designkonklusjon, ikke en test av VotV 0.10. |

En [spillertråd fra oktober](https://itch.io/t/7146086/help-with-the-tutorial) beskriver både glede over fysisk betjening og forvirring om signalterminalene. Dette er ett kvalitativt eksempel, ikke representativ brukerstatistikk. Vår konklusjon er at taktilt arbeid kan være interessant uten at spilleren må gjette hva «ferdig» betyr. Kopier prinsippet om en troverdig arbeidsplass; vurder hvert ekstra håndgrep etter om det gir et valg, en forståelig virkning eller en meningsfull observasjon.

## Anbefalt rekkefølge og spillerprøve

1. **Troverdig arbeidsflyt:** GP-01/02/08/11 for riktig veiledning; GP-03/05 for ærlige instrumenter og kjøp; verifisering av GP-06. Fullfør ett rutineforløp og ett avbrutt forløp. GP-04 og GP-10 må være rettet før lydavvik brukes til å lære spilleren bevisføring. GP-09 gjør avspillingens regler konsekvente.
2. **Forståelig første jobb:** frivillig veiledet normaljobb, tydelig aktiv oppgave, konsekvent ferdigkvittering og oversikt over tidligere arbeid.
3. **Interessant undersøkelse:** fenomenriktig kontroll, lesbar kryssdagsfremgang og merkbar oppgraderingsnytte. Juster GP-07 og tempo etter vanlig spilling.

Den første menneskelige prøven bør omfatte en rutinejobb, ett avbrudd/Continue og ett avvik uten ekstern forklaring. Deretter prøves en hel forsyningstur og et normalt døgn. Noter tid til første fullførte jobb, antall feil apparatbesøk, antall ganger spilleren trenger hjelp, og om spilleren selv kan forklare forskjellen på måling, original, kontroll og registrering. Kriteriet er at spilleren finner neste handling og forstår resultatet; en automatisk test som klikker kjente callbacks er ikke bevis på dette.

Eksisterende kollisjons-/krasjfeil beholder prioritet i TODO. Forslagene her er ikke merket implementert, og gjennomgangen starter ikke tidligere stoppet TODO-automasjon.

## Kjøringen av harnesset

Koordinator kjørte verktøyet mot den samme kilde-revisjonen og den eksisterende lagringen etter agentsammenstillingen. Kildekontrakten ble gjenkjent; 14 strukturerte funn fra workflow- og progresjonsagenten ble validert og samlet mot 11 scenarioer. Den konkrete lagringen bekrefter original hentet, ubetalt rutinejobb, aktivt servicefokus og motstridende printerhint. Lagringsfilen var identisk før/etter lesingen; den er ikke endret av harnesset.

Seks lette parser-/skjemakontroller består, inkludert eldre lagring uten guideFocus og avvisning av ukjent kildekontrakt. Dokumentkontrollen rapporterte null nye lenkefeil, og diffkontrollen består. Dette verifiserer analyseverktøy og dokumentasjon, ikke de elleve spillerreisene i motoren. Runtime og menneskelig spilltest er NOT_RUN. Det maskinlesbare resultatet med minimalt lagringssammendrag er beholdt lokalt utenfor offentlig Git.
