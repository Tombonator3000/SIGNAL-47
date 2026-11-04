# EASTWING-01: laboratorieutstyr

`build_lab.py` er redigerbar Blender-kilde for separate, lokalt forankrede rekvisitter. Ingen av delene er plassert i spillverdenen. Claude eier romplassering, samhandling, lys, kollisjon og Unreal-import.

## Gjenbruk og opphav

- Mikroskop, sentrifuge, vekt, kjøleskap, multimeter og loddebolt: original prosjektgeometri, modellert for 1986-referansene. Ingen av de tilgjengelige lokale modellfilnavnene ga passende ferdige modeller.
- Prøveglass og kolbe: original geometri. Prosjektets `ArtSource/PhotoLab/build_photolab.py:351-359` er gjenbruksgrunnlag for flaskeskulder, skrukork og inventaretikett. Kolben har en sammenhengende indre returflate og åpen hals.
- Oscilloskop: eksisterende **Tektronix RM565 Oscilloscope** av **Thusko**, CC-BY-4.0. Lisensen er kontrollert i `license.txt` inne i `/home/tom/Assets/signal47-free/sketchfab/oscilloscope_rm565.zip`. Originale UV-er og teksturer bevares. Den lisensierte modellen og avledede FBX/Blend-filer lagres privat under `/home/tom/Assets/signal47-private/EastWing/Props/Lab/`, ikke i Git. Den offentlige leveransen har en separat pekermanifest med attribusjon.
- Materialteksturer for malt stål og galvanisert metall refererer til den felles originale 2K-pakken i `../Textures/`. Små krem-, gummi- og glassflater bruker navngitte PBR-skalarverdier. Blender-glassets Transmission/IOR må settes eksplisitt i Unreal; FBX alene garanterer ikke glassuttrykket.

## Eksport og kontroll

Kjøringen skal bare startes av samordnende root etter `require-idle PASS` og produksjonsreservasjon. Kommando:

```sh
blender -b -t 4 --python /absolutt/sti/til/Props/Lab/build_lab.py
```

Blender-koordinater er meter, Z opp, apparatfront mot -Y. Pivot er (0,0,0) ved underlaget. Eksport bruker prosjektets `FBX_SCALE_UNITS`, frem -Y og opp Z. `.asset.json` dokumenterer faktiske dimensjoner, trekanttall, materialer, teksturfiler og LOD-er. `.blend` er et bibliotek med en redigerbar navngitt asset-samling. Det er ikke en ferdig spillscene.

Hver original rekvisitt får FBX, LOD1/LOD2, redigerbar Blend, lisensmanifest og et lite Blender-forhåndsbilde. Den enkle UCX-konvolutten er kun importgrunnlag: samhandlingsutstyr bør få Claudes presise kollisjon, og glass/optiske åpninger må ikke behandles som solid gameplaygeometri. Automatisk LOD-reduksjon må vurderes ved nærbruk av glasskolbe, mikroskoplinser og loddebolt.

Kilde-/Blenderkontroll er ikke Unreal-kontroll. Følg `.asset.json` og root-overleveringen for faktisk teststatus.
