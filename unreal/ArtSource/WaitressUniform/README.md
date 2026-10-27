# Servitrisens originale uniform

`build_uniform.py` lager et løst blått serveringsantrekk med hvit spisskrage, korte ermer og hvite mansjetter, et faktisk hvitt smekkeforkle med stropper, livbånd og lommer, samt lukkede mørke arbeidssko. Formene er konstruerte tøypaneler og ringer, ikke kopierte ansikter fra kroppsoverflaten. Retningen følger prosjektets eksisterende portrett av servitrisen.

Skriptet kjøres med Blender gjennom prosjektets sekvensielle kjørevern. Det mottar en privat eksport av den eksakte kroppen, måler skjelett og noen skalare yttergrenser, lager egne stofflater, overfører deformasjonvekter og eksporterer med det uendrede skjelettet.

```sh
blender -b -P build_uniform.py -- /privat/waitress_body.fbx /privat/uniform-output
```

Privat utmappe får `SKM_Waitress_Uniform.fbx`, `waitress_uniform_fitting.blend`, `uniform_report.json` og to forhåndsbilder. `--no-preview` hopper bare over bildene. Ingen cloth-simulering brukes. Importerte MetaHuman-kilder, skjelett, vektede eksporter og fitting-scenen skal ikke legges i offentlig Git.

Dette er forfatterkilde. En vellykket Blender-kjøring alene bekrefter ikke passform eller runtime. Bilder forfra/skrått må vurderes, deretter importeres antrekket på samme skjelett og kontrolleres med faktisk kroppstomgang, ansiktsanimasjon og disk i et pakket spill.
