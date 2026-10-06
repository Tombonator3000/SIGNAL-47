# PR78: kodehash-feil blokkerer opptak og pakking

6. oktober 2026, oppfølging av [P2-reviewet](https://github.com/Tombonator3000/SIGNAL-47/pull/78#discussion_r4193548943).

En lesefeil i `response.body()` ble tidligere ført som warning. Dermed kunne et opptak bli CAPTURED og pakkekontrollen bestå med et ufullstendig koderegister. Rettelsen gjelder eget opptaksverktøy og pakker, uten endringer i spillet.

Begge opptaksrutene registrerer hver observerte dokument-/skriptrespons før den asynkrone lesingen starter, også gjentatte URL-er. Hver oppføring går fra pending til hashed, failed eller cancelled. Feil, manglende hash og avvik mellom responslisten og hashkartet blokkerer opptaket. Ventingen inkluderer oppgaver som kommer til mens andre avventes.

Ved avslutning lukkes nettleserkonteksten, observerne kobles fra og alle registrerte oppgaver avventes før endelig manifest skrives. Konsoll- og HTTP-grensen kontrolleres på nytt. Hvis lukking eller venting feiler, kanselleres og avventes resterende oppgaver, opptaket får FAIL og den første feilårsaken beholdes. Det eksakte same-origin favicon-404-unntaket er bevart.

Ny policy `observed_responses_v1` krever en fullstendig responsliste og bekreftet lukket observasjon. Pakkeren kontrollerer hver respons mot hashkartet, som igjen kontrolleres mot Pages-artefaktet. Ukjent/null policy, uferdige/feilformede oppføringer og historiske hash-lese-warnings blokkerer pakking.

## Kontroller

- [13 målrettede regresjonstester](code_hash_checks.json): PASS. Begge opptaksrutene, gjentatt URL med senere feil, tilkomne oppgaver, timeout/kansellering, sen respons/konsollfeil, lukking som feiler, ny/mangelfull policy, pakkekontroll og CLI-exit 1. Rute og skjermbilder i disse testene er mocks, ikke spillbevis.
- [To små ekte Playwright-kontroller](code_hash_browser_checks.json): PASS. Faktiske dokument-/skriptbytes matches, responslytter fjernes etter lukking, og en tvungen body-lesefeil på en faktisk skriptrespons blokkerer. Ingen spillkjøring eller grafikkkontroll.
- Den eksisterende konsollregresjonen: 11 av 11 PASS. Python-syntaks og diffkontroll: PASS.
- Seks originale opptak revalidert uten å skrive i originalene: 794 av 794 kontroller av registrerte hasher, bilder og metadata PASS. Alle 84 registrerte kodefiloppføringer samsvarer med det uendrede Pages-artefaktet. De 234 PNG-ene og seks råmanifestene er uendret.

## Grense for de gamle opptakene

**Full responsdekning er UNVERIFIED for de seks opprinnelige opptakene.** De ble tatt med verktøyhash `b013332bda5d4eb8a48c8eb8181c1352c89c8ca2a03602e1619f64bce758e20a` og har ingen responsliste eller avslutningsmarkør. Fravær av hash-warnings beviser ikke at alle asynkrone oppgaver ble avventet. Ingen responsliste er rekonstruert fra hashkartet.

Tidligere rapporter og 788/788-resultatet er historiske. Den nye revalideringen gjelder registrerte kodehasher og bilde-/metadataintegritet, ikke full nettverksdekning. Bildene er fortsatt grunnlag for den manuelle K01-vurderingen; funnet får ingen ny automatisk visuell PASS.

Ingen ny tung spilltestrunde, assets, runtime- eller testendringer. Claude eier K01/lysrettelsen og den allerede avtalte samlede integrasjonsrunden.
