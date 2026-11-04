# Greyhill kodeanalyse og konkrete forbedringer for SIGNAL 47

**Status:** Referanse. **Gjelder:** SIGNAL / 47 i Unreal, Story og Freeform. **Sist kontrollert:** 4. november 2026.

Det mest nyttige fra Greyhill er sammenhengen mellom en fysisk hendelse, et forståelig varsel, en reaksjon og et resultat spilleren kan undersøke. SIGNAL / 47 har allerede mye av jaktgrunnlaget. Størst videre nytte ligger i lokale lydspor, trygg håndtering av fysiske bevis og møter som tåler avbrudd og Continue.

**Anbefalt retning:** Behold jaktforbedringene som allerede er flettet. Prioriter deretter én sammenhengende Freeform-hendelse der spilleren hører noe ved SARO, undersøker stedet og kan sammenligne et faktisk etterspor med et kontrolloppsett. For Story bør neste avgrensede arbeid være den eksisterende SC04-dørreisen, særlig fysisk passering og spillerens reaksjonsrom.

# Grunnlag og sikkerhet i funnene

Kilden er det lokale uttrekket av **Greyhill Incident, Steam-build 12751524, UE 4.27**, fra arbeidet 28. oktober. Dette gjelder cooked Blueprint-logikk. Shipping-programmets native maskinkode er ikke dekompilert.

En ny oversiktskontroll finner **111 tekstdisassembleringer med 963 funksjonsoppføringer**. De omfatter 110 assetfiler med 648 oppføringer og hovedkartet med 315. Alle 963 oppføringene har samsvar mellom deklarert og beregnet bytekodestørrelse. De eksisterende parserkvitteringene rapporterer PASS og byteidentisk rekonstruksjon for de 110 assetfilene og kartet. Kvitteringene er lest på nytt; parseren er ikke kjørt på nytt i dette oppdraget. Antallet omfatter også hendelser, delegater og innpakningsfunksjoner.

Hele filoversikten er kartlagt. Utvalgte kodebaner er fulgt nærmere i AI, spiller, dør, skjulested, inspeksjon, lydsone, kornsone, taklyd, skriptet skremmehendelse, found footage, UFO-landing og kartets lagring. Dette er ikke en påstand om at alle kodebaner i spillet er gjennomgått.

I teksten betyr **bekreftet** at handlingen finnes i den undersøkte koden. **Risiko** betyr en mulig konsekvens av denne kodeveien, uten en observert Greyhill-feil. **Forslag** er vår egen tilpasning. Ingen Greyhill-gjennomspilling eller motorjobb er utført her.

Kildehenvisningene G01 til G12 peker til katalogen nederst. Offseter er desimale posisjoner i Kismet-funksjonen, ikke maskinkodeadresser. Det private kontrollgrunnlaget bevarer filoversikt, SHA-256 og kontrolltall; råuttrekk og maskinlokale stier følger ikke denne rapporten.

# Det SIGNAL 47 allerede har

Den lokale koden er lest ved revisjon `89a550f63d870127379365686fe7918a6c559328`. GitHub er kontrollert separat. [PR381](https://github.com/Tombonator3000/SIGNAL-47/pull/381) er **flettet til main**, med merge-commit `1b262137ca3d120ef37d210b79d6de99ce8fd4dd`. PR-status overstyrer eldre lokal dokumenttekst som sier at arbeidet ennå ikke er flettet.

| Område | Kontrollert grunnlag | Betydning for videre arbeid |
| --- | --- | --- |
| SC02 syn og lyd | HorrorThreat har separat undersøkelsesmål, siste synsposisjon og bekreftet syn. PR381 forbedrer sporfølgingen. | Utvid og prøv eksisterende modell. Ingen ny jaktmotor. |
| SC08 og SC11 grep | PR381 krever vedvarende syn, rekkevidde og fri fysisk kontakt, og avbryter ved inngang i bilkabinen. | Neste kontroll gjelder den faktiske døren, glasset og modellen i verden. |
| SC04 dører og lokking | Eksisterende kode har dørlydpunkter, ruter, søk, lokking og tilbaketrekning. | Forbedre den konkrete spillerreisen og åpne funn PLAY-31-17/18/19. |
| Inne og ute | Main beregner akustisk rom fra spillerens sted. Ambience har myk nivå- og filterovergang. | Kontroller nye roms dekning før et nytt lydsonesystem vurderes. |
| Freeform-observasjoner | Director lagrer observasjoner; observasjonsformatet skiller hørt fra sett og kan vise foto-ID. | Bygg videre på eksisterende hendelses- og beviskjede. |
| Lagringsfeil | SaveStore bruker midlertidig fil, kontrollerer flytting og returnerer feil ved mislykket skriving. | Bevar dette. Greyhills checkpoint-varsel er en svakere referanse. |

PR381 beskriver bestått Linux-editorbygg, 74 Hunt-kontroller og native SC08/SC11-scenarier med henholdsvis 12 og 11 kontroller. Det er eksisterende leveringsbevis, ikke tester kjørt av denne analysen. Fletting beviser ikke installasjon, naturlig gjennomspilling eller menneskelig vurdering av spillfølelse. [PR381 og prøvegrenser](https://github.com/Tombonator3000/SIGNAL-47/pull/381).

# Prioriterte tiltak

P1 gjelder rettferdighet og en fungerende eksisterende spillerreise. P2 gjelder videre gameplay og samspill. P3 er et valgfritt uttrykkstiltak. Omfang er relativt: lite er en lokal utvidelse, middels berører flere eiere eller lagring. Dette er vurderinger, ikke tidsløfter. GH01 til GH10 er referanseforslag; de oppretter ikke nye produksjonsoppgaver og overstyrer ikke [gjeldende TODO](../../Active/TODO.md).

| ID | Tiltak | Prioritet | Omfang | Konkret gevinst |
| --- | --- | --- | --- | --- |
| GH01 | Fullfør kontrollen av eksisterende jaktregler | P1 | Lite til middels | Spilleren kan stole på at brutt kontakt faktisk hjelper. |
| GH02 | Gjør SC04-døren til en tydelig fysisk beslutning | P1 | Middels | Lukking, lokking og rutevalg gir et forståelig reaksjonsrom. |
| GH03 | Samordne faktiske lydhendelser og AI-spor | P2 | Middels | Apparatlyd kan undersøkes uten allvitende sporing. |
| GH04 | La underlag og soner følge riktig aktør | P2 | Lite til middels | Fottrinn blir pålitelig informasjon om sted og bevegelse. |
| GH05 | Gi bevisinspeksjon én trygg inn- og utgang | P2 | Middels | Bakside og detaljer kan undersøkes uten tap av objekt eller kontroll. |
| GH06 | Bruk en bevegelig taklyd i en undersøkelig hendelse | P2 | Lite til middels | Et kort romlig forløp gir et konkret neste steg. |
| GH07 | Kontroller romlyd ved terskler og Continue | P2 | Lite | Spilleren hører konsekvent forskjell mellom inne og ute. |
| GH08 | Samle hendelsesfaser og avbrudd | P2 | Middels | Lys, lyd og figur avsluttes riktig også når hendelsen avbrytes. |
| GH09 | Bruk stabile objektidentiteter og engangsregistrering | P2 | Middels | Bevis og deler overlever dobbel input og Continue uten duplikater. |
| GH10 | La eventuell found footage-modus eie sine effekter | P3 | Middels | Bilde og lyd kan skiftes uten å ødelegge andre innstillinger. |

# GH01 Fullfør kontrollen av eksisterende jaktregler

**Bekreftet i Greyhill:** AI lagrer siste observerte spillerposisjon ved offset 26876 og bestiller bevegelse mot den ved 15698. Vellykket persepsjon av spilleren går samtidig via 29612 til en felles bane som kan sette `CAN_SEE_PLAYER_?` ved 26444. En lydhendelse kan dermed bruke deler av samme jaktbane som syn. [G01]

**Bruk:** Behold strukturen oppdagelse, reaksjon, forfølgelse, tapt kontakt og undersøkelse. Behold SIGNAL / 47s forbedring der lyd gir et sted å undersøke og syn bekrefter spilleren. PR381 har allerede gjennomført sentrale deler av dette.

**Neste konkrete arbeid:** Kontroller én faktisk flukt rundt dekning i SC02 og ett fysisk kontaktbrudd i SC08/SC11. La den synlige modellen og kollisjonen være med. Et grep må avbrytes gjennom en fysisk sperre selv om et vindu tillater syn. Lydminne skal ikke erstattes av spillerens skjulte, løpende posisjon.

**Aksept:** Spilleren bryter sikt og går stille til et annet sted. Fienden undersøker sporet som var tilgjengelig. Et nytt grep krever ny sammenhengende kontakt. Dette viderefører den flettede løsningen, og er ikke en bestilling om å lage den om igjen.

# GH02 Gjør SC04-døren til en tydelig fysisk beslutning

**Bekreftet i Greyhill:** Dørbaner stopper aktører og skriver senere tilbake fart gjennom globale alien-oppslag. Bruddbanen lagrer en indeks fra ett oppslag og leser et nytt array. Den tar også et øyeblikksbilde av spillerens faktiske posisjon som `ALIEN_MOVE` ved 4904. Ved lukking settes lukketflagget ved 12975, mens aktørkollisjon slås av ved 13138. Flagg, synlig vinkel og fysisk sperre er derfor forskjellige forhold. [G03]

**Bruk:** Døren kan kjøpe tid, varsle med rasling eller banking og endre hvilken rute som er tilgjengelig. SIGNAL / 47 har allerede et dør- og rutegrunnlag; dette trenger en samlet spillerreise.

**Forbedring:** Én dørhendelse får én ansvarlig aktør og en lokal forespørsel om passering. Døren endrer ikke andre fienders basisfart. Fienden får ikke ny kunnskap om spillerens skjulte posisjon fordi en dør ble lukket. En blokkert åpning gir stopp, ny rute eller retrett.

Toms eksisterende forslag om banking og gjenkjennelig stemme fra service yard kan prøves innen PLAY-31-18. Å holde døren lukket må fortsatt gi en vei videre i historien. Forslaget er ikke en allerede vedtatt endelig scene.

**Aksept:** Spilleren kan undersøke varselet, trekke seg unna, bryte sikt og lukke døren. Figuren stanser eller passerer fysisk. Prøv lukking mens kroppen er i åpningen og et kort nytt forsøk etter fangst. Samordne med PLAY-31-17/18/19 fremfor en parallell dørimplementering.

# GH03 Samordne faktiske lydhendelser og AI-spor

**Bekreftet i Greyhill:** Spillerskritt spiller hørbar lyd og sender en separat `ReportNoiseEvent` ved 45704. Dørpakken har hørbare lydkall, men ingen direkte `ReportNoiseEvent` i det undersøkte materialet. En lyd spilleren hører er dermed ikke i seg selv bevis for en AI-stimulus. [G02, G03]

**Nå hos oss:** HorrorThreat avleder hørbare skritt fra bevegelse og avstand. SC04 og SC11 oppdager dørendringer og lager et punkt ved døren. Dette virker som et eksisterende grunnlag, men samme type kunnskap formidles flere steder.

**Forslag:** Når flere faktiske apparater skal påvirke fiender, la produsenten sende ett felles hendelsesformat: kilde-ID, hendelses-ID, sted, tidspunkt, type og styrke. AI-en vurderer hendelsen ut fra sin plassering og rolle. Opplevd lydnivå og AI-rekkevidde kan være forskjellige, men skal komme fra samme virkelige handling.

Ikke bygg en generell hendelsesplattform først. Start med én eksisterende dørlyd og én fysisk opptaker eller lokkelyd. En ny lyd kan styre et undersøkelsesmål uten å overskrive siste synsminne. Nysgjerrige Freeform-figurer skal kunne lytte eller trekke seg unna uten automatisk å bli jaktfiender.

**Aksept:** Opptakeren lager lyd ved sin egen posisjon. Figuren undersøker apparatet, ikke spilleren som står stille et annet sted. Avslått eller fjernet apparat sender ingen nye hendelser. Samme lydhendelse behandles én gang.

# GH04 La underlag og soner følge riktig aktør

**Bekreftet i Greyhill:** Når spilleren går inn i kornsonen, settes spillerens flagg og deretter `IS_IN_CORNFIELD_?` på alle aliens gjennom et klasseoppslag. Ved utgang nullstilles alle. Alienens fottrinn kan legge på kornlyd ut fra dette flagget. Det undersøkte mønsteret følger altså spillerens sonekryssing, ikke hver aliens faktiske oppholdssted. Klassen alene beviser heller ikke at korn blokkerer syn. [G04]

**Bruk:** Underlaget kan hjelpe spilleren å lese en fiende som foreløpig ikke er synlig. Grus utenfor, metall ved en trapp og betong i gangen kan gi en forståelig rute.

**Forbedring:** Bruk aktørens egen kontaktflate eller posisjon til materialvalg. SIGNAL / 47 har allerede forflytningsstyrte kontaktpunkter i Pale-bevegelsen. Utvid disse der lydmaterialet trenger mer presisjon, fremfor å innføre en ny uavhengig skrittklokke. Volumsoner bør holde rede på hvem som faktisk er inne, også når flere soner overlapper.

**Aksept:** Spilleren står inne mens figuren går på grus ute. Bare figurens riktige underlag høres. Ingen ekstra skritt under stillstand, pause eller etter at hendelsen er avsluttet.

# GH05 Gi bevisinspeksjon én trygg inn og utgang

**Bekreftet i Greyhill:** Inspect_Item lagrer verdens-transform ved 4897, slår av kollisjon og fester selve aktøren til kameraets komponent ved 1605. Det er originalgjenstanden som flyttes i denne banen. Ved retur settes kollisjonsprofilen hardt til `BlockAll` ved 5835. Avslutningen gir input tilbake i flere steg; en egen fangstkontroll avgjør om gange og spillerkontroll skal aktiveres. [G05]

**Bruk:** Et fysisk bevis kan ha en relevant bakside, serienummer, rifter, en kontakt eller en håndskrevet merknad. Det passer SIGNAL / 47s undersøkelsesløkke når detaljen hjelper spilleren å velge et forsøk eller sammenligne funn.

**Forslag:** Utvid eksisterende interaksjon med én inspeksjonskontekst og én oppryddingsbane. Bruk en presentasjonskopi når originalen må bli stående i verden. Hvis originalen flyttes, bevar parent, transform, skala, synlighet og kollisjonsregler samlet. Lukk, fangst, områdeskifte og lasting skal kunne avslutte samme kontekst uten dobbeltvirkning. Inspeksjonsavslutning skal respektere at en annen tilstand nå eier kontrollen.

**Første kandidat:** Ett eksisterende feltapparat eller bevis med en faktisk relevant bakside. Gjenbruk modell og registrering; lag ingen ny samlegjenstandsøkonomi.

**Aksept:** Åpne, roter og lukk objektet. Avbryt så inspeksjonen ved en eksisterende spillovergang og prøv Continue. Originalen har korrekt sted og egenskaper, og spilleren har riktig kontroll. Det skal ikke skapes et ekstra bevis.

# GH06 Bruk en bevegelig taklyd i en undersøkelig hendelse

**Bekreftet i Greyhill:** Taklydobjektet spawner lyd festet til roten ved 323, flytter roten til Target1 over seks sekunder og deretter til Target2 over to sekunder. Et punktlys slås på ved start og av etter bevegelsen. Dette er en skriptet lydkilde, ikke bevis for en navigerende alien på taket. [G07]

**Nå hos oss:** Freeform Director har allerede `roof_steps`, og observasjoner kan bevares med sted, tid og skillet mellom hørt og sett. Vi trenger ikke opprette samme hendelse på nytt.

**Forslag:** Utvid den eksisterende takhendelsen til en kort fysisk bane mellom to eller tre takankre. Lyden gir en grov bevegelsesretning. Hvis møtet skal produsere et etterspor, må det oppstå som en faktisk del av hendelsen og kunne undersøkes senere. En hørt taklyd skal fortsatt registreres som lyd, uten bekreftet identitet.

**Aksept:** Fra to lytteposisjoner skal banen oppfattes som det samme forløpet i verden. En natt uten hendelse gir verken registrert lydobservasjon eller et nytt spor. Continue og områdeskifte dobler ikke hendelsen. Ved virkelig figurbevegelse bør figurens egne kontakter produsere lyden, slik at en ekstra skriptet kilde ikke gir motstridende posisjon.

# GH07 Kontroller romlyd ved terskler og Continue

**Bekreftet i Greyhill:** House_Sound_Changer sjekker etter oppstart om spilleren allerede overlapper sonen, og velger deretter samme tosekunders fade som ved inn- og utgang. Den venter ikke bare på en fremtidig terskelkryssing. [G06]

**Nå hos oss:** Main beregner romtypen fra spillerens plassering hver frame. Ambience demper utelyden og endrer lavpassfilter med myke overganger. Dette er allerede en mer samlet inngang enn å la hvert rom styre hele miksen.

**Neste konkrete arbeid:** Kontroller at nye østfløyrom, garasje, lab og aktuelle terskler er dekket av de faktiske romgrensene. Undersøk dekning først dersom en lyd er feil. Ikke legg på en konkurrerende global rommiks.

**Aksept:** Start og Continue inne i et nytt rom; gå ut og straks inn igjen. Miksen skal bli riktig uten et ekstra dørklikk. Dørbanking og figurens lokale skritt må fortsatt være lesbare. Dette er romlig miksing, og beviser ikke full lydokklusjon gjennom alle vegger.

# GH08 Samle hendelsesfaser og avbrudd

**Bekreftet i Greyhill:** UFO-landingen har flere bevegelses-, vente-, lys- og dørgrener. En bane bruker ti sekunder mot mellompunkt, tjue mot sluttpunkt, tre sekunders venting og fire sekunders døråpning. `ALIENS_OUT_?` settes etter denne åpningen. Scripted_JumpScares har en engangssperre, varsel og venting. De undersøkte montagecallbackene for completion og interruption lagrer navn, men har ingen egen synlig opprydding. Andre ytre kallere kan fortsatt finnes. [G08, G09]

**Bruk:** Varsel før handling, og en fysisk fase som forklarer den neste, gir spilleren lesbar regi. Greyhills konkrete sekunder er referansetall, ikke ferdig balanse for vårt spill.

**Forslag:** For hendelser som skal tåle pause og Continue, bevar fase, fasealder og hvilke engangshandlinger som er utført. La avslutning eie sine lys, lyd, figur og kontrolltilstand. SIGNAL / 47 har allerede tids- og fasebasert kode i Abduction og Director; utvid den konkrete eieren som trenger det.

**Aksept:** Avbryt under varsel, hovedhandling og avvikling. Ingen gjenglemt lyd, lys eller inputlås. Last inn og kontroller den valgte policyen: fortsett fra en trygg lagret fase, eller start møtet kontrollert på nytt. SC04 har allerede en eksplisitt omstartspolicy ved lasting under jakt; den skal ikke erstattes uten et faktisk behov.

# GH09 Bruk stabile objektidentiteter og engangsregistrering

**Bekreftet i Greyhill:** Enter_Object lagrer et skjulesteds arrayindeks etter klasseoppslag. Inspect_Item blander objekttyper, inspeksjon, pickup og historiesignaler gjennom mange boolske flagg. Flere pickupbaner sender domenesignaler og skjuler objektet, men rekkefølgen varierer. Kartets lagring bruker også eksplisitte referansearrayer. [G05, G10, G11]

**Bruk:** Et objekt kan varsle at en konkret handling har skjedd, mens oppdraget bestemmer hva det betyr.

**Forslag:** Gjenbruk SIGNAL / 47s eksisterende ID-baserte interaksjon og originalvern. For nye bevis eller apparatdeler skal én stabil ID følge verdensobjekt, bæring, registrering og lagring. En engangsoperasjon skal avgjøre om objektet faktisk ble overført før tilhørende oppdragsresultat bekreftes. Unngå å identifisere vedvarende objekter med plass i et nytt globalt oppslag.

**Aksept:** Dobbeltklikk, bytt område og bruk Continue. Det finnes fortsatt én original, korrekt eierskap og én registrering. Mislykket lagring gir ingen falsk melding om at arbeidet er sikret.

Greyhills SaveGame kaller checkpointmeldingen direkte etter SaveGameToSlot uten en synlig test av bool-resultatet. SIGNAL / 47s WriteRecord returnerer allerede feil ved mislykket skriving. Bevar den kontrakten når nye interaksjoner kobles til lagring. [G11]

# GH10 La eventuell found footage modus eie sine effekter

**Bekreftet i Greyhill:** Modusen finner et postprosessvolum gjennom objektnavnet `FF_MODE_POST`. Aktivering setter kameraformat omtrent 1,33 og legger på lydmiks og støy. Deaktivering setter et fast 1,777778-format og kaller `ClearSoundMixModifiers`. Dette viser modusbytte; det beviser ikke hvilke VHS-effekter materialet inneholder. [G12]

**Bruk:** Samordnet bilde, lyd og HUD er et nyttig mønster dersom den eksisterende found footage-planen skal videreføres.

**Forslag:** Bruk en eksplisitt referanse til egne effekter. Bevar forrige kameraformat og fjern bare modusens egen lydmiks. Gjentatt aktivering skal ikke stable støy eller endre gjenopprettingsverdien. Prøv samtidig bruk av radio, mottaker og andre lydprioriteter.

**Aksept:** På, på igjen, av, pause og Continue etterlater korrekt kamera, lesbar HUD og øvrig lyd. Prioriteten er P3: dette forbedrer uttrykket, mens punktene over forbedrer den aktive spillerløkken.

# En konkret neste leveranse

Den anbefalte nye prototypen er **en undersøkelig takhendelse ved SARO**, bygget videre på eksisterende `roof_steps` og observasjonsregistrering. Den kobler Greyhills romlige varsel til Freeforms avtalte forsøk og etterforskning.

1. Spilleren velger et eksisterende feltoppsett og registrerer et kontrollgrunnlag. Bruk reelt tilgjengelig loggerutstyr; manglende loggerfunksjon må løses av det eksisterende loggerarbeidet først.
2. En kvalifisert hendelse spiller en kort bane over taket. Spilleren får retning gjennom lyden og kan velge å undersøke eller fortsette vanlig arbeid.
3. Bare faktisk hørt eller sett informasjon registreres. Opptak krever at apparatet faktisk kjørte under hendelsen.
4. Hvis hendelsen endrer en markør eller et fysisk objekt, kan spilleren undersøke og dokumentere dette etterpå. En natt uten besøk lar kontrollen være uendret.
5. Ved benken sammenlignes tidspunkt, oppsett og observasjon. Lyd alene bekrefter ikke at kilden var en alien.
6. Continue bevarer apparatet, registreringen og utfallet uten å spille hendelsen på nytt eller skape en ny original.

Foreslåtte berøringspunkter er eksisterende Director, observasjonsformat, lokal lydavspilling og loggerens etablerte grensesnitt. Hold dette samlet med FF-RESEARCH-04 og FF-RESEARCH-07. Start med én hendelse og ett kontrolloppsett; større utstyrssystem og flere møter trenger ikke bygges for å vurdere gevinsten.

For Story er den separate neste reisen SC04: varsel, valg ved døren, fysisk passering, brutt sikt, lokking, retrett og et kort nytt forsøk. Kontroller eksisterende produksjonskode før nye løsninger legges til.

# Hva vi bør la ligge

- Globale oppslag som endrer alle fiender fra én lokal dør eller sone.
- Skjult lesing av spillerposisjon som erstatning for siste faktiske spor.
- Automatisk jakt fra nysgjerrige Freeform-møter.
- Ny huking, våpenøkonomi, tastemosing eller lommelyktbatteri som følge av denne referansen.
- Et nytt inne/ute-system, en ny jaktmotor eller en ny observasjonsdatabase for funksjoner vi allerede har.
- Overføring av Greyhills originale assets, bytekode eller rekonstruerte logikk til det offentlige spillrepoet. Her brukes egne beskrivelser, kontrakter og implementeringer, i tråd med prosjektreglene.

# Kildekatalog

Greyhill-stiene nedenfor er relative til det private disassembleringsuttrekket. Filnavn, tekstlinjer, funksjoner og offseter identifiserer de undersøkte kodebanene. Originale assets, full bytekode, dekodet JSON og det maskinlokale manifestet beholdes utenfor repoet.

| Kilde | Fil og tekstlinjer | Funksjon eller kontrollpunkt |
| --- | --- | --- |
| G01 | `Character/Alien_AI_BP.uasset.txt`, 687–709, 1337–1367, 1504–1510 | ExecuteUbergraph_Alien_AI_BP; 15698, 26444, 26876, 26978, 29612. |
| G02 | `Character/BP_Player.uasset.txt`, 2036–2048 | ExecuteUbergraph_BP_Player; PlayFootstepSound, 45493 og 45704. |
| G03 | `Useable/Door_Universal_BP.uasset.txt`, 227–229, 455–459, 499–501, 625–628, 958 | ExecuteUbergraph_Door_Universal_BP; 4904, 10025, 11034, 11072, 12975, 13138, 19347. Se også dørstudien fra 28. oktober for full kontrollflyt. |
| G04 | `Other/CORN_ZONE_BP.uasset.txt`, 5–51; `Character/Alien_AI_BP.uasset.txt`, 1443–1451 | ExecuteUbergraph_CORN_ZONE_BP; 106, 441, 853, 1006. Alien-fottrinn 28495 og 28651. |
| G05 | `Useable/Inspect_Item_BP.uasset.txt`, 62–101, 118–129, 294–305, 336–347, 480–539 | ExecuteUbergraph_Inspect_Item_BP; input 91/162/406/455, fangstguard 934, transform 4897, feste 1605, profil 5835. |
| G06 | `Other/House_Sound_Changer_BP.uasset.txt`, 5–19, 43–46 | ExecuteUbergraph_House_Sound_Changer_BP; oppstartsoverlap 264, fadebaner 586/634 og 763/816. |
| G07 | `Other/Sound_Object_04_Alien_Shed_Roof_BP.uasset.txt`, 5–17 | ExecuteUbergraph_Sound_Object_04_Alien_Shed_Roof_BP; 35, 132, 187, 284, 323. |
| G08 | `Vehicle/UFO_Landing_Field_BP.uasset.txt`, 24–53, 117–120 | ExecuteUbergraph_UFO_Landing_Field_BP; 546, 643, 728, 876, 1170, 1393, 3235. |
| G09 | `Other/Scripted_JumpScares_BP.uasset.txt`, 13–25, 36–60 | ExecuteUbergraph_Scripted_JumpScares_BP; 664, 821, 877, 1009, 1138. |
| G10 | `Useable/Enter_Object_BP.uasset.txt`, 70–91 | ExecuteUbergraph_Enter_Object_BP; klasseoppslag 1792 og HIDEABLE_OBJECT_INDEX 2146. |
| G11 | `Levels/Game_Level.umap.txt`, 8050 og 8470–8482 | SaveGame; 30904 SaveGameToSlot, 30947 PlayCheckpointInfo. LoadGame starter ved tekstlinje 8482. |
| G12 | `Other/FF_MODE_BP.uasset.txt`, 17–19, 34–57 | ExecuteUbergraph_FF_MODE_BP; format 89/836, global clear 148, volumoppslag 536/573, miks 895. |

Egne kodehenvisninger bruker PR381s kildecommit `f8b6d5e593b94faf6fd78e001a804c5e045d74a5`. Git-blob-hash er kontrollert lik den lokale revisjonen for alle ti lenkede kodefiler. Senere endringer flytter dermed ikke analysegrunnlaget:

- [HorrorThreat, syn, lyd og undersøkelse](https://github.com/Tombonator3000/SIGNAL-47/blob/f8b6d5e593b94faf6fd78e001a804c5e045d74a5/unreal/Signal47/Source/Signal47/Private/S47HorrorThreat.cpp#L530).
- [SC08, grep og fysisk kontakt](https://github.com/Tombonator3000/SIGNAL-47/blob/f8b6d5e593b94faf6fd78e001a804c5e045d74a5/unreal/Signal47/Source/Signal47/Private/S47SC08.cpp#L374).
- [SC11, dørlyd og undersøkelse](https://github.com/Tombonator3000/SIGNAL-47/blob/f8b6d5e593b94faf6fd78e001a804c5e045d74a5/unreal/Signal47/Source/Signal47/Private/S47SC11.cpp#L548).
- [SC04, lagring og eksplisitt omstart av jakt](https://github.com/Tombonator3000/SIGNAL-47/blob/f8b6d5e593b94faf6fd78e001a804c5e045d74a5/unreal/Signal47/Source/Signal47/Private/S47SC04.cpp#L151).
- [Main, valg av akustisk rom](https://github.com/Tombonator3000/SIGNAL-47/blob/f8b6d5e593b94faf6fd78e001a804c5e045d74a5/unreal/Signal47/Source/Signal47/Private/S47Main.cpp#L173) og [Ambience, nivå og filter](https://github.com/Tombonator3000/SIGNAL-47/blob/f8b6d5e593b94faf6fd78e001a804c5e045d74a5/unreal/Signal47/Source/Signal47/Private/S47Ambience.cpp#L230).
- [Director, lagrede observasjoner](https://github.com/Tombonator3000/SIGNAL-47/blob/f8b6d5e593b94faf6fd78e001a804c5e045d74a5/unreal/Signal47/Source/Signal47/Private/S47FreeformDirector.cpp#L224) og [observasjonens betydning og format](https://github.com/Tombonator3000/SIGNAL-47/blob/f8b6d5e593b94faf6fd78e001a804c5e045d74a5/unreal/Signal47/Source/Signal47/Public/S47FreeformObservation.h#L6).
- [Inter, eksisterende ID-register](https://github.com/Tombonator3000/SIGNAL-47/blob/f8b6d5e593b94faf6fd78e001a804c5e045d74a5/unreal/Signal47/Source/Signal47/Private/S47Inter.cpp#L10) og [SaveStore, resultat fra skriving](https://github.com/Tombonator3000/SIGNAL-47/blob/f8b6d5e593b94faf6fd78e001a804c5e045d74a5/unreal/Signal47/Source/Signal47/Private/S47Saves.cpp#L358).

Tidligere analyser brukt som veivisere og kontrollgrunnlag: [jaktanalysen](GREYHILL_RESEARCH.md), [videre analyser](GREYHILL_NEXT_ANALYSES.md) og [dørkodeanalysen](GREYHILL_DOOR_CODE_ANALYSIS.md), alle fra 28. oktober. Referanseforslagene over tar hensyn til den undersøkte koden og kontrollert PR-status 4. november. Spillkode, installasjon og personlige lagringer endres ikke av at funnene registreres.
