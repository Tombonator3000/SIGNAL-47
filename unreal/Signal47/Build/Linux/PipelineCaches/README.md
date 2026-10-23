# SIGNAL / 47, Vulkan SM6 PSO-bundle

`S47_Signal47_SF_VULKAN_SM6.spc` er et cook-input, ikke en ferdig drivercache.
Unreal 5.8.4 bygger den om til `Signal47_SF_VULKAN_SM6.stable.upipelinecache`
ved Linux-cook. Den inneholder stabile referanser til faktisk brukte pipelines;
shaderbytekoden kommer fra den tilhørende cooken.

Et opptak dekker de 20 verdenskameraene og 18 lys-/strøm-/menytilstandene
ved 02:15 og 05:40, på Low/Clean, High/VHS og Ultra/VHS. Nye materialer,
Props, shaderinnstillinger eller renderbaner krever nye opptak og ny cook.
En åpnet bundle alene beviser ikke at nytt innhold er dekket.

Kjør én tung Unreal-jobb om gangen, med minst 10 GiB tilgjengelig. Fra
prosjektmappa, etter materialimport og en vellykket pakket cook:

```sh
python3 Scripts/pso-cache.py record --build ~/scratch-bm/Signal47-build \
  --out /absolutt/tom-bevismappe/pso --qualities high low ultra --resume
python3 Scripts/pso-cache.py expand --recordings /absolutt/tom-bevismappe/pso
bash Scripts/package-linux.sh
```

`record` fryser `.shk` fra samme cook før første tur. `--resume` gjenbruker
bare komplette, uforstyrrede opptak med identisk pakke, parametre og
kontrollerte filhasher. Forkastede opptak bevares utenfor det aktive treet.
`expand` krever hele matrisen og avviser endrede eller ekstra cachefiler.

Kontroller til slutt at cooken leste denne `.spc`, skrev mer enn null
graphics-PSO-er, og at pakket runtime åpnet samme binære cache-GUID og
behandlet graphics-oppgaver. `Scripts/pso-cache.py verify --help` beskriver
beviskontrollen. Runtime-loggen trenger `-LogCmds="LogRHI Verbose"`.
Mål deretter kald og varm oppstart i egne profiler, med en fersk NVIDIA-cache
for kaldprøven. Se `unreal/LIGHTING.md` for leveransens faktiske resultater.
