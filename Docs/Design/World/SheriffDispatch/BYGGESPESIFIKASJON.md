# Sheriff og dispatch: byggegrunnlag til Unreal

**Status:** Under arbeid (designforslag). **Gjelder:** Sheriffkontoret i Caliche. **Sist kontrollert:** 12. november 2026.

Konsept og byggegrunnlag, ikke implementert eller spilltestet. Behold lav krempussfasade, inntrukket tredør, gittervinduer, varmt praktisk lys, arkivskap, kart, skrivemaskin og analog sambandsradio. Ingen celler, kjeller eller våpenrom legges til.

## Mål og rom

Lokale cm-koordinater: X øst/høyre, Y nord/bakside, Z opp. Gate sør. Ikke verdensplassering. Fotavtrykk 1600 × 1600. Gulv Z=0, underside tak 320, parapet 400. Én etasje. Romgrenser er nominelle veggakser; yttervegg 30, innervegg 15 cm.

| Rom | X | Y |
|---|---|---|
| Resepsjon | 0–600 | 0–800 |
| Arkiv | 0–600 | 800–1600 |
| Midtkorridor | 600–1000 | 0–1600 |
| Dispatch | 1000–1600 | 0–800 |
| Sheriffkontor | 1000–1600 | 800–1600 |

Valgfri WC i kontorets nordøsthjørne X=1400–1600,Y=1400–1600, privat sørdør ved (1500,1400). Ingen gjennomgang eller obligatoriske spor i toalettet.

## Åpninger

Alle dørsentre har Z=0. Fri dør minst 120 × 220 cm etter karm, hovedinngang 150 cm bred. Ganglinjer minst 140 cm; dørslag må ikke redusere dette. Lag nisje eller juster hengsling der nødvendig. Skjær faktiske åpninger.

| ID | Forbindelse | X,Y |
|---|---|---|
| E1 | Gate ↔ resepsjon | 300,0 |
| D1 | Resepsjon ↔ korridor | 600,400 |
| D2 | Arkiv ↔ korridor | 600,1200 |
| D3 | Dispatch ↔ korridor | 1000,400 |
| D4 | Kontor ↔ korridor | 1000,1200 |
| D5 | Resepsjon ↔ arkiv | 300,800 |
| D6 | Dispatch ↔ kontor | 1300,800 |
| E2 | Korridor ↔ bakgård | 800,1600 |
| E3 | Arkiv ↔ vestgang ute | 0,1200 |

Korridorens sørende er lukket, ikke en ekstra gateinngang. Gittervinduer er ikke fluktruter. Utgangsrepos møter gulvnivå; minst 200 cm fri gang rundt bygget forbinder gate, vestgang og bakgård. Kontroller mot faktisk tomt. E2 og E3 skal virke begge veier i aktiv undersøkelsessituasjon; ingen uvarslet personalnøkkel ved jakt.

## Undersøkelse, sikt og ruter

Forslag: spilleren sammenligner en dispatch-logg med en arkivmappe. Dette er et romdesign-eksempel, ikke et nytt fast historieoppdrag eller implementert romtilgang.

Vestsløyfe: korridor → resepsjon → arkiv → korridor. Østsløyfe: korridor → dispatch → kontor → korridor. Med E1 blokkert når begge sløyfene E2/E3. Kontorene trenger ikke toalett som gjennomgang.

Arkivet får forskjøvede, ugjennomsiktige skapøyer ca. 200 cm høye med minst 140 cm passasje rundt. Kontoret får en fast, ugjennomsiktig oppbevaringsskjerm med to veier rundt; hold både D4 og D6 fri. Lav dispatch-disk og glass regnes ikke som skjul. H er siktbrudd, ikke automatisk immunitet mot en forfølger.

## Kontroll før detaljering

Kontroller romgraf med E1 blokkert og hver intern forbindelse blokkert separat. Test deretter reell kollisjon, dørslag, sikt og eventuell fiendenavigasjon i Unreal. Romgraf alene beviser ikke fungerende jakt. Alle ruter skal kunne brukes begge veier uten hopping, kryping eller flytting av møbler.

Plansjen er skjematisk; teksten styrer gråboksen ved avvik. Tilpass fotavtrykket til eksisterende tomt og bevar fungerende spillankre. Plansje og full ImageGen-prompt ligger i samme mappe.

Romgraf kontrollert 12. november: alle hovedrom når uteområdet med hovedinngangen blokkert, og ved separat blokkering av hver intern forbindelse. PASS gjelder bare forbindelsesgrafen; Unreal-test gjenstår.

Kjente bildeavvik: pilen i korridorens sørende stopper ved lukket vegg og er ikke en utgang; bruk E1 via resepsjonen. Bildets bruttoareal skal være 256 m² totalt; WC inngår i kontoret og skal ikke legges til som ekstra areal. WC bygges etter koordinatene over. Bildets «verified/OK» er diagramtekst, ikke dokumentert test av gangbredde eller runtime. Bare romforbindelsesgrafen er kontrollert.
