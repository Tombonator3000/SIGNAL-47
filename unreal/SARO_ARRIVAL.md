# SARO-ankomst og takeaway

24. oktober 2026, `codex/saro-arrival` fra a2c33e3d. Tom ber om skilt utenfor innkjøringen, parkering før SARO-skiltet, personlig bil som blir stående, bedre skilt/vinduer og håndtering av maten med Xbox.

## Endringer

- Besøksplassen ligger på eksisterende flatt SARO-gulv, fysisk Y=-0.60, X[-18.2,-7.8], Z[10.5,20.5]. Den personlige bilen parkeres i merket felt ved (-12.8,-0.60,14.3), mot nord. Hele bilkonturen må passe. Den ordinære servicebilen beholder sin egen plass og sine ruter. Asfalten har en separat ugjennomsiktig materialinstans med original RGB-tekstur; dinerens perforerte kantmaske brukes ikke. Den synlige flaten ligger på -0.576, 24 mm over uendret fysisk gulv, slik at den også dekker den gamle veien på -0.588. Prosedyregress utelates fra plassen og de smale gangstripene. Den nye besøksflaten og forbindelsen til innkjøringen utvider den gamle tillatte kjøremasken; alle ordinære hindringer kontrolleres fortsatt.
- SARO-skiltet flyttes fra kjørebanekanten til (-7.3,-0.60,15.5), med sokkel og ramme. Original tekstur beholdes. Tolv skilttrekanter fjernes fra det delte g435; de andre 228 trekantene og 19 originale gulv-/trinnbokser beholdes. Både gang- og kjørekollider flyttes sammen med skiltet. To skjermede skiltlamper og en vegglampe ved annexet tenner gradvis om kvelden. Vinduer får en belyst glassflate og karmer uten ugjennomsiktig bakplate.
- Parkert personlig bil og separat servicebil har stabil presentasjon før/etter ankomst og lasting. Servicepresentasjonen bruker samme originalmesh som den faktiske SARO 07, med lokale delvisibiliteter, ISM-data og egne kopierte materialparametre. `arrivalParkingVersion=2` bevarer nye trygge poser og reparerer gamle ramp-/service-/fallback-plasseringer.
- B på Xbox eller Q på tastatur åpner matmenyen på foten. D-pad/piltaster velger, A/Enter bekrefter og B/Escape lukker. Maten kan settes på en nærliggende synlig flate, stues bort, tas frem igjen eller kastes. `stowed` og `discarded` lagres uten å konsumere måltidet, flytte dialogen eller hoppe tolv minutter. Gamle matlagringer støttes. Posen tegnes mindre og lavere.

## Bygg og kort kontroll

`Signal47/Tools/saroarrival/prepare_cpu.py` gjør bare g435 CPU-lesbar før cook. Det samme gjelder de to wire-meshene via `Tools/highwaypower/prepare_cpu.py`. Importerte Web/Free-/lydassets legges ikke i offentlig Git.

`-S47SaroParking` prøver ordinær kjøring fra innkjøringen, full bilkontur, faktisk gange til østinngangen og disk-/verdenslasting. `-S47TakeawayPad` prøver faktisk syntetisk Slate-ruting med Q/B/D-pad/A, vanlig fysisk A-pickup og to ferske lastinger. Begge krever isolert developerprofil. Dette er ikke en prøve av fysisk Xbox-transport.

`-S47SaroViews -S47SaroShots=<ny mappe>` tar tolv spillerhøye kamerafixturer i dag, skumring og natt. De er visuell kontroll og påstår ikke kjørt spillerinput. Eksisterende takeaway-/service-diner-flyter inngår i et kort utvalg. Ingen full verdensrunde eller PSO-kø.

## Status

A1 ble avvist: den gamle SARO_WAY-grensen stoppet faktisk kjøring ved Z21.4. Fire øvrige diagnostiske prøver bestod. A2 rettet kjøregrensen og kjørte til plassen, men parkeringsprøven sjekket samme frame som utstigning, før den normale OnParked-handlingen etter 0.4 sekunder. Fixturen venter nå på den eksisterende overgangen og beholder alle harde ankomstkontroller. A2s tolv bilder avdekket også gress gjennom parkeringen; dette er rettet i sluttkilden med klart oppdelt visuell overflate og fysisk gulv. Råforsøk beholdes. Sluttkandidat A3 fra `5ae3a3fc` består editorbygg, Linux-pakking og fem målrettede pakkede kjøringer. Alle 1400 selvtestsjekker består. Faktisk kjøring parkerte ved (-12.817736,-0.599993,14.384659), etterfulgt av vanlig gange til østinngangen og fersk disk-/verdenslasting. Matprøven bruker syntetiske Slate-knapper og vanlig ray/walker/HUD-pickup; to nye stowed/discarded-lastinger og den eksisterende takeaway-prøvens ti disk-/verdenslastinger består. Service/diner består. Tolv dag-/skumrings-/nattbilder, tre parkeringsbilder og fire matbilder er faktisk vurdert.

Kilde-, pakke-, logg- og bildehasher: `Signal47/Tools/saroarrival/acceptance.json`. Rålogger, isolerte profiler, kandidater og bilder: `~/scratch-bm/Signal47-saro-arrival`. Ingen fysisk Xbox-prøve, menneskelig lydlytting, full110-/PSO-kø eller ny ytelsesgaranti. Merge og standardinstallasjon registreres separat nedenfor. Tidligere RT-minnevarsel er et eget eksisterende todo-punkt; ingen global grafikk-/budsjettjustering inngår.


## Levering, 24. oktober kl. 23:15

PR [154](https://github.com/Tombonator3000/SIGNAL-47/pull/154) er vanlig flettet til `unreal-port-core` i `86eb0f8c` etter kontroll av eksakt head/base, CLEAN og ingen påkrevde CI-kontroller. Samme testede A3 er installert i `~/Games/Signal47/Linux`: alle 40 payloadfiler er hashidentiske med kandidaten og alle 46 Saved-filer er byteidentiske med før installasjonen. Verifisert sikkerhetskopi: `/home/tom/scratch-bm/Signal47-saro-arrival/installed-backup-1792876403571799419`. Fem eide Content-lenker er fjernet og begge genererte OpenOrder-filer gjenopprettet. Ingen ny normal spillerøkt er startet.
