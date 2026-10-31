# Roswell: rettede armer og bakke-AI

**Status:** Referanse, levert lokalt. **Gjelder:** UE5.8.4/Linux/Vulkan, runtime `130f975f`, PR295/main `5f575100`. **Sist kontrollert:** 31. oktober 2026.

Installert kl.20:03 CET i den vanlige Linux-spillmappen. Den nye pakken beholder SARO-arbeidsstasjonene og historieløkken fra den forrige installasjonen. Alle 155 personlige Saved-elementer, inkludert 126 filer, er uendret. [Avgrenset installasjonskvittering](RoswellRepair-2026-10-31/install.json).

## Hva som ble rettet

- Den importerte Unreal-riggens venstre arm ligger på positiv X, motsatt av antakelsen i animasjonen. Armene peker nå utover fra den faktiske referanseriggen i idle, lytting, lusking, løp, grab og eldre vinduspeek. Original hud, rigg, LOD-er og Blender-/Tripo-kilder er beholdt. Den høye jakt-alienen er ikke endret.
- Bakke-AI brukte opp rutebudsjettet på utilgjengelige mål og kunne velge eget ståsted som et nytt mål. Figurene får nå rettferdige planleggingsturer og prøver andre kandidater. Manglende rutebudsjett behandles som venting, ikke som en ferdig skjulemanøver.
- Bevegelsen måler faktisk gulvhøyde ved hvert steg, slik at en liten høydeforskjell ikke gjør interpolert fotposisjon ugyldig. Tilnærminger til SARO-ramper inngår i rutekandidatene. Kollisjon, lukkede dører, takkanter og støttegrenser er beholdt.
- Dev-plasseringen prøver flere mål foran spilleren med de samme gulv-, sikt- og kollisjonskravene.

## Prøve og grenser

Editorbygg og pakke består med **1824 selvtester**. Den strengere prøven krever egen bevegelse, fotkontakt og lyd fra hver av seks bakkeindivider i Curious, deretter ny egen bevegelse eller spillerkontakt fra hver i Hunt. Editor og pakke består også ståtid, pause, Annex-tak og rydding. I pakken flytter alle seks seg 2,58 til 8,40 m i Curious og ytterligere 5,39 til 7,28 m i Hunt. [Editorresultater](RoswellRepair-2026-10-31/editor-ground-results.json), [pakkeresultater](RoswellRepair-2026-10-31/packaged-ground-results.json). Installert standardstarter består en kortere isolert prøve av ståtid, pause og alle seks Curious-figurer.

Prøvene bruker ordinære utviklerhandlingers callbacks i en isolert Case0 under `/tmp`, ikke personlige lagringer eller ekte menyklikk. Native Vulkan-bilder dekker fem armpositurer forfra og fra siden. Denne kinematiske positurprøven bekrefter deformasjon; den er ikke bevis for navigasjon. [Toms førbilde](../Playtest-2026-10-31/35-roswell-arm-clipping.png) har et annet kamera enn [etterbildet forfra](RoswellRepair-2026-10-31/idle-front.png) og [grab fra siden](RoswellRepair-2026-10-31/grab-side.png).

Ingen ny fullmatrise eller helnatt er kjørt. Naturlig møtefrekvens på hele spillruten, finere animasjonspolering og menneskelig vurdering av Foley/komfort står fortsatt åpent. Lydhendelser og kildeposisjoner er teknisk prøvd; dette er ikke en menneskelig lyttegodkjenning. MotionBricks er fortsatt bare registrert som uprøvd arbeidsflyt og er ikke brukt i rettelsen.

Etter levering er to eldre genererte testpayload pensjonert med prosjektets prune-verktøy. Gjeldende kandidat og to eldre komplette pakker, inkludert forrige installasjon som rollback, beholdes. Originalkilder, Saved, bilder og logger er bevart. Alle egne tunge jobber er avsluttet og reservasjonen er frigitt.
