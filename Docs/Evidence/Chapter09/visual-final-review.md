# Chapter09 — uavhengig visuell sluttkontroll

Vurdert 10. september 2026 av review-agenten, deretter utvidet med den aktive ruten og lupesyn fra samme bygg. **PASS for de observerbare visuelle kravene i både passiv og aktiv rute. Ingen kritisk visuell FAIL gjenstår i det inspiserte materialet.** Dette er bildeinspeksjon, ikke en godkjenning av mekanikker, kollisjon, lagring, lyd eller ytelse.

Alle tre bildeserienes byggmanifest oppgir commit `9329a2d819108796c6f731cc305f16dc3250c651`, ren arbeidsmappe ved bygging, Unity-kildehash `86a28b30ad94fc9e67aa0bf0558f0a51f511adadc76323fcf69eca7c72d8c8b9` og bygginnholdshash `6b0c99dc455267875ac836594e6f1478fb58515d337313032604325c2de6056b`. Git HEAD ble kontrollert mot samme commit. Denne rapportfilen er et senere dokumentasjonstillegg.

## Grunnlag og avgrensning

Originale runtime-bilder ved 1280×800 fra `Artifacts/Chapter09/Final-Passive-Control/` (C), `Artifacts/Chapter09/Final-Passive-Finish/` (F) og `Artifacts/Chapter09/Final-Active/` (A). `chapter09-result.json` ble brukt bare til å koble kontrollpunkt og bildefil. Ingen editor, spillinput, teleportering, retusjering eller bildebehandling ble brukt av kritikeren. Målene var de genererte `photolab-target-v1.png` og `field-target-v1.png`, samt eksisterende `control-room-v1.png` og `phone-desk-v1.png`. Målbildene er visuell retning, ikke runtime-bevis.

## Observasjon → spillerkonsekvens

| Visuelt krav | Status | Bildebevis og vurdering |
| --- | --- | --- |
| Fotolab, skala og arbeidslys | PASS | F10–12: fri midtgang, benkehøyde og forstørrer/kar/vask gir en forståelig fysisk arbeidsplass. Krem/grønt, varmt arkivlys og lokal rød safelight skiller funksjonene. Forstørrerens hode, belg og linse kan skilles fra bakgrunnen. Tidligere mørke hovedrekvisitter er rettet. |
| Kort, kart og instrumentstatus | PASS | F10 viser leselig **LOAD / TRANSFER / COLLECT**. F12 viser S-03, B-12 og forbindelsen uten at lampen dekker teksten. F13 har leselige CRT-er, **SCHEDULED TRACK / 042** og jevnt tak uten den tidligere utbrente flekken. F14 viser grønne receiver-LED-er; bildet alene sertifiserer ikke gjenopptaksmekanikken. |
| B-12 og synlig kontrollendring | PASS | C11→12: én bred lys stripe på referansen blir en smal, dreid flate; den varme lyspølen forsvinner. Stativ, rekkverk, pad og store antennesilhuetter gjør sted og skala lesbare. C13 viser tydelig B-12-identitet og kontrollmerking. Dette beviser to synlige tilstander, ikke animasjonens tidsforløp. |
| Nødvendig fotografisk trekk | PASS | C05: to lyse, parallelle vertikale striper kan pekes ut under den sentrale antennen, uten markør eller metadata. F04: høyre fotografi har en bred, blek stripe på en frontvendt flate **til venstre for den smale, dreide fysiske referansen**. I normalvisningen C12/C14 finnes bare den dreide referansen. Avviket er synlig også før høyre foto markeres; teksten trenger ikke erstatte bildet. |
| Fotoinspeksjon, sammenligning og respons | PASS | C03/05/07/08 og F04–07: tekst, to bilder, lupestyring, valgmuligheter og korrigerende tilbakemelding holder seg innen panelet. Begge opptak kan sees samtidig, med skilt nummer og opptakstid. Bildene underbygger den støttede observasjonen. Knappenes funksjon må bekreftes i inputtesten. |
| Lokal avslutning og nærrekvisitter | PASS | F08–09: rapport, avgrenset konklusjon, uløst −39 LY, returknapp og lisenskreditering er leselige uten beskjæring. F15: telefonens form, håndsett og knapper kan skilles ut i nærbildet. |

## Aktiv kontroll og faktisk endret lupeutsnitt

| Tilleggskontroll | Status | Bildebevis og vurdering |
| --- | --- | --- |
| Aktivt feltoppsett før/etter | PASS | A32→33/35: referansen dreies synlig, mens den varme lyspølen fortsatt lyser på pad og stativ. Dette skiller oppsettet visuelt fra den mørklagte passive prøven. Det normale søkerbildet A35 viser bare én fysisk referanse. |
| Aktivt kontrollfoto og sammenligning | PASS | A39 viser uten markør en ekstra bred, lys stripe på den frontvendte flaten til venstre, ved siden av den dreide og mørkere fysiske referansen. Fenomenet kan pekes ut uten metadata. A41 forklarer den avviste lampetolkningen innen panelet; A43–44 viser den aktive prøvens egen konklusjon om referanse/encoder, med leselig utfall og kreditering. |
| Forstørring og horisontal panorering | PASS | A45→46: utsnittet går fra 1.0× til vist 1.8×; antennen og dobbelstripen blir tydelig større, og ytterkantene beskjæres. A46→47: samme forstørring beholdes mens motivet flyttes mot venstre og mer av høyre antenne kommer fram. Markøren følger referansedetaljen. Dette er faktisk forskjellig bildeutsnitt, ikke bare en endret verdi i UI. |
| Gjenåpnet kontrollbilde | PASS visuelt | A48 viser begge opptak igjen med 1.0×, samme motiver og opptakstider, og begge markeringer synlige. Fotoidentitet, filhash og fravær av duplikater må fortsatt dokumenteres av tilstand-/filkontrollen; skjermbildet alene beviser dem ikke. |

## Gjenstående visuell ambisjon

Målbildenes materialrikdom er ikke fullt oppnådd: flere vegg-/metallflater og utepad er flate, stolene har kantede silhuetter, og mørkt terreng/himmel har lite dybde. Dette er synlig restarbeid i materialer, kontaktlys og former; rapporten hevder ikke målbildelikhet eller ferdigpolert sluttgrafikk. Det skjuler ikke nødvendige spor, handlinger eller rutemarkeringer i den kontrollerte ruten.

**UNVERIFIED i denne gjennomgangen:** kontinuerlig bevegelse/animert signatur, vertikal panorering og lupens yttergrenser, faktisk lyd og inputrespons, lagre/last, hovedreisens gjennomførbarhet, ytelse og pakket oppstart. Disse må avgjøres av de separate sluttkontrollene. Visuell PASS alene gir ikke hele kapitlet PASS.
