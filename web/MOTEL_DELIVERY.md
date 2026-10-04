# Sierra Motor Court: fasade, kontor og runde 7

Bygd etter kodekontrakten og grafikkbriefen på main `db47e04`, 4. oktober 2026. Spilleren kan gå fra innkjørselen til kontoret, undersøke gjesteprotokollen, den tomme kroken for nøkkel 6, telefonen og de to separate papirene, og fortsette langs gangveien til rom 6. Dette er en isolert, kjørbar modul for kapittel 4. Claude eier innkobling, dokumenttekster, dialog, lagring og den samlede spilltesten.

## Åpne leveransen

Fra `web/`, med repoets Node-avhengigheter installert:

```sh
python3 tools/motelpreview.py --port 8483
```

Åpne `http://127.0.0.1:8483/`. Forhåndsvisningen har fem faste utsnitt, High/Low, uavhengige lysbrytere, dørblad, synlig/skjult kontor og Recreate. Privat inngang og Vite-konfigurasjon genereres under `/tmp`; `main.ts`, `vite.config.ts` og Pages-inngangen endres ikke. Bakgrunnens vei, ørken, stjerner, skilt og SARO-lys er prøveomgivelser. Modulens tegnekall måles separat med denne bakgrunnen skjult i målebildet. FPS på ekte telefon kan ikke utledes av dette.

Grafikkgalleriet ligger i `src/assets/art/production/round7.html`. Det kan åpnes fra en lokal HTTP-server med `src/assets/art/` som rot. Galleriet og kontrollarkene er kildevisning, ikke spillbilder.

## Innkobling for Claude

- Opprett `new MotelFront()` etter `loadArt()`. Legg `group` i SARO og legg `zones`/`colliders` til områdets eksisterende lister. Ingen egen origo eller lysgruppe. Alle gangflater har y=0. Bygningen står vest for veien, x[-50,-41], z[22,70], dører mot +X.
- Fjern den gamle motellboksen, baldakinen, stolpene, bilen og de to gamle motellflomlysene fra `Exterior.motel()`. Behold eksisterende Sierra-skilt, skiltstolper, vei og veilys. Modulen tegner ikke et nytt Sierra-skilt; den har kollidere for de eksisterende skiltstolpene og eget rød/cyan-lys på sine materialer.
- `lot`, `walk`, `office` og `officeDoor` er fire soner i verdenskoordinater. Innkjørselen dekker x[-30,-28], z[2,10], hvor Claudes overgang kommer inn. Sonene overlapper med mer enn 0,6 m. Kontoret er omtrent 4,4 × 5,4 m, x[-45.6,-41.2], z[22.2,27.6]; inngangen er z[23.4,24.8].
- `anchors.entry=(-29,6)`, `officeInside=(-42.6,24.1)`, `room6Outside=(-39.5,55)` og `fromRoom6=(-39.5,55)`. Bruk deres `yaw` også. Rom 6-døra er ved (-41.07,0,55); dette er den endelige plasseringen. Det tidlige omtrentlige z58.7-forslaget er erstattet.
- Registrer de 11 treff-ID-ene: `officeDoor`, `register`, `keyBoard`, `officePhone`, `envelope`, `message`, `room6Door`, `otherDoors`, `iceMachine`, `car`, `pool`. `otherDoors` er én gruppe med sju bokser. Bruk aktivering/etiketter eller lag 31 når en interaksjon skal være av. `interior.visible` skjuler kontorets innredning; separate treffflater tilhører fremdeles kapittelkoden.
- `objs.room6DoorLeaf` er en hengselgruppe: Y=0 lukket, Y=+π/2 åpen inn mot rommet. De lukkede rommenes kollidere forblir bakgrunn. Dørhandlingen flytter spilleren til Claudes egne Room6-område; den lager ikke gangsoner inne i fasadens kulisser.
- `objs.officePhoneHandset`, `envelope`, `message` og `key6Hook` er egne grupper. Papirene er tomme og ligger på disken; Claude skriver de engelske tekstene. Felttelefonen har eik, to bjeller, sveiv, eget håndsett og kabel gjennom bakveggen.
- Kall `setRoom6Light(on)`, `setOfficeLight(on)` og `update(dt,t)`. `courtFlood = floodSet(10,'court',0.1)` bruker ti egne plasser. Nabodører deler noen lysplasser. Ingen `site`-plass eller ekte lys allokeres. Endringer i lampe- og vindusmaterialer følger begge kvalitetvariantene.
- Bare én levende modul eier `courtFlood`. Kall `dispose()` før utskifting. Den frigir egne geometrier, materialer, kvalitetstvillinger og lerretsteksturer. Delte `artTexture()`-samplere og `kit.M` frigjøres ikke.
- Modulen bruker bare allerede innlastede `concrete`, `asphalt`, `paper` og egne lerreter. Bildene i runde 7 er levert som kildefiler. Claude utvider `art.ts`, bestemmer innlasting og bytter dør-, vindus- og pussmaterialene samt dokumentunderlagene inn. Ingen ny, konkurrerende loader følger.

## Runde 7

Alle seks er RGB, sRGB og JPEG kvalitet 85. Innebygd ChatGPT-bildegenerator er brukt, seks kall totalt. Eksakt modellversjon er ikke eksponert. Originale genererte PNG-er er bevart utenfor Git under de dokumenterte lokale kildebanene. Eksportene er skalert fra disse; det er ikke påstått at generatoren leverte briefens mål direkte. Ingen kodebasert retusjering eller ekstra bilder utover behovet.

| Fil under `src/assets/art/` | Eksport | Bruk |
| --- | --- | --- |
| `motel/door_room_blank.jpg` | 512 × 1024 | 0,92 × 2,03 m, nummer i kode |
| `motel/window_night_lit.jpg` | 512 × 512 | 1,4 × 1,2 m, varme gardiner |
| `motel/window_night_dark.jpg` | 512 × 512 | Samme karm/gardiner, mørk lysvariant |
| `motel/tex_motel_wall.jpg` | 1024 × 1024 | 2,0 × 2,0 m per repeat |
| `docs/card_field_1947.jpg` | 1024 × 640 | 5 × 8 tommer, underlag for E11 |
| `docs/letter_paper_1947.jpg` | 1024 × 1365 | Brettet i tre, underlag for E13 |

Fulle prompts, bevarte kildehash, eksportoperasjoner og begrensninger: `motel/MOTEL_DOOR_WINDOWS.json` og `docs/MOTEL_PAPERS_WALL.json`. Det mørke vinduet er laget med det tente vinduets kilde som redigeringsmål. Begge er inspisert sammen. Pussen er inspisert i vanlig 2×2-repetisjon; små slitasjeflekker gjentas, og kantene er ikke påstått pikselidentiske. Papirene har ingen bokstaver. Feltkortet har 11 blå linjer, rød marg og kaffemerke. Brevet har to bretter. `THIRD_PARTY_NOTICES.md` bevarer eldre kreditering og registrerer disse prosjektassettene uten en oppfunnet CC0-lisens.

## Kontrollstatus

Kontrollbilder, kildehash og JSON ligger i `evidence/motel-2026-10-04/`:

- PASS: TypeScript uten emit og Python-syntaks etter siste rettelse. 69 eksisterende runtime-, test- og kjernedokumentfiler er hashkontrollert som uendret mot `db47e04`.
- PASS: 29/29 kontraktkontroller før Recreate, 30/30 etter. Alle 11 faktiske `Interaction.pick`-treff prøves samtidig med vanlig 2,4 m rekkevidde fra radiusklare ståpunkter. Telefonen prøves fra `officeInside`. Ekte `Player.update` går innkjørsel til kontor, kontor til rom 6 og retur, med radius 0,27 m.
- PASS: fem pålagte utsnitt × to kvalitetsnivåer × to skjermstørrelser, 20 uredigerte hovedbilder. DOM-viewport, canvas og filenes faktiske 1280×800/844×390 er kontrollert. Begge telefonutsnitt følger i tillegg. Separat lesende review har inspisert alle 20 hovedbilder.
- PASS: 15–38 modul-tegnekall i de 22 faste bildene, der de fem hovedutsnittene bruker 20–38. De målte første konstruksjonene i sluttversjonen tok 71,3 og 38,7 ms; tre Recreate-runder tok 23,7, 13,2 og 11,2 ms. Dette er prøvemaskinens målinger, ikke en FPS-måling eller en garanti for alle enheter.
- PASS: High/Low beholder lampe- og vindusbytter. Uavhengige brytere, åpent dørblad og tre Recreate-runder har kontrollbilder. Én modul og flomlys 10/10 etter hver utskifting. Egne ressursers dispose-hendelser bekreftes; ingen delt oppstartstekstur frigjøres. Ingen fangede konsollfeil eller advarsler i sluttkontrollen.
- PASS: seks assetfiler, to manifester, kilde-/referansehash og alle 15 gallerilenker, 57/57 kontroller. Galleriet har seks lastede bilder og ingen vannrett overflow ved DOM 1280×800 og 390×844. Galleribildenes faktiske pikselmål står separat i manifestet.

Visuell review fant svarte striper på ganggulvet fordi asfalt, betong og kontorgulv lå over hverandre på y=0. Gulvplatene er delt og møtes kant mot kant; soner, kollidere og treffflater er bevart. De to bildene `review-before-floor-fix-*` er merket historikk fra den forkastede geometrien. Alle andre runtimebilder viser sluttversjonen.

Forbedringsfunn til Claude: beholdt Sierra-skilt bruker `DoubleSide` med samme UV på begge sider, som i eksisterende `Exterior.ts`. Fra veiutsnittet (-27,6) ser man baksidens speilvendte tekst; oversiktsbildet viser riktig forside. En egen baksideflate med riktig UV kan rette dette ved innkobling. Denne eksisterende skiltflaten er ikke eid eller endret av MotelFront.

Kapittel 4, innlasting av de seks nye bildene i produktet, ekte tastatur-/berøringsreise, FPS, Safari og lyd er ikke verifisert av denne isolerte leveransen. Tidligere grønne prolog-, kapittel-1/2/3- og lagringstester er gjenbrukt som historikk for uendret kode. Claude utfører én samlet integrasjonstest etter innkobling.
