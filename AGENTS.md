# SIGNAL / 47 — arbeidsinstruks

## Gjeldende retning, 22. oktober 2026

Tom har bestemt at alt videre spillarbeid kun gjelder Unreal-versjonen i `unreal/`. Nettversjonen og Unity er historiske referanser. Følg `unreal/AGENTS.md` for nytt arbeid; ikke videreutvikle eller publiser web-versjonen.

Svar på norsk. Dette er kildekoden til SIGNAL / 47, et spill om kosmisk etterforskning ved SARO i New Mexico, 1986. Bevar Blender-kilder, spillkonstanter, fungerende interaksjoner, originale assets og lisenskreditering.

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
