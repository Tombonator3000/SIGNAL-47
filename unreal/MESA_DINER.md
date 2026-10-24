# Mesa Diner, 24. oktober 2026

Tom bestiller original Blender-diner med interiør, parkeringsplass, skilt og lastebil utenfor. Visuell referanse er `Concepts/Caliche-2026-10-23/01-mesa-diner.png`: mineralpuss, teal fliser, slitt rød stripe, burgunder båser og krom/Formica. Kilden er egne modeller og prosedyremalte PBR-kart, ingen nye eksterne assetkjøp.

Status: under arbeid på `codex/mesa-diner`. Native import, pakket visuell vurdering, merge og installasjon er ennå ikke utført.

Dinerens faktiske rot er Three(7979.95,4.309006389843889,1999.95), yaw0. Blenderlokal (x,y,z) blir Three(root.x+x,root.y+z,root.z-y). Eksisterende inngang, interaksjoner, mennesker, mat, spillerbil og lagring beholdes. Visuelle erstatninger må være atomiske og beholde hele originalen ved ufullstendig import.

Planlagt nødvendig kontroll: faktiske eksporterte mål/UV/materialer og tre LOD-er, fersk native avlesning, editorbygg og pakket dag/skumring/natt fra spillerhøyde, kort fysisk inngang-/interaksjonsprøve, eksisterende takeaway/disklasting og to gamle motiv. Ingen fullverdens- eller PSO-matrise i dette passet.

Første Blender- og native kontroll av elleve deler bestod, men utvendig R1 er diagnostikk. Ny fasaderunde etter Toms tilbakemelding: skilt forankres tett ved venstre fasade i Blender `[2.65,-6.8,0]`, høyere krem/rød gesims, dypere rammer, styrt malingsslitasje og ujevn asfalt/grus med lokale tørre planter. Interiør og fungerende ankre beholdes.

Kontrollpunkt før pakket kandidat: R2 har 110779 / 54448 / 25418 trekanter fordelt på elleve deler, faktisk kildekontroll320 PASS og lagret native47 PASS. Editorbygg består. Pakket visuell kontroll, merge og installasjon er fortsatt ikke utført.

D1 avdekket gulvlist gjennom døråpningen og en levetidsfeil i den nye audit-timeren. Råforsøket er bevart. D2 deler bare listen, retter timeren, bruker Walkers komplekse gulvtrace med uendret 10cm-krav og flytter sign-punktene fra 3.9cm til omtrent 55cm foran flaten. Sluttkilde/native-kontroll og målrettet pakke utføres på nytt for disse konkrete rettingene.
