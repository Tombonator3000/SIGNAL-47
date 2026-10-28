# Dører under jakt i Greyhill Incident med kodeanalyse

28. oktober 2026. Detaljstudie av dørpassering, lyd og lagring, som oppfølging til jaktanalysen for Signal / 47.

Greyhill lar dørlogikken styre både geometri, lyd, alien-fart, animasjoner og deler av jaktregien. Det gir konkrete hendelser å lære av, men koblingen er tett: flere baner skriver til alle aliens, én leser spillerens faktiske posisjon, og lagringen bevarer ikke en komplett pågående dørinteraksjon. For Signal 47 anbefales en dør med egne tilstander og én navngitt ansvarlig aktør, mens jaktminne og navigasjon eies av fienden.

Dette er statisk analyse av den tidligere dekodede UE 4.27-utgaven, Steam-build 12751524. 77 instruksjonsreferanser er kontrollert mot bytekodeposisjonene. Kodeblokkene i den private rekonstruksjonen er lesekode, ikke original C++ eller en bevist full rekonstruksjon av editorgrafen. Den separate C++-referansemodellen er ny kode for å forklare et eget design. Den er ikke koblet til spillet.

## Kode og kontrollgrunnlag

- [Selvstendig C++-referansemodell](Research/Greyhill/DoorStudy/DoorPolicy.hpp).
- [16 C++-testscenarioer](Research/Greyhill/DoorStudy/DoorPolicyTests.cpp).
- [Kontrollstatus](Research/Greyhill/DoorStudy/validation.json).
- Rekonstruert lesekode, instruksjonslister og originale bytekodeuttrykk beholdes i det private lokale uttrekket utenfor repoet.

Primærfunksjonen er `Door_Universal_BP.ExecuteUbergraph_Door_Universal_BP`, deklarert og beregnet bytekodestørrelse 19 939. Døren har 60 funksjonsoppføringer inkludert delegater, hendelser og små innpakningsfunksjoner. Kontrollflyten må følges gjennom hopp, flow-stack, latente callbacks og timeline-oppdateringer. En lineær gjennomlesning av instruksjonene gir feil tidsrekkefølge.

Alle adresser nedenfor er desimale Kismet-posisjoner i den navngitte funksjonen. De er ikke adresser i Shipping.exe.

## 1. Spilleren og fienden bruker ulike innganger

Spillerens `InteractOBJECT` går inn ved 12676 og sjekker i denne rekkefølgen:

1. Spilleren ser ikke gjennom dørhullet.
2. `CANT_OPEN_?` gir rasling og stopp.
3. Hvis døren er ulåst, sjekkes behov for verktøy og om en alien allerede bryter.
4. En interaksjonsport begrenser gjentakelser.
5. Lukket dør går til åpning; åpen dør går til lukking.
6. Låst dør sammenligner spillerens nøkkel-/verktøynummer med dørens nummer. Andre baner håndterer verktøykrav, oppdragskrav og replikk.

Den direkte hendelsen `DoorOpenEvent` ved 16531 hopper til 12916, etter flere av disse sjekkene. `DoorCloseEvent` ved 16655 hopper til 12975. Dette er forfatter-/systeminnganger, og de er ikke identiske med et vanlig spillerklikk. De kan ikke omtales som en enkelt universell ToggleDoor-funksjon.

Alien-overlap starter ved 15411. Aktøren castes til alien, låst dør avvises ved 15486 og lukket tilstand kreves ved 15501. Først deretter velges bruddbane via `ALIEN_MUST_BREAK_?`. Et sant bruddflagg betyr altså ikke i seg selv at en alien kan bryte en låst dør.

**Konsekvens for Signal 47:** skill mellom en vanlig forespørsel om passering og en uttrykkelig scenetillatelse til å forcere en sperre. En regissørinngang må ha en synlig, dokumentert kontrakt.

## 2. Vanlig alien-passering stopper overlappende aktører, men gjenstarter alle

Når `ALIEN_MUST_BREAK_?` er falsk, kalles `Alien_Must_Not_Break` ved 15580. Den går via 19654:

- Safe-door-varianten returnerer uten denne passeringen.
- Andre dører finner alle aliens og sjekker hvem som overlapper.
- Aktører som allerede fanger spilleren eller er døde, hoppes over.
- Overlappende aktørs `MaxWalkSpeed`, `STANDART_WALK_SPEED` og `FAST_WALK_SPEED` settes til null ved 10025, 10151 og 10255.
- En forsinkelse på 0,5 sekund ved 10300 går til 1947.

Callbacken ved 1947 gjør et nytt globalt alien-oppslag. I den løkken gjenopprettes fart 70/225, kollisjon deaktiveres og Timeline 3 startes. Det er ikke bare den ene aktøren som ble stoppet som får farten skrevet tilbake. Klasseverdien for fast gange var 270 i den første analysen; dørbanen skriver 225.

Dette er et bekreftet bredt sideutslag i koden. At det faktisk forstyrrer en annen jakt samtidig, er en uprøvd følge. Antall aktive fiender, sceneplassering og hendelsesrekkefølge avgjør om det blir synlig.

**Konsekvens for Signal 47:** døren skal gi en lokal passeringstillatelse til aktøren som ba om den. Den skal ikke kjenne eller endre den globale fiendelisten eller fiendens basisfart.

## 3. Bruddnavnet skjuler hva koden faktisk gjør

`NoAlienIsCurrentlyBreaking` starter ved 19759 og setter `ALIEN_IS_BREAKING_?` til true. Navnet beskriver inngangstilfellet, ikke den endelige tilstanden. Deretter leter den etter en overlappende alien.

Ved 11034 lagres indeksen til funnet fra ett `GetAllActorsOfClass`-array. Ved 11072 hentes et nytt array. Det nye arrayet brukes ved 4454, og aktøren settes som `CURRENT_ALIEN_ON_DOOR` ved 4513.

Dette er en indekskobling mellom to oppslag. Bytekoden viser ikke at referansen til overlap-aktøren bare føres direkte videre. Endret aktørsett eller rekkefølge kan derfor gi feil identitet. Det er en risiko, ikke en påvist feil i en faktisk Greyhill-kjøring.

**Konsekvens for Signal 47:** behold en direkte, gyldighetssjekket aktørreferanse. Hvis en annen alien kommer fram mens døren allerede er opptatt, må den vente eller velge annen rute. Den skal ikke overta et globalt indeksfelt.

## 4. Bruddbanen bruker faktisk spillerposisjon

Etter at en aktør er valgt:

| Adresse | Handling |
|---:|---|
| 4587 | Spill dørbruddlyd ved døren. |
| 4655, 4722, 4767 | Sett den valgte alienens tre fartsegenskaper til null. |
| 4812 til 4904 | Les spillerens root-posisjon og lagre den i `ALIEN_MOVE`. |
| 4936 | Start alienens dørmontasje. |

`ALIEN_MOVE` kommer her fra spillerens faktiske posisjon ved prøvetakingen. Den kommer ikke fra `LAST_SEEN_PLAYER_POSITION` eller et registrert lydsted. Det er ett øyeblikksbilde på denne banen, ikke bevis for løpende allvitende sporing.

For Greyhill kan dette være tilsiktet regi. For Signal 47s avtalte regel om syn, lyd og mistet kontakt må målet være et eksplisitt spor. En dør som er lukket, gir ikke i seg selv fienden kunnskap om hvor spilleren har gått videre.

## 5. Montasje, polling og reservebane går hver for seg

Bruddbanen kan ikke oversettes til «spill animasjonen, vent, åpne, fortsett». Flow-stack ved 4532 og 3672 fordeler arbeidet mellom flere baner:

| Gren | Hva bytekoden gjør |
|---|---|
| Montasje | Starter dørmontasjen ved 4936. Completion-/notify-hendelsene som er undersøkt her, lagrer navn i midlertidige variabler; de åpner ikke alene døren. |
| Polling | Sjekker lukketflagget ved 3677 og venter 0,2 sekund ved 3691 mens det er sant. Når det er falskt, kalles `Chase_Player` ved 3746 og fart 70/225 skrives tilbake. |
| Reserve | En forsinkelse på 9 sekunder ved 3966 går til 4021. Farten skrives tilbake, og hvis lukketflagget fortsatt er sant ved 4204, bestilles MoveTo mot `ALIEN_MOVE` ved 4214. Bruddflagg nullstilles ved 4431/4442. |

Dette er latente grener på samme Blueprint-instans. En timer her er ikke en blokkert tråd. Gjentatte kall med samme latente identitet må heller ikke modelleres som vilkårlig mange uavhengige timere. Tidsatferd ved avbrudd, pause og overlap med andre kall er ikke kjørt.

MoveTo ved 4214 beviser en navigasjonsforespørsel, ikke at aktøren fysisk passerer en fortsatt stengt dør. OnFail-banen ved 5956 returnerer etter å ha lagret bevegelsesresultatet. OnSuccess ved 5984 går videre til `Alien_Walk_Around` ved 6011.

**Konsekvens for Signal 47:** én eier og én eksplisitt transaksjon for passeringen. Timeout skal beholde en trygg fysisk sperre og be om ny rute eller kontrollert tilbaketrekning. Den skal ikke flytte fienden forbi døren.

## 6. Lukketflagget er ikke et bevis på fysisk blokkering

Ved lukking settes `DOOR_CLOSED_?` til true ved 12975 før animasjonen starter. Kollisjon på hele aktøren slås av ved 13138, og Timeline 1 startes ved 13149.

Ved vanlig åpning deaktiveres kollisjon ved 1698 før Timeline 0. Lukketflagget settes til false fra timeline-oppdateringen ved 17454, ikke bare fra en Finished-hendelse. Alien-passering med Timeline 3 setter også flagget fra en Update-bane ved 19347.

De serialiserte timeline-malene 0, 1 og 3 har lengde omtrent 0,7 sekund. Dette er malverdier, ikke en målt gjennomløpstid. Dørbanene bruker også egne forsinkede kollisjonskall. Mesh-rotasjonene utføres med Sweep=false.

Dermed må tre ting holdes fra hverandre: logisk lukketflagg, synlig dørvinkel og fysisk kollisjon. Denne analysen beviser ikke at Greyhill klipper gjennom dører, men koden gir heller ingen garanti om at de tre alltid skifter samtidig.

**Konsekvens for Signal 47:** gjenopprett mesh og kollisjon før aktører slippes løs. Bruk faktisk geometri til siktprøver. Referansemodellen er konservativ og holder passasjen blokkert til en gyldig animasjonsbekreftelse; Unreal-adapteren må håndtere en opptatt døråpning før den begynner å lukke.

## 7. Dørlyd og hørselsstimulus er adskilte ting

Døren bruker `PlaySoundAtLocation` og `SpawnSoundAttached`. Det finnes ingen direkte `ReportNoiseEvent`-import i den undersøkte dørpakken. Spillerskritt har derimot et eksplisitt slikt kall i BP_Player. En hørbar dørlyd skal derfor ikke automatisk omtales som en bekreftet hendelse som alle aliens reagerer på. Eventuell rapportering fra andre systemer er ikke utelukket.

Et annet konkret funn er søkelyden i skuret. En dørrelatert hageskur-bane henter `Sound_Object_03_Searching_Shed_BP` og kaller stoppfunksjonen ved 8837. I lydobjektet fader `ShedSearching Stop` lyden ut over fem sekunder ved 209. Dette er en lydhendelse; det beviser ikke at en simulert alien går ut av skuret.

For Signal 47 bør en tilsiktet lydlokk sende én registrert stimulus med sted og årsak. Atmosfærelyd kan fortsatt være regi, men må ikke presenteres som pålitelig posisjonsinformasjon om fienden hvis den ikke kommer fra den.

## 8. Lagring gjenoppretter verdenstilstander, ikke en hel dørtransaksjon

I `Game_Level.SaveGame` finnes blant annet:

| Adresse | Lagret verdi |
|---:|---|
| 1113 og 1176 | Spillerens kontroll- og gangeflagg. |
| 2542 og 3161 | Alien-transformer og AI-aktivitet. |
| 4608, 5227 og 5850 | Alien-synlighet, død og helse. |
| 19722 og 21828 | Dørenes lukket- og låstflagg. |
| 22541 | Ett bestemt dørtilfelles behov for verktøybrudd. |
| 28731 | Et bestemt triggerflagg for dørrasling. |

Alien-arrayene har 19 plasser med en eksplisitt tom plass på indeks 9. 18 faktiske instanser føres tilbake i de andre plassene. Dør-arrayene har 66 oppføringer i en fast, eksplisitt referanserekkefølge. Det er en konkret lagringslayout, ikke et generelt oppslag på stabile dør-ID-er.

I den undersøkte SaveGame-funksjonen er det ikke funnet serialisering av `LAST_SEEN_PLAYER_POSITION`, `IS_CHASING_?`, `CAN_SEE_PLAYER_?`, `ALIEN_IS_BREAKING_?`, `IS_CURRENTLY_BREAKING_?`, `BREAK_COUNT` eller aktiv timeline-posisjon.

Dette beviser ikke at spillet lover å lagre midt i disse tilstandene. Checkpoints kan være plassert for å unngå dem. En mulig feil ved fri lagring under en verktøyinteraksjon er at kontrollflagg bevares uten den latente sekvensen som setter kontrollen tilbake. Det må prøves før det kalles en observert feil.

I LoadGame gjenopprettes dørflagg ved 35158 til 42828 og låser ved 42946 til 50616. Dørens egen BeginPlay planlegger separat `CheckIfDoorIsClosed` etter 0,05 sekund; den velger en timeline for visuell stilling. LoadGame-funksjonen er synkron i dette uttrekket. Det er ikke grunnlag for å erklære en faktisk oppstartsrace bare fordi systemene er adskilt.

## 9. En bekreftet kontrollflytsvakhet ved checkpoint

`Game_Level.SaveGame` lagrer returverdien fra `SaveGameToSlot` ved 30904. Den neste toppnivåinstruksjonen ved 30947 kaller `PlayCheckpointInfo`, uten å teste bool-resultatet.

Det innebærer at denne banen kan vise checkpointmeldingen også når lagringskallet returnerer false. Ingen diskfeil er framprovosert i Greyhill. Dette er et utsagn om koden, ikke om hvor ofte en feil forekommer.

Vår egen kode skiller utfallet eksplisitt:

```cpp
SaveNotice NoticeAfterDiskWrite(bool succeeded) {
    return succeeded ? SaveNotice::CheckpointSaved
                     : SaveNotice::SaveFailed;
}
```

Unreal-integrasjon må i tillegg vente på faktisk avslutning av asynkron skriving, hvis den brukes. En bestilt lagring er ikke en ferdig lagring.

## 10. Selvstendig C++-modell for Signal 47

[DoorPolicy.hpp](Research/Greyhill/DoorStudy/DoorPolicy.hpp) er en liten, engine-uavhengig modell. Den gjenskaper ikke Greyhills globale oppslag og tidsgrener. Den uttrykker ønsket kontrakt for en egen løsning:

- `Closed`, `Warning`, `Opening`, `Open` og `Closing` er egne tilstander.
- Én aktør eier en pågående passering. Duplikatforespørsler spiller ikke varselet om igjen og starter ikke nedtellingen på nytt.
- En låst dør trenger uttrykkelig tillatelse fra scenen for å forseres.
- En animasjon må bekrefte fysisk endepunkt før passasjen blir tilgjengelig.
- En callback må passe både instansens epoch og gjeldende generasjon. Gammel animasjon kan ikke fullføre en ny passering eller en annen instans med samme dør-ID.
- Avbrudd og timeout beholder passasjen blokkert. Restore av et lagret åpent endepunkt gjør modellen logisk åpen med en gang; hostens separate restore-sperre må holde aktører igjen til mesh og kollisjon er satt.
- Et lagringsøyeblikk midt i en bevegelse normaliseres bevisst til lukket dør ved lasting. Det er en valgt checkpoint-policy, ikke nøyaktig fortsettelse av animasjonen.

Eksempel fra modellen:

```cpp
Outcome RequestIntrusion(std::uint64_t actor, bool intentStillValid,
                         bool mayForce = false);
void Advance(double dt, bool paused = false);
bool MotionComplete(Ticket ticket, bool paused = false);
SavedDoor Capture() const;
void Restore(const SavedDoor& saved);
```

Adapteren skal beholde og gjenbruke hendelsens faktiske ticket. Den må ikke ved callbacktid slå opp en ny ticket for å få en foreldet animasjon godkjent. Gyldighet mot destruerte Unreal-aktører krever også en svak objekt-/komponentreferanse. Modellens epoch er prosesslokal, ikke en varig lagrings-ID.

Pause fryser `Advance`, og `MotionComplete(..., true)` endrer ikke tilstand. Hosten må fryse animasjonen eller beholde completion-hendelsen til pause oppheves. Restore spiller ikke lyder på nytt; adapteren setter mesh og kollisjon direkte mens spilleren fortsatt holdes igjen.

Prøveverdiene 1,2 sekund varsel, 3 sekunder forsering og 4 sekunder bevegelsestimeout er våre illustrasjonsverdier. De er ikke Greyhill-funn eller målt Signal 47-balansering.

## 11. Hva som mangler før Unreal-integrasjon

Referansemodellen endrer ingen actor-fart og eier ikke spillerposisjon. Den vet ikke hvor fienden ser eller hører. Jaktobjektet må gi et gyldig lokalt motiv og trekke det tilbake når kontakten er brutt eller ruten avbrytes.

En Unreal-adapter må knytte dør-ID til lagringssystemet, verifisere objektgyldighet, styre mesh/animasjon, vedlikeholde kollisjon og navigasjonslenke, teste om åpningen er opptatt og sende eventuell lydstimulus gjennom prosjektets eksisterende lyd-/persepsjonsgrensesnitt. Et blend- eller montage-notify alene er ikke nok som fysisk garanti uten denne koblingen.

Ingen adapter, ny fiende, kollisjonskanal, animasjon eller gameplay-modul er lagt inn i Signal 47. De fire jaktsekvensene, kuttet huking, bevarte originaler og ingen tidstap ved fangst i nye saker er fortsatt føringer.

## 12. Verifisering og presis teststatus

| Kontroll | Status |
|---|---|
| 77 adressehenvisninger mot serialiserte posisjonsindekser | PASS |
| Størrelsessamsvar for de undersøkte funksjonene | PASS |
| Første uttrekks byteidentiske rekonstruksjon av 111 pakker | PASS, tidligere utført |
| C++17-kompilering og linking, MSVC 14.44, `/W4 /WX` | PASS, exit code 0 |
| 16 skrevne testscenarioer | Kode følger med; 0 kjørt |
| Kjøring av native testprogram | UNVERIFIED: Windows svarte «Ingen tilgang» ved startforsøket |
| Unreal, ekte input, navigasjon, geometri, pause og disk-I/O | UNVERIFIED, ikke kjørt |

Testkoden dekker eierskap, duplikater, lås, avbrudd, timeout, pause, callback-identitet mellom instanser, stabil restore og melding ved mislykket skriving. Den ligger i [DoorPolicyTests.cpp](Research/Greyhill/DoorStudy/DoorPolicyTests.cpp). Koden er kompilert, men det er ingen påstand om at scenarioene består i kjøring. Windows-blokkeringen ble ikke forsøkt omgått.

Den lesende gjennomgangen av lagring og modellen ble gjort med en separat hjelper. Den avdekket to grenser som er tatt inn i siste kode: instans-epoch i tickets og eksplisitt pauseargument for completion. Den lokale instruksjonsvisningen er også rettet slik at entry-etiketter bare vises i den funksjonen de faktisk peker til. Bytekoden og adressene er uendret.

## 13. Anbefalt bruk i de avtalte scenene

SC04 bør være den første praktiske prøven: fienden når annexdøren, spilleren hører et varsel og får tid til å velge omvei. En registrert lydkilde lokker fienden til lydstedet. Lukket dør bryter sikt etter faktisk geometri; søkeminnet flytter seg ikke automatisk til spillerens nåværende posisjon.

SC11 kan bruke samme kontrakt med andre ruter og tidsbudsjett. Oppgaven og bevisene bevares ved fangst og lasting. Tidsstyrte dørhendelser som var påbegynt før en omstart får ikke påvirke en ny jakt. Ingen separat nøkkel-, batteri- eller våpenøkonomi trengs for å få denne løkken til å fungere.
