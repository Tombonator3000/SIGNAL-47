# Voices of the Void som referanse for SIGNAL / 47

Voices of the Void er en nyttig referanse for fysisk instrumentarbeid, rom med tydelige funksjoner og uro som oppstår i en kjent arbeidsrutine. Den største verdien for SIGNAL / 47 ligger i hvordan disse delene kan støtte etterforskningen. Et omfattende system for overlevelse, innkjøp og tilfeldig innhold vil derimot øke produksjonsbyrden og svekke rammen på 5–6 timer.

Det ble ikke funnet verifisert, offentlig kildekode til selve spillet. Det finnes offentlig kode for modding, kart og Blender-import. To konkrete kandidater har tydelige gjenbrukslisenser: **VotvIO under MIT** og **VotV Pixelated under SIL Open Font License 1.1**. Fontens Regular-fil mangler imidlertid norske bokstaver. Ingen av kandidatene er importert i SIGNAL / 47. [VotvIO](https://github.com/pelmentor/VotvIO), [fontprosjektet](https://github.com/TeradaSeqo/Font-Votv-Pixelated).

## Grunnlag og avgrensning

Vurderingen gjelder offentlige kilder tilgjengelige 12. september 2026. Itch-siden beskriver et signalspill ved et isolert laboratorium i sveitsiske fjell, med over 45 dager og over 200 signaler. Det offisielle arkivet viser **0.9.0n**, datert 16. mai 2026, som siste stabile utgave. Utviklerloggen for denne versjonen er publisert 19. mai. FAQ-en oppgir **Unreal Engine 4.27.2**. Dette er en annen motor og et betydelig større innholdsmål enn SIGNAL / 47. [Spillsiden](https://mrdrnose.itch.io/votv), [arkivet](https://archive.votv.dev/games/votv/), [FAQ](https://votv.dev/faq).

Dekningen omfatter spillsiden, alle tolv bilder i utviklerens galleri, det offisielle arkivet, FAQ og tilgjengelig legal-side, samt 31 offentlige utviklerposter. Postene er indeksert og søkt etter relevante emner; utvalgte logger, særlig 0.9.0n, 0.9.0, 0.8.0 og 0.6.2, er nærmere gjennomgått. Sju kode-/fontprosjekter har versjonsfestet register. Moddokumentasjon og ytterligere GitHub-treff er kontrollert separat. Registeret over utviklerposter gjelder selve innleggene, ikke kommentarfeltene.

Dette er en kilde- og referansevurdering, ikke egen gjennomspilling eller ytelsesmåling av VotV. Private Discord-kanaler, Patreon-innhold og alle skjulte spillhendelser er ikke kartlagt. Utviklerens humoristiske og uklare changelog-punkter behandles ikke som en fullstendig funksjonsspesifikasjon. Spillarkiver er ikke lastet ned eller dekompilert. Ingen antakelse om gjenbrukstillatelse bygger på at en fil kan lastes ned gratis.

SIGNAL / 47-grunnlaget er hovedgrenen på `f476b1ec884030e67971948121eb32ef05ec399d`, etter fletting av PR12. [Designbibel 13](../../DesignBible13/design-bible.md) setter arbeidsrammen: tre steder, seks kapitler og epilog, 18 hovedoppgaver og to avslutninger. 330 minutter er et planmål, ikke målt spilletid. [Gjeldende overlevering](../../CURRENT_HANDOFF.md) skiller spillutgave, testkandidater og åpne tester. Anbefalingene nedenfor er forslag; de endrer ikke kanon eller implementert oppførsel.

## Spilldesign og historie

### Arbeid som skaper forståelse

VotVs offentlige beskrivelse knytter mottak og bearbeiding av signaler til spillerens daglige arbeid. 0.9.0-loggen beskriver også fysiske PC-deler og oppgraderingsmoduler. Den overførbare ideen er at apparatets deler og spillerens handlinger gjør prosessen begripelig. [Spillsiden](https://mrdrnose.itch.io/votv), [0.9.0](https://mrdrnose.itch.io/votv/devlog/1474386/voices-of-the-void-alpha-090).

**Anbefaling for SIGNAL / 47:** La en kort arbeidskjede ende i et bevis som kan brukes senere: kalibrere, observere, registrere, sammenligne og trekke en avgrenset slutning. Vi har allerede signalinnstilling, utskrift, fysisk fotografering og fremkalling. Neste investering bør gjøre sammenhengen mellom disse tydeligere. Et nytt minispill bør bare innføres dersom det gir en ny type undersøkelse.

Driftsfeil kan gi en naturlig grunn til å forlate et trygt bord. I vårt format bør hver nødvendig reparasjon enten lære spilleren noe eller avdekke et spor. En sikring som må skiftes mange ganger, uten ny informasjon, bruker av historiens tidsbudsjett. Én eksisterende servicehandling kan i stedet kobles til en protokoll som senere viser seg å være korrigert. Dette er et forslag til kobling av eksisterende materiale, ikke en ny obligatorisk oppgave.

### Normaltilstand før avvik

VotVs logger skiller mellom signaler, hendelser, tilfeldige hendelser og mindre hendelser. Det dokumenterer flere innholdskategorier, men gir ikke innsyn i hele spillets interne hendelsesalgoritme. [0.8.0](https://mrdrnose.itch.io/votv/devlog/784476/voices-of-the-void-pre-alpha-080).

**Anbefaling:** Et avvik i SIGNAL / 47 bør være forståelig gjennom noe spilleren først har lært fungerer normalt. Behold en rolig kontrollmåling, et kjent instrument og en pålitelig original. La deretter én observasjon bryte forventningen. Avviket må ikke samtidig endre tid, rom, tekst og lyd på vilkårlige måter; det ville gjøre undersøkelsen umulig å resonnere om.

Prologens faste 47 sekunder og B-12s aktive/passive sammenligning er allerede slike holdepunkter. De skal bevares. Små stemningshendelser kan legges mellom oppgaver, men nødvendig bevisføring bør være forfattet og garantert tilgjengelig. En tilfeldig trekning skal ikke bestemme om spilleren forstår slutten.

### Historie gjennom gjenstander

Original og korrigert protokoll er en sterkere retning for dette prosjektet enn en stor samling løsrevne skrekknotater. Hvert dokument bør ha en avsender, dato, praktisk funksjon og en bestemt opplysning spilleren kan kontrollere. Valgfritt materiale kan gi personlighet og arbeidsliv, mens hovedslutningen må kunne støttes av den nødvendige beviskjeden.

Den foreslåtte Nora/Tomás-historien bør bruke det samme prinsippet: tidligere handlinger rekonstrueres gjennom instrumenter, originalfoto og uavhengige tidsmerker. En urovekkende stemme er en observasjon, ikke automatisk en sann forklaring. Denne retningen følger vår egen designbibel; den er ikke hentet fra eller en påstand om VotVs komplette historie.

## Visuell retning og Blender

Utviklergalleriet viser blant annet separate instrumentseksjoner i kontrollrommet, antenner med lesbare bærende konstruksjoner, tekniske skap og rør, opplyste ganglinjer og vinduer som holder anlegget synlig fra innsiden. Dette er observasjoner av publiserte spillbilder, ikke nye konseptbilder eller SIGNAL / 47-skjermbilder. Alle tolv bildeadresser er bevart i [galleriregisteret](gallery.json). [Originalgalleri](https://mrdrnose.itch.io/votv).

**Vår tilpasning:** SARO skal oppleves som en arbeidsplass i New Mexico i 1986. Bruk solbleket maling, slitte kontaktflater, støv langs kanter, serviceetiketter og et lite antall personlige spor. Prioriter ordnet bruksslitasje ved de viktige instrumentene. Sveitsisk skog, VotVs planløsning, særegne figurer og rekvisitter inngår ikke i denne retningen.

| Modellfamilie | Konkret neste forbedring | Aksept ved spillkamera |
|---|---|---|
| CRT og instrumentpanel | Tydelig ramme, skjermdybde, separate betjeningsflater og ventilasjon | Hovedavlesning og aktiv kontroll kan skilles uten å gå inn i modellen |
| Tastatur og brytere | Lesbar silhuett, noen taktile nøkkelkontroller, tilpasset normaler/materiale | Små detaljer flimrer ikke; tastaturet ligger faktisk på bordet |
| Arbeidsstol | Troverdig sokkel, sete og rygg, måltilpasset arbeidsplassen | Fri sikt og bevegelse mellom spiller, bord og panel |
| Arkivbord og mappe | Bruk Archive17-familien; kontroller papirtykkelse, mappeledd og kontaktflater | Original og korrigert dokument kan undersøkes uten overlapp |
| Tekniske skap | Synlig formål gjennom håndtak, måler, etikett og kabelføring | Interaktiv del fremstår tydeligere enn dekorasjonen |
| Uteanlegg | Antennesilhuett, fundament og enkle forbindelser mellom funksjonelle deler | Landemerket gjenkjennes fra faktisk rute og mørkeavstand |

Dette er en prioritert modelliste, ikke en bestilling på alle objektene samtidig. Først forbedres den arbeidsplassen spilleren bruker lengst. Bevar redigerbare Blender-filer, materialfamilier, korrekt skala, pivoter og eksportkontroller. Flere polygoner alene er ikke et kvalitetsmål.

Magnific kan senere være nyttig for originale overflatereferanser eller et egnet organisk objekt dersom tilgjengelig funksjon og resultat passer behovet. Presise hengsler, papir, tastatur og instrumentmål er gode kandidater for direkte Blender-arbeid. Genererte modeller trenger fortsatt opprydding, UV-er, materialkontroll, collider og Unity-inspeksjon. Ingen Magnific-generering eller tjenesteverifikasjon inngår i denne vurderingen.

## Kode og arkitektur som kan overføres

### Datadefinisjoner og faktisk oppførsel

Fusion-dokumentasjonen viser tabeller for blant annet signaler og gjenstander. Den understreker også at en ny rad ikke automatisk implementerer en funksjon: en matoppføring gjør ikke alene et objekt spiselig, og en tastoppføring tilfører ikke handlingen. [Modifisering av DataTables](https://questwalker.github.io/votv-modding-wiki/docs/blueprint-modding/using-fusion/modifying-datatables/).

I SIGNAL / 47 er [SignalProfile.cs](../../../Unity/Assets/Signal47/Runtime/Signals/SignalProfile.cs) allerede en `ScriptableObject` med målverdier, toleranser og beskrivelse. Utvid denne eksisterende løsningen når nye signaloppgaver faktisk krever det. Unngå å opprette en parallell innholdsplattform for de samme dataene.

For flere dokumentoppgaver er en mulig senere oppdeling:

| Del | Ansvar | Skal ikke inneholde |
|---|---|---|
| Bevisdefinisjon | Stabil ID, tittel, tekst og relasjon til original/korrigering | Spillerens nåværende progresjon |
| Bevistilstand | Funnet, undersøkt, sammenlignet og støttet slutning | En kopi av originalbildet som kan overskrives av UI |
| Hendelsesdefinisjon | Forutsetninger, berørt område og tillatt virkning | Referanser til objekter fra en tidligere scene |
| Hendelsestilstand | Utløst/fullført, eventuelt lagret variant | Ny tilfeldig trekning hver gang saken lastes |

Dette er et designforslag. Dagens [Notebook](../../../Unity/Assets/Signal47/Runtime/Investigation/Notebook.cs), [FieldCamera](../../../Unity/Assets/Signal47/Runtime/Investigation/FieldCamera.cs) og kapitteltilstand skal kartlegges før refaktorering. Splitt bare når neste integrerte oppgave demonstrerer et konkret behov.

### Hendelser som tåler avbrudd

VoidMod beskriver tilpassede historie-/tilfeldige hendelser, tidsbetingelser og variabler per lagring. Dette er fellesskapets modrammeverk, ikke en offentlig spesifikasjon av VotVs interne kode. [VoidMod](https://thunderstore.io/c/voices-of-the-void/p/Gatohost/VoidMod/).

**Forslag til vår kontrakt:** En mindre arkivhendelse får først lov til å starte når nødvendig bevis er registrert, dokumentvisningen er lukket, spilleren er i riktig område og ingen annen sekvens eller lagring eier overgangen. Markering og konsekvens må tåle gjentatt kall. Ved gjenoppretting settes den fullførte tilstanden uten at lyd, utskrift eller belønning spilles en gang til. Den eksisterende [PrologueDirector](../../../Unity/Assets/Signal47/Runtime/Events/PrologueDirector.cs) har allerede fast tidsforløp og egen gjenoppretting; den erstattes ikke av denne foreslåtte mekanismen.

`libvotv` undersøker en annen motorutfordring: et objekt på en adresse er ikke nødvendigvis det samme objektet som tidligere lå der. Den inspiserte `ObjectLifetimeTracker.hpp` skiller identitet fra gjenbrukt plass. For Unity er den relevante lærdommen å rydde sceneabonnementer og bruke stabile bevis-ID-er, ikke å kopiere Unreal-spesifikke minneteknikker. Ingen eksplisitt lisens ble funnet for dette biblioteket. [Kilden](https://github.com/modestimpala/libvotv/blob/04bc63b3f98ba32edb064342b034c74d24fe2c81/include/ObjectLifetimeTracker.hpp).

### Lagring, lyd og ytelse

[FieldCamera](../../../Unity/Assets/Signal47/Runtime/Investigation/FieldCamera.cs) har allerede en `SaveReady`-port som krever ferdig eksport. Menu14 og Recovery15 har egne kontroller for avbrytelse, arkivering og tidligere saker. Neste oppgave er å bevare disse kontraktene ved integrasjon av arkivet. Nytt lagringsformat eller en generell skyplattform følger ikke av denne referansen.

[SignalConsole](../../../Unity/Assets/Signal47/Runtime/Signals/SignalConsole.cs) oppdaterer spekteret med 0,1 sekunds intervall. Begrenset skjermoppdatering er dermed allerede innført. Fremtidig optimalisering bør måle faktisk flaskehals: skjermtegning, transparens, skygger, fysikk eller objekttall. VotVs FAQ på itch nevner volumetrisk lys som en mulig kostnad, men dette er ikke et benchmark for Unity-bygget vårt. [Spillsiden/FAQ](https://mrdrnose.itch.io/votv).

For lyd bør neste pass skille nødvendige signaler og tale fra generell romlyd. Nåværende `PlayerSettings` har hovedvolum; separate kategorier er et forslag, ikke allerede ferdige lydinnstillinger. Viktige avlesninger må også være tilgjengelige visuelt. Kontroller lydposisjon og demping etter last, og mål eventuelle topper i den faktiske spilleren før miks omtales som verifisert.

## Offentlig kode og materiale: gjenbruksregister

Lisensvurderingen gjelder de oppgitte filene og versjonene. Underliggende spillinnhold og avhengigheter kan ha egne vilkår. Fullstendige commit-ID-er er lagret i [repositories.json](repositories.json).

| Prosjekt | Faktisk innhold og lisensfunn | Verdi for SIGNAL / 47 |
|---|---|---|
| [VotvIO](https://github.com/pelmentor/VotvIO) | Blender-importør for VotV-data; MIT. README, NOTICE, manifest, koordinatkode og utvalgt scenebygging lest | God referanse for skala, transformasjoner, delte meshdata og importmanifest. Ingen direkte Unity-komponent |
| [VotV Pixelated](https://github.com/TeradaSeqo/Font-Votv-Pixelated) | OTF-fonter; OFL 1.1; reservert navn «VotV Pixelated» | Mulig avgrenset instrumentfont. Regular består ikke norsk tegnbehov |
| [votv-map](https://github.com/Questwalker/votv-map) | Uoffisielt nettbasert kart; GPLv3. `map_movement.js` bruker Leaflet, bildekart og egne koordinater | Mønster for internt oppgavekart med separate markører og ruter. Kopiert kode krever at gjeldende GPL-vilkår følges |
| [libvotv](https://github.com/modestimpala/libvotv) | C++-hjelpere rundt Unreal/UE4SS. Ingen eksplisitt lisens funnet | Lesereferanse for identitet og levetid. Ikke klarert for innbygging |
| [VotV-RE-UE4SS](https://github.com/modestimpala/VotV-RE-UE4SS) | Spilltilpasset modverktøy; MIT på rotnivå, separate avhengigheter | Nyttig hvis man skal lage en VotV-mod; liten direkte verdi for vårt Unity-spill |
| [VotV_ghostmapping](https://github.com/NynrahGhost/VotV_ghostmapping) | Eldre plassholderprosjekt; ingen eksplisitt lisens funnet | Viser modreferanser, ikke original spillimplementasjon. Nyere wiki peker til en annen fork |
| [VotVChaosMod](https://github.com/modestimpala/VotVChaosMod) | Twitch-styrte effekter; rotlisens oppgir CC BY 4.0. README og lisens vurdert | Lav prioritet. Enkle interne hendelsesknapper kan utvikles selv dersom testing trenger dem |

GPL forbyr ikke kommersiell bruk. Det avgjørende er hvilke deler som kopieres, kombineres og distribueres, og hvilke vilkår dette utløser. Her foreslås egen Unity-implementasjon av generelle kartprinsipper; ingen kartkode eller spillkart er innlemmet. For MIT-kode må relevant copyright- og lisensinformasjon følge med ved gjenbruk. Fontbruk må følge OFL, inkludert navnevilkår ved endring. [Kartlisens](https://github.com/Questwalker/votv-map/blob/0ff2102780755eb98d6d87ed1501d66c6ac46d53/LICENSE.md), [VotvIO NOTICE](https://github.com/pelmentor/VotvIO/blob/5cc1c6c7336c067d33aa3caaedb8241409f42611/NOTICE.md), [OFL](https://github.com/TeradaSeqo/Font-Votv-Pixelated/blob/223df554a1513a1b349705a245d883e1f82e1282/OFL.txt).

### VotvIO: den mest relevante verktøyreferansen

`convert.py` samler enheter og koordinatkonvertering på ett sted. `assemble.py` mellomlagrer meshdata og skriver et manifest med objektantall, advarsler og uløste klasser. Dette gir konkrete ideer til vår Blender-kvalitetskontroll. VotvIOs akser og enheter gjelder Unreal–Blender og skal ikke overføres blindt til Unity. Utvidelsen oppgir Blender 4.2 som minimum; kjøring er ikke testet her. [Koordinatkode](https://github.com/pelmentor/VotvIO/blob/5cc1c6c7336c067d33aa3caaedb8241409f42611/votvio/convert.py), [scenebygging](https://github.com/pelmentor/VotvIO/blob/5cc1c6c7336c067d33aa3caaedb8241409f42611/votvio/assemble.py).

En passende egen forbedring er å utvide eksisterende eksportbevis med objektets forventede mål, rotasjon, pivot, materialer og manglende deler. Test en asymmetrisk modell gjennom hele eksport-/importløpet; en kube alene avdekker ikke feil speiling. VotvIOs NOTICE presiserer at verktøylisensen ikke gir rettigheter til spillinnholdet det leser. Importøren er derfor ingen snarvei til VotV-modeller i SIGNAL / 47.

### Fonten: konkret kompatibilitetsfunn

Regular-OTF-filen ved den registrerte revisjonen ble kontrollert for tegnkart. Den mangler **ÆØÅæøå**, tankestreker, Unicode-minus, ellipsetegn og gradtegn; også enkelte ASCII-tegn som `$`, `&`, `@` og omvendt skråstrek mangler. Tall og de testede engelske bokstavene finnes. Resultatet og filens SHA-256 står i [font-audit.json](font-audit.json).

Behold eksisterende hovedfont. Et eventuelt senere forsøk med denne fonten bør begrenses til et definert sett instrumenttegn og få kontrollert fallback i Unity. Lisensmessig tilgjengelig betyr ikke automatisk egnet for vårt språk eller skjermavstand.

### Treff som ikke gir oss spillets kildekode

Ghostmappings er ifølge modwikien referanseobjekter uten den opprinnelige implementasjonen. Fusion og VoidMod er fellesskapsverktøy. Fusion-siden oppgir dessuten en pakke beregnet for `pa081_0008`, som ikke samsvarer med den nåværende stabile 0.9.0n. [Ghostmappings](https://questwalker.github.io/votv-modding-wiki/docs/blueprint-modding/using-ghostmappings/), [Fusion-versjon](https://thunderstore.io/c/voices-of-the-void/p/NynrahGhost/Fusion/).

GitHub-treffet `MrDrNose/votv07_official` var tomt ved kontroll. `Voices-Of-The-Void-by-Mrdrnose/.github` inneholdt en profilside og en lisens, ikke spillets motorprosjekt; en ekstern nedlastingslenke der ble ikke brukt. Navn som ligner utviklerens navn er ikke tilstrekkelig bekreftelse på offisiell kilde. Ingen generell gjenbrukslisens for VotVs modeller, lyd eller historie ble funnet på de undersøkte offisielle sidene. [Tomt treff](https://github.com/MrDrNose/votv07_official), [profilrepo](https://github.com/Voices-Of-The-Void-by-Mrdrnose/.github), [offisiell legal-side](https://votv.dev/legal/).

## UI, kart og avgrensning

For SIGNAL / 47 bør fysisk instrument-UI støtte atmosfæren, mens dokumentlesing og nødvendig hjelpetekst må tåle skriftstørrelse og skjermformat. Pause, tilbake og lukk skal gi entydig kontroll tilbake til spilleren. Startmeny, checkpoint, gjenoppretting og originalfoto er eksisterende arbeid; neste pass bør ferdigstille sammenhengende brukerreise før nye dekorative menysystemer.

Kart bør ha to formål: et lesbart kart for spilleren og et internt kart for produksjonen. Det interne kartet kan vise innganger, bevis, nødvendige forutsetninger og returruter i separate lag. Bruk designbibelens eksisterende kart som utgangspunkt. Et komplett nettbasert kartverktøy blir først aktuelt dersom statiske kart ikke lenger er tilstrekkelige.

| Prioritet | Ta videre | Avgrensning |
|---|---|---|
| Nå | CRT/tastatur/stol, dokumentlesbarhet og kontroll over tilbake/lukk | Eksisterende rom og arbeidsflyt |
| Nå | Lagre-/lastetester og fysiske objektkontroller | Utvid berørte kontroller; bevar originalfoto |
| Neste | Sammenhengende signal–arkiv-beviskjede | Gjenbruk P04; test forståelse før flere oppgaver |
| Neste | Lydhierarki og en kort rolig overgang | Ingen obligatorisk subtil lydledetråd |
| Senere | Datadrevne hendelsesdefinisjoner og kartlag | Bare når flere faktiske oppgaver trenger det |
| Utelat fra nåværende ramme | Sult/søvnøkonomi, omfattende crafting, butikkgrind, full fysikk på alt og kjøretøysimulering | Bruker produksjonstid uten nødvendig bidrag til beviskjeden |
| Utelat fra nåværende ramme | Co-op, Twitch-integrasjon, mod-SDK og fri import av nettvideo | Store nye tekniske og innholdsmessige forpliktelser |

Den offisielle VotV-FAQ-en beskriver selv spillet som bygget for én spiller og oppgir at flerspiller ville kreve omfattende omarbeiding. Fellesskapets forsøk på modding endrer ikke den avgrensningen. For SIGNAL / 47 støtter dette prioriteringen av én fullstendig spillerreise. [Offisiell FAQ](https://votv.dev/faq).

## Regresjonstester vi kan lære av

0.9.0n-loggen omtaler rettelser knyttet til dobbel bruk, lang tekst, tap av objektdata ved lasting, medieinnsetting, lydtilstand, tutorial og stolens plassering. 0.8.0 omtaler blant annet menyoverganger og lagrede tegninger; 0.6.2 har eksempler på festede objekter og handlingsmenyer som havner utenfor skjermen. Dette er historiske feilklasser, ikke påstander om at de fortsatt finnes i VotV. [0.9.0n](https://mrdrnose.itch.io/votv/devlog/1527871/voices-of-the-void-alpha-090-n), [0.8.0](https://mrdrnose.itch.io/votv/devlog/784476/voices-of-the-void-pre-alpha-080), [0.6.2](https://mrdrnose.itch.io/votv/devlog/588165/voices-of-the-void-demo-062-de-source-fied-pt1-random-bs-fishing-update).

Følgende er **foreslåtte aksepttester for SIGNAL / 47**, ikke nye beståtte tester:

| ID | Egen testhandling | Forventet resultat |
|---|---|---|
| V01 | Aktiver utskrift eller slutningsknapp raskt to ganger | Én overgang og ett bevis; ingen duplikat |
| V02 | Åpne dokument, pause, lukk og gjenoppta | Riktig fokus og bevegelseskontroll; ingen fastlåst visning |
| V03 | Vis lengste dokument med norsk tegnsett og større tekst | Alt nødvendig innhold er tilgjengelig; knappene kan nås |
| V04 | Last checkpoint etter avsluttet sekvens | Konsekvensen består uten nytt telefonanrop eller ny utskrift |
| V05 | Forsøk checkpoint mens fotoeksport pågår | Ingen sak som peker på et uferdig originalfoto |
| V06 | Avbryt gjenoppretting eller simuler kopieringsfeil | Gjeldende sak og originaler er bevart; tydelig tilbakemelding |
| V07 | Åpne og lukk mappe, last støttet tilstand og undersøk bordet | Korrekt posisjon, skala, materialer og dokumentidentitet |
| V08 | Bruk alternative tillatte input på samme bevis | Samme dokumenterte handling; ingen utilsiktet sletting |
| V09 | Last en innendørs sak og gå ut/inn | Lydkilder og romdemping samsvarer med stedet |
| V10 | Bruk CRT fra nærmeste tillatte posisjon ved stolen | Fri sikt, lesbar avlesning og tilgjengelig interaksjon |
| V11 | Start/avbryt sceneovergang mens en effekt er aktiv | Ingen etterlatte callbacks, lydkilder eller låst kontroll |
| V12 | Mål meny og travleste arbeidsplass i pakket spiller | Frame-time og allokeringer dokumentert for samme bygg og maskin |

Berørte eksisterende tester skal gjenbrukes der de allerede dekker utfallet. Native input, subjektiv lydvurdering og målinger for nye kandidater krever fortsatt faktisk kjøring. Kildelesing og skjermbilder erstatter ikke disse portene.

## Anbefalt neste produksjonssteg

Den mest nyttige neste leveransen er **én sammenhengende undersøkelse fra instrumentbord til arkivbevis**, med den eksisterende modellfamilien. Et foreslått testvindu er 15–20 minutter; dette er en avgrensning for prototypen, ikke et nytt løfte om kapittellengde.

1. **Bevisforståelse:** Fullfør P04/P05-prøven med en ny leser. Spilleren skal kunne forklare hva korreksjonen endrer, og hva dokumentene ikke beviser. Egen gjennomgang teller ikke som blindtest.
2. **Arbeidsplass:** Gjør ett samlet Blender-pass på CRT, tastatur og stol. Kontroller silhuett, mål, sikt og betjeningsavstand i Unity før flere modeller produseres.
3. **Integrasjon:** Koble arkivprøven til den etablerte ruten når bevis- og lagringskontrakten er avklart. Én inngang, én støttet slutning og en tydelig retur er tilstrekkelig.
4. **Stemning:** Prøv én kort, ikke-obligatorisk overgang etter at dokumentet er lukket. Bevar tidsreglene og nødvendig bevisføring. Fjern hendelsen hvis den forstyrrer forståelsen.
5. **Verifikasjon:** Kjør relevante V01–V12-kontroller, inspiser samme pakkede spillutgave og gjennomfør brukerreisen når skjermen er tilgjengelig. Oppdater eksisterende overlevering med faktisk kilde-, bygg- og teststatus.

Arbeid på modeller og lesbarhet kan fortsette mens blindtesten er åpen. Utvidelse av historien som avhenger av en uforstått slutning bør vente på resultatet. Først etter en vellykket helhet bør flere rom og oppgaver produseres etter samme mønster.

## Kilderegister og etterprøvbarhet

Alle nettlenker i rapporten ble vurdert 12. september 2026. For GitHub er undersøkte revisjoner låst i registeret; lenker til spesifikk kode bruker disse revisjonene. Nettsider uten oppgitt publiseringsdato er datert gjennom tilgangsdatoen, ikke antatt forfatterdato.

| Register | Innhold |
|---|---|
| [devlog-index.json](devlog-index.json) | 31 innlegg fra mrdrnose: titler, direkte URL-er, ordantall, teksthash og emnesøk. En indeks, ikke fullstendige gjengivelser |
| [repositories.json](repositories.json) | Sju prosjekter: forfatter/repo, revisjon, siste push og GitHubs lisensmetadata. Manuel filvurdering står ovenfor |
| [gallery.json](gallery.json) | Tolv direkte bilder fra utviklerens itch-galleri. Referanser, ingen lisensierte SIGNAL-assets |
| [font-audit.json](font-audit.json) | Filidentitet, metode, tegnkart og manglende tegn i Regular-fonten |

Supplerende primærkilder er den [offisielle devlog-indeksen](https://mrdrnose.itch.io/votv/devlog), [Blender-verktøyets arkitekturnotat](https://github.com/pelmentor/VotvIO/blob/5cc1c6c7336c067d33aa3caaedb8241409f42611/docs/ARC.md), [modwikiens verktøyoversikt](https://questwalker.github.io/votv-modding-wiki/docs/useful-tools/) og [Blueprint-introduksjon](https://questwalker.github.io/votv-modding-wiki/docs/blueprint-modding/). Modwikien er en primærkilde for fellesskapets egne arbeidsmåter, ikke utviklerens offisielle SDK. Arkivets nedlastinger er byggepakker med versjon og kontrollsum; de er ikke i seg selv en kildekodeutgivelse.
