# Spillbar kveldstart og kveld/natt/gry

Arbeid på `codex/evening-runtime`, fra `96383c9f`. Codex eier trinn 1 og 2, også spillogikken. Claude integrerer. Ingen push, PR eller oppdatering av `~/Games/Signal47` er bestilt eller utført.

## Runtime

Ny natt fra tittelen begynner i personbilen på den eksisterende riksveien, sør for Caliche. WASD kjører, R slår lokalradioen av/på og E går ut når bilen står. Mesa Diner er valgfri; bestill hos servitrisen eller ved disken. Burger/sandwich, vann, spising, takeaway og avslag går gjennom manuspakken. Mat, tallerken, kopp og pose vises i verden/hånden. Personbilen har egen utvendig modell og parkering; den gjenbruker eksisterende kjøre- og dashbordrigg. SARO 07 beholder eget arbeidsliv og kilometerteller.

Faktisk parkering på SAROs pad setter `intro_arrived` og starter skiftklokka 18:00 én gang. Telefonen i kontrollrommet ringer. D01 spilles bare i telefonkanalen; Dale finnes ikke som fysisk figur. Samtalens gren og neste linje kan gjenopptas. D01_BRIEF legger overleveringsdokumentene i notatboken og slipper den eksisterende prologens rack-/kalibreringsforløp videre.

`FS47EveningPack` kompilerer den deterministiske JSON-eksporten direkte. Det finnes ikke en håndskrevet kopi av manusdialogen. Bare navngitte trigger-scener kan startes; choice-scener krever et gyldig valg. Betalingen og den ventende serveringsscenen ligger i samme serialiserte tilstand. Dokument-ID-er tildeles én gang. Eksterne resultatflagg settes fra faktisk runtime, og kan ikke skrives av manusets effekter. Senere faser er datadrevne porter, ikke ferdig kontor-/trafostasjon-/VHS-spill.

Lokalverten i innkjøringen har egne undertekster og taus taleplassholder merket «voice recording pending». Ingen Earl- eller Kinski-opptak erstatter verten. Den eksisterende klarerte countrymusikken får et 45 sekunders mellomrom mellom innslagene. Radioens linje, lesetid, av/på og musikkposisjon lagres separat fra den fysiske samtalen. Utstigning og samtaler pauser mottakeren. Nye takes og VHS er fortsatt egen senere produksjon. Den strenge kataloghjelperen i `Scripts/evening-voice-catalog.py` måler faktiske godkjente WAV-filer; den produserer ingen stemme.

## Tid, lys og gamle saker

Se hele tabellen med gamle og nye hendelsestider i [EVENING_TIMELINE.md](EVENING_TIMELINE.md). Ny tidslinje har residual 00:13:47, kapittelporter 02:20/03:20/04:10 og uendret finale 05:29:00 til 05:29:47. 1947-opptaket har sin egen uendrede tid.

Vanlig aktivitet leder til tidskomprimering i den eksisterende survey-faden; spilleren må ikke vente seks virkelige timer. Senere kapitteloverganger går bare fremover. Ny natt holder fortellertid ved 05:20 ved svært lang frivillig utforskning, og siste kjøretur bruker avstand, slik at ingen avreise eller rygging ruller klokka tilbake. Denne komprimeringen er ikke en måling av total spilletid.

Lagringsdata har `v=3`, med eksplisitt `timelineVersion=2` for nye saker. Saker uten tidslinjeversjon blir uttrykkelig merket `timelineVersion=1` ved ny lagring og beholder den gamle 02:13-historien. Ingen gammel bildedato, utskrift eller dokumenttekst omskrives. En legacy-merknad legges i notatboken. Åpningen kan lagres før, under og etter bestillingen og under overleveringen. Kort transport/parkering fullføres før disk-snapshot. Den gamle prologens survey har fortsatt kontrollpunkt ved residualen.

Ettermiddag, vestsol, skumring og blåtime bruker den levende himmelen og native sol/skylight. Morgenkurven `Dawn()` beholder sin tidligere betydning for historien. Felles visuelle dagslysporter bruker `Daylight()`. Detaljer og kurver i [EVENING_SKY.md](EVENING_SKY.md).

## Kontrollgrunnlag

- Editorbygg: PASS etter retting av UE 5.8 JSON-nøkler og kompilatorfeil. Endelig kildebygg oppdateres etter runtimeprøvene.
- Første `-S47SelfTest`: PASS, 1245 ytre sjekker; pakktesten inneholder dessuten 60 interne kontrollpunkter. Dette er API-/tilstandstester, ikke menneskelig gjennomspilling.
- Pakkeeksport `build_pack.py --check` og stemmekatalogens 12 isolerte kontroller: PASS. Katalogtestene brukte midlertidige syntetiske PCM-filer, ikke produksjonsstemmer.
- Bildet av innkjøringen ble faktisk rendret i Unreal. Det avdekket klokkeslett i to andre HUD-felt; disse er rettet og skal tas på nytt.
- Sammenhengende kjøring, diner, telefon, disk-last og pakket regresjon: under arbeid. Resultat og bildehenvisninger oppdateres før levering.
- Menneskelig gjennomspilling, lytting og målt total spilletid: UNVERIFIED. Faktisk film/trafo/kontor/anomaly spotting hører til senere trinn.

Evidens ligger i `/home/tom/scratch-bm/Signal47-evening-runtime/`. Kilde, editor-kandidat, pakket kandidat og installert standard holdes atskilt. Alle tester bruker isolert `UserDir`.

## Integrasjon og kilder

`unreal-port-core` er flettet inn igjen i `f0fe3293`, inklusive Claudes bensinstasjon. Bensinstasjonens modell, `Config/Props/town.json` og `S47Props` har bare innkommende mergeendringer, ingen ny implementering fra denne oppgaven.

Prosjektbiblioteket ble søkt først og én gang bredere. EngineWire-profilen ble lest; den er en UNVERIFIED editorverktøyreferanse og gir ingen kompatibel runtimeadapter å gjenbruke. Implementeringen bruker prosjektets egne systemer. GitHub-kontroll 23. oktober viste `PRIVATE`, standardgren `main`, og ingen åpne PR-er mot `unreal-port-core`. Ingen synlighet er endret.
