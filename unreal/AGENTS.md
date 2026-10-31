# SIGNAL / 47 unreal: arbeidsinstruks

Denne fila gjelder alt under `unreal/`. For denne mappa erstatter den Unity-reglene i rotens `AGENTS.md`. Les `README.md`, `../Docs/Active/STATUS.md`, `../Docs/Active/TODO.md`, `../Docs/Active/DECISIONS.md`, `../Docs/Active/Guides/PORTERING.md` og de siste oppføringene i `log.md` før du starter, og kontroller faktisk Git-status.

- Unreal-utgaven er spillet (Toms beslutning 22. oktober 2026). `web/` er historisk referanse og endres ikke.
- `Unity/` er arkiv og referanse. Modellene der leses av importskriptet, men ingenting i `Unity/` redigeres.
- Repoet er offentlig. Kjøpte eller lisensierte assets legges ikke inn. Modellene i `Unity/Assets/Signal47/Art` er egne (Blender) eller CC0 (Poly Haven); se `web/THIRD_PARTY_NOTICES.md` og manifestene ved siden av modellene.
- Unreal: logikken er portert fra `web/src` (se `PORTERING.md`). Nye forbedringer (lys, modeller, mekanikk) er ønsket; når noe avviker fra nettversjonen, si det i en kort kommentar. Verdenen bygges fra eksporten i `Tools/webexport`, men kan byttes ut eller forbedres der Tom ber om det. Binærfiler (uasset) går gjennom Git LFS. Binaries, Intermediate, Saved, DerivedDataCache og de genererte mappene under Content (`Web`, `WebData`, `Audio`, `Licensed`) committes aldri.
- Bygg ikke editor-målet mens editoren er åpen.
- Test etter risiko, bestemt av Tom 28. oktober 2026, skjerpet 30. oktober: under utvikling er fart og iterasjon viktigere enn bevis. Én tung jobb om gangen og ingen tester samtidig med Toms spilløkt.

## Gjenbruk først, bestemt av Tom 31. oktober 2026

- Før ny modellering: søk i prosjektbiblioteket, eksisterende prosjektassets, Toms Tripo-modeller og eide originalkilder. Søk deretter i relevante eksterne assetbiblioteker når lokale modeller ikke dekker behovet. Bruk gjenbruk og tilpasning når det gir best samlet resultat.
- Codex koordinerer søk, valg, tilpasning, plassering, lys, rigging, eksport og retting. Tom vurderer uttrykk, spillfølelse og komfort; ordinær feilretting forblir agentens arbeid.
- Standardrekvisitter bør normalt gjenbrukes. Ny eller prosedyrisk geometri er riktig når spillmekanikk, særpreg, nøyaktig passform eller samlet tidsbruk tilsier det. Ingen absolutt regel om aldri å bygge en stol.
- Velg modeller som passer New Mexico i 1986, spillets materialuttrykk og ytelsesbudsjett. Kontroller lisens og kilde per asset; gratis nedlasting er ikke automatisk fri offentlig redistribusjon. Eksisterende regel om private/lisensierte assets utenfor offentlig Git gjelder fortsatt.
- Bruk fungerende Blender-skript/bpy, CLI eller MCP etter behov. Et bestemt modellnavn, high effort, MCP eller BlenderKit er ikke et krav. Ved bruk av en tilkobling eller et bibliotek: bekreft faktisk sceneadgang eller gjør et lite søk før arbeidet bygger på tilgjengeligheten. Ingen ny installasjon eller betaling følger automatisk av denne regelen.
- Bevar redigerbare kilder og originale rigger. Kontroller skala, akser, pivot, normaler, UV-er, materialer, kollisjon og LOD ved eksport/import. Alle detaljnivåer skal ha samme koordinatramme; kontroller faktisk geometri og plassering også når avstanden endrer LOD. En rigg er først teknisk prøvd når relevante bevegelser og deformasjoner er sett i Unreal.
- Teknisk utskifting skal bevare eksisterende verdensplassering, rotasjon, terrengkontakt og spillankre. Kontroller bygg, tilhørende dørblad/hengsler, gulv, rekvisitter og interaksjoner samlet. Ved utskifting av flere bygg: sammenlign hele bygningsgruppen mot forrige fungerende plassering og orientering. En prøve av usynlig kollisjon eller like trekanttall beviser ikke at den synlige modellen står riktig.
- Material-/lysarbeid kan bruke [lookdev-pakken](Signal47/Tools/lookdev/README.md): eksakte objekter/slotter, private kildekopier, selektive masker og bevart normal-/UV-/rigggrunnlag. Bakeresultater og native Unreal-glass/volumlys holdes atskilt; Blender-prosedyrenoder og tåke følger ikke automatisk med FBX. Dag og natt har egne profiler. Previewriggen erstatter ikke automatisk spillets himmel-, tids- eller strømdrivere.
- Bruk en rask lavoppløst forhåndsvisning før kostbar rendering. Endelig kontroll for spillet skjer i Unreal, med før/etter som dekker endringen. Dette inngår i den eksisterende raske sjekken, ikke en ny full testmatrise. Ingen tung jobb under Toms spilløkt.

## Iterer først, test tungt mot slutten (Tom 30. oktober 2026)

Tom: «Alt for mye testing, nå mer iterering, forenkle og ta tunge tester heller mot slutten av ferdig spill.»

| Når | Hva som kreves | Hva som ikke kreves |
|---|---|---|
| Vanlig endring (kode, scenario, modell, lys, rom) | Bygg pluss én rask sjekk av det som er endret: én kontrakt, ett skjermbilde eller én kort kjøring. | Kvitteringer, SHA-er, frosne kildefiler, ferske lesere, private kø-serier, PASS-rapporter, full regresjon. |
| Visuelt arbeid (rom, modeller, materialer, lys) | Før- og etterbilde. Iterer mange ganger på kort tid; vis Tom bildet når det er et tydelig steg. | Måling av hver flate, materialaudit per objekt, egne bevisdokumenter. |
| PR og merge | Bygg og den raske sjekken over. Små, hyppige PR-er. | Full regresjon eller fullmatrise per PR. |
| Lokal installasjon så Tom kan spille | Pakking, selvtest og en kort røyktest av det nye. | Fullmatrisen. |
| Slutten av en akt | Alle scenariokontrakter én gang samlet. | |
| Før offentlig release eller når Tom ber om det | Fullmatrisen og helnattskontrakten. | |

Logg kort: hva, gren, resultat, i én til tre linjer i `log.md` eller overleveringen. Ingen egne leveringsrapporter under `Docs/Evidence/` for vanlig arbeid; bare for installasjon og release. Når noe feiler: rett det og gå videre, uten å bevare og dokumentere hvert mislykket forsøk. Det som fortsatt gjelder: Saved og personlige lagringer røres ikke, ingen offentlig release eller endring av repoets synlighet uten Tom, kjøpte og lisensierte assets holdes utenfor Git.

- Kanon fra `web/memory.md` gjelder: 1420.405 MHz, 47 sekunder, 27 antenner, az 026. Spilltekst på engelsk (New Mexico, 1986). Dokumenter og logg på norsk, uten emoji og uten tankestrek.
- Felles oppgaveliste og arbeidsdeling med Codex: `OPPGAVER.md`.
- Logg vesentlige resultater i `log.md` med dato og klokkeslett (Europe/Oslo). Oppdater berørt hovedfil i `../Docs/Active/`; `memory.md` og `todo.md` er korte pekere og skal ikke fylles med nye kopier. `OPPGAVER.md` er bare koordinering/eierskap. Følg `../Docs/AGENTS.md` for kategorier, statusmerking og lenkekontroll.

## PR-er går mot main, bestemt 27. oktober 2026

- Alle PR-er for spillet går mot `main`. Lag nye grener fra oppdatert `origin/main`, ikke fra en annen arbeidsgren.
- Sjekk grunnlagsgrenen før fletting, for eksempel med `gh pr view <nr> --json baseRefName`. Er den ikke `main`, rett den med `gh pr edit <nr> --base main` før fletting. Før fletting gjelder «Iterer først» ovenfor: bygg og én rask sjekk.
- `unreal-port-core` var avtalt integrasjonsgren for historiearbeidet fram til 26. oktober (`HISTORIE_IMPLEMENTERINGSPLAN.md`), og PR #147 til #166 og #168 gikk dit med vilje. Main fikk det meste via PR #167, men de 17 siste commitene (PR #163 til #166 og #168, med SC01) ble aldri flettet videre. Det er det regelen over skal hindre: ingen arbeid skal bli stående på en sidegren. Flettingen inn i main står øverst i `OPPGAVER.md`, og `unreal-port-core` slettes når den er flettet.

## Varig grense for lokale testbygg, bestemt 25. oktober 2026

- Behold gjeldende verifiserte bygg og maksimalt **to eldre komplette testbygg** lokalt. Installerte standardfiler beholdes alltid. Aktive pakkekjøringer, testkandidater som fortsatt undersøkes og nødvendige sammenligningsbygg beskyttes til oppgaven er avsluttet; tell og rydd dem når de frigis.
- Etter vellykket pakking og avsluttet test/levering: kjør `python3 Signal47/Scripts/prune-test-builds.py --current /absolutt/sti/til/gjeldende/Linux` fra `unreal/`. Oppgi andre aktive kandidatmapper med `--protect`. Verktøyet lager en tørrkjøringsplan og beholder de to nyeste eldre pakkene. Kontroller planen og kjør samme kommando med `--apply` når ingen Unreal-/spill-/pakkejobb er aktiv. Dette er vanlig autorisert vedlikehold; regelen krever ikke en ny brukerforespørsel for hver opprydding.
- Verktøyet pensjonerer bare genererte pakkecontainere og den tilhørende spillbinærfilen/debugfilen i gamle `Signal47*`-arbeidsmapper under `~/scratch-bm`. Kilder, Blender-/modellfiler, assets, `Saved`, personlige lagringer, logger, skjermbilder, testbevis, Engine-filer og selve mappeskallet bevares. Gamle lette mappeskall teller ikke som komplette testbygg. Slett aldri hele blandede arbeids-/bevismapper bare på grunn av navn eller alder.
- Ikke rydd en kandidat mens den bygges, kjøres, testes eller er nødvendig for en åpen undersøkelse. Verktøyet nekter `--apply` ved aktive Unreal-/spill-/pakkeprosesser og avviser symlink-/filendringer. Ved påkrevd midlertidig unntak fra antallsgrensen: dokumenter hvilke bygg som er beskyttet og hvorfor, og fullfør oppryddingen når behovet bortfaller. Før faktisk resultat og gjenværende bygg i oppdragets logg; tørrkjøring er ikke utført sletting.
