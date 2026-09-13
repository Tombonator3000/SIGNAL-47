# WorldCase22 — hele verden og neste sammenhengende sak

Produksjonsdesign, 13. september 2026. Grunnlag kontrollert ved `60b97e5f1da15f820dde71110bfecbc0fe1d56bb`: [gjeldende overlevering](../CURRENT_HANDOFF.md), [DesignBible13](../DesignBible13/design-bible.md), [kildetekster](../DesignBible13/story-material.md), de tre eksisterende områdekartene, [justert P04/P05-papirprøve](../Menu14/PAPER_CASE.md), `ChapterInvestigation.cs`, `ChapterSave.cs` og `ArchiveStudy.cs`. Dette dokumentet konkretiserer den etablerte trestedsrammen. Det erstatter ikke bibelens historie eller dens historiske PDF.

Statusene nedenfor er grunnlinjen før WorldCase22-integrasjonen. Faktisk levert funksjon og testresultater skal leses i WorldCase22s leveringsrapport. Et generert områdebilde er et **konseptkart**, ikke et spillbilde, en geografisk oppmåling eller bevis på bygde områder.

## Verden, størrelse og visuelt kart

Hele spillet bruker tre separate Unity-områder, seks tilgjengelige interiører og SARO som gjenbrukt knutepunkt. New Mexico, 1986; kald ørkennatt, lave mesaer, lokale varme arbeidslys, institusjonsgrønt metall og kremfarget papir. Ingen ny by, åpen motorvei eller fjerde destinasjon. Reisene er overganger med bevart sak; eventuell kjørestemning er valgfri og må ikke bli nødvendig kjørefysikk, drivstoffjakt eller tidsfyll.

Det nye bildet viser alle tre områdene samlet, med SARO som største klynge, gammel målestasjon som en mindre avsidesliggende klynge og Sierra Motor Court ved en enkel adkomstvei. Lokale gangsløyfer tegnes heltrukket, reiseoverganger stiplet. Illustrert ørken mellom klyngene er kulisse, ikke spillbart areal. Bruk nordpil som illustrasjonsorientering, uten kilometer, koordinater eller oppdiktede instrumentazimuter. I SARO-utsnittet ligger B-12 sør for S-03s oppstillingsflate slik den eksisterende installasjonsteksten sier.

Bildetekst: **SIGNAL / 47 — OMRÅDEKART · KONSEPT / PLANLAGT VERDEN · IKKE MÅLESTOKK**. Tegnforklaringen skiller eksisterende SARO-kjerne, separat arkivprøve og planlagte steder. Detaljer, etiketter og ruter kan kontrolleres mot [map-spec.json](map-spec.json); maskinlesbar spesifikasjon gjelder foran feil eller fantasitekst i et generert bilde.

## De tre stedene

| Område / sone | Spillerens funksjon | Kapittel / oppgaver | Grunnlinje før WorldCase22 |
| --- | --- | --- | --- |
| SARO / kontrollrom | Mottaker, telefon, utskrift; senere C, prøveprotokoll og avslutning | K1; K5–K6; P01/P13–P17 | Rom og K1-interaksjoner finnes; senere C-/finalefunksjon planlagt |
| SARO / fotolab | Framkall, undersøk originaler, sammenlign kontroll og lever rapport | K1; senere K5–K6/E; P02–P03/P13/P17–P18 | Fungerende fysisk kjede og to ekte foto; senere prøver planlagt |
| SARO / arkiv | Original/korrigert 1947-rapport, vedlikeholdskort, indeks og kartkobling | K2 / P04–P05 | Archive16/17 er separat scene; fysisk romkobling og lagring mangler i grunnlinjen |
| SARO / servicegård | Kjent ganglinje mellom laboratoriearbeid og feltprøve | K1; K5–K6 | Eksisterende |
| SARO / S-03 | Motorlogg og første eksponering | K1 / P02 | Eksisterende |
| SARO / B-12 | Kontroller den fysiske referansen, dokumenter rest; senere stimulusprøve | K1/P03, K5/P13–P15, K6/P17 | Aktiv/passiv K1-prøve eksisterer; senere funksjoner planlagt |
| SARO / port og reisepunkt | Velg kun oppdaget, støttet og bygget destinasjon | K2/K5, områdeoverganger | Reisefunksjon planlagt; eksisterende ganglinje/port beholdes |
| STATION 01 / feltbygning | Én liten hytte med benk, gammel logg og protokoll | K3 / P09 | Planlagt |
| STATION 01 / transitflate A | Sammenlign historisk oppstilling med faste terrengmerker | K3 / P06 | Planlagt |
| STATION 01 / fastmerker og lokal lampe | Dokumenter flyttet merke og trygg nullprøve | K3 / P06–P07 | Planlagt |
| STATION 01 / kabelsløyfe og brudd | Følg kabelen, undersøk kuttflater og ta nærfoto | K3 / P08 | Planlagt |
| STATION 01 / ankomst og retur | Kort sløyfe tilbake til bilen, ingen terrenglabyrint | K3 | Planlagt |
| SIERRA MOTOR COURT / parkering og gangvei | Les kontorskilt, finn rom 6 og reis tilbake | K4 | Planlagt |
| SIERRA MOTOR COURT / kontor | Beskjed, konvolutt og normal menneskelig kontekst | K4 / P10 | Planlagt |
| SIERRA MOTOR COURT / rom 6 | Nora, bevis på bordet, feltkort, brev, rettelse og returlinje | K4 / P10–P12 | Planlagt |

Interiørtaket er SARO 3 + målestasjon 1 + motell 2 = 6. Motellbasseng, øvrige rom, fjerne bygninger og veistrekninger er bakgrunn. De blir ikke nye oppgaver eller låste dører som antyder uprodusert innhold.

## Rute og kunnskapsporter

Hovedruten er **SARO K1 → SARO-arkiv K2 → STATION 01 K3 → SIERRA MOTOR COURT K4 → SARO K5 → SARO K6 → SARO epilog**. Lokale returer til de samme kildene er alltid mulige når området er aktivt. P06/P07 og P11/P12 har valgfri innbyrdes rekkefølge; sammenstillingen etter dem krever begge nødvendige resultater.

| Port | Spilleren må ha | Synlig resultat |
| --- | --- | --- |
| K1 → K2 | Fullført lokal B-12-rapport og bevart fotogrunnlag | Henvisning til arkivets referansehistorikk; K1-originalene forblir tilgjengelige |
| P04 | Original og korrigert protokoll sammenlignet | Registrert at C ble utelatt og funnet ble omforklart; ingen motivkonklusjon |
| P05 | P04 og kobling mellom dagens B-12, STATION 01 og riktig fastmerke i arkivindeksen | Old Survey Station blir en oppdaget destinasjon |
| K2 → K3, senere områdeproduksjon | P05, nødvendige kilder bevart, Nora-kontakt og gyldig checkpoint | Reisepunkt åpnes først når målscenen faktisk finnes og kan gjenopptas |
| K3 → K4 | Feltmerke, nullprøve, dokumentert kabelbrudd og tidslogg/P09 | Nora bekrefter møtet; Sierra Motor Court åpnes |
| K4 → K5 | Nødvendige samtaledeler, originaler og signert rettelse | Retur til kjent SARO med konkret stimulusoppgave |
| K5 → K6 | Ny respons, A–B–C-sammenstilling og maskeringsprøve | Begge protokoller forklares med faktiske bevis; morgenserien holdes tilbake |
| K6 → epilog | Valgt fysisk protokoll og arkivert slutteksponering | Ferdig rapport med begge historiske versjoner og Noras rettelse |

Produksjonskartet kan vise hele ruten og A/B/C-nettet. Spillerkartet viser først SARO; STATION 01 vises etter P05, motell etter det relevante kontakt-/feltfunnet. Det viser aldri fremtidige oppgaver eller hele A–B–C-løsningen før P14. En oppdaget destinasjon som ikke er bygget i testkandidaten merkes tydelig som senere innhold, uten en aktiv reiseknapp til en tom scene.

## Sammenhengende produksjonsleveranse: lokal rapport → arkivslutning → feltmål

WorldCase22 skal utvide den faktiske kapittelkjeden med både P04 og P05, lesbar dokumentvisning, gjenåpning av kilder, kartoppdagelse og gjenopptakbar fremdrift. Det er én sammenhengende spillerhandling gjennom flere nødvendige steg. Den separate Archive16/17-prøven er nyttig gjenbruksmateriale, men skal ikke forveksles med denne integrasjonen. En koblet saksvisning fra det eksisterende rapportpunktet kan levere funksjonen før et nytt gangbart arkivrom er produsert. Rom, reise og full K3 lukkes ikke av en meny alene.

| Steg | Handling og svar | Tilstand som må bevares |
| --- | --- | --- |
| 1. Start fra rapporten | Spilleren fullfører eksisterende aktive eller passive B-12-rute. Rapporten gir et tydelig neste spørsmål om referansens opphav. | Eksisterende kapittel er fortsatt fullført, med opprinnelig metode, notebook og to uendrede foto |
| 2. Åpne kildene | Åpne original E07, korrigert E06, moderne B-12-kort og indeksskjema i ønsket rekkefølge. Lukk, gjenåpne og bytt uten å miste sammenheng. | Besøkte kilder og bekreftede funn; et sidebesøk betyr ikke bevist forståelse |
| 3. Begrunn P04 | Sammenlign oppstillingen og velg en kildebasert endring. Feil svar gir presis tilbakemelding, beholder kildene og tillater nytt forsøk. | P04 kan bare registreres med de to relevante kildene besøkt; feil svar åpner ingen port |
| 4. Begrunn P05 | Sammenhold B-12s STATION 01-ID med arkivindeksen og merket: omrisstrekant med kort vannrett strek under. Velg støttet destinasjon. | P05 krever P04 og de to P05-kildene; generisk trekant uten understrek avvises |
| 5. Se neste feltmål | Kart/saksoversikt viser Old Survey Station og begrunnelsen. Begge protokoller og alle indekskilder er fortsatt lesbare. | Oppdaget destinasjon; ingen falsk påstand om tilgjengelig målscene |
| 6. Gjenoppta | Lagre, avslutt og last ved delvis kildelesing, P04 og P05. Gammel fullført v1-sak åpner den nye kjeden uten å miste gammel informasjon. | Samme kildestatus, funn og kartoppdagelse; originale fotobytes beholdt; ugyldig ny tilstand må ikke åpne senere porter |

P04s støttede funn: originalen beskriver en vedvarende referanse og A/B/C; servicekopien utelater den lukkende siktelinjen og kaller merket framkallingsfeil. Dokumentene beviser en endring, ikke motiv, kabelkutterens identitet eller Tomás' skjebne.

P05s støttede funn: dagens B-12 viderefører oppmålingen STATION 01; indeksens sted-ID og fastmerke matcher; den daterte hylsen peker til OLD SURVEY STATION. Årstallet kommer fra arkivkilden. `−39 LY` er ingen kalenderkode. Motellet er ikke neste reisemål.

E08 brukes i denne integrasjonen som tydelig merket **arkivindeks / skjema**. Det foregir ikke å være ferdig historisk negativ eller foto tatt av spilleren. E08s endelige negativ, orientering, transitblokkering og sikteretning må produseres fra samme konkrete oppstilling før P06. Utvidet design innebærer ingen rett til å generere en vilkårlig fotoløsning og tilpasse fysikken etterpå.

## Lesbarhet, feilhåndtering og bevis

Dokumenter skal ha stabil tittel, kilde-ID og tydelig valgt side. Hovedtekst må kunne leses i den eksisterende 1280×800-presentasjonen, med faktisk kontroll også av sammenlignings- og feiltilstandene. Lange tekster må få reell plass eller rulling; krymping til uleselig skrift er ikke en løsning. Tegnformen for fastmerket suppleres med tekst, så farge alene ikke avgjør P05. Ingen «riktig»-markering før spilleren har sendt inn slutningen.

Bekreftede funn og besøkte kilder bevares etter lukking og gjenstart. Endring i et nytt saksfelt skal valideres mot forutsetningene, så en korrupt tilstand ikke kan gi P05 uten P04. En gammel v1-sak får en tom ny arkivtilstand; den gamle `complete`-verdien skal ikke nullstilles for å få et nytt kapittel til å virke. Lagringsfeil skal være synlige og må ikke presenteres som vellykket reise eller sikker gjenopptakelse.

Nødvendig leveringsbevis: faktisk Unity-bygg; aktive og passive K1-fullføringer; for tidlig tilgang; begge dokumentrekkefølger; uriktige slutninger og feil symbol; P04/P05; lukk/gjenåpne; gammel sak; delvis og fullført ny lagring; ugyldig tilstand; begge originalfoto; originale runtime-bilder ved relevante lesestater; pakket kandidat kjørt fra samme launcher. Native input, blind forståelsestest og målt spilletid holdes atskilt fra API-tester.

En ekstern leser har ennå ikke bevist at papirgåten er forståelig uten forklaring. Dette hindrer ikke en reversibel, tydelig merket kandidat og objektive lagrings-/lesbarhetstester, men M1 og full K2-ferdigstatus forblir åpne. Hvis kildene løses raskt, justeres tidsbudsjettet fremfor å forlenge gange, venting eller antall klikk.

## Veikart for hele den avgrensede produksjonen

| Sammenhengende leveranse | Omfang | Ferdig når |
| --- | --- | --- |
| WorldCase22 / P04–P05-kjeden | Rapporttilgang, kildelesbarhet, to slutninger, kartoppdagelse og lagring | Faktiske bygg-/runtime-/gjenstartbevis for hele kjeden; presis kandidatstatus |
| Arkivrom og første områdeovergang | Fysisk arkiv koblet til SARO, Nora-kontakt, feltpakke og trygg tur/retur til ny scene | Ganglinje og overgang virker; retur bevarer kilder, bilder og sted; ingen tomme destinasjoner |
| Målestasjon / hele K3 | Én hytte, transit/merker, nullprøve, kabelkutt, to ekte feltfoto og tidslogg | P06/P07 i begge rekkefølger, begrunnet P08/P09 og komplett gjenstart |
| Motell / hele K4 | Kontor, rom 6, sittende Nora og tre bevisførende samtaledeler | Mild/kritisk tone og P11/P12-rekkefølge gir samme nødvendige fakta; originaler bevares |
| SARO-retur / hele K5 | Valgt stimulus, fysisk frakobling, svarfoto, A–B–C og maskering | Valgt avbrudd finnes i respons/logg; aktivt svar skilles fra frosset bilde |
| Finale og epilog / K6 + E | To informerte protokoller, fysisk utførelse, sjette eksponering og ferdig mappe | Begge utfall, før-valg-last, uteblitt fototiming og ferdig sak virker |
| Samlet spillprøve | Førstegangsspiller, lyd, ytelse, lesbarhet, tilgjengelighet og pakke | Målt helhet og korrigerte vesentlige feil; lengde og kandidatopprykk dokumenteres |

Varighetsmålet beholdes: K1 50 + K2 45 + K3 60 + K4 45 + K5 65 + K6 55 + E 10 = **330 minutter**, innenfor ønsket 5–6 timer. Dette er fortsatt et produksjonsbudsjett, ikke målt spilletid. De 18 hovedoppgavene og seks påkrevde nye eksponeringene beholdes. Ingen kamp, overlevelsesmeter, tilfeldig hendelsesgenerator eller ekstra nøkkel-/sikringsjakt legges til for å fylle rammen.

## Konstanter og avklarte avgrensninger

Unity/Blender og gjeldende lisenskreditering beholdes. 1986/1947, åpningen 23:41, den faktiske pauserbare 47-sekundershendelsen, 1419.900/1420.110/1420.405 MHz, 4/7-grupperingen og eksisterende S-03/B-12-verdier omskrives ikke av kartet. Originalfoto fryses ved eksponering. Nye skjemapunkter er ikke nye motorazimuter. Nora/Tomás-forløpet og begge avslutningene følger bibelens arbeidsretning; fullføring lover ikke gjenoppliving.

Vulkan-splatprøven er separat forskning. Konseptkartets terreng skal bygges med ordinær Unity/Blender-produksjon inntil splats faktisk kan bestå nødvendig integrasjon. Verdenskartet gir ingen grunn til å gjenåpne den kjente mislykkede OpenGL-kombinasjonen.
