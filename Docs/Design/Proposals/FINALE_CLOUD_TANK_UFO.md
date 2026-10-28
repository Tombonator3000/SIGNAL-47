> **Status: Under arbeid** · Klassifisert 28.10.2026. Gjelder det dokumenterte omfanget; [gjeldende leveringsstatus](../../Active/STATUS.md).

# Finaleønske: gigantisk UFO bak voksende cloud tank-skyer

Dato: 28. oktober 2026. Bestilt av Tom i samtalen om cloud tank-effekten.

## Bestilling og status

**Ønsket innhold:** Et episk, langsomt øyeblikk i siste eller nest siste scenario der en gigantisk UFO kommer frem bak skyer som vokser, velter og ruller som en klassisk cloud tank-effekt. Plasseringen skal gi størst mulig virkning på spilleren.

**Anbefalt plassering:** Hovedavsløringen i **SC13: Siste vei mot Roswell**, ved det ufrivillige motorstoppet på **C 8.15**. SC12 kan få et kort, ufullstendig varsel i himmelen etter at varslingen og ruteplanleggingen er ferdig. Hele fartøyet skal ikke avsløres der.

Status: **ønsket og registrert for senere produksjon**. Plasseringen i SC13 er en dramaturgisk anbefaling innenfor Toms bestilling. Ingen ny effekt, modell, scenekode, lyd eller ferdig spillsekvens er levert. Utseende, spillerreaksjon og 60+ FPS er **UNVERIFIED**.

Dette dokumentet er et nytt regitillegg til [historierevisjonen](../Story/HISTORIE_SC02_SC13_REVISJON.md), [implementeringsplanen](../Story/HISTORIE_IMPLEMENTERINGSPLAN.md) og [hovedforløpet](../Story/MISSION_FLOW_1800_ROSWELL.md). Det erstatter ikke slutten eller innfører et nytt scenario. Avvik fra den tidligere regien er den tydeligere, fysiske fartøyavsløringen i SC13. Genererte manusfiler er ikke endret.

## Konseptbilder og presisert skyretning

Tre [konseptbilder av cloud tank-skyer](../../../unreal/Concepts/CloudTankUFO-2026-10-28/README.md) er bevart med prompter og kontrollsummer: SARO v1, korrigert SARO v2 og Caliche v1. Toms presisering er at to skybanker skal velte fremover fra hver sin side, med åpen himmel mellom. V2 er gjeldende referanse for skyretningen. Bildene er AI-generert visuell regi, ikke runtime eller målfast by-/arraylayout.

## Hvorfor SC13 gir sterkest effekt

SC12 er Mesa Diners trygge pusterom: kjente mennesker, kildekontroll, vellykket varsel til Ward og en begrunnet reise med originalene. En full UFO-avsløring der ville brukt opp forventningen før siste kjøretur og konkurrert med spillerens forståelse av hvorfor Reyes reiser.

I SC13 er bilen det siste trygge stedet. Når motoren stopper og en tilsynelatende skyfront viser seg å skjule noe enormt, brytes den tryggheten. Nattens utvikling fra ett lys til flere ankomster får en fysisk målestokk.

La fartøyavsløringen være det store ytre bildet. Den eksisterende slutten med tomt førersete, uttrekk og 1947-bildet med Reyes som tredje silhuett beholder siste emosjonelle slag. Ingen boss, luftkamp, alternativ seier eller sikker forklaring på hensikten tilføyes.

**Bevar faste kontrakter:**

- SC12s varsel lykkes før avreise. Originalene skal fortsatt til budet ved 10.4.
- C 8.15 er stedet reisen avbrytes; 10.4 nås ikke.
- THE EVENT forblir **05:29:00 til 05:29:47**. Kamerauttrekket starter deretter ved 05:29:47 og beholder eksisterende varighet på 34 sekunder. UFO-avsløringen legges inn parallelt med denne regien, uten å flytte faste overganger.
- 1420.405 MHz, 47 sekunder, 27 antenner og az 026 beholder sin betydning. Ingen ny tallkode eller sammenblanding med stjernehullet.
- Eksisterende tilfeldige bortføringer og himmelhendelser må ikke starte samtidig med finalen.
- Nye saker følger senere innholdsrevisjon. Gamle lagringer får ingen bakoverrettede krav.

## Regi og mulig tidsfordeling

Dette er et første timingforslag for prototype, ikke målt eller låst spilletid. Vurder hele finalen med lyd før fordelingen bestemmes.

| Fortellertid | Forslag til bilde og lyd |
|---|---|
| Før 05:29:00 | Under siste kjøretur vokser en mørk skyfront langsomt. Fjerne lys gir antydning uten å vise skroget. Ingen ekstra ventepost eller tvungen himmeloppgave. |
| 05:29:00 til 05:29:06 | Motorstopp ved C. Motorlyden faller bort. Skyene fortsetter å rulle med en mørk kjerne og svak glød bak. |
| 05:29:06 til 05:29:19 | En enorm buet kant kommer frem. Gløden vandrer langs skyene før selve formen blir lesbar. UFO-en beveger seg svært lite. |
| 05:29:19 til 05:29:27 | Mer av undersiden blir synlig. Hold bildet noen sekunder. Små lys på skroget og landskap/bil gir størrelsesreferanser. |
| 05:29:27 til 05:29:47 | Hold fartøyet delvis lesbart mens eksisterende THE EVENT fortsetter. Bevar nøkkel-/dørforsøk, radiostemmer og øvrige signaler; unngå å konkurrere med dem. |
| Fra 05:29:47, eksisterende 34 sekunders uttrekk | Tomt førersete og kamerauttrekk følger eksisterende regi. UFO/skyfront må ikke dekke avsluttende bildespråk eller svekke 1947-bildet. Ingen ekstra varighet legges til. |

Hvis denne fordelingen presser den eksisterende slutten, begynn skyoppbyggingen tidligere i SC13 og forkort den tydelige fartøyvisningen. Ikke forleng THE EVENT eller kamerauttrekket for å få plass til ekstra opptak.

Timinggrunnlaget er kontrollert i [S47Chapter6.cpp](../../../unreal/Signal47/Source/Signal47/Private/S47Chapter6.cpp): EVENT = 47, PULL = 34; StartPullout starter etter EVENT og registrerer 05:29:47. En tidligere generell idé om en frittstående 40-sekunders sekvens er derfor ikke en ny tidskontrakt.

Kameraet skal i utgangspunktet følge eksisterende spillerperspektiv og finaleregi. Bruk plassering, frontruteutsyn, lyd og lys til å gjøre avsløringen naturlig synlig. Unngå en ny brå kamerarykk eller ubegrunnet utstigning. Prøv også spillere som ser ned, ser bakover eller har annen FOV.

**Visuelt mål:** Tunge, sammenhengende skymasser med mørke kjerner, kalde side-/bakbelyste kanter og langsom intern rulling. Enkelte tynne skyslør passerer foran skroget og viser dybde. Formen skal gradvis forstås som et fartøy, ikke dukke frem ved en enkel opacity-fade.

**Lydidé:** La motorstopp og fravær av kjent lyd skape rom før en lav, langsomt voksende resonans. Unngå at et høyt lydstøt gjør avsløringen til et kort jumpscare. Endelig miks og menneskelig reaksjon må prøves.

## Anbefalt sanntidsløsning

Bruk en hybrid der den fine væskebevegelsen lages under produksjon og bakes til animasjonsteksturer. Spillet trenger da å spille og plassere animasjonene, ikke løse en stor fluid-simulering.

| Lag | Mulig gjennomføring |
|---|---|
| Fjern bakgrunn | Behold godkjent kveld-/nattehimmel. Legg lokal skyfront i finalesektoren. |
| Store cloud tank-former | Flere originale bakte flipbooks på plasserte kort eller Niagara-sprites, med ulike faser og varianter. |
| Skyer foran UFO-en | Egen gruppe med styrte baner som faktisk dekker og gradvis avdekker skroget. |
| UFO | Original, redigerbar modell med tydelig silhuett og få lesbare lys. Liten translasjon, stor opplevd masse. |
| Glød og luft | Styrt glød i skymaterialene, supplert med begrenset Volumetric Fog og få lys der det gir synlig gevinst. |

Niagara Baker kan fange et Niagara-system til flipbook. En egen fluid-simulering eller filmet, rettighetsklar cloud tank kan brukes som produksjonskilde. Bakt røyk er en visuell tilnærming, ikke en fysisk simulering av salt-/ferskvannsgrensen.

Ved større kamerabevegelse kan flate kort avsløres. Prøv da flere opptaksvinkler, sammensatte kryssende kort eller et lite lokalt volum i området nær skroget. Velg dette etter kamerabehov og måling, ikke som automatisk høyeste kvalitetsnivå.

## Materialoppskrift for sprite-varianten

Første prototype, med innstillinger kontrollert mot prosjektets Unreal-versjon:

1. Material Domain: **Surface**. Blend Mode: **Translucent**.
2. Shading Model: **Default Lit**. Start med **Volumetric Directional** som Translucency Lighting Mode og passende normaldata. Sammenlign med rimeligere, regissert lys i et Unlit-materiale hvis dynamisk lys ikke gir nok verdi.
3. Bruk opasitetsmaske fra egen sky-flipbook. Multipliser med partikkelalfa og en myk inn-/utfading.
4. Bruk **Depth Fade** for å myke opp skjæringer med opak geometri. Dette erstatter ikke forgrunnsskyenes tildekking eller en plan for translucency-sortering.
5. Skill mørk skybase, kantlys og lokal UFO-glød i egne parametere. Ikke lys opp hele skyfronten jevnt.
6. Interpoler mellom flipbook-ruter der oppsettet støtter det. Et 8 x 8-atlas har bare 64 bilder; unngå synlige trinn ved svært langsom avspilling. Flere klipp og forskjøvede faser reduserer gjentakelse.
7. Bruk cutout-geometri rundt skyformen når det passer, og begrens antall overlappende lag.

**Teknisk rettelse til AI-oversikten:** Volumetric Advanced er ikke en vanlig Shading Model eller Blend Mode for sprites. **Volumetric Advanced Material Output** er en materialnode for Volumetric Cloud-oppsett med **Volume**-domene. Surface ForwardShading er en alternativ lysbane for transparente overflater, ikke en generell innstilling for eldre Unreal-versjoner.

## Niagara og det kunstige høydelaget

Det meste av den indre rullingen skal ligge i flipbooken. Curl Noise flytter partikkelposisjoner; den skaper ikke i seg selv intern væskebevegelse i en sprite.

- Bruk et begrenset start-burst og få vedlikeholdspartikler fremfor høy, kontinuerlig spawn gjennom hele finalen.
- Plasser skygruppene i definerte soner. Bruk flere ulike størrelser og faser uten å gjøre hver partikkel heldekkende.
- I Unreal er 1500 til 3000 standardenheter omtrent 15 til 30 meter. Dimensjoner skyene ut fra fartøy, faktisk kamera og skjermdekning; disse tallene er ikke universelle.
- Definer et **LayerHeight**. Før skyene mykt mot dette høydelaget, samtidig som de ekspanderer horisontalt.
- Bare en Z-hastighetskurve over partikkelalder gir ikke et felles høydelag dersom startposisjonene varierer.
- Bruk svak Curl Noise med stor romlig skala. Ikke kopier styrke 300 til 500 eller høy frekvens som en garanti for cloud tank-utseende.
- Hvis en egen modul begrenser Z-bevegelsen, må modulrekkefølgen hindre at senere krefter gjeninnfører den uønskede hastigheten.
- Sett og kontroller bounds for hele den regisserte bevegelsen, så skyene ikke blir borte midt i avsløringen.

Mulige styringsparametere: **CloudGrowth**, **CloudSpread**, **LayerHeight**, **RevealAmount**, **GlowIntensity** og **SequencePhase**. Navnene er forslag til implementering, ikke eksisterende API-er.

## Lys og eksponering

Bruk et svakt side-/baklys til å lese skyvolumene og fartøyglød som bygger seg opp innenfra. Animer gjerne gløden direkte i materialet for presis regi og begrenset kostnad. Emissive glød er ikke automatisk fysisk belysning av alt rundt.

**Volumetric Scattering Intensity** styrer et lys sitt bidrag til **Volumetric Fog**. Vanlige transparente sprites tilfører ikke automatisk volumetrisk tetthet, og et punktlys gir ikke av seg selv fysiske stråler gjennom kortene. Dersom ekte stråler trengs, må tåkevolum og lysoppsett etableres og måles separat.

Ikke behandle 4 til 10 i scattering-intensitet som en fast oppskrift. Begynn lavt og vurder hele bildet. Begrens lysradius og volumetriske skygger. Hold eksponeringen kontrollert så den økende gløden ikke gjør skrog og skyer uleselige.

## Ytelse og kvalitetsnivåer

60 FPS gir **16,67 ms per bilde for hele spillet**. Et første, uprøvd arbeidsbudsjett er **1 til 2 ms ekstra GPU-tid for finale-effekten** på avtalt målmaskin og oppløsning. Dette er et mål, ikke en måling eller garanti. Registrer absolutt CPU-/GPU-tid, minne og hakk når effekten starter.

Få partikler kan fortsatt være dyre hvis de dekker hele skjermen i mange transparente lag. Prioriter skjermdekning, overdraw, materialkostnad og lys før antall partikler alene. Bruk profilegpu og relevante kompleksitetsvisninger.

| Nivå | Foreslått innhold |
|---|---|
| Low | Samme fartøysilhuett, timing og avsløring. Færre skyoverlapp, enklere materialer og hovedsakelig regissert glød. |
| High | Flere lokale skyvarianter og bedre lysrespons. Begrenset ekte tåke hvis målingen forsvarer den. |
| Produksjon | Detaljert simulering kan brukes til å lage flipbooks. Ikke likestill dette med runtime-kostnaden. |

Forhåndslast modell, teksturer og lyd før motorstoppet. Prøv varm og kald innlasting. Ikke løft hele himmelens eller tåkevolumets kvalitet permanent for ett kort øyeblikk.

## Produksjonsrekkefølge og aksept

- [ ] Lag en enkel SC13-prototype med faktisk frontruteutsyn, UFO-silhuett og eksisterende finale.
- [ ] Prøv plassering og timing mot SC12-pusterommet og 1947-bildet. Juster først etter sammenhengende spilling med lyd.
- [ ] Lag originale skyklipp og original UFO-modell, med redigerbare produksjonskilder og dokumentert lisens.
- [ ] Bygg materialer og Niagara-grupper. Koble avspilling til én eier i eksisterende historiekoordinator, eventuelt Sequencer for avgrenset regi.
- [ ] Versjoner fase og fullført-status. Pause fryser riktig klokke, Continue gjenskaper avtalt fase eller sikkert checkpoint, og sekvensen dobbelstarter ikke.
- [ ] Prøv ny sak, gammel sak, pause/meny, diskrestore, alternative blikkretninger/FOV og hele overgangen til sluttmenyen.
- [ ] Kontroller at skyene faktisk skjuler UFO-en, at sortering og bounds holder, og at Low bevarer avsløringens lesbarhet.
- [ ] Ta ekte runtime-bilder før/under/etter avsløringen og mål ytelse ved maksimal skjermdekning på High og Low.
- [ ] Spilltest om øyeblikket oppleves langsomt, enormt og truende, og om 1947-bildet fortsatt har siste virkning. Spillerpåvirkning kan ikke godkjennes ut fra et stillbilde.
- [ ] Kjør relevante scenarioprøver og påkrevd fullmatrise før en senere PR som endrer spillet. Før kode, pakket kandidat, installasjon og release separat.

## Referanser og kildeavgrensning

Toms fem vedlagte bilder og limte AI-oversikt er idé- og stemningsreferanser. AI-oversiktens instruksjoner er ikke prosjektkrav eller verifisert Unreal-dokumentasjon. Filmreferansen er cloud tank-utseendet i *Close Encounters of the Third Kind* (1977), ikke en bestilling om å kopiere fartøy, filmklipp eller lyd.

- [Referanseklippet fra samtalen, Close Encounters Cloud Tank Shots](https://www.youtube.com/watch?v=pYVybOyMz-A&t=62s). Brukes som visuell referanse; klippet er ikke teknisk verifisert her.
- [Epic: Niagara Baker](https://dev.epicgames.com/community/learning/tutorials/Gxxq/niagara)
- [Epic: Lit Translucency](https://dev.epicgames.com/documentation/unreal-engine/lit-translucency-in-unreal-engine)
- [Epic: Volumetric Cloud Component](https://dev.epicgames.com/documentation/unreal-engine/volumetric-cloud-component-in-unreal-engine)
- [Epic: Volumetric Fog](https://dev.epicgames.com/documentation/unreal-engine/volumetric-fog-in-unreal-engine)
- [Epic: Niagara Example Pack, Best Practices](https://dev.epicgames.com/community/learning/courses/qXe/unreal-engine-niagara-examples-pack/L7aY/unreal-engine-niagara-example-pack-best-practices)

Epic-kildene ble lest i den foregående samtalevurderingen. Endelig node-/moduloppsett må kontrolleres i prosjektets UE 5.8.4. Ingen filmstillbilder eller tredjepartsressurser legges i det offentlige repoet som del av dette designnotatet.
