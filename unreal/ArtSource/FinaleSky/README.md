# SC13: originalt fartøy og lokale skyer

Status: **Under arbeid. Motor, Blender, native import, bilder og ytelse er UNVERIFIED.**

[source-contract.json](source-contract.json) og [produksjonsverktøyene](../../Signal47/Tools/finalesky/README.md) er redigerbare originalkilder for SIGNAL / 47. De bruker ingen tredjepartsmodeller, bildeteksturer eller fluidsimuleringer. Konseptbildene i [CloudTankUFO](../../Concepts/CloudTankUFO-2026-10-28/README.md) er bevart som regi, ikke brukt som materialer eller målfast produksjonsbevis.

Fartøyet forfattes som 167 navngitte, lukkede deler: sammenhengende buet skrog, fire dreide bånd, radiale undersidekasser, mørke lameller, kantfordypninger, 16 små lys og et sentralt optisk brønnpunkt. Tre egne LOD-er har henholdsvis 15 268, 10 004 og 7 076 triangler. Ren CPU-kontroll av oppskriften kontrollerer ikke-degenererte flater, motsatt rettede nabokanter og positivt signert volum. Dette er ikke Blender- eller normalaksept.

Blender-jobben skriver bare til en ny eksportmappe. `FinaleSky.blend` beholder hver originaldel med material og UV i `EDITABLE_ORIGINAL_SOLIDS`, samt tre separate eksportmesher. Jobben lagrer, åpner den faktiske filen igjen og sammenligner posisjoner, triangler, materialtilordning, UV og hjørnenormaler før manifestet får `SOURCE_REOPEN_PASS_NATIVE_UNVERIFIED`. Eksisterende kilder overskrives ikke.

Blender-basis er meter og Z opp. Den etablerte FBX-løypen bruker `-Y/Z`, `forceFrontX` og importrotasjon 90 grader. Forventet native-basis er `(Bx, -By, Bz) * 100` centimeter med uendret hjørnerekkefølge og `(u, 1-v)`. Dette er forankret i den lokale UE5.8.4-importøren, ikke bare i bounds. Native-leseren må deretter påvise orienterte faktiske saved-triangler, navngitte materialgrupper, UV, alle tre render-LOD-er og full renderpayload før en separat PID kan skrive bindingen.

To endelige, lokale volum har et originalt analytisk tetthetsfelt med sammenhengende kjerner, foldet forkant og langsom intern rulling. Feltene speiles og kommer fra hver sin side. Oppskriften bruker ikke en ny global sky, høydeendring eller simulering. Low og High har samme volum, fase og opake fartøy, men 28 og 56 stråleprøver. SceneDepth begrenser hver integrasjon mot faktisk opak geometri, slik at sky foran skroget kan skjule det, mens sky bak ikke males over skroget. Fartøyet bruker opake materialer hele tiden. Den faktiske åpne passasjen, tildekkingen, sorteringen og silhuetten må vurderes i runtime.

Chapter6 eier fase, pause, område og opprydding. Nye saker krever den komplette, lagrede SC12-ruten; klokkeslett alene aktiverer ikke endringen. Manglende eller avvist kunstbinding lar den opprinnelige avslutningen gå videre. Testen godtar da ikke det nye bildet som levert. EVENT 47 sekunder, uttrekk 34 sekunder og originalbildet fra 1947 er uendret.

Valget av lokalt volum fremfor de foreslåtte flate flipbook-kortene skyldes den eksisterende bevegelige avslutningskameraet og kravet om faktisk dybdetildekking. Prosjektbibliotekets UDS- og UE-Niagara-referanser var metodekunnskap; UDS finnes ikke i prosjektet, og ingen kode eller assets derfra er kopiert. Kompatibilitetsgrunnlaget er prosjektets eksisterende FBX/materialløype og lokal UE5.8.4. Utseende, spillerreaksjon, pakking og 60 FPS er fortsatt UNVERIFIED.
