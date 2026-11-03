# Freeform: saksløkken

**Status:** Under arbeid. **Gjelder:** Freeform-modusens hovedprogresjon ved SARO. **Sist kontrollert:** 3. november 2026.

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

## Belønning og progresjon

- **Første gyldige forsøk:** SARO-ledelsen godkjenner et servicetillegg: ett relé legges i depotet. Forskning hjelper driften.
- **Støttet konklusjon:** et telex fra regionarkivet med en eldre, relevant observasjon (fortellerbelønning), saken lukkes med konklusjonen bevart, og adgangsflagget `access.fieldlogger` settes. Flagget er kroken for feltloggeren (FF-RESEARCH-04), som da kan lånes ut som neste nivå.
- **Neste spørsmål:** en himmelkilde-konklusjon åpner nivå 2 (sideriske 3 min 56 s per døgn som test mot jordisk tid); en jordisk konklusjon åpner peiling av senderen med feltloggeren. Nivå 2 er beskrevet her, ikke implementert i første leveranse.

## Dagsplanen (GP-17)

OPS viser øverst **TONIGHT'S PLAN**: planlagt forsøk, tidsvindu og hva som mangler (for eksempel «Generator fuel can: 0 in depot. Buy at Caliche»). Planen er et hjelpemiddel, ikke en tvang; rutinejobbene står som før.

## Sakene videre og arbeidsdeling

| Sak | Kilder | Eier |
| --- | --- | --- |
| The repeated interval | extra_pulse, feeder_fault, wrong_source | Claude, første leveranse |
| Disagreeing records | time_disagreement, duplicate_print, unsigned_telex | Claude, neste leveranse med samme motor |
| Visitors around the base | roof_steps, observer, landing_trace, fotografier, sheriff-/radiotips | Foreslått Codex: hendelsesspor fra Director inn i saksmotoren, himmelbaner (FF-RESEARCH-01) og etterspor (FF-RESEARCH-07) |
| Feltlogger som andre målested | FF-RESEARCH-04/05 bak `access.fieldlogger` | Foreslått Codex etter første leveranse |

Saksmotoren (`S47FreeformCases`) er ren logikk uten verdenstilgang, lagres under `cases` i Freeform-lagringen og kan testes isolert. Eksisterende funn, originaler, arkiv, betaling og `MysterySteps` beholdes uendret; saksbrettet bygger oppå dem.

## Aksept for første leveranse

- En spiller med et gjennomgått ekstrapulsavvik ser saken på OPS, velger et forsøk ut fra forutsigelsene og forstår hva som trengs.
- Forsøket gir hørbart og skriftlig resultat og utelukker riktig forklaring etter den skjulte årsaken.
- Etter to riktige forsøk godtas konklusjonen; før det forklares hva som mangler.
- Alt overlever Lagre/Fortsett. Rutinearbeid, nødveier og eksisterende saksspor fungerer som før.
- Menneskelig vurdering av forståelighet og tempo gjenstår etter leveransen.
