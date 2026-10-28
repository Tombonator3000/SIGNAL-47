# Kreditering for Sierra Motor Court-produksjonen

## Inkludert originalarbeid

Motellet, fasadedekoren, resepsjonens treinnredning, det originale skiltet, bassenget, gangveiene, parkeringsdetaljene, valnøttpanelet i rom 6, prosedyrematerialene, UV-klargjøringen, LOD-ene og eksportskriptene ble laget for SIGNAL / 47 med hjelp fra Blender/Python. Prosjektets eksisterende geometrikonvensjoner ble gjenbrukt for den statiske nøkkeltavlen og telefonbasen. Dette dokumentet merker ikke disse prosjektutviklede ressursene som CC0; bruken følger SIGNAL / 47-prosjektets egne vilkår.

`T_Sierra_Asphalt.jpg` er kopiert fra prosjektets originale `web/src/assets/art/road/tex_oldroad_asphalt.jpg`. Kildemerknaden i `web/THIRD_PARTY_NOTICES.md`, runde 11 (5. oktober 2026), beskriver det som et originalt, oppdiktet bilde laget for SIGNAL / 47 med den innebygde bildegeneratoren i ChatGPT og bearbeidet/kontrollert for prosjektet. Det hevdes **ikke** å være CC0. Kildeprompter og genererte originaler dokumenteres av `web/src/assets/art/production/round11_manifest.json` og den tilhørende mappen `round11_qa/sources/`.

Brukerens fotografier av KiMo Theatre og motellrom ble brukt som visuelle referanser. De vedlagte fotografiene, vannmerkene og gjenkjennelige kunstverkene inngår ikke som modellteksturer. Fasademønsteret og romdetaljene er nylaget geometri.

## Inkluderte CC0-materialkilder

De flyttbare teksturatlasene inkluderer farge- og ruhetsdetaljer fra disse Poly Haven-teksturene:

| Ressurs | Opphavsperson | Lisens | Kilde |
| --- | --- | --- | --- |
| Painted Plaster Wall (`painted_plaster_wall`) | Rob Tuytel | CC0-1.0 | https://polyhaven.com/a/painted_plaster_wall |
| Concrete Floor 02 (`concrete_floor_02`) | Rob Tuytel | CC0-1.0 | https://polyhaven.com/a/concrete_floor_02 |

Lisensreferanse: https://polyhaven.com/license

Det lokale prosjektets `Unity/Assets/Signal47/Art/Visual10/source-manifest.json` dokumenterer kildeadresser, opphavspersoner, CC0-lisens og opprinnelige filhasher. `provenance.json` ved siden av dette dokumentet oppgir inndatafilene som brukes i V2-materialgrafene. Normalkartene i denne pakken er nybakte tangentnormaler fra kildens bump-nettverk; de er ikke kopier av Poly Havens OpenGL-normalkart.

## Utelatte lokale modellreferanser og ressurser

Den opprinnelige lokale filen `test-v2/Sierra_Motor_Court_Test_V2_Kelvora.blend` inngår ikke i den rensede produksjonspakken. Den inneholder tre eksisterende spillbiler laget av **Daniel Zhabotinsky**, under CC-BY-4.0:

- Fairheaven LT '80: https://sketchfab.com/3d-models/fairheaven-lt-80-low-poly-model-e2678da920cc4be68dbc193727919ffb
- Negotiator '80: https://sketchfab.com/3d-models/negotiator-80-low-poly-model-892b2684fb37442299bfad9222cf331c
- Fairheaven SW '84: https://sketchfab.com/3d-models/fairheaven-sw-84-low-poly-model-aa0becb6e854422596cac6b21bf79787

Opphavsperson: https://sketchfab.com/DanielZhabotinsky
Lisens: https://creativecommons.org/licenses/by/4.0/

Disse modellene og bildeavhengighetene deres er utelatt fordi pakkens omfang er begrenset til originalt innhold og CC0-innhold. Utelatelsen innebærer ingen påstand om at CC-BY forbyr bruken. Spillet beholder de eksisterende bilene og krediteringene separat. Den fullstendige krediteringsteksten finnes fortsatt i den lokale V2-filens `CREDITS.txt`.

Følgende er også utelatt:

- Eksisterende KELVORA-TV-geometri og medier, opprinnelige interaktive telefonrør, nøkler, dører og bevisobjekter, den originale egenproduserte sengen og figurer: beholdes gjennom spillet under kjøring og dupliseres ikke her.
- Presentasjons-HDRI-en `kloppenheim_06_puresky` av Greg Zaal og Jarod Guest, Poly Haven, CC0: https://polyhaven.com/a/kloppenheim_06_puresky . Verken HDRI-en eller scenebelysningen fra den inngår eller er bakt inn i grunnfargen.
- Presentasjonsterreng, fjerne åsrygger, testveier, den enkle bilmodellen som ble brukt som plassholder, lokale kamera- og lysoppsett og referansefotografier.

Den endelige låste filoversikten for pakken og rapportene fra Blender-opprydding og verifikasjon i Unreal dokumenterer hva som faktisk inngår. Denne krediteringsfilen beskriver opphav og tiltenkt omfang; den sertifiserer ikke uavhengig hver eksporterte binærfil.
