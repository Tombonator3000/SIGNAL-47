# Fôrbutikk / Feed & Seed: byggegrunnlag til Unreal

**Status:** Under arbeid (designforslag). **Gjelder:** Planlagt bygg i Caliche, Unreal-utgaven. **Sist kontrollert:** 12. november 2026.

Dette er konsept og byggegrunnlag, ikke implementert eller spilltestet.

Behold kremfarget slitt puss, treveranda, korrugert metalltak, falmet FEED & SEED-skilt, sekker og treinnredning. Bygget skal føles som en liten lokal forretning i 1986.

## Rom og hovedmål

Fotavtrykk 1600 × 2000 cm. Gulv Z=0, underside tak Z=400, parapet ca. Z=480. Ingen etasjer over eller under. Sørplass og lastegård kan ligge Z=-45; angi terrengovergangene eksplisitt.

| Rom | X | Y |
|---|---|---|
| Butikk | 0–1000 | 0–1000 |
| Kontor | 1000–1600 | 0–600 |
| Pakkerom/personal | 1000–1600 | 600–1000 |
| Fôrlager | 0–1000 | 1000–2000 |
| Lastemottak | 1000–1600 | 1000–2000 |

Valgfri privat WC-alkove i pakkerom X=1000–1200,Y=800–1000, dør mot øst ved (1200,900). Ikke utvid alkoven inn i kontor- eller lagerforbindelsene.

## Åpninger (dørsenter X,Y, alle ved Z=0)

| ID | Forbindelse | X,Y |
|---|---|---|
| E1 | Sørplass ↔ butikk | 500,0 |
| D1 | Butikk ↔ kontor | 1000,300 |
| D2 | Kontor ↔ pakkerom | 1300,600 |
| D3 | Butikk ↔ fôrlager | 300,1000 |
| D4 | Pakkerom ↔ lastemottak | 1300,1000 |
| D5 | Fôrlager ↔ lastemottak | 1000,1500 |
| E2 | Lastemottak ↔ østlig lasterepos | 1600,1500 |
| E3 | Fôrlager ↔ nordlig bakgård | 300,2000 |

E2 går til fast lasterepos i Z=0, ikke rett ut over en kant. Reserver repos X=1600–1800,Y=1300–1700 og rampe langs østfasaden fra Y=1300 mot Y=400 med 900 cm løp og 45 cm fall. Minst 150 cm bredde, plane repos i begge ender, rekkverk og fri dørslagssone. Rampen vender mot sørplassen. E1 får 900 cm rampe langs sørverandaen fra øst mot inngangsrepos, med samme fall og fri bredde. E3 må også ha trinnfri terrengforbindelse: reserver egen 900 cm rampe bak bygget fra nordlig dørrepos mot bakgården. Bakrampen kan legges langs nordfasaden mot øst som vist i plansjen, med eget flatt dørrepos. Dersom terrenget allerede møter gulvnivået der, utgår denne siste rampen etter kontroll. Ingen påkrevd rute skal baseres på å hoppe fra lasterampen. Tomtebehovet omfatter disse rampene.

## Undersøkelse og ruter

Forslag: spilleren sammenligner en leveringsseddel på kontoret med merkede varer i lageret. Dette er en situasjon for romdesign, ikke et nytt fast historieoppdrag. Løype: butikk → kontor → pakkerom → lastemottak → fôrlager → butikk. Fra lageret kan spilleren velge E3 eller E2 via lastemottaket. Med E1 sperret har begge ender av løypen fortsatt vei ut.

To forskjøvede reoløyer i lageret, ca. 200 cm høye, får ganglinjer på begge sider. I butikken brukes et ugjennomsiktig vareskap til siktbrudd; lave sekker er dekor og regnes ikke automatisk som skjul. Vogner og pallevarer skal ikke stå i de frie passasjene eller rampene.

## Felles byggekrav

Planakser X mot øst/høyre og Y mot nord/bakside er lokale centimeterkoordinater; de er ikke verdenskoordinater. Gaten er sør. Alle romgrenser er nominelle veggakser. Yttervegg 30 cm og innervegg 15 cm; ta hensyn til tykkelsen ved møblering. 1 uu = 1 cm. Åpninger minst 120 × 220 cm fri etter karm, hovedinngang 150 cm. Frie ganglinjer minst 140 cm, utvendig omvei minst 200 cm. Dørslag skal ikke redusere disse målene; bruk dørnisje eller juster hengsling ved behov. Skjær faktiske åpninger gjennom hele veggen. Ingen rute skal kreve hopping, kryping eller flytting av møbler.

Alle oppgitte dører skal fungere begge veier når undersøkelse og mulig jakt er aktiv. Vis låser dersom de senere innføres, og bevar kjent alternativ utgang. H betyr foreslått siktbrudd bak ugjennomsiktig inventar, ikke garantert trygghet. Møbler som ikke stopper sikt teller ikke som skjul. WC er en bevisst privat blindvei uten obligatoriske spor.

Behold byggets eksisterende identitet og spillankre når dette senere integreres. Foreslått fotavtrykk må tilpasses faktisk tomt; ikke flytt nabobygg eller fungerende interaksjoner automatisk. Dette oppdraget leverer design, ikke nye spillrom i runtime.

## Kontroll før detaljering

Blokker hele hovedinngangen og kontroller vei til side-/bakutgang fra hvert viktig undersøkelsesrom. Blokker deretter én intern forbindelse om gangen. Bygg gråboks med faktisk spillerkapsel, dører og inventar; prøv begge retninger, frihøyde, dørslag, sikt og eventuell fiendes navigasjon i Unreal. Ingen trapper eller kjeller inngår. Romgrafkontroll er bare en designkontroll og dokumenterer ikke fungerende jakt, kollisjon eller AI.

Den genererte plansjen er skjematisk. Koordinatene i dette dokumentet styrer gråboksen ved avvik. Bildets detaljer og tekst må kontrolleres før modellering. Plansje og prompt ligger ved siden av dette dokumentet.


Romgraf kontrollert 12. november: alle hovedrom når uteområdet med hovedinngangen blokkert, og ved separat blokkering av hver intern forbindelse. PASS gjelder bare forbindelsesgrafen; Unreal-test gjenstår.

Plansjens rampelengder og møbler er skjematiske: alle tre ramper skal ha 900 cm faktisk løp og 45 cm høydeforskjell pluss plane repos. Ikke mål rampelengde fra bildepiksler. H-symbolene viser høye reoler som siktbrudd; verifiser at hyllene er ugjennomsiktige i øyehøyde.
