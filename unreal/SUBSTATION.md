# Caliche Junction Substation

Tom ba 24. oktober 2026 om flere originale 3D-modeller og en plassert strømtrafo som kan undersøkes. Kontrollen av runtime, Props-konfigurasjon og eksisterende manus fant Caliche Junction bare som planlagt innhold. Dette passet lager det fysiske stedet og en avgrenset serviceinspeksjon.

## Sted og kontrakt

Anlegget plasseres ved eksisterende servicegrusvei før STATION 01, i det allerede gangbare rsPasture-området. Root er Three `(7890, -1.10, 536.20)` med 180 graders yaw. Modellen bruker Blender meter/Z opp; importens asymmetriske prøve kontrollerer `(2,4,6)` til Unreal `(200,-400,600)` centimeter.

Transformator, bussføring, kontrollskap, statuslampe, servicehendel, gjerde, port, fundament og grus er separate originale modeller. Kontrollfronten står utenfor høyspentgjerdet. Originale Blender-kilder, teksturer og eksport beholdes under `Signal47/Tools/substation`. Stasjonen overtar ingen av STATION 01s identiteter, og SAROs interne K3 endres ikke.

## Gjeldende status

Modellkilden og alle ti native deler er kontrollert: 304 kildekontroller og 41 native kontroller PASS. Tre faktiske LOD-er per del gir totalt 52 994 / 27 130 / 16 950 trekanter. Editorbygget er bestått. Pakket spill og installasjon avventer kort runtimekontroll. Verifisering begrenses til det nye stedet, adkomst og service/lagring, spillerbilder i relevante lysforhold og et kort gammelt regresjonsutvalg. Ingen full 110-runde er bestilt.

Prosjektbiblioteket ble søkt før implementering. Profilene `x-2108195576206488058` og `x-2111849852350894319` er generelle, uverifiserte Blender/Unreal-arbeidsflyter. Ingen geometri eller kode derfra brukes. Importoppsettet gjenbruker prosjektets egen målte FBX-/materialoppskrift fra `Tools/radioarray/import_assets.py`.
