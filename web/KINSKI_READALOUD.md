# Kinski-opplesning

Bestilt direkte av Tom 19. oktober 2026. Første arbeidsgren: `codex/kinski-read-aloud`, fra main `bada636`. Den tidligere telefonleveransen er flettet inn etter Toms beskjed. Beskrivelsesutvidelsen er bestilt direkte av Tom på `codex/kinski-descriptions`, fra main `95719a4`. GitHub Actions bygger og publiserer nettspillet ved push til main.

## Bruk

Åpne Settings på tittelskjermen eller pausemenyen, og sett **Kinski read-aloud** til **On**. I et dokument velger du **Read document** for selve papirteksten eller **Read description** for forklaringen ved siden av. I en samtale velger du **Read aloud**. Mottakerkonsollen, oppgavepanelene, bevisbordet, notatbokas Tasks, Notes, Kit, People og Map samt et åpnet fritt fotografi har **Read description**. Knappen følger teksten som faktisk vises. Bytte av fane, hint, svar eller fotografi stopper den gamle lesingen. Papirer og saksfoto åpnes og leses som før. Knappen blir **Stop reading** under lasting og avspilling. Valget huskes på denne enheten.

Hold høyre museknapp eller **Zoom** for et nærmere blikk. Etter 650 ms med samme undersøk-tekst begynner stemmen, dersom ingen annen lesing eller telefonsamtale pågår. Slipp Zoom eller se på en annen ting for å stoppe. **V**, eller knappen øverst til høyre, leser den aktuelle eller siste korte beskrivelsen på nytt. Knappen kan dermed brukes etter at en melding har forsvunnet. Radio- og telefonteksting erstatter ikke denne beskrivelsen. Kapitteloppsummeringen kan også gjentas med V etter overgangen.

Stemmen er en syntetisk Kinski-parodi, generert lokalt i VoiceStudio med OmniVoice. Ingen av innspillingene er Klaus Kinski som leser spillets tekst. Spillets eksisterende engelske tekst brukes ordrett. Kartet har i tillegg 22 korte beskrivelsesfraser av sine eksisterende steder, kryss, linjer og hendelsesmarkeringer. Radiostemmene og historien beholder sin eksisterende virkemåte.

Lydfilene følger begge byggene. Enkeltfilbygget `dist-single/index.html` har lydene innebygd og kan brukes uten nett eller en kjørende VoiceStudio. Dokumentopplesning henter klippet som spilles; opptil fire dekodede fortellerklipp og 32 MiB holdes i minnet. De 24 korte telefonklippene forhåndslastes sammen med spillets vanlige lyder, slik at svar og teksting kan starte uten en ny nettforespørsel. Hovedvolumet styrer også opplesningen.

Kinski-valget er av som standard. Papirer og vanlige samtaler leses ved knappetrykk. Telefonstemmer starter automatisk når du svarer, ringer opp eller åpner telefonsamtalen. Ved nye valg i Ward-samtalen leses bare nye replikker. Lukk panel, bytt tekst, trykk Stop reading eller åpne pausemenyen for å stoppe. En innkommende telefonstemme avbryter fortelleren. Manuelt valgt lesing kan avbryte telefonstemmen. Spilleren får beskjed dersom teksten mangler en innspilling. Endret eller dynamisk tekst erstattes aldri med en feil replikk.

## Akseptansekriterier

1. Valget er av fra start, kan slås på i begge innstillingsmenyer og huskes etter gjenåpning.
2. Dokumenter og samtaler med opptak spilles fra ekte medfølgende lydfiler, i tekstens rekkefølge, ved et uttrykkelig knappetrykk.
3. Stop, lukk og ny tekst stopper manuell opplesning, også under lasting/dekoding. Telefonen settes på vent sammen med lydklokken ved pause; den stoppes ved avslutning, nye valg, last og ny natt.
4. Vanlig bygg virker under `/SIGNAL-47/`, og enkeltfilbygget har ingen ekstern avhengighet for stemmen.
5. Manglende opptak gir synlig tilbakemelding; mobilens tekstpaneler og innstillinger kan rulles og brukes.

## Tekstdekning

Det utvidede korpuset dekker spillets eksisterende beskrivelser: undersøk-tekst på ting og antenner, notater med tidsstempler, utstyr og personer, oppgavepanelenes instrukser, hint og svar, bevisbordets synlige tekst, kartets steder og hendelsesmarkeringer, fotoenes tekster, papirer og samtaler. Korpuset har 1 555 unike klipp og 18 791 ord, totalt 45 411 332 byte MP3 og 1 time, 32 minutter og 21 sekunder. De 337 eksisterende MP3-filene er bevart uendret, kontrollert med SHA-256 mot main `95719a4`.

Hele teksttreff brukes først. Dynamiske beskrivelser settes ellers sammen av ordrette innspilte tekstbiter og en liten tallordbank. Klokkeslett, fortegn, desimaler og innledende nuller beholdes, også i antennevinkler og `-39 LY`. Den korte antenneforkortelsen `el` uttales «elevation»; signaturen `T.` uttales «Tee». Selve teksten og tallene beholdes. Ukjente ord fører til synlig beskjed om manglende opptak; en halv beskrivelse blir aldri spilt som om den var komplett. Spillet trenger ingen TTS-tjeneste under spilling.

Blyantmarkører, rene papirskillelinjer og oppgavenes avkryssingssymboler leses ikke opp. Knapplabel, feilsøkingsmenyer, tekniske feilmeldinger og rulletekstens kildeangivelser er ikke fortellerbeskrivelser. Radioprogrammets eksisterende pratelyder beholdes.

## Verifisering og kildegrunnlag

Resultatene og avgrensningen av tekstkorpuset føres i `production/kinski/` og `log.md`. Automatisk filkontroll, talegjenkjenning og nettleseravspilling bekrefter tekniske egenskaper. Hvor lik, sint og morsom stemmen høres ut, må vurderes ved lytting.

Telefonutvidelsen bruker det eksisterende telefonfilteret, 320 til 3200 Hz, og SFX-/hovedvolumet. Teksting og samtaleslutt beregnes fra hele opptakets faktiske varighet. Spillerasiden i samtalen til Nora forblir stille. Den samme Tomás-filen brukes både i hytta og på returlinjen i rom 6. Prologens taleløse anrop og nedtellingen på 47 sekunder beholder sine tider.

[Telefonmanuset](production/kinski/telefonmanus.md) samler alle 24 talereplikkene. Testgrunnlaget ligger i `production/kinski/qa/README.md`.

Prosjektbiblioteket er søkt før implementering. Gjenbrukte kilder er VoiceStudio-oppsettet og SIGNAL / 47s eksisterende `AudioSys`, dokumentvisning, samtalepaneler og innstillingslagring. Ingen ekstra TTS-tjeneste eller nettleserstemme brukes under spilling.


Tidligere telefon- og opplesningsprøver ligger i `production/kinski/qa/`. [Beskrivelsesutvidelsens sluttkontroll](production/kinski/descriptions-qa/README.md) har 115/115 Pages og 116/116 offline PASS, i tillegg til telefonprøvene og full spillregresjon. Enkeltfilbygget er 92 698 822 byte. Lytting og opplevd likhet eller sinne er fortsatt UNVERIFIED.
