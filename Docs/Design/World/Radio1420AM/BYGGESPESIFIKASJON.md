# 1420 AM: byggegrunnlag til Unreal

**Status:** Under arbeid (designforslag). **Gjelder:** Radiostasjonen i Caliche. **Sist kontrollert:** 12. november 2026.

Konsept og byggegrunnlag, ikke implementert eller spilltestet. Behold krempuss, grønn tredør, stort studiovindu mot gaten, mørkt tre, akustikkplater og analoge båndmaskiner. AM-stasjonen er ikke observatoriets 1420.405 MHz.

## Mål og rom

Lokale koordinater i cm: X øst/høyre, Y nord/bakside, Z opp. Gate i sør. Ikke verdensplassering. Fotavtrykk 1400 × 1200. Gulv Z=0, underside tak 300, parapet 380. Ingen kjeller eller øvre etasje. Romgrenser er veggakser; yttervegg 30, innervegg 15 cm.

| Rom | X | Y |
|---|---|---|
| Resepsjon | 0–400 | 0–600 |
| Kontrollrom | 400–1000 | 0–600 |
| Direktestudio | 1000–1400 | 0–600 |
| Båndarkiv | 0–400 | 600–1200 |
| Utstyrsrom | 400–1000 | 600–1200 |
| Personalrom | 1000–1400 | 600–1200 |

Valgfri WC i personalrom X=1200–1400,Y=1000–1200, dør på sørvegg ved (1300,1000). Privat blindvei uten obligatoriske spor. Utstyrsrom betyr lydutstyr med skjermede rackskap, ikke eksponert høyspenningssender.

## Åpninger

Alle dørsentre nedenfor har Z=0. Fri bredde 120, høyde 220 cm etter karm; hovedinngang 150 bred. Karm, hengsling og nisjer skal bevare minst 140 cm fri ganglinje. Skjær faktiske veggåpninger.

| ID | Forbindelse | X,Y |
|---|---|---|
| E1 | Gate ↔ resepsjon | 200,0 |
| D1 | Resepsjon ↔ kontrollrom | 400,300 |
| D2 | Kontrollrom ↔ studio | 1000,300 |
| D3 | Resepsjon ↔ arkiv | 200,600 |
| D4 | Kontrollrom ↔ utstyr | 700,600 |
| D5 | Studio ↔ personal | 1200,600 |
| D6 | Arkiv ↔ utstyr | 400,900 |
| D7 | Utstyr ↔ personal | 1000,900 |
| E2 | Personal ↔ østgang ute | 1400,900 |
| E3 | Arkiv ↔ bakgård | 200,1200 |

Akustikkglass i veggen mellom kontrollrom og studio må ikke erstatte D2 eller oppta dørslaget. Studiovindu mot gaten er ikke skjul eller utgang. Terreng og repos ved E1–E3 møter gulvnivået; tilpass faktisk tomt før integrasjon. Utvendig omvei minst 200 cm fri fra gate via begge sideganger til bakgård. Mast er ikke en klatrerute.

## Undersøkelse, sikt og ruter

Stasjonen har en etablert rolle ved kildekontroll av eldre opptak. Romforslag: spilleren følger en loggføring fra kontrollrom til båndarkiv. Dette oppretter ikke et nytt fast historieoppdrag.

Rundløype: resepsjon → kontrollrom → studio → personal → utstyr → arkiv → resepsjon. D4 gir en snarvei mellom kontrollrom og utstyr. Alle undersøkelsesrom har minst to interne dørforbindelser. E2/E3 fungerer når E1 er blokkert. Alle fluktdører virker begge veier uten ukjent nøkkel eller ny mekanikk.

Forskjøvede båndskap ca. 200 cm høye i arkivet og lukkede rackskap i utstyrsrommet gir siktbrudd, med minst 140 cm passasje på begge sider. Behold serviceavstander og ventilasjon. Mikserbord og studiostol må ikke sperre D2 eller D5. Glass og lave bord gir ikke skjul. H markerer foreslått siktbrudd, ikke automatisk trygghet.

## Kontroll før detaljering

Blokker E1 og deretter hver intern forbindelse separat; kontroller vei fra kontrollrom, studio, arkiv og utstyr til uteområdet. Test begge retninger med faktisk spillerkapsel og dørkollisjon i Unreal; sjekk sikt og eventuell fiendenavigasjon. En romgraf er bare en designkontroll. Ingen hopping, ventilkryping eller flytting av møbler skal kreves.

Plansjen er skjematisk; teksten styrer gråboksen ved avvik. Bevar fungerende verdensplassering, interaksjoner og nabobygg når forslaget tilpasses faktisk tomt. Plansje og full ImageGen-prompt ligger i samme mappe.

Romgraf kontrollert 12. november: alle hovedrom når uteområdet med hovedinngangen blokkert, og ved separat blokkering av hver intern forbindelse. PASS gjelder bare forbindelsesgrafen; Unreal-test gjenstår.

Bildeavvik: valgfri WC er merket 140 × 140 cm i bildet; bruk 200 × 200 cm etter romkoordinatene over. Møbelplassering og dørslag i bildet er skjematiske, ikke målte frie gangbredder.
