# Læring fra Voices of the Void for SIGNAL / 47

**Status:** Referanse. **Gjelder:** Gameplay, undersøkelsesregler, fysisk interaksjon og forslag til videre Unreal-arbeid. **Sist kontrollert:** 31. oktober 2026.

Tom ba om å bevare analysen i Git og knytte den til arbeidslisten. Dette notatet samler egne beskrivelser av funn og designforslag. Aktiv prioritering og avkrysning føres bare i [TODO](../../Active/TODO.md#læring-fra-voices-of-the-void). Gjeldende historie og kanon styres av [DECISIONS](../../Active/DECISIONS.md).

## Hovedlærdom og spillerløkke

Et troverdig arbeidssted gir spilleren regler å lære. Arbeidet får mening når justeringer, fysisk utstyr, målinger og hendelser påvirker hverandre. Et avvik blir sterkere når spilleren kjenner normaltilstanden og selv kan undersøke hva som har endret seg.

VotVs grunnløkke er å hente inn signaler, analysere og bearbeide dem, selge resultatet og få poeng. Dette er beskrevet av [utvikleren](https://mrdrnose.itch.io/votv). For SIGNAL / 47 anbefales denne løkken:

**Normal arbeidsoppgave → avvik → mulig forklaring → fysisk kontroll → sikre bevis → sammenligne → velge neste handling.**

Belønningen i vår ene natt kan være bedre begrunnet kunnskap, tillit og nye handlingsmuligheter. En kontrollert måling kan gi grunnlag for en samtale eller beslutning; et godt foto kan åpne en sammenligning. Omfattende butikkøkonomi, behovsmålere og gjentatt tilfeldig vedlikehold har lavere prioritet.

## Konkrete funn og mulig bruk

Funnene nedenfor gjelder statisk logikk i den lokalt undersøkte VotV-utgaven 0.9.0n. De dokumenterer verken spilleropplevd timing, balanse eller at en bestemt gren er utløst under en ny spilltest. Kolonnen for SIGNAL / 47 er vårt designforslag.

| System | Funn i VotV | Mulig bruk i SIGNAL / 47 |
| --- | --- | --- |
| Instrumentavvik | `ticker_dishUncalib` velger en antenne og setter en vedvarende presisjonstilstand. | Lær en normalreferanse. La en fysisk kontroll skille apparatfeil, lokal interferens og fenomen. Et avvik trenger et diagnostiserbart mønster. |
| Begrenset vedlikeholdspress | `ticker_serverBreaker` øker presset over tid, men avstår fra nye tilfeldige brudd når nok servere allerede er ødelagt. Dette er en lokal stoppregel for mekanismen, ikke en global grense for all skade. | Begrens samtidige problemer og gi rom etter fullført arbeid. Hver feil bør gi et valg eller en kontroll, fremfor bare en ekstra tur. |
| Virkning av reparasjon | `serverBox.fix` oppdaterer både lokal feiltilstand og samlet drift, og varsler andre systemer. | En vellykket kontroll skal merkes: normal lampe, stabil kanal, renere utslag eller en ny sammenlignbar måling. Ferdig arbeid må ha konsekvenser. |
| Trinnvis belønning | `lib_getFunc.getSignalPrice` har diskrete verdinivåer. Funksjonen alene fastslår ikke hva alle nivåene betyr i den samlede økonomien. | Skill observasjon, kontrollert måling, uavhengig bekreftelse og dokumentert avvik. Knytt nye svar og valg til faktisk underlag. |
| Fysiske medier | `driveSlot` kontrollerer objekttype og opptatt plass, og har egne tilstander og hendelser for innsetting og uttak. | Bruk tidsriktige spoler, kassetter, negativholdere og utskrifter. Gjør medium, plassering, pågående arbeid og avvisning forståelig. |
| Selektiv fysikk | `prop.canPickup` skiller løse gjenstander fra statiske eller fastlåste. `propThrown` undersøker bevegelsesbanen; masse og fart påvirker en treffreaksjon, og fysisk materiale velger trefflyd. | Gi løse rekvisitter forskjellig tyngdefølelse og pålitelige treff. Fastmonterte instrumenter trenger stabil plassering og forutsigbar bruk. |
| Bevegelseslyd | `comp_physicsImpact` knytter skrapelyd til bevegelse og rotasjon, med begrensning og glatting av lydendringer. | La stol, verktøykasse, glass og papir høres forskjellige ut. Dette kan gi fysisk nærvær og uro. En eventuell regel om at en trussel hører lyden er vårt eget forslag; den er ikke dokumentert av dette VotV-funnet. |
| Skrekk gjennom kjente objekter | `event_lightsTurnoffer` undersøker dørtilstand og bruker eksisterende dører og lysbryter med forsinkelser. | Etabler normal bruk først. Senere kan samme utstyr endre seg på en måte spilleren kan undersøke. Følg de avtalte hendelsene og trusselreglene. |
| Vedvarende arbeidssted | `saveSlot` lagrer objekter, transformasjoner og øktinformasjon. Dette er ikke bevis på feilfri gjenoppretting av alle objektklasser. | Bevar gjennomførte kontroller, sikrede originaler og apparattilstand gjennom lokal retry. En faresekvens kan prøves igjen uten å gjenta ferdig etterforskning. |

## Anbefalt første prioritet

**Gjør én eksisterende SARO-/feltreise forståelig fra normalreferanse til varig resultat.** Knytt dette til STORY-05 og vanlig spillerprøve. Kartlegg først hvilke ledd som allerede fungerer, og rett bare de konkrete hullene.

En sammenhengende prøve bør omfatte:

1. Spilleren utfører en normal kontroll og forstår hva et normalt resultat betyr.
2. Et avvik gir grunnlag for en forklaring som kan undersøkes.
3. Spilleren velger og utfører den relevante fysiske kontrollen.
4. Et foto, en måling eller et medium bevares med riktig kilde og tilstand.
5. Ved retur gir sammenligningen en avgrenset konklusjon og en forståelig neste handling.
6. Avbrudd og lokal retry bevarer fullført arbeid; en mislykket kontroll kan prøves igjen med konkret tilbakemelding.

Bruk eksisterende T01, array- og feltinnhold der det passer den gjeldende historien. T01 er allerede levert og installert. SC08/SC09 inngår i installert SC01 til SC13-kilde gjennom PR260; SC09s fremkalling og SARO-samtale er målrettet kontrollert. LOGIC-08 er rettet gjennom PR272. Dokumentlesing skal fortsatt ikke telles som en egen utført lydprøve. Disse oppgavene skal ikke opprettes på nytt som ubeskrevet nyproduksjon.

Mottakerkilden har allerede en kvalitetsberegning og godkjenningsgrenser for frekvens, gain, båndbredde og retning. Start med sammenhengen mellom handling, lyd, utslag, kontrollresultat og journal. Det er et bedre første steg enn å legge til mange nye systemer samtidig.

Dette er første prioritet **innenfor de nye VotV-forslagene**. De eksisterende P1-ene PLAY-31-16, PLAY-31-17 og PLAY-31-19 om SARO-krasj, dørfremdrift og fastlåsing står foran. Brukerens pågående spilltest og avtalt tidspunkt for retting gjelder fortsatt.

## Videre rekkefølge

Etter den sammenhengende kontrollreisen anbefales:

1. **Fysiske medier og apparatfeedback:** presis plassering, forståelig avvisning, synlig/hørbart resultat og bevart tilstand.
2. **Hendelser som bruker kjente regler:** tydelig årsakskjede, responsrom og samordning med eksisterende dører, lys og lyd. Knytt til STORY-05 og gjeldende SC04-oppgaver fremfor et parallelt trusselsystem.
3. **Selektiv løs fysikk:** prøv én liten sammenhengende situasjon med en verktøykasse eller kopp, materiallyd, stabile treff og lagring.
4. **Vedlikehold og belønningsnivåer:** vurder først når spillerprøven viser behov for flere valg. Prioriter evidenskvalitet og konsekvenser fremfor en omfattende poengøkonomi.

Dette er anbefalinger, ikke en bestilling om å implementere alle systemene nå.

## Fysikkregler for en senere avgrenset prøve

- **Fastmontert:** mottaker, tavler og store instrumenter har stabile plasser og bruksankre.
- **Styrt bevegelse:** dører, skuffer, brytere og medieholdere har begrenset bevegelse og tydelige brukstilstander.
- **Løst:** utvalgte kopper, bokser og verktøy kan bæres, settes ned og veltes.
- Bæring bør stoppes av vegger. Tunge gjenstander trenger plass, stabile kollisjoner og avgrensede reaksjoner.
- Viktige spor skal kunne gjenfinnes etter uhell. Sikring av kunnskap og originaler skal tåle lokal retry.
- Bruk eksisterende Unreal-fysikk til avgrensede rekvisitter. Bevar fungerende spiller-, bil- og instrumentstyring med mindre en konkret oppgave krever endring.
- Pause, lasting og områdebytte må gi én konsistent objekttilstand. Ingen ekstra oppdrag fullføres bare ved å laste inn en sak.

## Kilder, kompatibilitet og kontrollgrenser

- **VotV:** lokal utgave 0.9.0n, Unreal 4.27.2. Verifisert KisMetScript og dekodet Blueprint-instruksjonsdata er undersøkt. Server-/antenne-tickerne, prisfunksjonen, kastelogikken og lyshendelsen bestod både tilbakekompilering og kontroll av rekonstruerte pakke-byter. `prop`, `serverBox`, `driveSlot`, `comp_physicsImpact` og `saveSlot` bestod pakke-bytekontrollen, men har ikke en verifisert KisMetScript-utgave.
- **Begrensning:** `mainPlayer`, `mainGamemode` og `lib` bestod ikke pakke-bytekontrollen. Eksakte bæregrenser, utholdenhetsrater og globale effektivitetsformler er derfor ikke sikre fasiter her. VotV-tall og motoravhengig oppførsel skal ikke behandles som innstillinger for UE 5.8.4.
- **Lokal analyseindeks:** `RESULTAT.json`, `evidence/toolchain.json`, `evidence/kismet-results.json` og `evidence/bytecode-results.json` bevarer versjoner og status utenfor offentlig Git. Spillfiler, utpakkede assets, pseudokode, Blueprint-kode og Ghidra-prosjekt følger ikke dette notatet.
- **SIGNAL / 47:** [designbibelen](../../DesignBible-2026-10-27/DESIGNBIBEL.md), [gjeldende føringer](../../Active/DECISIONS.md), [arbeidslisten](../../Active/TODO.md) og [mottakerlogikken](../../../unreal/Signal47/Source/Signal47/Private/S47Rx.cpp). Kanon beholdes: New Mexico 1986, 1420.405 MHz, 27 antenner, azimut 026 og den faste 47-sekundershendelsen.
- **Prosjektbibliotek:** [spillverktøyvurderingen 29. oktober](https://github.com/Tombonator3000/prosjektbibliotek/blob/main/docs/SIGNAL47_NYE_REPOER_2026-10-29.md) ble brukt for kompatibilitetsvurderingen av selektiv Unreal-fysikk. Bibliotekets nye integrasjoner var UNVERIFIED; ingen ny fysikkmotor eller plugin er innført her.

Notatet er en designreferanse basert på statisk analyse. Spilleropplevd balanse, forståelse, lyd og fysikkfølelse er ikke prøvd på nytt i denne dokumentleveransen. Videre implementering skal være egen kode tilpasset SIGNAL / 47.
