> **Status: Referanse** · Klassifisert 28.10.2026. Gjelder det dokumenterte omfanget; [gjeldende leveringsstatus](../../Active/STATUS.md).

# Kapittel seks og slutten av natta

Arbeid på `port-ch6`, fra `9a44ebb1`, 22. oktober 2026. Repoets synlighet ble kontrollert som PUBLIC. Ingen push, PR, publisering eller endring av genererte assets. Kapittel fem finnes ikke på denne grenen og er ikke opprettet her.

## Filer

Nye moduler:

- `Signal47/Source/Signal47/Public/S47Chapter6.h` og `Private/S47Chapter6.cpp`.
- `Signal47/Source/Signal47/Public/SS47Ending.h` og `Private/SS47Ending.cpp`.

Koblinger og nødvendige utvidelser:

- `S47Chapters.cpp`: registrerer kapittel seks etter de eksisterende kapitlene.
- `S47HUD.cpp`: én selvtestlinje, Away-krokene, ch6 sist i ClockAfter og siktet skjult i bilen.
- `S47Flow.h/.cpp`: main.ts sin `finale()` som `Finale()`.
- `S47World.h/.cpp`: `CabWantsKey` i den eksisterende cab-prompten. Kameraet bruker eksisterende `OldControl.Camera`, videre gjennom `AS47Walker::CameraOverride`; Walker endres ikke.
- `S47Web.h/.cpp`: en merket sluttblokk for tåke, daggryløft og solhøyde.
- `S47Away.h/.cpp`: ch6-krokene og seenStep for siste mila, THE EVENT og figuren; gjenbruk av FLASH-overlegget.
- `SS47AwayOverlay.h/.cpp`: et valgfritt argument som lar FLASH tone hele bildet, uten det ugjennomsiktige svarte belegget bortføringen trenger. Standardoppførselen beholdes for øvrige brukere.
- `S47EngineSound.h/.cpp`: den faktiske solenoidgrafen fra engineSound.ts, som manglet bak det eksisterende API-et.
- `Scripts/screenshots.sh`: tre kjøringer, for avreise, THE EVENT og sluttittelen.
- Denne rapporten, `memory.md`, `todo.md` og `log.md`.

Ingen endring i `S47Story`, `S47Main`, `S47Walker`, Build.cs, nettversjonen, Unity eller eksportøren.

## Portert innhold

`Chapter6.ts` er portert med de samme stadiene, verdiene og engelske tekstene: kjøringen fra dinerparkeringen ved 05:20, klokka ved 55 mph, radiotekstene ved milene, vendingen tilbake og hintene ved stillstand. Den siste mila har fartsutjevningen, kravet om innkjøring fra før 6,95, CB-åpningen 47 sekunder fra linja, grensen på 40 sekunder / 200 meter, tekstene, lukkingen ved 12,8 sekunder og klokka over de resterende meterne.

THE EVENT har motorstansen, de døde instrumentene, fire og sju, fargene på frontlysene, skriveren og telefonen gjennom cab-høyttaleren, Tomás og Nora, koppen, bærebølgen, FLASH, stillheten og lyspulsen. Nøkkel og dør tar E/Q i main.ts sin rekkefølge. En tidlig nøkkelturn erstatter dashbordets egen telling med sine elleve blink; en sen venter på tellingen. Nye forsøk og tekstingen har de samme sperrene som i kilden.

Bilen styrer og bremser til planlagt `StopS`. Figuren står ni meter videre, får lyset og glødepunktene fra eksporten og forsvinner i FLASH. Eksportens `world.oldRoad.glow.points` har 333 punkter; de siste sju er akkurat punktene OldRoad-konstruktøren oppretter for figuren. `FS47ThirdFigure` og `S47FlashFrames` gjenbrukes.

Kamerauttrekket bruker front, high, sunSide, top og stjernene ved az 31 / el 48, eller fire stillbilder på 8,5 sekunder. Alle beregninger er i three.js-rom; bare tegningen konverteres. Tåke, daggryløft og solhøyde beregnes fra kamerahøyden og `SUN_MESA.top` / `horizonDip`.

`Ending.ts` og main.ts sin finale er portert: svart, tittel, kreditering med Skip etter fire sekunder, hele 1947-fotografiet på et 1200 x 800-lerret og tilbake til tittelen. Alle ventetider og overganger er bevart, inkludert 60 ms innledningene, 5,6 sekunder for tittelen, 46 sekunder for rulleteksten og 14 sekunder for fotografiet. Krediteringsteksten og den valgfrie kjøpte lyden er bevart. Slate-tid og callbacks erstatter nettleserens sanntidstimere og promises.

Kapittel seks serialiseres ikke. `SaveBlock()` er `Not on the road.`; starten lager ikke et nytt lagringspunkt. Fortsett skal derfor fortsatt åpne siste dinerlagring.

## Integrasjon som gjenstår

1. Den parallelle kapittel-fem-porten må koble sin `OnEnd` til `US47Chapter6::BeginChapter6(nullptr)`, etter dinerlagringen, og fjerne sitt sluttkort. `BeginChapter6` deaktiverer kapittel fem gjennom `Host->Chapter("ch5")`. `JumpChapter6` gjenbruker `JumpChapter4` for de nøyaktige ferdigtilstandene til kapittel 1 til 3, skriver kapittel 4 som Prologue gjør, og bruker en nullsjekket JSON-overgang for kapittel 5. Integratoren bør erstatte kommentert stand-in med kapittel fem sin egen `BeginChapter5` og direkte tilstandsoppdatering når klassen finnes.
2. Den parallelle verdensporten må binde `AS47Web::OnDawnLift` / `OnSunElevation`, eller lese `DawnLift()` / `SunElevation()`. Verdiene er implementert, og tåken endres faktisk. Dagens `M_WebSky` har bare et eksportert panorama og Brightness; det har ingen dynamisk daggry- eller solrenderer. Derfor er et visuelt korrekt daggry, soloppgangen over mesaen og høydeavhengig horisont fortsatt en rendereravhengighet i denne grenen. Ingen håndlaget erstatningsgeometri er lagt til.
3. Den eksisterende motorporten har fremdeles plassholdere for blant annet dekk/vind, motorens ekstra rumling, misfire og startmotor. Solenoiden som kapittel seks trenger, er koblet til den ekte grafen her. Motorstansens avbremsing av opptakene var allerede implementert.
4. `FS47ThirdFigure` har fra før en halvgrense i stedet for shaderens dither ved fade. Kapitlet sender originalens fadeverdier, men dette native avviket i den eksisterende figurporten består.

## Tester og bevis

Registrerte resultater:

- PASS: siste `Scripts/build-editor.sh` etter solenoidkoblingen, UE 5.8.4, uten kompilatorfeil eller advarsler (`/tmp/s47-ch6-build6.log`). En navnekollisjon med Unreals `UP` ble rettet til `S47_UP` med kommentar.
- PASS: 996 sjekker i siste `-S47SelfTest`, inkludert kapittel seks, Ending og elleve kontroller av den faktisk rendrede solenoidgrafen. Den første kjøringen hadde 985 sjekker før lydgrafen ble lagt til. Dekker blant annet CB-utløsning, avstandsklokke, stopp og rygging, manglende innkjøring, nærgrensene, nøkkel/dør og tidsperrene, elleve blink, kamerastilling, stillbilder, siste blikk, tittel/credits/foto, Skip og dispose.
- PASS: avreise fra dinerparkeringen med kapittel seks, ingen modal, runtime-bilde.
- PASS: `chapter6:line` med autopilot, kryssing av C, motorstans og bremsing til null i kjørefeltet; runtime-bilde viser figuren og det blå båndet i frontlyset.
- PASS: normal THE EVENT og hele kamerauttrekket fram til sluttittelen. Hendelsesloggen har skriver ved 18,405 s, telefon ved 21,006 s, Tomás ved 23,814 s, Nora ved 26,805 s, kopp ved 29,608 s, bærebølge ved 31,400 s og stillhet ved 34,008 s. Overgang til kamerauttrekket ved 47,004 s, svart ved uttrekkets 34,000 s. Avviket fra terskelen er den første renderte rammen etter den, som i originalen. Tittelbildet er visuelt kontrollert ved 1280 x 800.

- PASS: en 285-sekunders kjøring fra `chapter6:lastmile`, med innstillingene `calmFlash=true` og `stillShots=true`, gjennom siste mila, THE EVENT, uttrekket, tittel, kreditering, fotografi og tilbake til tittelen på SARO. CB åpnet med 918,348 meter igjen og klokka 05:28:13; lukket etter 12,823 sekunder ved 05:28:25. Finale gikk fra tittel 08:02:47,737 til kreditering 08:02:54,796, fotografi 08:03:42,056 og vanlig tittel 08:04:01,649. Kjøringen avsluttet med fase 0 og exit 0, uten fokusfeilen fra den første finaleprøven. Returbildet er visuelt kontrollert; kreditering/foto er bekreftet som runtime-overganger, uten egen skjermbildeinspeksjon.

Kjøringene bruker `UnrealEditor ... -game -RenderOffscreen -nosound`, ikke et pakket bygg. Generert Content ble midlertidig lenket fra hovedkopien; alle fire lenker ble fjernet før commit. Loggene og bildene ligger i `/tmp/s47-ch6-evidence/`; byggloggene i `/tmp/s47-ch6-build*.log`.

Editorloggen inneholder eksisterende advarsler om importmaterialenes InstancedStaticMeshes-flagg og manglende håndtak fra den eldre eksporten, samt Python-feil i motorens eksperimentelle EditorToolset / ToolsetRegistry. Dette er ikke en ren editorlogg, selv når spillrøyktesten består. Første finaleprøve avdekket dessuten at Ending måtte kunne ta Slate-fokus; det er rettet før siste kjøring.

UNVERIFIED: pakket bygg og hele `screenshots.sh`, menneskelig input og lytting, faktisk Fortsett fra kapittel fem sin dinerlagring, hele 13 km fra parkeringsplassen og visuell likhet med nettversjonens dynamiske himmel. Dette er en kapittelleveranse, ikke en ferdig utgave av hele spillet.

## Kilder og bibliotek

Porten følger de komplette originalene i `~/SIGNAL-47/web/src/story/Chapter6.ts`, `ui/Ending.ts`, de relevante delene av `main.ts` / `Prologue.ts`, `drive/engineSound.ts`, inventarene og `web/tools/chapter6.py`. Eksisterende native kapittel 4, World, OldRoad, Truck, CB, Walkie, FlashFrames, TheThird og Flow er rammeverket.

Prosjektbiblioteket ble søkt først med porteringsbehov og Unreal/C++/TypeScript. Et bredere søk ga generelle editor-MCP-verktøy. Profilen `repoer/prosjekter/ChiR24--Unreal_mcp.md` ble lest: MIT, oppgitt UE 5.0 til 5.8, men UNVERIFIED i biblioteket og uten en relevant løsning for denne porteringsløypen. Ingen bibliotekskode, installasjon eller nye assets ble gjenbrukt.
