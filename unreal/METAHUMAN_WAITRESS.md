# MetaHuman-servitrisen på Mesa Diner

Gjeldende standard etter denne leveransen er [atmosfære-R1](ATMOSPHERE_2026-10-27.md). MetaHuman-assets og funksjon er bevart; installert Low er på nytt kontrollert med tre replikker, pause og diskrestore. Punkt 5 er dermed også ferdig levert. R3-statusen nedenfor er historikken for selve figurintegrasjonen.

MetaHuman-servitrisen er levert lokalt 27.10.2026 kl. 12:32 CET. Pakket R3 er installert byteidentisk i `~/Games/Signal47/Linux`; 68 payloadnoder og alle 70 personlige Saved-noder med 50 filer er kontrollert. Faktisk standardstarter er prøvd med isolert profil på Low og består åtte kontroller. Ingen ny GitHub-spillutgave eller merge er gjort.

## Hva som er rettet

Den tidligere rapporten var et gammelt oppstartsnotat, selv om figur, rigg, teksturer og 43 taleanimasjoner allerede fantes lokalt. Gamle Claude-økter og to venteløkker er avsluttet etter Toms overtakelsesbestilling. Ingen annen Unreal-/byggejobb ble stoppet. Kildene før overtakelsen er bevart under `takeover-codex/baseline`.

Det var også konkrete kodefeil. Talevalget brukte første klipp i hele replikklisten i stedet for faktisk aktiv lydkilde. `ActiveClip` følger nå riktig klipp og lydklokke, også mellom klipp, under pause og etter stopp. Ansiktets CopyPose-node kopierte ikke nødvendigvis kroppens uttrykkskurver; runtime kobler dette eksplisitt før RigLogic. Tomgangen må tidsvikles eksplisitt ved sampling, og blikkfjæren bruker korte delsteg ved hitch.

Den første pakkede hårvarianten ble avvist: frisyren var synlig i editoren, men forsvant i Linux-pakken. En editorfunksjon for kortmesh returnerer alltid null i et pakket spill. Etter rettet LOD-valg var geometrien fortsatt usynlig i faktisk pakke. R3 bruker derfor frisyrenes eksisterende LOD1-hårkort som vanlig mesh, festet til hodet med invers referansetransform. Et vanlig tosidig maskert materiale bruker atlasets coverage-alfa på UV0. Dette gir synlig hår som følger hodet uten strandsimulering eller Groom-spesifikke vertex-attributter. Eksakt videre rotårsak i Groom-renderløypa er ikke fastslått.

## Det spilleren får

Én rigget MetaHuman bak den eksisterende disken erstatter den gamle enkle figuren. Hun har pust/tomgang, blunk, rolig blikk mot spilleren og eksisterende dinerinteressepunkter, samt taleanimasjon på faktisk lydklokke. Original engelsk dialog, meny, matlogikk, historietilstand og lagringsformat er beholdt. Dale og Kinski er fortsatt stemmer, ikke nye fysiske NPC-er.

Uniformen er en original Blender-kilde med løs blå overdel/skjørt, hvit krage og mansjetter, forkle og lukkede arbeidssko. Tidlige pass med hull ved hofte, krageinterseksjon og tær gjennom sko er avvist. V5 er importert på kroppens eksakte skjelett. [Faktisk pakket samtalebilde](ArtSource/WaitressUniform/Preview/waitress-speaking-high.png) og [bilde etter lasting](ArtSource/WaitressUniform/Preview/waitress-after-load-high.png) viser gjeldende resultat.

Dette er en fungerende figurpilot. Finere stoffstruktur, blyant bak øret, eksakt samsvar med portrettets alder/utseende og egne skjenke-/tørkegester er videre visuell polering. Det er ikke levert nye diner- eller motelmodeller i denne runden.

## Faktisk kontroll og avgrensning

- Alle 43 takes er lastet, har riktig ansiktsskjelett, positiv lengde og minst 100 uttrykkskurver. Kropp, ansikt og uniform bruker de eksisterende riktige skjelettene.
- Pakket R3 består 1 586 eksisterende selvtester, inkludert fem nye kontroller av aktivt lydklipp og pause/stopp.
- De tre faktiske velkomstreplikkene er spilt gjennom eksisterende lyd-/dialogkode. 209 målepunkter viser like kropps-/ansiktskjevekurver, maksimum 0,585880 og 23,628729 graders kjevebevegelse. CopyPose er koblet også etter flere frames.
- Lydklokken står stille under suspend. Disklagring er lest med en ny store-instans, spilltilstanden gjenstartet og nøyaktig én servitrise gjenoppstått.
- Tre talebilder, bildet etter lasting og siste skjermbilde er åpnet og vurdert fra samme R3. High kjører 55 spillsekunder og 1600 × 900. R2s skalperte figur er bevart som avvist bevis.
- Dette er en API-styrt scenariokontroll med faktisk lydkilde og rigg, ikke menneskelig input eller lytting. Ingen fullnatt, full 110-bilders merge-matrise eller FPS-påstand inngår. En kjent Vulkan-advarsel om én 1 MiB bildeallokering ved avslutning står åpen; prosessen avslutter normalt.

Kilde-/assetmanifestet binder 12 979 runtimefiler. `acceptance-r3.json` binder bilde-, logg-, CSV- og pakkehash. Alle tunge jobber har kjørt sekvensielt gjennom prosjektets kjørevern. Daghimmel, nattehimmel/stjerner, terreng og personlige lagringer er ikke endret av denne funksjonen.

## Kilder og videre arbeid

Gjeldende kode er `S47Person`, `S47Waitress`, aktiv-klippkoblingen i RecordedDialogue/Evening og den eksplisitt opt-in `S47PersonReview`. [Verktøynotatet](Signal47/Tools/metahuman/README.md) beskriver import og hårmateriale. [Uniformkilden](ArtSource/WaitressUniform/README.md) beskriver Blender-løypa. `integrate.py --apply` er sperret, siden den gamle løypa ellers overskriver rettet runtime og rapport fra utdatert staging.

MetaHuman-kropper, DNA, teksturer, skjelett, hår, FBX og fitting-scene er private lisensierte kilder i ignorerte mapper. Offentlig repo får egen kode, forfatterkilde, dokumentasjon og ferdig renderte bilder. Kreditering til Epic Games er lagt i pakkens `CREDITS.txt` og den eksisterende kreditgeneratoren. Ren kildeklone uten lokale nødvendige assets beholder den gamle enkle figuren.

Prosjektbiblioteket ble søkt før feilsøking. NVIDIA Audio2Face-notatet var referanse, ikke gjenbrukt kode eller ny avhengighet; ingen ny plugin eller tjeneste ble tatt inn. Lokal UE 5.8.4-kilde og faktiske bilder avgjorde løsningene.

Avtalt punkt 5, atmosfære og ytelsesrom, er nå levert i atmosfære-R1. Oppfølgingen skal ikke starte Claude igjen eller duplisere denne oppgaven. SC01 holdes separat. Motel-/dinerkonsepter og 14 unike New Mexico-fotoreferanser er allerede lagret som egne designkilder.

## Lokal levering og opprydding

`installation-r3.json` og `installed-verification.json` dokumenterer identisk payload og uendrede personlige lagringer etter oppstart. Low-bildet viser normal dialog, som dekker ansiktet; det brukes som oppstartsbevis, ikke som Low-næraksept av ansikt/hår. High-bildene over er egen faktisk figuraksept.

Canonical prune er anvendt på fem eldre komplette payloadsett, med kilder, Blender-/MetaHuman-assets, Saved og bevis bevart. Gjeldende R3 og to eldre MetaHuman-mellomkandidater beholdes; aktiv separat SC01-R4 er beskyttet. Preinstallasjonens reflink-backup ble verifisert før skriving; dens gamle genererte spillpayload er senere pensjonert av den dokumenterte oppryddingsplanen. Ingen eksisterende personlig Saved er slettet eller gjenopprettet.
