# Forslag til bedre spillerreise, skrekk og uro

**Status:** Under arbeid. **Gjelder:** Forslag for åpningen og SC01 til SC13 i Unreal. **Sist kontrollert:** 31. oktober 2026.

Tom ba om forbedringsforslag gjennom hele historien og mer skrekk og uro. Senere samme økt ba han om å vente med retting til hans gjennomspilling er ferdig. Dette dokumentet er derfor forslag til vurdering etter spilltesten, ikke nye implementerte scener eller en godkjenning av alle ideene. [TODO](../../Active/TODO.md) fører åpne feil og videre arbeid; [logikkpasset](../../Active/Reports/LOGIKKPASS_2026-10-31.md) dokumenterer kildefunnene.

## Anbefalt retning

Natten bør utvikle seg fra «jeg kjenner dette stedet» til «noe kjenner meg». La spilleren først lære normale lyder, rutiner og referanser. Senere endres én av dem på en måte spilleren kan undersøke. Det faglige arbeidet blir da en kilde til uro: spilleren gjør en god kontroll, men resultatet er verre enn den første mistanken.

Prioriter logiske mål og rettferdige fienderegler først. Når spilleren ikke vet om en dør er feil eller om en trigger mangler, blir uroen lett til feilsøking. Bevar ingen alien før rundt02, fire jakter, de to kontrollmetodene, rolige Rom6/diner og finalens47sekunder med34sekunders uttrekk. Ingen ny fiendetype eller stemmerolle trengs for forslagene her.

**Prioritet A** betyr anbefalt første pass etter feilretting. **B** er senere polering. Liten/middels beskriver omfang, ikke tidsløfte. Følelsen må vurderes med faktisk spilling; dette er regi og designvurderinger fra kode/manus.

## Etterforskningstavlen: oversikt mens saken vokser

**Ny bestilling fra Tom 31. oktober:** Spor som spilleren kobler sammen skal automatisk danne oversiktlige grupper med tilhørende spørsmål og notater. Organiser lokalt og stabilt etter etablerte koblinger, med luft mellom kort og plass for nye funn. Visuell gruppering skal støtte spillerens arbeid uten å røpe nye bevisforbindelser. Bevar manuell plassering, lesbar tekst og lagring av oppsettet.

Ved feil tråd skal bare én vanlig, sammenhengende linje slakke og fade bort. Fjern den stiplede avvisningsvarianten og kontroller at drag-/slipp-overgangen ikke viser to parallelle effekter. PLAY-31-24/25 i [TODO](../../Active/TODO.md) fører gjennomføringen; dette er registrerte forbedringer, ikke levert tavleatferd.

## Stjernehullet og meteorregnet

**Ny bestilling fra Tom 31. oktober:** Gjør hullet i stjernene større og lettere å oppdage. Signalet klargjør meteorregnet, men utgangen fra bygget starter passasjen. La nok meteorer gå visuelt bak tomrommet til at fraværet blir tydelig: lysstreker forsvinner ved kanten og kan dukke opp igjen på den andre siden, mens feltet selv forblir tom himmel. Timing, tetthet og hullets fremkomst må fungere sammen og gi spilleren tid til å se opp. Bevar antennenes retning og den frie kamerastyringen. [Regi og kildegrunnlag](../Proposals/NIAGARA_VFX_2026-10-28.md); PLAY-31-23 i [TODO](../../Active/TODO.md) fører endringen. Dette er en ny bestilling, ikke levert himmeleffekt.

## Alienbevegelse og fysisk troverdighet

**Ny bestilling fra Tom 31. oktober:** Bedre gange, løping, crab walk og griping, med naturlige overganger mellom tilstandene. PLAY-31-22 i [TODO](../../Active/TODO.md) fører animasjonsarbeidet. Dagens Tripo-skin er et teknisk utgangspunkt, ikke visuell sluttaksept.

**Regi og gameplay:** La rolig gange og lytting ha en tydelig annen rytme enn målrettet løping. Lav kryping/sidegang skal ha overbevisende støtte i hender og føtter, tilpasset figuren. Hode og overkropp kan orientere seg mot en lyd før kroppen følger, men ledd og balanse må fortsatt henge sammen. Overgangen til jakt skal være lesbar og gripingen ha et kort forvarsel og faktisk kontakt. Bevegelse, fottrinn og fangst må følge samme fart og posisjon.

Gjenbruk egne rigger og egnede klipp før ny produksjon. De eldre fellesmodell-klippene er kandidater som må vurderes mot Tripo-riggen. Kontroller animasjon i den faktiske gangen, arkivet, døråpningene og ute, med bøying/senking der høyden krever det. En skummel positur skal ikke skape urettferdig kollisjon, fotglidning eller fastlåsing. Dette forbedrer eksisterende møter og innfører ingen ny jakt.

## Bærbar feltradio og varsling ved alienmøter

**Ny bestilling fra Tom 31. oktober:** En walkietalkie ligger på skrivebordet ved spillstart og kan plukkes opp og tas med. Først er den et vanlig arbeidsredskap. Når en alien faktisk er nær, brytes den normale mottakingen av rar skurring. Avviket får en hørbar kilde som spilleren allerede kjenner, og kan skape uro før fienden er tydelig synlig. Nærhet bør påvirke klang/intensitet uten et presist avstandstall. La støyen trekke seg tilbake når nærheten opphører; unngå konstant høy støy gjennom hele møtet.

**Gameplay:** Pickup og bæring må lagres, bordmodellen fjernes når den tas, og eldre saker må kunne hente radioen uten omstart. Nødvendige eksisterende samtaler skal fortsatt være nåbare om radioen ikke er tatt. Gjenbruk en egnet radio fra prosjektet og eksisterende radiolyd der det passer. Den nye SARO-feltradioen er et eget objekt; Tomás' batteriløse radio i Rom6 og dens 4+7-/EVENT-funksjon bevares.

**Lydregi:** Tom ber også om musikk eller lydcue når aliens kommer. Prøv en kort markering ved møtestart/oppdagelse og la radio, skritt og stillhet bære resten. Samordne signalene, så uroen ikke blir tre samtidige alarmer. Hendelser og nærhet skal følge faktiske fiender, uten å røpe fremtidige spawns eller legge alienvarsler inn i den rolige åpningen. Vanlig radio kan introduseres før første møte. Bevar taleforståelighet, lydvalg og mulighet for tekstlig indikasjon.

**Videre idé, ikke besluttet:** Dale kan senere kontaktes via feltradioen. Det kan også gi grunnlag for å kjenne igjen en gjentatt stemme ved service-yard-døren. Tidspunkt, replikker og eventuell ny samtale avklares mot eksisterende historie; ingen ny stemmerolle trengs. PLAY-31-20/21 i [TODO](../../Active/TODO.md) fører bestilt radio/lyd. Funksjonene er ikke implementert ennå.

## Åpningen, egen bil og ankomst

**Gameplay:** Gjør første arbeidsmål og riktig besøksinngang tydelige. Ankomst skal godta god parkering uten at spilleren må gjette en bestemt vinkel. Matstopp forblir valgfritt, og egen bil får en konsekvent rolle når den senere åpnes for bruk.

**Uro:** La innkjøringen være vanlig. Et svakt radiosignal i et lavpunkt kan vende naturlig tilbake, og SARO møter spilleren med ventilasjon, avkjølende metall og fjerne servoer. Ingen tydelig overnaturlig hendelse behøves her. Dette etablerer lydene som senere kan forsvinne.

**Virkning:** Spilleren får et normalt sted å miste tryggheten i. **Liten / A.**

## SC01, kveldsrunden

**Gameplay:** Hver rutine bør svare på tre spørsmål: hva kontrolleres, hva viser normal drift, og hva gjør jeg etterpå? Knytt20:30-permen til den konkrete referanselinjen som senere endres i SC07. Gjenbruk dagens turer fremfor å legge til flere henteoppgaver.

**Uro:** Etter klasket mot ruta er avtrykket det eneste sikre funnet. La spilleren lete i et tomt utsyn før journalen konkluderer. En normal ventilasjonslyd kan stanse under en senere arbeidsøkt og starte igjen når den er ferdig. Velg ett slikt avvik, ikke en ny effekt for hver oppgave. Ingen kropp eller gjenkjennelig alien før02.

**Virkning:** Arbeidets forutsigbarhet begynner å svikte. **Liten / A.**

## Fra00:13 til trafoen

**Gameplay:** Gjør årsakskjeden lesbar: signalet, K3 ute, strømbruddet, ordren om Caliche og T01-kontrollens normalverdi. Etter gjenoppretting må mål og lampe slutte å kreve det som allerede er utført. Faktiske tidsstempler skal stemme med spillerens handlinger.

**Uro:** La den vanlige trafoduren komme tydelig tilbake etter korrekt T01. På vei mot bilen kan en del av det normale lydbildet falle bort. Pausen forbereder SC02 uten å legge et tidlig monster inn i trafoturen.

**Virkning:** Spilleren rekker å kjenne mestring og lettelse før situasjonen brytes. **Liten / A.**

## SC02, tilbake til bilen

**Gameplay:** Første jakt skal lære samme regel som senere gjelder: lyd tiltrekker, opak dekning bryter sikt, og bilen gir flukt. Vis en forståelig retning til bilen før faren oppstår. Lokal fangst bevarer utført T01. Usett avreise må få en sannferdig vei videre eller en tydelig gjenopptakelse.

**Skrekk:** Begynn med en kropp som står unaturlig stille. La én hodevending følge et faktisk skritt. Etter brutt sikt bør spilleren høre at den undersøker siste kjente sted, ikke få inntrykk av at den alltid vet hvor spilleren er. Hold lyden stedlig og lesbar.

**Virkning:** Spilleren blir redd og lærer samtidig at egne valg virker. **Middels / A.**

## SC03, hjelpen som kom

**Gameplay:** Vis et konkret sted å vente ved stasjonen. Under dagens korte venting kan spilleren få ett valgfritt vitnespørsmål eller undersøke en eksisterende observasjon. Etter første kontakt med Ortiz må målet følge det som faktisk skjer. Ikke gi en redningsoppgave spilleren umulig kan utføre.

**Skrekk:** La Ortiz først være et menneskelig avbrekk, opptatt av en vanlig arbeidsordre. Etter forsvinningen fortsetter servicevognens motor mens termosen ruller og stanser. Stillheten i skikkelsene ved lyskanten får stå uten ny jakt eller en ekstra høy skvettdlyd. Etterpå gir de varme, etterlatte tingene tapet tyngde.

**Virkning:** Returen for å sikre originalene får en menneskelig grunn. **Liten/middels / A.**

## SC04, sikre originalene

**Gameplay:** Bruk samme navn på HUD, fysisk etikett og dokument. Gjør plansperrens virkning konkret: automatikk stanser, mottaker og nødvendig manuelt arbeid fortsetter. Dørdekning og telefonlokking skal kunne oppdages gjennom rom og lyd, uten å kreve en løsningsoppskrift.

**Skrekk:** Gjenbruk det tidligere Dale-fragmentet med identisk pause, fra feil dør. La spilleren få tid til å kjenne det igjen. Bak en lukket dør kan fottrinnene stanse og lytte før de går videre. Den korte stillheten tvinger spilleren til å velge når døren skal åpnes.

**Nytt spillerfunn 31. oktober, spilltid 03:03:** Tom opplever at dørlukking ikke gir fremdrift, og at alien kommer plutselig fra gangen og inn i et arkiv som er for trangt til å unnslippe. Dette er en gameplayfeil som må undersøkes, ikke en ønsket skrekkeffekt. PLAY-31-17 i [TODO](../../Active/TODO.md) fører dørlukking/progresjon. Logg/kilde viser at trygg dørtilstand og avsluttet fienderunde er forskjellige vakter; HUD må gjøre handlingene forståelige.

**Tilleggsfunn kl.03:05:** Alien kan ifølge Tom sette seg fast i inngangsdøren. PLAY-31-19 fører passering, dør-/kroppsoverlapp og gjenoppretting av ruten. Fastlåsing skal ikke brukes som løsningen på jakten eller gi permanent progresjonsstopp.

**Toms alternative regi, til utprøving gjennom PLAY-31-18:** La møtet starte utenfor ved service yard, med banking og det gjenkjennelige Dale-fragmentet. Spilleren hører hvor stemmen kommer fra, velger å åpne og ser alien stå fysisk utenfor terskelen. En kort oppdagelses-/reaksjonspause gir tid til å rygge før jakten begynner. Dette er et alternativ til anbefalingen om stemmen fra Operations-døren ovenfor, ikke en ekstra alien som dukker opp inne samtidig.

Gi ruten plass til å snu og bryte sikt. Arkivet skal ikke være en uunngåelig felle. Spilleren må kunne trekke fienden bort fra passasjen eller ut i gården, komme seg inn og stenge etter den. En kjent lydkilde kan brukes som lokkemiddel når spilleren har oppdaget sammenhengen. Fienden må gå den faktiske ruten gjennom åpen dør, miste sikt bak lukket dør og reagere hørbart med skritt, banking eller lytting. Møtet skal gi uro gjennom et valg spilleren forstår og konsekvenser som stemmer med rommet.

Kvitter at hver relevant dør er sikret, og vis om spilleren fremdeles er utsatt, må lure fienden ut eller mangler et originaldokument. Hvis låsing tas inn, må låsen fungere fysisk og overleve lagring; en lukkelyd alene er ikke en lås. Å holde ytterdøren lukket skal ha en forståelig videre vei og ikke utløse teleport eller permanent progresjonsstopp. Dette erstatter eventuelt den eksisterende SC04-jakten; antallet jakter økes ikke. Ingen nye stemmeopptak bestilles før eksisterende Dale-materiale er vurdert.

**Virkning:** Kjente rom og kjent stemme blir upålitelige, mens farens regler er pålitelige. **Middels / A.**

## SC05, samtalen som ligger foran

**Gameplay:** Skill residualen som oppstod00:13 fra kontrollen spilleren gjør nå. Etter utskriften er telefonen neste mål. Notatboken bør skille hva som ble sagt fra hva spilleren faktisk observerte; tidsforløp alene er ikke bevis på at noe ble sett eller hørt.

**Uro:** Bygg videre på dagens47sekunders etterspill med ett konkret eksisterende romobjekt eller en liten gjenkjennelig lyd. La stillheten vare før hendelsen. Vent med en forklarende konklusjon til spilleren har hatt mulighet til å oppdage sammenhengen. Ikke legg en ekstra skvett oppå selve oppdagelsen.

**Virkning:** Spilleren forstår selv at samtalen lå foran rommet i tid. **Middels / A.**

## SC06, det andre motivet

**Gameplay:** Gjør forskningssløyfen tydelig: første bilde, endret betingelse, andre bilde. Aktiv og passiv kontroll er to gyldige valg, ikke to obligatoriske sjekklister. Ved tåket print skal meldingen si at negativet er bevart og nøyaktig hva som må gjøres om igjen.

**Uro:** La spillerens egen korrekte kontroll skape stillheten ved B-12 når motor eller annen normal kilde isoleres. Når kameraet senkes, er utsynet tomt. Bildeforskjellen senere skal stemme med faktisk eksponering. Ingen ny figur behøves ute for å gjøre forsøket skremmende.

**Virkning:** En normal forklaring blir systematisk fjernet av spilleren selv. **Liten/middels / A.**

## SC07, det Dale ikke leverte inn

**Gameplay:** Prioriter original linje, rettet linje og feltmappens hovedspor. Gjør private sidepapirer og VHS tydelig valgfrie. Bruk samme referanseord som i20:30-permen. Etter innsikten skal ett konkret spørsmål lede til Earl og arrayet.

**Uro:** En lysflate utenfor kontorvinduet kan ha flyttet seg mens spilleren leste. Endringen skjer først etter at panelet lukkes; den må ikke simulere at spilltiden gikk bak en pause. La en jordnær forklaring fortsatt være mulig. Et enkelt romlig avvik er nok, uten figur som hopper frem.

**Virkning:** Lesing og nysgjerrighet føles utsatt, uten å straffe spilleren for å åpne dokumenter. **Liten / B.**

## SC08, de peker på deg

**Gameplay:** Skill Earls andrehåndsrapport fra en egen utført lydprøve. Senere kan en kort isolert avspilling med gjentatt fragment gjøre spilleren til direkte vitne. Ved arrayet skal A, B og REF finnes med servicearket, og konklusjonen må følge faktiske ståsteder og observasjoner.

**Skrekk:** La servoene starte bak spilleren etter første forflytning. Spilleren snur seg, undersøker retningen og prøver tilbakeveien. Etter flukten kan antennene holde det forlatte stedet kort før de går til026. Bevegelsen må kunne knyttes til testen, ikke til tilfeldig ambientaktivitet.

**Virkning:** Spilleren demonstrerer selv at de er blitt referansen. Jakten kommer etter en oppdagelse med mening. **Middels / A.**

## SC09, den avbrutte oppstillingen

**Gameplay:** Samle feltarbeidet om to spørsmål: hva flyttet referansen, og hva skjedde med kabelen? Oppdater neste handling når ett spor er sikret. Bruk Tomás-navnet konsekvent, og la Nora-samtalen vente på begge ferdige feltkopier.

**Uro:** Gi kabelsnittene og den tomme foten et rolig observasjonsøyeblikk. En enkelt avsøkende lysflate kan passere hytta uten å starte en femte jakt. Den må ikke stenge nødvendig retur eller teleportere spilleren; faren oppleves nær fordi man velger å vente på passasjen.

**Virkning:** Fare kan være til stede uten at hvert feltbesøk blir en forfølgelse. **Liten/middels / B.**

## SC10, Rom6

**Gameplay:** Gi samtalen en stigende orden: rettelsen, kabelen, broren og hva Reyes skal gjøre nå. Etterpå oppsummeres neste konkrete oppdrag. Spilleren skal kunne forlate rommet uten å huske hver enkelt replikk.

**Uro:** Behold rommet som reell trygghet. Noras presise kunnskap og en pause før en vanskelig opplysning bærer uroen. La vanlig motell-TV eller rørlyd følge avreisen, uten monsterlyd eller et bakholdsangrep ved bilen. En trygg scene kan være urovekkende gjennom hva den avslører.

**Virkning:** Pustepausen gjør både Noras advarsel og neste angrep sterkere. **Liten / A.**

## SC11, SARO blir avsøkt

**Gameplay:** Rett telex-innlastingen først. Vis hvilke originaler som er sikret og hva som mangler. Gi et kort orienteringsøyeblikk etter fjernarkene før aktivt søk. Hvis lokal lampebryter tas inn, skal forskjellen være lesbar uten å slå av søkelyset utenfra.

**Skrekk:** Søkelyset passerer kjente vinduer før skrittene kommer fra en annen kant inne. En lukket dør gir midlertidig skjul: lyset glir forbi, fottrinn stanser, og spilleren velger når det er trygt å bevege seg. Ikke bruk maksimal musikk hele tiden; stillheten må ha praktisk betydning.

**Virkning:** Nattens største jakt krever at spilleren leser to sansekilder, ikke bare løper fra en raskere fiende. **Middels / A.**

## SC12, det dere rekker å sende

**Gameplay:** Tre kilder, tre tydelige ID-er og én grensetid. Wards bekreftelse gir et synlig faktum om mottatt varsel, adskilt fra overlevering av originalene. Kart, patruljeark, avreise og hint må samles rundt kurér10.4 og passering av C8.15. Et fysisk ta-/les-øyeblikk for patruljearket kan erstatte dagens automatiske dokumenttildeling.

**Uro:** Behold kaffe, menneskestemmer og kjøleskapsdur. Et fjernt lys kan sees gjennom vinduet uten reaksjonskrav eller ny hendelseskjede. Folkene er slitne mennesker; unngå en ny antydning om at alle i dineren er blitt erstattet. La varslingen oppleves som en faktisk seier.

**Virkning:** Avreisen er et begrunnet valg med noe verdifullt å bevare. **Liten / A.**

## SC13, siste vei mot Roswell

**Gameplay:** Bevar det trygge avreisepunktet og vis kurérmålet på eksisterende kart/skilt. Snu- og stillstandshint må støtte samme mål. Motorstoppet skal oppleves ufrivillig, uten antydning om glemt bensinoppgave eller feil rute. Finalens47sekunder og34sekunders uttrekk endres ikke.

**Skrekk og storhet:** La siste trygge kjørestrekning ha vanlig motor og radio. Når egen motorlyd forsvinner, gi fraværet plass før fartøyets størrelse blir tydelig. Bruk skyenes tildekking til gradvis skala. Tomt førersete og1947-bildet får hver sin rolige avsløring; ingen ekstra skvettdlyd behøves etter sluttbildet.

**Virkning:** Fluktbehovet går over i erkjennelsen av hva fotografiet viser. **Liten/middels / A.**

## Fem forslag å prioritere etter spilltesten

1. Samme gjennomførbare neste mål på HUD, gjenstand og notatbok gjennom hele reisen.
2. Etabler normale romlyder og bruk senere fravær eller feil plassering av de samme lydene.
3. Gjør Dale-fragmentet og47sekunders etterspill til gjenkjennelige observasjoner spilleren får tid til å forstå selv.
4. Gi fienden hørbar undersøkelse av siste kjente sted og en kort lyttestopp bak dør. Bevar rettferdig sikt og dekning.
5. Beskytt Rom6 og dineren som reelle pustepauser, og la skyavsløringen og sluttfotografiet få ro.

## Felles rammer for eventuell produksjon

- Ingen økning fra fire til fem jakter og ingen ny fienderegel bare for å få flere skrekktopper.
- Småhendelser får lagrede engangsflagg, bruker aktiv spilletid og må vente under dialog, jakt, nødvendig kildekontroll og finale.
- Paneler og pause må ikke gi skjult fare eller påstå at spilleren har observert noe som skjedde utenfor opplevelsen.
- Velg få tydelige variasjoner per akt. Flimring, bankelyder eller skritt ved hvert objekt blir forventet bakgrunnsstøy.
- Spilleren må kunne undersøke mistanken. En lyd som peker mot en dør bør ha en rimelig kilde, et etterspor eller en bevisst uavklart observasjon.
- Bruk tekst/transkripsjon som tilgjengelig støtte for nødvendig informasjon. Skrekk kan ligge i lyd, men hovedprogresjonen må ikke kreve perfekt hørsel.
- Bevar godkjente miljøer, kanon, lagringer, originalbilder og eksisterende assets. Forslagene er tenkt for dagens systemer, uten kjøp eller motorbytte.

Grunnlag: gjeldende godkjente tidslinje, produksjonsmanuset og kodegjennomgangen av SC01–SC13. Prosjektbiblioteket ble søkt først og deretter bredere; treffene gjaldt Blender-uttrykk, ikke dokumenterte løsninger for denne runtime-logikken. Ingen ekstern skill, plugin eller kode er tatt inn. Forslagene her er prosjektspesifikke vurderinger, ikke målte forbedringer.
