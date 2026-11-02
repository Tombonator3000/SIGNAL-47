# Trafostasjon: sammenhengende grus ved rampen

**Status:** Gjeldende leveringsbevis. **Gjelder:** Runtime `bfa1c26d`, UE5.8.4/Linux/Vulkan, [PR340](https://github.com/Tombonator3000/SIGNAL-47/pull/340). **Sist kontrollert:** 2. november 2026 kl. 07:03 CET.

Installert til standardstarteren. En inklusiv interiørtest i grusgeneratoren behandlet rampens nøyaktige kantpunkter som innside. Konturklipping fjernet dermed en smal stripe inn mot betongen. En streng interiørtest lar kantpunktene beholde den planlagte grushøyden og betongleppen på 1,5 cm. Dette lukker åpningen langs begge sider og tåen på alle tre LOD-er.

Bare ApronGrade er importert på nytt. De andre tre Field-modellene og de eksisterende materialene er byteidentiske. Redigerbar full Blender-kilde, målrettede FBX-/GLB-eksporter, samlet GLB og runtimekontrakt er oppdatert. Kildens faktiske native importgrunnlag ble kontrollert før endring. Plassering, gulv, kollisjon og T02-fotoankre beholdes. Kulverten er fortsatt fotografisk referanse i T02 og har modellert innløp/avløpsform; den skal derfor ikke fjernes.

## Kontrollert resultat

75 kildeprøver av sider/tå består på alle tre LOD-er. Selektiv nativeimport består 27 kontroller med bevarte materialer, renderflagg, LOD-grenser og NoCollision. LOD-trekanttall er nå 3588/1217/454, mot tidligere 3522/1166/411. [Avgrenset resultat](SubstationApron-2026-11-02/summary.json).

Editorbygg og målrettet native før/etter består. Faktisk gange opp og tilbake er omtrent 12 m. Ferdig pakke består 1997 selvtester, 15 målrettede kontroller og de to gjennomsette bildene nedenfor. Den eksisterende Story-prøven består gangadkomst, lukket gjerde, fysiske picks, journal/service/faktisk lampe/sammenligning og vanlig disk/Flow-restore. Dette er lokal inspeksjon, ikke en full ny T02/SC02-gjennomspilling.

Installert Fortsett består. Alle 164 personlige Saved-elementer er uendret. Ingen offentlig spillrelease.

## Samme kamera før og etter

[Kant før](SubstationApron-2026-11-02/before-day-edge-lod2.png) og [kant etter, ferdig pakke](SubstationApron-2026-11-02/after-day-edge-lod2.png).

[Tå før](SubstationApron-2026-11-02/before-day-toe-lod2.png) og [tå etter, ferdig pakke](SubstationApron-2026-11-02/after-day-toe-lod2.png).

Begge seriene bruker samme kamerastilling og tvunget visuell LOD2. Kildeflatene og nativeantall dekker alle tre LOD-er; dette er ikke tre separate GPU-bildeprøver. Grusen er fortsatt dekorativ NoCollision. Gangprøven måler det eksisterende analytiske gulvet, ikke ny fysisk autoritet fra grustrekantene. Første plassering på den gangbare diner-stien og inspeksjonskameraene er eksplisitte fixtures; videre gange bruker vanlig TestMove/Resolve uten mellomplassering. Dette retter ikke WORLD01/04 eller motellvegger.
