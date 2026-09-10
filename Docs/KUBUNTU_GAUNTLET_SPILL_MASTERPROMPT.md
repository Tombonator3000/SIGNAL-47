# SIGNAL / 47 — arbeidsordre for et helt spillbart kapittel

Bruk @gauntlet-loop og @brainstorming. Bruk Dream Loop til å løfte grafikken gjennom sammenligning med det kjørende spillet. Les faktiske skill-instruksjoner, gjenbruk dokumentert læring og finn nye, kostnadsfrie skills når de løser konkrete produksjonsproblemer.

Jeg synes fremgangen har vært for oppstykket. Nå bestiller jeg en større, sammenhengende leveranse: **et helt første kapittel av SIGNAL / 47 som jeg kan starte, spille ferdig, lagre og fortsette på Kubuntu-maskinen.** Ta ansvar for spilldesign, programmering, visuell utforming, bevegelse, lyd og kvalitet som en erfaren spillutvikler.

Dette er en arbeidsordre for bygging. Idéarbeidet skal munne ut i et valgt design, og designet skal bli spillbart. Små commits er interne sjekkpunkter. Et enkelt nytt objekt, en framkallingsknapp, en testscene, en plan eller en oppdatert statusfil avslutter ikke oppdraget.

Svar på norsk. Gi korte, konkrete oppdateringer og fortsett arbeidet gjennom dem. Bevar prosjektets særpreg og fungerende deler. Du har mandat til å ta vanlige, reversible design- og implementeringsvalg innenfor denne bestillingen uten ny godkjenning for hvert steg.

## 1. Arbeid i riktig SIGNAL / 47-prosjekt

Dette er prosjektspesifikke holdepunkter kontrollert 10. september 2026. Kontroller dem på nytt før endringer; de er ikke en ordre om å tilbakestille nyere arbeid.

| Område | Kjent utgangspunkt |
| --- | --- |
| Prosjekt | SIGNAL / 47, førstepersons kosmisk etterforskning ved SARO i New Mexico, 1986 |
| Kanonisk repo | Privat: https://github.com/Tombonator3000/SIGNAL-47 |
| Kanonisk arbeidsmappe | `/home/tombonator3000t/.codex/.chatgpt-projects/g-p-6aa05604a4708191a65139c1d5f89363/SIGNAL-47` |
| Unity-prosjekt | `Unity/` under denne arbeidsmappen |
| Hovedscene | `Unity/Assets/Signal47/Scenes/Prototype/SARO_Prologue.unity` |
| Sist kontrollerte main | `8a6b4d9f72b222d4cc8e124553d64b84770cd0e3`, feltkamera 08; PR 1–5 var flettet |
| Senere lokal dokumentasjon | `docs/current-handoff-09`, med overleveringsrettelse `203afab`; dette er ikke en nyere gameplay-versjon |
| Verktøykjede | Unity 6000.3.22f1, URP 17.3.0, Input System 1.20.0, Blender 4.5.13 LTS |
| Leveranseplattform | Behold nåværende Linux desktop-versjon; ingen motor- eller plattformmigrering er bestilt |
| Sist verifiserte spiller | `Artifacts/Releases/FieldCamera08-af5a85d/Signal47.x86_64` |
| Sist verifiserte arkiv | `Artifacts/Releases/SIGNAL47-FieldCamera08-Linux.tar.gz` |
| Referanse-PC | Intel Core Ultra 5 225U / Intel ARL, OpenGLCore, 1280×800 Ultra, 75 Hz i tidligere måling |
| Kostnader | Ingen nye kjøp, abonnementer eller fakturerbare API-kall uten særskilt avtale |

De relative prosjektstiene i denne arbeidsordren er relative til den kanoniske arbeidsmappen, med mindre annet står. Et rent repo inneholder ikke nødvendigvis lokale spillpakker eller installerte verktøy.

Les gjeldende AGENTS.md og deretter relevante deler av README, `Docs/CURRENT_HANDOFF.md`, `Docs/FIELD_CAMERA_08.md`, `Docs/SERVICE_YARD_07.md`, `Docs/CONTROL_ROOM_PASS_06.md`, `Docs/WORLD_AREA_PASS_05.md`, lisensoversikten og tilhørende bevis. I ChatGPT-prosjektets referansemappe finnes blant annet `START_HER_SIGNAL_47.md`, `SAMLET_DOKUMENTASJON.md`, `OPPRINNELIG_SAMTALE.md` og `STATUS_PASS08_MERGET.md`.

Alle synkroniserte filer under `sources/` er skrivebeskyttet referansemateriale. Den gamle startfilen peker på commit 79c0742 og et gammelt «neste lille steg»; dette er historikk. **Denne bestillingen utvider leveransemålet til et helt kapittel.** Bruk nyere kode og faktisk kjøring som fasit for hva som finnes.

Kontroller lokal status, remote, grener, åpne PR-er, aktive byggejobber og hvilken versjon eventuell åpen Unity-editor bruker. Bevar lokale endringer og andre prosjekter. Bruk en egen arbeidsgren fra riktig gameplay-utgangspunkt; hent med relevante dokumentasjonsrettelser. Ikke lag et konkurrerende spillprosjekt eller ta med urelaterte endringer fra andre samtidige oppgaver. Integrer avtalte bidrag fra dette oppdragets subagenter kontrollert.

## 2. Bevar spillets identitet og kjente kontrakter

Spillets kjerne er: **Spilleren oppdager et avvik, undersøker det med fysiske verktøy, danner en hypotese og tester den i verden. Svaret åpner et vanskeligere spørsmål.**

Bevar førsteperson, fri bevegelse, fysisk 1986-utstyr, sparsom og urovekkende kosmisk etterforskning, lesbare miljøer og tilbakeholden fortelling. Stilen er stilisert realisme: troverdige proporsjoner og materialer, institusjonsgrønt og krem inne, kald ørkennatt og store antennesilhuetter ute, med lokale varme arbeidslys.

Følgende konstanter skal bestå: kalibrering 1419.900 MHz, interferens 1420.110 MHz, anomali 1420.405 MHz, pulsmønster 4/7, avstand -39 LY og den etablerte hendelsen 47 spillsekunder etter at telefonlinjen dør. Bevar de opprinnelige modellene, lisensierte ressurser, redigerbare Blender-kilder, Unity-metadata og kreditering, inkludert VT323 og Scott Buckley.

Bevar fungerende bruk av mottaker, signaljustering, fysisk utskrift, telefon og håndsett, kaffekopp, notatbok, antennebevegelse, servicegård og kamera. Dagens kontroller er WASD/mus, E for interaksjon, Tab for notatbok, Escape for lukk/pause, C for søker og Space for eksponering. Nye kontroller skal forklares i spillet og ikke kollidere med eksisterende handlinger.

Den større visjonen om bil, veier, motell og Roswell består. Et motellutkast ved prologscenen er ikke bevis på ferdig motellspill eller riktig geografi. La første kapittel få den lokale utstrekningen innholdet trenger; ikke bruk en ny by, kjøresimulator eller tom ørken som erstatning for etterforskning. Kamp, crafting og økonomisystemer inngår bare dersom de allerede er vedtatt og nødvendige; de skal ikke introduseres som standard spillfyll.

## 3. Gjør en kort oppstartskontroll, og gå videre

Bekreft separat tilgang til filer, terminal, Unity-bygging, kjørbar spiller, faktisk input, skjermbilder, lyd og målinger. Bruk VERIFISERT, TILGJENGELIG MEN UPRØVD, KREVER OPPSETT eller UTILGJENGELIG.

Start den eksisterende versjonen gjennom normal spillstart når tilgangen tillater det. Opplev prologen og kameraundersøkelsen, og behold noen originale bilder som utgangspunkt. Eksisterende tester og bevis kan gjenbrukes der de fortsatt gjelder. Ikke bruk hele økten på å gjenta gammel dokumentasjon eller måle uendret kode om igjen.

Tidligere rapportert resultat er 34 kontrollpunkter med ekte tastatur/mus og 74,987 fps over 185,832 sekunder. Dette dokumenterer feltkamera-prototypen på den oppgitte PC-en, ikke kvaliteten eller ytelsen til det nye kapitlet.

Oppsummer de viktigste hullene kort. Kjent utgangspunkt er at kameraet bare har ett motiv, fotografiet vises umiddelbart, sammenligningen er enkel, full spillagring mangler, og flere miljøer fortsatt har prototypepreg. Kontroller hvilke av disse som fortsatt gjelder.

Bruk den fungerende Unity/Blender-flyten. Skriptstyrt Blender-produksjon er gyldig når den faktisk kjøres og eksport/import er kontrollert. Ingen MCP-bro skal kalles tilkoblet uten en vellykket prøve. Ikke installer en ny integrasjon bare for å fylle en verktøyliste.

Dersom maskintilgang mangler, gjør tilgjengelig kode-, design- og assetarbeid først. Oppgi deretter minste nødvendige lokale handling. En utilgjengelig valgfri bildegenerator er ikke samme blokkering som manglende tilgang til selve prosjektet. Ikke hev at denne teksten oppretter PC-tilgang.

## 4. Brainstorm én gang og velg en sammenhengende retning

Bruk Brainstorming til å utfordre designet og gjøre det gjennomførbart. Dette er et eksisterende prosjekt med mandat til en større leveranse. Bruk min forhåndsautorisasjon til rutinevalg: presenter valgt retning og gå videre. Still spørsmål bare når svaret er nødvendig for omfang, eksisterende kanon, faktisk tilgang eller kostnader. Miljøets overordnede regler gjelder fortsatt.

Vurder disse tre retningene kort mot spillerverdi, særpreg, innholdsbehov og teknisk risiko:

| Retning | Styrke | Hovedrisiko |
| --- | --- | --- |
| Et tett SARO-kapittel med feltfoto, fotolab, hypoteser og ny feltprøve | Bygger videre på fungerende systemer og gjør dem til en hel etterforskning | Kan bli en kjede av automatiske «les og bekreft»-handlinger |
| Tidlig bilreise til et spillbart motell | Større geografisk variasjon og sterk New Mexico-stemning | Kjøretøy og nye steder kan spise kapasiteten før mysteriet fungerer |
| Dypere instrument- og signalanalyse inne på SARO | Tydelig faglig identitet og mye gjenbruk | Kan bli mer av den samme terminalbruken |

**Anbefalt og bestilt utgangspunkt er det tette SARO-kapitlet**, med instrumentanalyse som støtte. Bruk et nytt tilgjengelig arbeidsrom og et avgrenset feltpunkt til å gi opplevelsen fysisk utstrekning. Bil og motell er senere innhold, med mindre faktiske nyere prosjektbeslutninger krever dem nå.

Det dristige grepet er et fotografisk avvik spilleren selv kan oppdage og teste. Første bilde, en samtidig observasjon og en kontrollert ny eksponering gir ikke samme forklaring. En konkret detalj i motivet skal bære dette: velg for eksempel en merking, en siktelinje eller et fysisk mønster som lar seg modellere og etterprøve. Bestem nøyaktig detalj og årsakssammenheng før implementering. Dette er et nytt designmandat, ikke en påstand om ferdig kanon.

Angrip ideen før du bygger: Hva er den mest sannsynlige feiltolkningen? Hvor blir spilleren stående uten retning? Hva kan spilleren faktisk gjøre for å teste en vanlig forklaring? Hvilken handling gir ny kunnskap? Rett svakhetene og skriv ett kort valgt design med start, midtparti, signaturøyeblikk og utfall. Deretter bygg.

## 5. Lever kapitlet «Den andre eksponeringen»

Arbeidstittelen er et forslag. Leveransemålet er et komplett første kapittel fra normal ny spillstart til en avsluttet lokal sak og en tilbakeholden åpning mot neste del av mysteriet.

Eksisterende prolog og pass08 er grunnlaget som skal bevares. De teller ikke som ny innholdsleveranse. **Framkalling med ett nytt spor er et internt sjekkpunkt; den etterfølgende tolkningen, feltprøven, konsekvensen og avslutningen skal også bygges og fullføres.**

Sikt mot omtrent **20–30 minutter for en førstegangsspiller**, inklusive eksisterende prolog, dersom innholdet bærer det. Dette er en innholdsambisjon, ikke en produksjonsfrist eller dokumentert spilletid. Ingen kunstige ventetider, gjentatte turer uten ny informasjon eller langsommere gange for å fylle minutter. Oppgi forskjellen mellom antatt førstegangsvarighet og faktisk målt testtur.

Kapitlet skal ha følgende forløp:

| Del | Hva spilleren gjør | Hva som må være nytt eller vesentlig bedre |
| --- | --- | --- |
| Nattevakten | Lærer stedet og verktøyene, kalibrerer, oppdager signalet, leser utskriften og opplever telefonhendelsen | En lesbar åpning og naturlig overgang videre; det gamle prologsluttkortet skal ikke feilaktig avslutte hele kapitlet |
| Første feltarbeid | Undersøker S-03, tar et relevant foto og bruker miljøet til å orientere seg | Bildet får en rolle i en større sak; tydelig lokal geografi og meningsfull observasjon |
| Fotolaben | Tar hånd om eksponert film og framkaller fysisk ved en arbeidsstasjon | Et kort forståelig handlingsforløp, et bevis som blir tilgjengelig gjennom handling, og god visuell/lydlig respons |
| Tolkningen | Undersøker bildet, sammenholder konkrete trekk med logg/kart og velger en hypotese | Spilleren må se sammenhengen selv; spillet skal ikke automatisk skrive løsningen idet bildet åpnes |
| Kontrollforsøket | Besøker et nytt tilgjengelig feltpunkt eller en relevant ny del av anlegget, velger en målemetode og tar en ny observasjon/eksponering | En handling som kan støtte eller avkrefte hypotesen, og en reell forskjell fra første fototur |
| Signaturøyeblikket | Sammenligner resultatene og oppdager et avvik som den vanlige forklaringen ikke dekker | En faktisk synlig, spillbar oppdagelse med tilbakeholden lyd/lysrespons; ingen uforklart tekstbasert fasit |
| Kapittelslutt | Samler en begrunnet konklusjon, ser følge av sine valg og fullfører nattens lokale undersøkelse | Tydelig avslutning, lagret progresjon og et konkret uløst spørsmål; ikke bare «kommer snart» |

Fotolab og feltpunkt skal høre fysisk og funksjonelt til anlegget. Et lite arkivhjørne kan støtte tolkningen uten å bli en egen omfattende arkivsimulator. Hver ny sone må gi spilleren noe å undersøke eller gjøre som ikke finnes i den forrige.

## 6. Åtte bindende akseptansekriterier

Registrer kriteriene før bygging. Konkrete detaljer kan avklares fra prosjektet, men leveransens bredde skal ikke reduseres til «framkalling fungerer» etterpå.

1. **Hel reise:** Ny spillstart fører gjennom eksisterende prolog, feltarbeid, fotolab, tolkning, kontrollforsøk og en ny kapittelslutt uten utviklerkommandoer, teleportering eller redigering av lagringsfiler.
2. **Spillbar romlig utvidelse:** Kontrollrom, servicegård, nytt arbeidsrom og et meningsfullt nytt feltpunkt er tilgjengelige og lette å forstå som sammenhengende steder. Ut- og returveier fungerer med faktisk kollisjon og normal spillerbevegelse.
3. **Etterforskning med konsekvens:** Spilleren utfører minst to forskjellige undersøkelser og tester minst én plausibel alternativ forklaring. Minst ett valg endrer hva spilleren gjør eller hvilke observasjoner vedkommende får, med synlig respons før sluttkortet. Feil hypotese gir nyttig informasjon og en vei videre.
4. **Fotografisk beviskjede:** Minst to relevante eksponeringer, fysisk framkalling og interaktiv bildeundersøkelse inngår i løsningen. Et nødvendig visuelt trekk kan pekes ut i bildene. Bevisene har stabil identitet, kan gjenåpnes og dupliseres ikke ved gjentatt bruk.
5. **Signatur og avslutning:** Den nye oppdagelsen oppstår gjennom spillerens handlinger og kan underbygges av bevisene. Kapitlet avslutter den lokale oppgaven og lar den større kosmiske årsaken stå åpen. Eksisterende spillkonstanter og telefonforløp er bevart.
6. **Varig og robust tilstand:** Nytt spill, pause, avbrudd, omstart og lagre/avslutte/starte/fortsette fungerer. Minst ett mellompunkt før framkalling og ett etter ny feltprøve kan gjenopptas uten å miste nødvendige bevis eller låse progresjonen. Tidligere eksporterte bilder bevares.
7. **Samlet presentasjon og kontroll:** De sentrale spillbare sonene og hendelsene er løftet til samme visuelle nivå, med lesbar UI, responsive kontroller og sammenhengende lyd. De vesentlige avvikene mot de valgte målbildene er rettet i runtime. Innstillinger for lyd og mus fungerer og lagres.
8. **Verifisert leveranse:** Relevant regresjon, ny brukerreise, gjenopptakelse, visuell kontroll og målt ytelse er dokumentert på sluttbygget. Riktig lokal spillpakke starter fra de leverte instruksjonene. Ingen kritisk FAIL skjules av en totalscore; nødvendige UNVERIFIED-punkter oppgis som gjenstående.

Et manus med to spor er ikke to spillbare undersøkelser. To knapper som alltid gir samme handling er ikke et meningsfullt valg. En bevistekst som hevder at bildet inneholder noe, erstatter ikke et synlig trekk i bildet.

Hovedreisen, den fotografiske beviskjeden, lagre/fortsette, kapittelutfallet, de konkrete visuelle kravene, ytelseskravene og oppstart av sluttpakken må være PASS før leveransen kalles et ferdig kapittel. Supplerende forhold som GPU-tid og subjektiv lydvurdering kan stå åpne med uttrykkelige begrensninger; dette gir ikke adgang til å merke et annet utestet kjernekrav som oppfylt.

## 7. Bygg systemene som dette forløpet trenger

Les de faktiske grensene i eksisterende kode før oppdeling. Relevante innganger finnes i `Unity/Assets/Signal47/Runtime/`: `Core/GameSession.cs`, `Events/PrologueDirector.cs`, `Investigation/FieldCamera.cs`, `Investigation/Notebook.cs`, `Investigation/ServiceYardInvestigation.cs` og `UI/HUDController.cs`. Sceneproduksjonen ligger i `Unity/Assets/Signal47/Editor/`, blant annet `Signal47SceneBuilder.cs` og de eksisterende pass-filene. Bekreft stiene.

**Progresjon:** Skill mellom hendelsesrekkefølge, bevisinnhold, presentasjon og varig lagring. Bruk stabile identifikatorer og tydelige overgangsvilkår. Unngå at ett stort UI-script blir ansvarlig for hele kapitlet. Utvid fungerende komponenter; ikke skriv om motorgrunnlaget uten en påvist grunn.

**Analog fotografering:** Nåværende `FieldCamera.FinishPhoto` gjør bildet umiddelbart tilgjengelig. Innfør eksplisitt håndtering av eksponert, uframkalt, framkalt og undersøkt bevis. Bakgrunnseksport kan fortsette teknisk, men spilleren skal forstå hvorfor bildet ikke kan undersøkes før framkalling. Bevar tidligere eksportfiler og gjør mislykket lagring forståelig.

Kamerautsikt og nødvendig verdenstilstand fastlåses ved eksponering. Framkalling, gjenåpning og lasting skal vise den samme eksponeringen; de skal ikke ta et nytt bilde av dagens verden. Eventuelle filmspesifikke effekter må følge en definert, lagret regel knyttet til opptaket.

**Framkalling:** Gjør det til en kort fysisk oppgave med en meningsfull handling, en synlig overgang og et resultat som tas i bruk. Ikke bygg et fullt kjemisimulatorsystem. Ikke bruk en lang nedtelling som eneste innhold. En avbrutt prosess skal kunne gjenopptas eller gjentas uten permanent tap.

**Undersøkelse og hypoteser:** Gi nødvendig zoom, panorering eller sidevis sammenligning og enkel markering/kobling av relevante trekk. Bevar tekst og kontroller som redigerbar UI. Skill rå observasjon, spillerens hypotese og bekreftet funn. Hjelp kan gis gradvis på forespørsel uten å røpe løsningen ved første åpning.

**Fotografisk fenomen:** Bildematerialet skal produseres fra den faktiske spillscenen. Dersom fiksjonen krever at film ser noe øyet ikke gjør, implementer det som en bevisst effekt i spillets opptakssystem og dokumenter teknikken. Ikke bruk et generert ferdigfoto som skjult erstatning for spillerens opptak. QA-bilder skal alltid vise det uendrede spillet, også når spillet selv viser et fiktivt fotografisk fenomen.

**Kontrollforsøk:** Bestem en konkret alternativ forklaring og en fysisk prøve som skiller den fra hovedhypotesen. Et mulig mønster er passiv observasjon mot en kontrollert lokal kalibreringshandling, med forskjellig målesituasjon og bevis. Dette er et designforslag som skal konkretiseres, ikke ferdig lore. Begge veier må være forståelige og kunne fullføres.

**Lagring:** Bruk versjonert format og trygg filskriving. Lagre bevis, kapitteltilstand og nødvendig verdens-/spillerstatus på sammenhengende kontrollpunkter. Håndter manglende foto, ufullstendig eller ugyldig lagring uten krasj eller taus datatap. Test faktisk avslutning og gjenstart av programmet. Nytt spill skal skille mellom å nullstille aktiv sak og å slette brukerens eksportarkiv.

## 8. Løft grafikken med en tilpasset Dream Loop

Bruk eksisterende mål under `Docs/VisualTargets/`, særlig `control-room-v1.png`, `phone-desk-v1.png` og FieldCamera08-materialet. Se også verdensretningen i pass05. Dette er mål og referanser, ikke dokumentasjon av ferdig grafikk.

Lag manglende målvisninger med tilgjengelig bildegenerering når de gir en konkret produksjonsretning. Prioriter tre utsnitt: normal utsikt ved arbeidsplassen, fotolaben under bruk og det nye feltpunktet ved signaturøyeblikket. Bruk aktuelle spillbilder som utgangspunkt, bevar førstepersonskamera og byggbar romstruktur, og merk resultatene tydelig som **genererte målvisninger**. Nye foreløpige mål er ikke automatisk brukerens tidligere godkjenning.

Integrer ressursene i den virkelige scenen, ta originale runtime-bilder fra tilsvarende kamera/tilstand og sammenlign. La en egen kritiker vurdere kamera, skala, store former, lys, materialer og lesbarhet. Be om konkrete avvik med bildebevis, spillerkonsekvens og foreslått rettelse. En visuell score er ikke et ferdigkriterium for kapitlet.

Prioriter de største opplevde svakhetene: rom som fortsatt ligner tilfeldige kuber, feil proporsjoner, mørke interaksjoner, flate overflater og ulogiske lys. Hovedrekvisittene skal tåle nærbilder. Fyll nye soner med noen få troverdige, funksjonelle objekter fremfor vilkårlig rot.

Velg ressurser etter behov: eksisterende assets først, deretter egnede gratisressurser med kontrollerte lisenser, egen Blender-modellering og begrunnet prosedyregeometri. Bevar redigerbar kilde og dokumenter eksport. Kontroller skala, normaler, UV-er, materialer og kollisjon i Unity. Test én representativ ny ressurs gjennom hele kjeden tidlig.

Dream Loop-ressurser kan omtale fakturerbare bilde-til-3D-tjenester. En eksisterende API-nøkkel er ikke et kjøpsmandat. Bruk kostnadsfrie alternativer og tilgjengelige inkluderte verktøy. Ingen produksjonsvei skal avhenge av en utgift som ikke er avtalt.

Hvis en visuell rettelse ikke hjelper etter flere reelle forsøk, undersøk om problemet ligger i romutforming, lysrigg eller assetvalg før nye småjusteringer. Behold målretningen og dokumenter begrensningen. Hele spillet skal fortsatt kunne bygges, spilles og måles mens grafikken forbedres.

## 9. Gjør bevegelse, lyd og grensesnitt til en del av kapitlet

Prøv ganghastighet, stopp, vending, dørpassasje og nærinteraksjon under faktisk bevegelse. Unngå å introdusere kamerabevegelser som gjør presis bildeinnramming vanskelig. Mouse sensitivity, relevante lydnivåer og vindus-/fullskjermvalg skal være forståelige; innstillinger som tilbys, må virke og bevares.

UI skal vise situasjon og neste mulige handling uten å levere alle svarene. Spilleren skal skjønne forskjellen på kamera i bruk, uframkalt film, tilgjengelig bilde og bekreftet funn. Rett beskjæring, fokuskonflikter og feil knappetilstand. Ikke godkjenn et panel bare fordi skjermbildet ser ryddig ut; prøv handlingene.

La lyd følge materialer, rom og hendelser: filmhåndtering, mekanikk, skriver, telefon, instrumenter, vind og fotolab. Kontroller start, gjentakelse, pause, overganger og avslutning. Bevar kontrasten mellom normal drift og avviket. Ikke la viktig informasjon avhenge utelukkende av lyd; gi tilsvarende observerbar informasjon der det er nødvendig.

Skille mellom kildeanalyse, målt lydutgang og faktisk lytting. Dokumenter lytting bare dersom du faktisk kan høre resultatet. Den tidligere lydmålingen sertifiserer ikke den nye miksen. Manglende lytting skal stå som et konkret åpent kvalitetspunkt; den skal ikke skjules eller stanse uavhengig arbeid.

## 10. Bruk skills og subagenter med konkrete oppgaver

Finn først @gauntlet-loop og @brainstorming i miljøets katalog og etablerte skill-mapper. Gauntlet er tidligere lagret på `/home/tombonator3000t/.codex/skills/gauntlet-loop/SKILL.md`. Les installert versjon. Hvis Brainstorming mangler lokalt, les originalkilden eller installer en gjennomgått versjon gjennom støttet skill-installer.

Hvis en skill fortsatt er utilgjengelig, følg arbeidsmetoden beskrevet her og oppgi begrensningen kort. Fortsett byggingen med fungerende verktøy; en manglende skill-meny skal ikke bli sluttpunktet for oppdraget.

Jeg gir klarsignal til relevante, gjennomgåtte, kostnadsfrie skill-installasjoner. Les innhold og avhengigheter før kjøring, bevar lisenser og unngå duplikater. Søk målrettet etter konkrete hull, for eksempel Unity-automatisering, Blender-assetflyt, spillagring, lydkontroll eller faktisk inputtesting. Ikke bruk tiden på å samle en stor katalog.

Noter navn, kilde, versjon/commit, nødvendig tilgang, installasjonssted og hva et faktisk forsøk viste. Skill mellom lest, installert og prøvd. Hvis innlasting krever en senere tur eller omstart, si det; ingen lokal fil skal omtales som automatisk synkronisert til andre maskiner.

Når utført arbeid gir en gjentakbar metode, bruk skill-creator til å forbedre en relevant skill eller lage én smal ny. Les instruksjonen først. Dokumenter nødvendige innganger, fremgangsmåte, forventet resultat, verifisering og vanlige feil. Ikke lagre uprøvde antakelser som etablert praksis.

Dette oppdraget tilpasser skill-metodene til et allerede autorisert prosjekt: kort idéarbeid, valgte løsninger og vedvarende bygging. Brainstormings generelle godkjenningsrunder skal ikke føre til spørsmål om allerede delegerte rutinevalg. Dream Loops visuelle stoppregler skal ikke avslutte et uferdig kapittel. Skill-tekster gir heller ingen ny betalings- eller publiseringsautorisasjon. Faktiske system- og verktøyregler gjelder.

Bruk subagenter når de er tilgjengelige og kan arbeide uavhengig. En egnet fordeling er progresjon/lagring, miljø/rekvisitter og kritisk vurdering. Hovedagenten eier brukerreisen og integrasjonen. Avtal filansvar og datagrensesnitt før parallelle endringer. Del samme Unity-prosjekt uten samtidige editorbygg; bruk isolerte arbeidskopier når nødvendig. Grafiske tester som styrer tastatur/mus må kjøre én om gangen med eksisterende lås.

En kritiker bør få kort prosjektkontekst, målbildene, de faktiske bildene og kriteriene. Rapporter skal skille mellom inspeksjon, kjøring og antakelser. Hovedagenten må selv kontrollere sammensatt resultat; en ferdigmelding fra en underagent er ikke bevis på at kapitlet virker.

## 11. Arbeid i fire sammenhengende produksjonspakker

Bruk Gauntlet-løkken: **Definer → Inspiser → Implementer → Kjør → Vurder → Korriger → Lagre sjekkpunkt.**

1. **Hele kapitlets forløp:** Koble sammen åpning, eksisterende prolog, første foto, fotolab, tolkning, kontrollforsøk og avslutning tidlig. Midlertidig enkel grafikk er tillatt som internt arbeidsstadium. Fullfør et første gjennomspill og finn brudd i sammenheng og progresjon.
2. **Etterforskning og varig progresjon:** Gjør sporene synlige og tolkbare, bygg feil og alternative hypoteser, meningsfullt valg, faktisk bildebehandling og lagre/fortsette. Kontroller at innholdet fortsatt fungerer i ulik rekkefølge der dette er tillatt.
3. **Samlet opplevelse:** Løft alle sentrale soner, objekter, UI, lyd og bevegelse mot den valgte retningen. Prøv signaturøyeblikket i bevegelse og i sammenheng med resten av kapitlet. Rett kjedelige eller uklare ledd, ikke bare tekniske feil.
4. **Stabilisering og levering:** Spill begge relevante valgveier, prøv avbrudd og gjenopptakelse, mål sluttbygget, gjennomfør kritisk gjennomgang, rett funn og pakk akkurat den kontrollerte utgaven.

En pakke kan inneholde flere commits. Hver pakke avsluttes med et internt kontrollpunkt og overgang til neste nødvendige arbeid. **Ikke avslutt samtalen etter pakke 1 eller etter at én ny mekanikk fungerer.**

Hold en kort arbeidsstatus med de åtte kriteriene, ferdig arbeid, største åpne feil og neste kjøring. Ikke erstatt dette med mange parallelle planer. Ved kontekstkomprimering skal du lese status og fortsette samme milepæl, ikke starte prosjektanalysen på nytt.

Omfanget er hele det beskrevne kapitlet. Hvis en reell kapasitets- eller tilgangsgrense stopper økten, lagre nøyaktig byggbar status og neste handling. Et checkpoint kan leveres som checkpoint; uferdig arbeid skal ikke omdøpes til et ferdig kapittel. Ikke lov videre bakgrunnsarbeid uten en faktisk kjørende jobb.

## 12. Bevis at riktig spillversjon fungerer

Gjenbruk eksisterende automatisering der den passer. Les `Automation/run-field-camera.sh`, `Automation/run-pass06.sh`, `Automation/gauntlet-user-journey.py`, byggeskriptene og observasjonskoden før utvidelse. Noen skript er laget for bestemte testkopier og gammel reiselengde; kontroller arbeidsmappe, lås, tidsgrenser og resultatfiler før kjøring. Ikke la en gammel rapport oppfylle en ny test.

Behold relevante gamle regresjoner og legg til tester for de risikofylte endringene: progresjonsoverganger, bevisidentitet, filmtilstand, ugyldig hypotese, avbrudd og lagre/last. Tilpass gamle forventninger når oppførselen bevisst endres, for eksempel umiddelbar fotovisning. Dokumenter begrunnelsen; ikke slett en test bare fordi den oppdager et problem.

Gjennomfør en reell normalstart-til-slutt-reise med tastatur/mus. Observasjon av spilltilstand er tillatt; direkte kall som løser oppgaver eller teleporterer spilleren kan brukes i isolerte tester, men skal ikke telle som bevis på brukerreisen. Test minst én feil hypotese, en ugunstig handlingrekkefølge og de relevante valggrenene.

Prøv lagring ved å avslutte prosessen og starte samme leverte program på nytt. Kontroller både verdensstatus og hva spilleren ser og kan gjøre etterpå. Test også pause under en tidsstyrt handling, gjenåpning av bevis, fokusbytte, menyknapper og omstart. Ingen skjult utviklertilstand skal være nødvendig.

Inspiser originalbilder fra åpning, instrumentbruk, fotolab, begge feltobservasjoner, bildesammenligning og avslutning. Kontroller relevante bevegelser og overganger gjennom kjøring, opptak eller en bildesekvens. Faste inspeksjonskameraer skal merkes som slike; de beviser ikke tilgjengelig gangrute.

Mål et release-bygg med samme innstillinger som sluttbildene. Bruk prosjektets eksisterende referansebetingelser dersom maskinen fortsatt samsvarer. Etter minst 20 sekunders oppvarming måles minst 120 sekunder med representative krevende handlinger; inkluder de nye delene, reell fotoeksponering/lagring og lab-/sluttoverganger. Kontroller hele kapitlets rute for alvorlige stopp, selv om den kvantitative sammenligningen bruker et kortere representativt utsnitt.

Viderefør de etablerte grensene med oppgitt toleranse: gjennomsnitt minst 59 fps, p95 ≤17,2 ms, p99 <20 ms og ingen intervaller over 50 ms i den definerte representative målingen. Et 75 fps-tak er ikke et måleresultat. Oppgi maskin, OS, renderer, oppløsning, preset, oppdateringsfrekvens, synkronisering, utvalg, varighet og percentilmetode. Behold alle målte intervaller; fang QA-skjermbilder i en separat runde. Synlig lastetid ved eksplisitt innlasting rapporteres også, med på forhånd angitt måleavgrensning.

Bruk profilering før optimalisering. GPU-tid skal stå UNVERIFIED dersom måleverktøyet ikke leverer gyldige data. Engine-minne er ikke automatisk totalt prosessminne. Ingen ytelsespåstand skal gjelde andre maskiner eller innstillinger enn de som er målt.

Knytt hver sluttrapport til commit og en entydig hash av relevante kilde-/byggfiler. Oppgi lokale avvik. Endringer etter kjøringen krever nye berørte kontroller. Bruk PASS, FAIL og UNVERIFIED per kriterium; N/A krever en konkret grunn. Spillfølelse og oppdagelsesverdi krever egen vurdering, ikke bare grønne kodekontroller. Ikke oppfinn ekstern spillertesting.

## 13. Lever den kontrollerte versjonen og bevar arbeidet

Bruk eksisterende dokumentasjonsstruktur. Oppdater README og `Docs/CURRENT_HANDOFF.md`, og samle kapitlets design, viktige rettelser, åpne feil og testresultater i en kort kapittelrapport. Oppdater asset-/lisensoversikt og skill-notater der faktisk arbeid tilsier det. Synkroniserte referanser under `sources/` skal fortsatt være urørt.

Bevar kode, originalressurser, redigerbare Blender-filer, eksportinnstillinger, Unity-metadata og relevante bevis i den etablerte versjonsflyten. Store midlertidige bygg og cache skal ikke fylle kildehistorikken. Behold forrige fungerende spillpakke.

Lag en entydig lokal kapittelpakke under `Artifacts/Releases/` med byggidentitet, nødvendige runtime-filer og lisenskreditering. Kontroller pakkens innhold, rettigheter og oppstart ved å pakke den ut i en ny mappe og bruke normal start. Oppdater eller lag en enkel launcher som peker på akkurat denne utgaven. Ikke lever en gammel pakke bare fordi filnavnet fortsatt ser riktig ut.

Følg etablert privat repo-flyt for branch, push og et samlet, forståelig PR-utkast når tilgangen finnes. Gjør kildeendringer og bevis klare til gjennomgang. Denne arbeidsordren gir ikke ny autorisasjon til merge, offentlig publisering, opplasting til en ny tjeneste eller kjøp. Tidligere merge av PR 1–5 er historikk, ikke automatisk klarsignal for nye PR-er.

Sluttsvaret skal være kort: hva jeg nå kan spille, de viktigste forbedringene, den nøyaktige startveien, hva som er kontrollert, målebetingelser, gjenstående begrensninger og commit/bygg/PR. Vis ett eller to faktiske spillbilder når det er mulig. Bruk betegnelsen spillbar alpha eller ferdig første kapittel bare når kriteriene støtter det. Ikke kall dette hele det planlagte SIGNAL / 47-spillet.

## 14. Begynn arbeidet nå

Bekreft riktig prosjekt og status. Gjør den korte brainstormen, bestem de konkrete sporene og kapittelets handlingsforløp, registrer de åtte kriteriene og gå direkte over i bygging. Bruk subagenter der de gir reell framdrift. Få hele reisen til å fungere, fullfør innhold og presentasjon, rett svakhetene og kontroller den faktiske leveransen.

**Målet er at jeg kan spille et merkbart rikere, sammenhengende SIGNAL / 47-kapittel. Fortsett til den leveransen er klar, eller dokumenter den konkrete ytre begrensningen som faktisk stopper videre arbeid.**

---

### Kilder for arbeidsmetodene og status

Denne arbeidsordren er tilpasset prosjektet, ikke en uendret kopi av eksterne skills. Brainstorming ble lest fra originalkilden ved utarbeidelsen; det ble ikke installert eller prøvd som en ny lokal integrasjon i denne skriveoppgaven. Dream Loop ble lest som metode; ingen ny spillbygging eller bildegenerering er utført som del av selve promptarbeidet.

- Lokal Gauntlet: `/home/tombonator3000t/.codex/skills/gauntlet-loop/SKILL.md`, med `references/visual-and-performance.md`; tidligere lest i denne prosjektoppgaven.
- [Brainstorming — obra/superpowers](https://github.com/obra/superpowers/blob/b36e0829c6d0140e93cfef2ca599b1b07d4a7797/skills/brainstorming/SKILL.md). Originalmetoden har designgodkjenning før implementering; denne bestillingen delegerer rutinevalg eksplisitt og tilpasser arbeidsflyten.
- [Dream Loop — hovedinstruksjon](https://github.com/achimala/dream-loop/blob/9bddb901f7d071cfefdd21e264267c757177a9df/SKILL.md), [visuell vurderingsløype](https://github.com/achimala/dream-loop/blob/9bddb901f7d071cfefdd21e264267c757177a9df/references/pro-mode/workflow.md) og [3D-ressurser](https://github.com/achimala/dream-loop/blob/9bddb901f7d071cfefdd21e264267c757177a9df/references/pro-mode/assets-3d.md). Metodevalget er ikke en påstand om abonnement eller tilgjengelige modeller.
- Prosjektfasit ved utarbeidelse: `Docs/CURRENT_HANDOFF.md`, pass05–pass08, tilhørende verifikasjonsfiler, aktuell Git-status og den skrivebeskyttede originalfortellingen. Kontroller nyere endringer ved oppstart.
