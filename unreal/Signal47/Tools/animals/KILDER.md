# Kilder: dyrene i Freeform

**Status:** Gjeldende. **Gjelder:** `src/*.glb`, importert av `import_animals.py` til `/Game/Free/Animals` (ignorert av Git, som de andre frie modellene). Brukes av `S47Animals` (ANIMALS-20261108). **Sist kontrollert:** 8. november 2026.

| Fil | Brukes som | Modell og opphav | Lisens | Kilde |
|---|---|---|---|---|
| `src/quaternius_wolf.glb` | coyoten | Wolf, Quaternius, 1928 trekanter, 51 ben, klipp Walk, Gallop, Idle, Idle_2, Idle_2_HeadLow, Eating og flere | CC0 1.0 | [Poly Pizza](https://poly.pizza/m/P1gU3Qkr9r), fil `static.poly.pizza/f1d12388-e39b-4157-b32a-646a1d089fc4.glb` |
| `src/quaternius_husky.glb` | gårdshunden og hundene i Caliche | Husky, Quaternius, 1822 trekanter, 49 ben, samme klipp og Attack | CC0 1.0 | [Poly Pizza](https://poly.pizza/m/wcWiuEqwzq), fil `static.poly.pizza/611d25c7-430f-4bb5-ab2c-d8f5f3cb9712.glb` |

Lisensen er lest av modellsidens egne data, `"Licence":"CC0 1.0"`, med Quaternius som opphavsperson, 8. november 2026. Filene er lastet ned uten innlogging. CC0 krever ingen kreditering, men opphavet føres her og i `Docs/Research/FREE_ASSET_CATALOG.md`.

Filene er ikke endret. Spillet farger dem selv med prosjektets lyse materiale:
- coyoten får støvbrun pels og lys buk
- hunden får mørkebrun pels og kremfarget bryst
- øynene på begge lyser når lykta eller frontlysene treffer dem

Spillet skalerer coyoten til 70 cm og hunden til 62 cm høyde, og snur dem etter sitt eget skjelett, med hodet foran kroppen. Uten de importerte modellene er coyoten de gamle boksene, og hunden høres bare.

Ingen modell er fra et spill, og ingenting er hentet fra `~/Desktop/Gard-modeller-Blender`. Stilen er lavpolygon. Om den passer på nært hold om natta, må Tom se.

## Kandidater som ikke ble brukt

Disse er ikke lastet ned:
- Coyote fra Poly by Google, CC-BY 3.0, statisk og uten skjelett: [poly.pizza/m/auVAs_kT6nE](https://poly.pizza/m/auVAs_kT6nE).
- Mourning dove fra Poly by Google, CC-BY 3.0, statisk: [poly.pizza/m/1cF8DTp2sAi](https://poly.pizza/m/1cF8DTp2sAi).

Dette ble lastet ned og vurdert, men ikke brukt: Pigeon fra Quaternius, CC0, er en tegneseriefigur uten flygeklipp, [poly.pizza/m/9NGlBTpDEr](https://poly.pizza/m/9NGlBTpDEr).

Fuglene som letter er fortsatt spillets egne små former med vingeslag. Lyden av vingene er ekte, se `Tools/nightsounds/KILDER.md`.

## Bruk

Fra `unreal/Signal47`, gjennom køen, én tung jobb om gangen:

```sh
python3 ~/.claude/skills/signal47-nattskift/scripts/q.py Tools/animals/import_animals.sh
```

Skriptet leser tilbake modellen og klippene og skriver `S47ANIMALS DONE PASS` eller hva som mangler.
