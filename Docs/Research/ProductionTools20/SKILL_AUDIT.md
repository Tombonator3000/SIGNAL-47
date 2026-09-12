# Gjennomgang av stoppunkter

Brukeren ba uttrykkelig om å finne og fjerne regler som får Astra til å avslutte for tidlig. Gjennomført 13. september 2026. Endringene retter lokale arbeidsinstruksjoner; de er ingen garanti mot alle fremtidige feil eller en endring av modellen.

## Omfang og metode

Fulltekstsøk over **47 lokale `SKILL.md`-filer**, totalt **497 205 tegn**, fant 167 treff med første stopp-/godkjenningsmønster. Deretter ble relevante kontekster, ruting, tidsmål, avslutningsregler og egne referanser vurdert manuelt. Også filer uten første søketreff ble undersøkt for andre formuleringer, blant annet «ask before» ved batching. [Registeret](skill-audit.json) har én vurdering per fil, før-/etterhash og endringsstatus.

Dette er en statisk gjennomgang med målrettet manuell vurdering, ikke kjøring av alle skills, semantisk linje-for-linje-revisjon av samtlige 497 205 tegn eller rekursiv gjennomgang av alle plugin-referanser og skript. Tjue av filene er cachede artifact-templates, ikke tjue nye aktive SIGNAL-skills.

Synlige overordnede instruksjoner, speilprosjektets AGENTS/START_HER/SAMLET/GAUNTLET, gjeldende overlevering og instruksjons-/godkjenningsrelaterte konfigurasjonsfelt inngikk også i vurderingen. Konfigurasjonen hadde allerede `approval_policy = never`; det var derfor ingen grunn til å svekke tilgangsinnstillinger. Verktøyenes egne tillatelser og runtime-instruksjoner er ikke omprogrammert.

## Endret

| Fil | Observert problem | Rettelse |
| --- | --- | --- |
| `brainstorming/SKILL.md` | Absolutt godkjenning før enhver implementering, ny godkjenning per oppgave/designseksjon og obligatorisk overgang til manglende `writing-plans`. | Omskrevet til kontekst/design → autorisert arbeid → verifikasjon/levering. Manglende nødvendige svar kreves fortsatt; rimelige valg og eksisterende autorisasjon brukes direkte. |
| `game-production/SKILL.md` | «One meaningful part» og ferdig slice kunne tolkes som hele oppdraget. | Hele bestillingen styrer ferdigstatus; delsteg bevarer fremdrift. Uavhengig arbeid fortsetter når én test mangler. |
| `gauntlet-loop/SKILL.md` | Standard på én slice og uklart skille mellom checkpoint og ferdig bestilling. | Første slice er første steg; resterende krav og autorisert levering inngår før avslutning. Rene vurderinger skal fortsatt besvares som vurderinger. |
| `blender-mcp/SKILL.md` | «Stop» etter hjelpeverktøyfeil kunne tolkes som slutt på oppdraget. | Diagnostiser, sjekk delvis resultat, bruk verifisert alternativ og fortsett. |
| `hatch-pet/SKILL.md` | Implisitt 30-minuttersgrense motsa senere krav om fortsatt arbeid; enkelte feil avsluttet hele løpet. | Tidsmålet er et checkpoint. Feil blokkerer berørt generering/pakking, mens diagnose og uavhengig arbeid fortsetter. QA-grenser og regelen om én chroma-pass er beholdt. |

Fem aktive personlige skills er faktisk endret. Fire prosjektverktøy-skills ligger også under [Automation/Skills](../../../Automation/Skills); pet-skillen er bare endret i brukerens personlige installasjon. Brainstormings upstream-proveniens og MIT-lisens er bevart, og `SOURCE.json` markerer nå at installasjonen er lokalt tilpasset.

`~/.codex/AGENTS.md` er opprettet med brukerens preferanse om fullført autorisert arbeid. Den avklarer også at tekniske batchgrenser ikke i seg selv krever ny godkjenning. Dette løser den unødvendige mellomgodkjenningen funnet i Google Drive Comments uten å gjøre en ustabil endring i plugin-cachen. Resultatkontroll og vern mot duplikater kreves fortsatt mellom kall.

Repoets nye [AGENTS.md](../../../AGENTS.md) knytter samme arbeidsform til SIGNAL / 47 og peker på faktisk overlevering fremfor utdatert speilstatus. De synkroniserte referansefilene er urørt.

## Beholdt og hvorfor

42 øvrige skill-filer er byteidentiske med før gjennomgangen. Det betyr ikke at alle 42 formuleringer styrer foran brukerens konkrete instruks: eksempelvis må Sites sin publiseringsgodkjenning ses i lys av eventuell allerede gitt autorisasjon, og Visualize sin stillhetsregel vike for gjeldende overordnede kommunikasjonskrav.

Følgende er reelle krav som ikke bør fjernes: riktig mottaker/fil før eksterne endringer; tilgang til den faktisk bestilte live-arbeidsboken; bevaring av native dokumentstruktur; avklart betalt API-rute; gyldige eksportfiler; faktisk rendering og verifikasjon; ingen duplikatmutasjoner; eksisterende repo-beskyttelser og korrekt lisensgrunnlag. En slik mangel blokkerer sin avhengige handling, ikke automatisk alle andre oppgaver.

En ferdig verifisert leveranse, et rent vurderingsoppdrag som er besvart eller et eksplisitt brukerbestemt tak er legitime sluttpunkter. Endringen skal ikke starte endeløs valgfri polering eller nye ubedte prosjekter.

## Kontroll av endringene

Alle fem endrede skills består [skill-creator-valideringen](skill-validation.json). Første valideringsforsøk fant at bundled Python manglet PyYAML; kontrollen ble fullført i et eksisterende kompatibelt miljø. Ingen runtime ble endret. Dette illustrerer også forskjellen mellom et manglende hjelpebibliotek og en blokkert oppgave.

Egenkontroll av forventet regelutfall, **ikke en uavhengig modelltest**:

| Scenario | Utfallet de reviderte instruksjonene krever |
| --- | --- |
| «Fortsett og merge» innen gjeldende SIGNAL-oppdrag | Implementer, kontroller, opprett vanlig PR, flett når relevante krav er møtt, verifiser resultat. Ingen ny godkjenning bare fordi en skill brukes. |
| «Er dette verktøyet nyttig?» | Undersøk og eventuelt kjør en liten reversibel prøve; ikke migrer produktet. |
| Første av flere bestilte modeller er ferdig | Checkpoint og videre til de øvrige nødvendige modellene. |
| `writing-plans` finnes ikke | Lag nødvendig plan direkte og bruk eksisterende implementeringsflyt. |
| Skrivebordet er kjent låst | Behold native input som UNVERIFIED; fullfør tilgjengelig kilde-, asset-, bygg- og API-arbeid. |
| Hjelper feiler etter mulig filskriving | Kontroller hva som ble skrevet før nytt forsøk; diagnostiser eller velg en egnet alternativ rute. |
| Bestilt kommentarmengde overskrider én teknisk batch | Flere kontrollerte kall innen samme autorisasjon; ikke be om samme godkjenning mellom batchene. |
| Ny betaling, ny mottaker eller uklar destruktiv erstatning | Fullfør forberedelsen, avklar den manglende autorisasjonen før handlingen. |
| Brukerens tidsgrense nås | Respekter brukergrensen; et internt tidsanslag alene avslutter ikke bestillingen. |

Førkopier av alle fem personlige skill-mapper og det opprinnelige skanneregisteret ligger lokalt i `~/signal47-tools/skill-backups/2026-09-13-continuity/`. De ligger utenfor aktive skill-mapper og lastes ikke som konkurrerende instruksjoner. Nye oppgaver kan lese de oppdaterte filene; allerede innlastet katalogtekst blir ikke nødvendigvis oppdatert i en løpende oppgave.
