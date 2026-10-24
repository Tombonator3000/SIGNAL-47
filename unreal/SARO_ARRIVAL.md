# SARO-ankomst og takeaway

24. oktober 2026, `codex/saro-arrival` fra a2c33e3d. Tom ber om skilt utenfor innkjøringen, parkering før SARO-skiltet, personlig bil som blir stående, bedre skilt/vinduer og håndtering av maten med Xbox.

## Endringer

- Besøksplassen ligger på eksisterende flatt SARO-gulv, topp Y=-0.60, X[-18.2,-7.8], Z[10.5,20.5]. Den personlige bilen parkeres i merket felt ved (-12.8,-0.60,14.3), mot nord. Hele bilkonturen må passe. Den ordinære servicebilen beholder sin egen plass og sine ruter.
- SARO-skiltet flyttes fra kjørebanekanten til (-7.3,-0.60,15.5), med sokkel og ramme. Original tekstur beholdes. Tolv skilttrekanter fjernes fra det delte g435; de andre 228 trekantene og 19 originale gulv-/trinnbokser beholdes. Både gang- og kjørekollider flyttes sammen med skiltet. Vinduer får en belyst glassflate og karmer uten ugjennomsiktig bakplate.
- Parkert personlig bil og separat servicebil har stabil presentasjon før/etter ankomst og lasting. Servicepresentasjonen bruker samme originalmesh som den faktiske SARO 07, med lokale delvisibiliteter, ISM-data og egne kopierte materialparametre. `arrivalParkingVersion=2` bevarer nye trygge poser og reparerer gamle ramp-/service-/fallback-plasseringer.
- B på Xbox eller Q på tastatur åpner matmenyen på foten. D-pad/piltaster velger, A/Enter bekrefter og B/Escape lukker. Maten kan settes på en nærliggende synlig flate, stues bort, tas frem igjen eller kastes. `stowed` og `discarded` lagres uten å konsumere måltidet, flytte dialogen eller hoppe tolv minutter. Gamle matlagringer støttes. Posen tegnes mindre og lavere.

## Bygg og kort kontroll

`Signal47/Tools/saroarrival/prepare_cpu.py` gjør bare g435 CPU-lesbar før cook. Det samme gjelder de to wire-meshene via `Tools/highwaypower/prepare_cpu.py`. Importerte Web/Free-/lydassets legges ikke i offentlig Git.

`-S47SaroParking` prøver ordinær kjøring fra innkjøringen, full bilkontur, faktisk gange til østinngangen og disk-/verdenslasting. `-S47TakeawayPad` prøver faktisk syntetisk Slate-ruting med Q/B/D-pad/A, vanlig fysisk A-pickup og to ferske lastinger. Begge krever isolert developerprofil. Dette er ikke en prøve av fysisk Xbox-transport.

`-S47SaroViews -S47SaroShots=<ny mappe>` tar tolv spillerhøye kamerafixturer i dag, skumring og natt. De er visuell kontroll og påstår ikke kjørt spillerinput. Eksisterende takeaway-/service-diner-flyter inngår i et kort utvalg. Ingen full verdensrunde eller PSO-kø.

## Status

Kilde og CPU-forberedelse er ferdige. Editorbygg, pakket kontroll, faktisk bil-/matflyt, visuell vurdering og levering oppdateres når de er gjennomført. Tidligere RT-minnevarsel er et eget eksisterende todo-punkt; ingen global grafikk-/budsjettjustering inngår.
