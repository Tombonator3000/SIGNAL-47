# Lys ved dineren: opptaksverktøy og kontrollgrunnlag

Status: PREPARED. Spillopptak og visuell vurdering er UNVERIFIED. Ingen bilder er produsert i denne klargjøringen.

Claude bestilte kontrollen 6. oktober 2026 i melding 100 i den autoriserte økten. Verktøyet bygges mot main `ea6bdd0`, mens Claude retter dobbel belysning i Ultra og kjører den avtalte integrasjonsrekka. Opptaket venter på neste Pages-leveranse og Claudes bekreftelse. Den gamle leveransen fra PR #79 skal ikke brukes til å bedømme rettelsen.

## Avtalt opptak

- Kapittel 5: faktisk kjøretur fra SARO til dineren med kapitlets nattklokke. Framoverbilder hver 40. meter i de siste 300 meterne av den lastede kjøreruten.
- Innkjøring: fasade og skilt fra faktisk kjørt pose før parkering, med frontlysstatus dokumentert. Det røde lyset på panseret og frontlys mot fasaden vurderes i disse bildene.
- Parkering: fasade og skilt med frontlys av. Et eventuelt holdt førerhusblikk etter parkering merkes som en egen kameradiagnose og ikke som vanlig fotkamera.
- Kapittel 6: de første 300 meterne ut fra dinerplassen ved kapitlets daggry, målt langs faktisk kjørt bane. Oppsettshopp til kapitlet merkes; ingen plassering brukes på selve kjøreruten.
- Ultra: egen diagnose til fots på plassen gjennom `world.enter('diner')`. Ved ankomst brukes den faktiske posisjonen etter at spilleren går ut. Etter den fullførte avgangsruten brukes et eksplisitt diagnostisk oppsett: `stopDriving()`, `enter('diner')` og `placeAtDiner('arrive')`. `stopDriving()` setter bilene tilbake der turen startet. Posisjoner før og etter dette oppsettet dokumenteres; det er ikke fortsettelse av kjøreruten. En kort tidsserie dokumenterer falske lysplasser og ekte spotlys for manuell vurdering av dobbel belysning og lys ved bilens gamle pose.
- Seks oppsett: Low, High og Ultra i 844x390 og 1280x800. Desktop-kontekst, DPR 1 og nivåets vanlige bildeinnstilling. Telefonstørrelsen er ikke en test på fysisk telefon.

Parkering slår av frontlysene i runtime. Et parkert bilde kan derfor ikke dokumentere hvordan tente frontlys treffer fasaden. Bilder før og etter parkeringen må ha forskjellige etiketter og metadata. Den parkerte kameradiagnosen viser midlertidig kabinen og skjuler bilens skall for å gi utsyn. Den endrer ikke kjøring eller frontlys og gjenoppretter synlighet og kamera, også ved opptaksfeil.

## Kjøring etter leveranse

Kjør hvert oppsett etter tur, med en ny utdatakatalog. Sett `S47_URL` til den kontrollerte Pages-leveransen og `S47_CHROMIUM` til den installerte nettleseren ved behov.

```sh
python3 tools/drivelook.py shots/dinerlight_low_844x390 844x390 --trip dinerlight --quality low
python3 tools/drivelook.py shots/dinerlight_high_844x390 844x390 --trip dinerlight --quality high
python3 tools/drivelook.py shots/dinerlight_ultra_844x390 844x390 --trip dinerlight --quality ultra
python3 tools/drivelook.py shots/dinerlight_low_1280x800 1280x800 --trip dinerlight --quality low
python3 tools/drivelook.py shots/dinerlight_high_1280x800 1280x800 --trip dinerlight --quality high
python3 tools/drivelook.py shots/dinerlight_ultra_1280x800 1280x800 --trip dinerlight --quality ultra
```

Før opptak kontrolleres Pages-jobb, commit og faktisk lastet kode mot leveransen. Hver dokument- og skriptrespons skal ha fullstendig SHA-256 og ferdig avsluttet observasjon. Feil under responslesing, sen konsollfeil eller uventet HTTP-feil blokkerer leveringen.

## Leveranse etter opptak

Råbilder og manifest bevares. Ferdig pakke skal ha kontaktark, `review.md`, `review.json` og én samlet funnliste med bilde, koordinater, blikkretning, klokke, nivå, størrelse og alvor P1 til P3. Registrerte lyskandidater er diagnostiske data, ikke automatisk visuell PASS. Funn i runtime sendes til Claude.

## Avgrensning

Klargjøringen endrer bare Codex sitt opptaksverktøy og denne produksjonsmappen, samt eget UTC-avsnitt i loggen. Runtime, assets, eksisterende spilltester, Unity, read-only sources og Voices of the Void-arkivet bevares. Claude eier runtime-rettelsen og én samlet integrasjonsrunde. Codex gjentar ikke uendrede tunge grønne spilltester.

## Utførte kontroller

- PASS: 28 målrettede tester av rutegeometri, 40 meters intervaller, kvalitetsvalg, lysdata, to separate forløp og feil under opptak og avslutning. Runneren kjøres mot API-dobler. Én test utfører 12 scenarier med de faktiske JavaScript-hjelperne for kamera, synlighet, frosset rendertid og gjenoppretting. Resultat og filhasher står i `tool-checks.json`; kjøringen kan gjentas med `python3 tool_check.py --result /tmp/dinerlight-tool-checks.json`.
- PASS: Python-syntaks, parsing av fire nye JavaScript-uttrykk, diffkontroll og sammenligning mot main. De eksisterende Kessler-/konsoll-/hashhjelperne og den gamle rutekroppen er uendret, se `syntax_checks.json`.
- Utført: separat statisk gjennomgang fant og fikk rettet startbilde/metadata, utsyn fra parkert kabin, korrekt kilde for avreiseruten og fotdiagnosens faktiske områdesjekk. Gjennomgangen kjørte ikke spillet.
- UNVERIFIED: alle spillopptak, faktisk lys, manuell kjøring, fysisk telefon, lyd og fps på ekte maskinvare. Uendrede grønne spilltester gjenbrukes. Claude eier den pågående integrasjonsrekka for runtime-rettelsen.

Mock-kontroller gir ikke bevis for spillkjøring eller ferdige spillbilder. Lysdata sammenligner observerte posisjoner og fargetone; de leser ikke Ultras interne kildeutvalg og gir ingen autoritativ konklusjon om belysningen.
