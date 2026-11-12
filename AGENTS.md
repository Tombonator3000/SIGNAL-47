# SIGNAL / 47 — arbeidsinstruks

## Gjeldende retning, 22. oktober 2026

Tom har bestemt at alt videre spillarbeid kun gjelder Unreal-versjonen i `unreal/`. Nettversjonen og Unity er historiske referanser. Følg `unreal/AGENTS.md` for nytt arbeid; ikke videreutvikle eller publiser web-versjonen.

Svar på norsk. Dette er kildekoden til SIGNAL / 47, et spill om kosmisk etterforskning ved SARO i New Mexico, 1986. Bevar Blender-kilder, spillkonstanter, fungerende interaksjoner, originale assets og lisenskreditering.

## Gjenbruk av modeller

Tom har 31. oktober bestemt gjenbruk først: søk i eksisterende prosjektassets, Tripo-modeller og bibliotek før ny modellering. Følg den konkrete arbeidsflyten i [Unreal-reglene](unreal/AGENTS.md); bevar fungerende verdensplasseringer og spillankre ved utskifting.

## Historiske spor

`web/` (three.js) og `Unity/` er arkiv og referanse. Reglene i `web/AGENTS.md` og resten av denne fila gjelder bare hvis Tom ber om arbeid der.

## Finn gjeldende sannhet

Kontroller faktisk Git-/PR-status og repoets synlighet. ChatGPT-prosjektets `sources/` er skrivebeskyttede historiske referanser; gammel omtale av privat repo, PR1 eller neste produksjonspass overstyrer ikke nyere kontrollert status. Ikke rediger synkroniserte referanser. Hold kilde, lokal kandidat, pakket bygg og standardstarter atskilt.

## Fullfør det autoriserte oppdraget

Bruk eksisterende beslutninger. Brainstorming- og Gauntlet-skillene kan brukes når de passer. Skill-valg, designnotat, ny gren eller et ferdig delsteg utløser ikke automatisk krav om ny brukergodkjenning.

Kartlegg hele bestillingen og senere korrigeringer, og fullfør det som er bestilt, inkludert PR og levering når det hører med. Avslutt valgfri polering når kravene er møtt. En vurdering av et verktøy gir ikke grunn til en ubedt motormigrering eller ny spillfunksjon.

Brukeren har i SIGNAL / 47-samtalen godkjent vanlig PR og merge uten egen forhåndsgjennomgang. Bruk relevant eksisterende autorisasjon, verifiser riktig head og påkrevde kontroller, og kontroller faktisk resultat. Dette gir ikke rett til å omgå beskyttelser, kjøpe, endre synlighet, kontakte nye mottakere eller publisere spillutgaver utover gitt autorisasjon. Nyere brukerbeskjeder går foran.

## Størrelse på videre leveranser

Brukeren ba 13. september om større inkrementer og subagenter. Ved videreføring: samle en hel sammenhengende spillerreise med nødvendige kilder, handlinger, tilbakemelding, lagring og levering. Deltester er interne checkpoints; ikke avslutt ved bare én modell eller én kosmetisk rettelse når den bestilte reisen gjenstår. Prioriter hele neste feltsekvens/kapittel fremfor nye verktøypass uten produksjonsbehov. Bruk subagenter innenfor gjeldende autorisasjon med tydelige filansvar; bygg og grafiske tester kjøres fortsatt sekvensielt på denne maskinen.

## Avhengigheter og bevis

Utilgjengelig skrivebord eller testutstyr blokkerer den berørte testen, ikke alt utviklingsarbeid. Bruk eksisterende Unity-/Blender-automatisering og isolerte testprofiler der dette gir nyttig bevis. Ikke omgå skjermlås eller gjenta forespørsler om en allerede kjent utilgjengelig handling. Bevar kjent fungerende standard når kandidaten mangler nødvendige tester for å bli standard.

Under utvikling gjelder «Iterer først, test tungt mot slutten» i `unreal/AGENTS.md` (Tom 30. oktober 2026): bygg og én rask sjekk, ingen kvitteringsløyper. Ved installasjon og release: skill konseptbilder, Blender-render, runtime-bilder, API-scenario, ekte input og ytelsesmåling, og merk PASS/FAIL/UNVERIFIED med faktisk grunnlag. Merge lukker ikke uutførte tester. En designbibel for 5–6 timer er ikke implementert eller målt spilletid. Oppdater gjeldende overlevering med resultat, bevis og nøyaktige gjenstående avhengigheter; ikke kall hele spillet ferdig fordi ett steg er ferdig.

## Gjeldende dokumentasjon

Start i [Docs/README.md](Docs/README.md). `Docs/Active/STATUS.md`, `TODO.md` og `DECISIONS.md` er de eneste samlede kildene til leveringsstatus, aktiv arbeidsliste og beslutninger. `unreal/OPPGAVER.md` fører koordinering, `unreal/log.md` historikk. Følg [dokumentreglene](Docs/AGENTS.md); ikke opprett parallelle «current»-filer eller kopier gamle statuspåstander inn i nye oversikter.

## Fast regel: undersøkelse, sniking, jakt og flukt i bygg

Bestemt av Tom 12. november 2026. Gjelder alle nye og reviderte plansjer og bygg i SIGNAL / 47, inkludert konsept, romplan, blokkering og ferdig miljø.

- Planlegg bygget for at spilleren undersøker noe og kan bli oppdaget eller jaget. Vis formålet med rommene og veien inn, mellom spor og ut.
- Viktige undersøkelsesområder skal ha minst to brukbare rutevalg: dører mellom rom, rundløyper, alternativ korridor eller troverdig ekstra utgang. Hovedkorridoren og samme trapp skal ikke være eneste vei ut av alle områder. Flere dører alene er ikke nok; tegn sammenhengende ruter gjennom faktiske åpninger.
- Legg inn forbindelsesdører mellom naturlig tilstøtende rom. Unngå en serie rom med bare én dør tilbake til samme jaktkorridor. Behold troverdig funksjon og privatliv; ikke lag gjennomgang mellom toalett og klasserom bare for å få flere dører.
- Lag siktbrudd, skjul og plass til å snike rundt møbler uten å sperre ganglinjene. Gjennomsiktig glass eller lav dekor gir ikke automatisk skjul. Skjul bryter sikt; det gir ikke automatisk immunitet mot en trussel som så spilleren gå inn.
- Spre innganger og utganger slik at spilleren kan skifte retning og slippe unna. Når byggets størrelse tillater det, gi kjeller og andre utsatte etasjer en alternativ forbindelse til uteområdet. En liten enromsbygning kan løse dette med to utganger og utvendig rute.
- Fluktruter skal fungere i begge retninger. En påkrevd flukt skal ikke plutselig kreve en ukjent nøkkel, en uvarslet interaksjon eller en mekanikk spillet ikke har. Marker eventuelle låser og enveispunkter, og bevar en lesbar alternativ rute når jakt kan starte.
- Plansjer skal vise dørenes veggåpninger, forbindelser, etasjer/trapper, utganger, rundløyper, siktbrudd og foreslåtte skjul. Tekst og bilde må være enige. Angi presise forbindelser og mål i byggegrunnlaget hvis bildet er skjematisk.
- Kontroller ruten med blokkert hovedinngang og blokkert hovedtrapp før detaljering. Test deretter med spillerens faktiske kollisjon, dører, trapp, sikt og eventuell fiende i Unreal. En romgraf er en designkontroll, ikke bevis på fungerende jakt eller AI.
- Rolige rom og avgrensede blindveier kan brukes bevisst. Ikke legg obligatoriske spor i en felle uten en kjent løsning. Bygget skal støtte mulig jakt; dette pålegger ikke en jakt i alle bygg.

Eksempel: [Caliche-skolen V2](Docs/Design/World/CalicheSchool_V2/BYGGESPESIFIKASJON_V2.md). Se også [designbibelen](Docs/DesignBible-2026-10-27/DESIGNBIBEL.md).

## Fast regel: viktige gjenstander lages etter referansepakke (Tom 12. november 2026)

Gjelder viktige gjenstander spilleren går tett på (telefonkiosk, pumper, SARO-konsoller, lekeplass, sentrale rekvisitter). Bygg følger i tillegg byggeregelen over.

1. **Referansepakke først**, laget av ChatGPT: heltebilde, ortografisk firevinkelsark og et ark med trykte etiketter og skilt. Bare oppdiktede merker; aldri ekte merker, og aldri Cosmos-modeller eller -bilder som referanse.
2. **Én gjenstand om gangen**, modellert av Fable etter arket: ekte avfasninger, kurver, bulker og slitasje i formen; trekantbudsjett etter bruk (omtrent 5 000–20 000 for en heltegjenstand).
3. **Bakte PBR-kart** fra en detaljert modell: farge, normal og ruhet/metall/AO i 4K (2K for små ting); slitasje fra krumning og AO; etiketter som egen tekstur.
4. **Kritikk i spillet:** bilder fra spillet side om side med referansen, høyst fire runder, og bilder til Tom underveis.
5. **Tid:** sett av timer per gjenstand, ikke minutter, og bland ikke gjenstander inn i en byggerunde.

## Fast regel: animasjon av NPC-er, aliens og dyr (Tom 12. november 2026)

Gjelder all ny og endret animasjon i spillet. Byggeregelen og regelen for viktige gjenstander over gjelder i tillegg for modellene.

1. **Velg kilde etter type bevegelse.**
   - *Mekanisk og prosedyrisk* (antenner, dører, luker, bilen, tentakler, pust, flimmer, hode som følger noe): lages med kode eller nøkkelrammer av Fable i Blender eller direkte i spillet.
   - *Gange, løp, sniking, sitting, gestikulering for mennesker, humanoide aliens og dyr*: start fra ekte bevegelse (opptak eller lisensierte/egne klipp med kjent lisens), og la Fable tilpasse (retarget), rense og bake den til vårt skjelett. Ren håndlaget nøkkelramme-gange for figurer brukes bare som blokkering, aldri som ferdig.
   - Klipp uten avklart lisens (for eksempel Free100Animations) holdes utenfor Git og pakker til lisensen er klar.
2. **Brief med tall:** skjelett og navn på bein, lengde i sekunder og bilder per sekund, løkke eller ikke, rotbevegelse (root motion) eller på stedet, fart i m/s for gange, og hvilke overganger klippet må passe med.
3. **Se før ferdig:** bilder fra flere vinkler og en kort video (front, side, ovenfra; fot­kontakt synlig) for hver runde. Ingen «ferdig» uten at noen har sett bevegelsen.
4. **Sjekk:** føtter som ikke glir eller går gjennom bakken, hender og skuldre uten knekk, løkkepunkt uten hopp, riktig fart mot rotbevegelsen, ingen skala på beina, samme ramme i Blender og Unreal.
5. **Prøv i spillet:** samme sekvens før og etter i Unreal (`Tools/animals/check_rig.py` og lignende), kollisjon og fotlyder, og at atferden (AI) bruker klippet riktig. Spillet er målstreken, ikke Blender.
6. **Én figur og ett klipp om gangen**, opptil fire kritikkrunder med bilder/video til Tom; bevar dagens fungerende klipp til det nye er godkjent.
7. **Stil:** troverdig 1986-realisme for folk og dyr; aliens etter designbibelen (stille, rolige, få bevegelser, aldri tegneserieaktig).
8. **Kilder vi skal prøve først** (se [gratisressursene](Docs/Research/FREE_ASSET_CATALOG.md)): *Free100Animations* (`~/Assets/signal47-free/_inbox/free100animations-20261111/`, Manny/Quinn: gange og jogg i åtte retninger, tomgang, hopp, fall, treff, død; lisens ikke oppgitt – prøves lokalt, aldri i Git eller pakke før vilkårene er avklart); *MetaHuman Animator Markerless Motion Capture* (video til animasjon; krever Toms Epic-innlogging på Fab); Quaternius CC0-dyr (ulv/husky, i bruk). Prøv dem før noe lages fra bunnen av.
