# Radiostasjon og array

Arbeidsgren `codex/radio-array-assets`, basert på `ed061aca`.
Toms bestilling 23. oktober 2026: Blender-radiostasjon med interiør etter konseptark,
og en mer realistisk radioantenne som erstatter spillets arraymodell.

Kildekandidat `66f41e22`: originale modeller, native UE-assets og integrasjon, inkludert inngangssonens radiusklaring.
Blender-kildekontroll og eksportkontroll holdes adskilt fra Unreal-import og pakket spillprøve.
Den installerte utgaven kjørte ved første kontroll kl. 21:39; Tom lukket den etter forespørselen.
Ingen brukerprosess er stoppet. Andre oppgavers Unreal-jobber avvikles før våre tunge jobber starter.

Detaljer, kilder, gjenskaping og importkontrakt: `Signal47/Tools/radioarray/README.md`.

## Verifisert

- Blender 5.2.2: ni antenne-FBX tilbakeimportert med riktige bounds/pivoter/materialer og null nullarealtriangler. Antennens tre nivåer har 26 210 / 18 250 / 6 510 trekanter.
- Stasjon: 32/32 eksportkontroller. 43 268 interiørtrekanter. Rensingen fjerner kun nullareal; før/etter-hash av synlige koordinater, UV og materialer er identisk. Alle 1136 redigerbare kildeobjekter er bevart.
- UE 5.8.4: asymmetrisk akseprøve `(2,4,6)` meter gir `(200,-400,600)` cm. Legacy FBX trenger eksplisitt policy og +90 graders importrotasjon. Stasjonens Props-yaw er -90 grader.
- Frisk UE-prosess: 68/68 assetkontroller, med eksakte trekanttall også for alle seks uniontrim-meshene. Se `Tools/radioarray/verification/unreal-assets.json`.
- Editorbygg: PASS, 156,48 s. Linux-pakking: PASS, først 222,91 s og sluttpakke 92,69 s, ingen materialkompileringsfeil.
- Pakket selvtest: PASS, 1278. Runtime logger 27 antenner i tre instansgrupper og seks riktig utskiftede bymesh.
- Pakket arraybevegelse: PASS, eksisterende hold-sekvens kjører på de nye delene.
- Pakket lukket lobbydør: PASS, stopper spilleren ved X8019,8 etter 0,9 m; gulvavvik maksimalt 0,04 m.

Kandidat og detaljbevis: `~/scratch-bm/Signal47-radioarray/candidate` og `~/scratch-bm/Signal47-radioarray/runtime`.
En åpen-dør-prøve ble avbrutt av maskinporten før første ramme da en annen oppgave startet et bygg samtidig.
Den regnes som ugyldig bevis og tas på nytt; ingen fremmed prosess ble stoppet.

Åpen-dør-prøven fant et gap mellom radius-eroderte gangsoner. Approach-sonen er utvidet 0,6 m; statisk kontroll med faktisk Inside-logikk reproduserer gammel stopp og bekrefter 0,31 m overlapp i den nye. Sluttpakken tas gjennom nye runtimeprøver.

Gjenstår før installasjon: åpne døren via vanlig brukssporing og gå inn, buzzer-dialog,
visuell gjennomgang av fasade/interiør/array, pilotregresjon og kontrollert installasjon med lagringene bevart.
