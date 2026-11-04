# Freeform: saksløkken

**Status:** Under arbeid. **Gjelder:** Freeform-modusens hovedprogresjon ved SARO. **Sist kontrollert:** 4. november 2026.

Tom bestemte 3. november at gameplay-løkken er førsteprioritet foran grafikk og annet arbeid. Dette dokumentet er designet som Claude implementerer først, bygget på [gameplaygjennomgangen](../../Research/Gameplay/WORKFLOW_REVIEW.md) (GP-01 til GP-20 og Codex' sammenligning med Signal Simulator, Voices of the Void og Greyhill Incident) og FF-RESEARCH-00 til 10 i [TODO](../../Active/TODO.md). Implementert status føres i [STATUS](../../Active/STATUS.md); dette er et design, ikke bevis på ferdig funksjon.

## Kjernen i én setning

**Du driver et ekte observatorium for å betale regningene, men det du egentlig gjør er å teste forklaringer på det som ikke stemmer.**

Signal Simulator og VotV belønner kapasitet: bedre utstyr gir flere signaler og mer penger. Greyhill belønner overlevelse gjennom en forfattet trussel. SIGNAL / 47 skal belønne **resonnement**: spilleren lager en forutsigelse, gjør et fysisk forsøk og *hører eller ser* om forutsigelsen holdt. Det er spillets egenart.

## Tre lag

| Lag | Tid | Spilleren gjør | Status |
| --- | --- | --- | --- |
| Håndverk | Minutter | Peker antenner, stiller mottaker, tar opptak, henter original, lytter, arkiverer | Finnes |
| Skift | Én dag og natt | Planlegger natten, tar rutinejobber for penger, gjør ett kontrollforsøk, håndterer feil og hendelser | Nytt: dagsplan og kontrollforsøk |
| Sak | Flere døgn | Avviker forklaringer én etter én til saken kan konkluderes; konklusjonen åpner adgang og et nytt spørsmål | Nytt: saksbrett, hypoteser, belønning |

## Saksløkken

**Avvik → saken åpnes med tre mulige forklaringer → velg et forsøk med en tydelig forutsigelse → forbered (drivstoff, tidsvindu, deler) → gjennomfør forsøket ved instrumentene → resultatet utelukker eller beholder forklaringer → konkluder når forsøkene bærer det → adgang og neste spørsmål.**

Prinsipper:

1. **Forutsigelse før forsøk.** Hvert forsøk sier hva hver forklaring forventer: «Hvis matespenningen er årsaken, forsvinner ekstratikkene på aggregatstrøm.» Spilleren lærer vitenskapelig metode uten tekstvegg.
2. **Øyeblikket er hørbart.** Ekstratikkene spilles live under opptaket. Spilleren hører resultatet i det det skjer, før papiret bekrefter det.
3. **Ingen gjetting lønner seg.** En konklusjon godtas bare når de gjennomførte forsøkene har utelukket de andre forklaringene. Feil gjetning gir en forklaring på hva som mangler, aldri straff.
4. **Negative resultater er fremgang.** Et forsøk som utelukker en forklaring er like verdifullt som et som støtter en.
5. **Sannheten er fast per spill.** Hver sak har én skjult, frøbasert årsak. To spillere kan få forskjellige svar, så løsningen kan ikke slås opp, bare undersøkes.
6. **Penger og kunnskap er forskjellige valutaer.** Rutine betaler driften ($20 per jobb, uendret). Saksfremgang gir adgang som ikke kan kjøpes med flere rutinejobber.
7. **Forberedelse binder lagene sammen.** Strømforsøket krever en drivstoffkanne på aggregatet; gjentakelsen krever samme tidsvindu neste natt. Caliche-turen og vedlikeholdet får en grunn utover drift.

## Sak 1: «The repeated interval» (implementeres først)

Åpnes når et ekstrapulsavvik er gjennomgått eller registrert. Forklaringer og forsøk:

| Forsøk | Spilleren gjør | Matespenning | Lokal sender | Himmelkilde |
| --- | --- | --- | --- | --- |
| **Aggregatstrøm** | Start aggregatet (en drivstoffkanne) og ta hele opptaket på aggregatstrøm | Tikkene forsvinner | Tikkene blir | Tikkene blir |
| **Ved siden av kilden** | Pek 0,6 grader ved siden av pulsaren og ta opptaket | Tikkene blir | Tikkene blir | Tikkene forsvinner |
| **Gjenta i samme vindu** | Ta opptaket på en senere dag innen 90 minutter av det opprinnelige klokkeslettet | Tikkene blir | Tikkene forsvinner (sporadisk sender) | Tikkene blir |

To forsøk er alltid nok til å skille de tre forklaringene, men spilleren må velge hvilke. Hvert forsøk er en egen jobb i OPS-listen med egen arbeidsseddel, utført med de eksisterende apparatene. Forsøksjobben betaler som rutine når den registreres, så forskning koster tid og forberedelse, ikke inntekt.

Ugyldige forsøk er tydelige: stopper aggregatet under opptaket, eller er spilleren utenfor tidsvinduet, sier originalen det, og forsøket kan tas på nytt.

## Sak 2: «Disagreeing records» (implementert)

Åpnes når et tids-, duplikat- eller telexavvik er beholdt. Det målbare er mottakerens egen teller mot den uavhengige telleren (DELTA +3,2 s eller 0) på originalen.

| Forsøk | Spilleren gjør | Feil i tidsreléet | Nettspenningsstyrt klokke | Noen bruker stasjonen om natta |
| --- | --- | --- | --- | --- |
| **Bytt tidsreléet** | Ett relé fra depotet settes inn når opptaket starter og brukes opp | Avviket forsvinner | Avviket blir | Avviket blir |
| **Aggregatstrøm** | Hele opptaket på aggregatstrøm, frakoblet nettet | Avviket blir | Avviket forsvinner | Avviket blir |
| **Stengt stasjon** | Begge dørene stengt og spilleren i kontrollrommet under hele opptaket | Avviket blir | Avviket blir | Avviket forsvinner |

Svarene kommer fra et verksted, kraftlaget eller sheriffens nattlogg. Den siste forklaringen er menneskelig, ikke utenomjordisk, og gir saken uro uten å gjøre alle avvik til samme gåte. Motoren er datadrevet: hver sak definerer forklaringer, forsøk, forutsigelser, målbar størrelse og svar, slik at sak 3 kan legges til uten ny motorlogikk.

## Sak 3: «Visitors around the base» (saksmotor implementert, feltforsøk gjenstår)

Codex' beholdte feltobservasjoner (`FS47FreeformObservation`) blir spor på saksbrettet med dag, klokkeslett, sted, retning, hørt/sett og fotografi. Første spor åpner saken. Sporene er observasjoner, ikke støtte for én forklaring. Forklaringer og forsøk følger Codex' forslag:

| Forsøk | Spilleren gjør | Vind og løse beslag | Folk fra området | Uforklart besøk |
| --- | --- | --- | --- | --- |
| **Sikre takbeslaget** | Merk og fest platen, lytt fra samme punkt en lignende vindnatt | Impaktene stopper | Impaktene fortsetter | Impaktene fortsetter |
| **Kontrollert adkomst** | Forsegl serviceporten og legg sporstriper før samme vindu | Forsegling hel, ingen spor | Forsegling brutt, sammenhengende spor | Forsegling hel, ingen spor |
| **To samtidige målesteder** | Feltloggeren og takreferansen tar samtidige originaler | Ingen samtidig forstyrrelse | Ingen samtidig forstyrrelse | Samtidig forstyrrelse begge steder |

Det tredje forsøket krever feltloggeren, som lånes ut når sak 1 er konkludert. Slik binder sakene seg sammen.

**Kontrakt mellom saksmotor og verden (FieldWork):** forsøkene utføres i verden, ikke ved mottakeren.
- `Session->PlannedFieldTest(Line)` gir planlagt feltforsøk eller -1.
- `Session->FieldTestExpectsPresent(Line,Test)` gir hva den skjulte årsaken lar forsøket vise. Kallet svarer bare for det planlagte forsøket. Verden skal iscenesette svaret fysisk (lyd, spor, forsegling, loggerutslag), ikke bare som tekst.
- `Session->ReportFieldTest(Line,Test,bValid,bPresent,Evidence)` registrerer det spilleren faktisk sikret. Uteblitt vindnatt, batteritap, manglende original eller brutt oppsett rapporteres som `bValid=false` og gir ikke utelukkelse.

### Første fysiske feltforsøk: sikre takbeslaget

**Implementeringsgrunnlag kontrollert 4. november; forsøket er fortsatt ikke implementert.** `S47FreeformDirector.cpp` har lydankeret `roof_steps` ved (-1,5; 3,5; -1), spiller betongsteg og beholder en observasjon når spilleren er nær. Dette er ikke et eksisterende løst beslag eller bekreftet takadkomst. Et fysisk festepunkt må først modelleres/plasseres ved en tilgjengelig serviceadkomst eller takkant og kontrolleres i motoren. Dagens feltlogger måler elektrisk støy; opptaket er ikke automatisk bevis på samtidige taklyder.

Den eksisterende `FWind.Strength` i `S47Wind.h` er vegetasjonsutslag, ikke vindhastighet i m/s. Den følger prosessklokken via `S47WorldLife.cpp`, og vindfasen overlever ikke Fortsett. Dersom verdien brukes, må den beskrives som en spillintern vindindikator. Forsøket leser den eksisterende verdien; det skal ikke endre verdens vind for å få et gyldig resultat. Terskler og observasjonstid må balanseres og kontrolleres i motoren før levering.

Planlagt gjennomføring:

1. Spilleren velger takforsøket i OPS og finner et synlig beslag med referansemerke. Handling krever faktisk nærhet og fri sikt.
2. Før festing registreres et fullstendig observasjonsvindu ved referansen: varighet, presenterte/hørte slag og vindindikatorens middel og minimum. Manglende lyd er ikke negativt bevis uten full dekning.
3. Spilleren merker og fester beslaget fysisk. Modellen viser tilstandsendringen. Et nytt vindu ved samme referanse må ha sammenlignbare vindforhold.
4. Den skjulte årsaken styrer hendelsen i verden: festing stanser vindslag, mens de andre årsakene fortsatt kan gi observerbare slag. `FieldTestExpectsPresent` brukes til iscenesettelsen, aldri som direkte svar fra en resultatknapp.
5. Inspeksjon sikrer en original med plan-ID, dag/tid, referanse, før-/etterdekning, slag og vindgrunnlag. Først da kalles `ReportFieldTest(2,0,...)` én gang med det faktisk sikrede resultatet. Utilstrekkelig vind, fravær, søvn/tidsgap eller brutt oppsett gir ugyldig forsøk og ingen utelukkelse.

Lagre forsøksfasen, plan-ID, oppsettets integritet, dekningsvarighet, slag og vindstatistikk som egen struktur etter Directors eksisterende lagringsmønster. Fortsett skal ikke konstruere lyd eller observasjonstid i ettertid; et avbrutt sammenligningsgrunnlag krever ny referansemåling. Avbrytelse eller endret plan-ID rydder bare det uferdige forsøket og beholder allerede sikrede spor.

Aksepten må dekke alle tre årsaker, tilstrekkelig/for svak vind, brutt observasjonsvindu, avbrytelse, Fortsett før og etter festing og gjentatt inspeksjon uten dobbel belønning. Et rent kall til saksmotoren beviser ikke det fysiske forsøket; minst én faktisk spillerreise må vise oppsett, hørbart signal og sikret original.

## Belønning og progresjon

- **Første gyldige forsøk (én gang på tvers av sakene):** SARO-ledelsen godkjenner et servicetillegg: ett relé legges i depotet. Forskning hjelper driften.
- **Støttet konklusjon:** et telex fra regionarkivet med en eldre, relevant observasjon (fortellerbelønning), saken lukkes med konklusjonen bevart, og adgangsflagget `access.fieldlogger` settes. Flagget er kroken for feltloggeren (FF-RESEARCH-04), som da kan lånes ut som neste nivå.
- **Neste spørsmål:** en himmelkilde-konklusjon åpner nivå 2 (sideriske 3 min 56 s per døgn som test mot jordisk tid); en jordisk konklusjon åpner peiling av senderen med feltloggeren. Nivå 2 er beskrevet her, ikke implementert i første leveranse.

## Dagsplanen (GP-17)

OPS viser øverst **TONIGHT'S PLAN**: planlagt forsøk, tidsvindu og hva som mangler (for eksempel «Generator fuel can: 0 in depot. Buy at Caliche»). Planen er et hjelpemiddel, ikke en tvang; rutinejobbene står som før.

**Oppfølging bestilt 4. november, for natt til 5. november:** Det aktive valget skal gi samme neste sted og handling i OPS og dagboken. Et forsøksvindu viser konkret dag, start og slutt, og spilleren kan velge annet arbeid mens forsøket venter. Utført og avsluttet oppdrag skal straks forsvinne fra OPS-PC-ens aktive liste. Rutinejobber avsluttes ved registrering/betaling; oppdrag med nødvendig fysisk arkivering blir stående til siste påkrevde handling er utført. Resultater og originaler beholdes i historikk/arkiv, også etter Continue, uten ny betaling.

Den prioriterte oppfølgingen av foto, saksbrett, logger, første skift, vedlikehold og bevislogikk føres én gang i [nattens gameplay-oppgaver](../../Active/TODO.md#forenkling-av-gameplay-natt-til-5-november-2026), FF-LOOP-NIGHT-01 til 09. Dette er planlagt forbedring, ikke ferdig implementasjon.

## Sakene videre og arbeidsdeling

| Sak | Kilder | Eier |
| --- | --- | --- |
| The repeated interval | extra_pulse, feeder_fault, wrong_source | Claude, første leveranse |
| Disagreeing records | time_disagreement, duplicate_print, unsigned_telex | Claude, implementert 3. november (se under) |
| Visitors around the base | roof_steps, observer, landing_trace, feeder_presence, array_presence (observasjoner fra Director) | Saksmotor og saksbrett: Claude, implementert 3. november. Fysiske feltforsøk i verden: Codex |
| Feltlogger som andre målested | FF-RESEARCH-04/05 bak `access.fieldlogger` | Foreslått Codex etter første leveranse |

Saksmotoren (`S47FreeformCases`) er ren logikk uten verdenstilgang, lagres under `cases` i Freeform-lagringen og kan testes isolert. Eksisterende funn, originaler, arkiv, betaling og `MysterySteps` beholdes uendret; saksbrettet bygger oppå dem.

## Aksept for første leveranse

- En spiller med et gjennomgått ekstrapulsavvik ser saken på OPS, velger et forsøk ut fra forutsigelsene og forstår hva som trengs.
- Forsøket gir hørbart og skriftlig resultat og utelukker riktig forklaring etter den skjulte årsaken.
- Etter to riktige forsøk godtas konklusjonen; før det forklares hva som mangler.
- Alt overlever Lagre/Fortsett. Rutinearbeid, nødveier og eksisterende saksspor fungerer som før.
- Menneskelig vurdering av forståelighet og tempo gjenstår etter leveransen.
