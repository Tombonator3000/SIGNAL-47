# Ressurser og kreditering — Resources19

Kontrollert 13. september 2026. Lydfilene her er en prøvepakke, ikke den aktive Unity-miksen. De er fullengde Vorbis-konverteringer med −3 dB nivåmargin. Originalenes SHA-256, varighet, kanaler, samplingsfrekvens og bearbeidede filhash er registrert i [manifestet](audio-manifest.json).

- **Echoes of the Past — isaiah658.** [Kilde](https://opengameart.org/content/echoes-of-the-past), [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/). `Audio/echoes_of_the_past.ogg`.
- **Outpost / Dirty Rain — Tsorthan Grove.** [Kilde](https://opengameart.org/content/the-world-fell-silent), [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/). `Audio/outpost_loop.ogg`, `Audio/dirty_rain_loop.ogg`.
- **12 Ambient Machine Sounds — Michael Brigida and students.** [Kilde](https://opengameart.org/content/12-ambient-machine-sounds), [CC BY 3.0](https://creativecommons.org/licenses/by/3.0/). Alle øvrige filer i `Audio/`. OLPC-utvalget ble organisert og klargjort av Dr. Richard Boulanger (Dr. B). Opptakene ble gjort 1999–2007 ved Berklee Studios og i Boston-området under Associate Professor Michael Brigida, av over 250 Music Synthesis-studenter i Advanced Sampling Class ved Berklee College of Music. Diane Douglas og Colman O’Reilly redigerte de opprinnelige opptakene. Lastet opp på OpenGameArt av bart. SIGNAL / 47 har konvertert formatet og senket nivået 3 dB; originalopphavet beholdes.

CC BY-krediteringen og endringsopplysningen skal følge filene ved videre bruk. CC0-sporene krediteres frivillig. Ingen av de nye lydfilene er lagt i spillerbygget; dersom de integreres senere, må relevante krediteringer også inn i spillets leveranse.

## Modellarbeid

`Unity/Blender/Source/resources19_dish.py`, `SIGNAL47_resources19_dish.blend` og de to R19 FBX-filene er original geometri laget for SIGNAL / 47. Eksisterende telefon og antennegeometri er bevart i kildefamiliene. NASA-modellen er kun lastet ned som lokal kandidat; ingen av dens geometri eller teksturer er brukt i R19-modellene.

- [NASA 70 meter dish](https://science.nasa.gov/3d-resources/70-meter-dish/), kilde NASA/Ames Research Center. [NASA 3D Resources](https://github.com/nasa/NASA-3D-Resources) omtaler samlingen som gratis og uten copyright; [bruksretningslinjene](https://www.nasa.gov/nasa-brand-center/images-and-media/) gjelder fortsatt, inkludert særregler for logo og inntrykk av støtte. Filen oppbevares lokalt som vurderingsmateriale, ikke som ferdig SARO-antenne.
- [NRAO VLA](https://www.vla.nrao.edu/) og [azimutlager](https://www.vla.nrao.edu/genpub/work/azbear/) er konstruksjonsreferanser, ikke en tildelt assetlisens. Vi bruker egne modeller og kopierer ikke nettsidebilder til spillet.
