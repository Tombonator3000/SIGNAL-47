# SIGNAL / 47, Vulkan SM6 PSO-bundle

`S47_Signal47_SF_VULKAN_SM6.spc` er et cook-input, ikke en ferdig drivercache. Leveransen 23.10.2026 inneholder 4215 stabile PSO-er, 602 grupper og 37 480 stabile nøkkelreferanser. Filen er 92 902 byte, SHA256:

`adc3bfa7a80f891caa687416fe064dd0206ea1c4d8963e6160cf117b7037babd`

Opptaket `Signal47-gfx-2026-10-23/pso-final-v3` dekker 38 tilstander ved 02:15/05:40 på Low/Clean, High/VHS og Ultra/VHS, 228 tilstander i 60 korte, ukonkurrerende prosesser. Frosne stable keys, ELF/containerhash, bilder/logg/CSV og faktiske .rec-filer knyttes i recordings.json. Opptaks-ELF har SHA256 `8611c1938b815f1cb786ebb5ae004c64234567177995417aa3f9da17be64ba95`. Ekspansjon på UE 5.8.4 gir STBLCACH v8/SF_VULKAN_SM6 og komplett beviskjede i expanded.json.

Linux-cook bygger dette om til `Signal47_SF_VULKAN_SM6.stable.upipelinecache`: 4215 stabile varianter løses til 602 unike (0 avvist, 3613 sammenslått), deretter filtrerer motoren tre mulige ustøttede TSR-computevarianter. Resultat 599 PSO-er: 222 graphics, 310 compute, 67 RT, 156 709 byte. Binær SHA256 er `f3c1e45b3cf329c20bbfd37fd41138a21cf38a1944b392d095b77d2950466293`, GUID `01A16826498D7808B71EA771B4891BF2` i denne cooken. Pakket runtime åpner samme binære GUID, køer 599 og behandler 222 bundled graphics-oppgaver. `pso-bundle-proof.json` har 9/9 PASS. GUID og binær hash er cookresultat, ikke varig navn på alle fremtidige bygg.

Etter opptaket er kun diagnostisk uniform-strømstyring og restore korrigert i C++; gamle matrix-dark-PNG-er er derfor ikke faktisk av-bevis. Selve shaderpermutasjonene og alle 9243 cooked render-/asset-/shaderfiler er byteidentiske. Global.shk er byteidentisk; Signal47.shk har endret serialiseringsrekkefølge, men eksakt samme 21 184 logiske tupler og outputhasher. De frosne nøklene er ikke endret. Se static/final-cook-equivalence.json og stable-keys-logical-comparison.json i bevisroten. Siste pakke/ELF er ikke byteidentisk med opptakspakken; korrekt strømtilstand dokumenteres separat i sluttpakkens tests/final-indicators (sju tilfeller/36 fasebilder) og after/final-state-correction, begge PASS.

Materialenes genererte Base/Mat-assets følger ikke Git. Kjør `S47_ONLY=materials`-integrasjonen fra øverste del av unreal/LIGHTING.md før pakking med eldre Content. Masters-only reparerer grafene og setter ikke 118 nye MI-profiler. Nye materialer, Props, shaderinnstillinger eller renderbaner krever ny cachevurdering/opptak; en åpnet gammel bundle er ikke dekning av nytt innhold.

Kjør én tung Unreal-jobb om gangen, minst 10 GiB tilgjengelig. Fra prosjektmappa etter materialimport og vellykket pakket cook:

```sh
python3 Scripts/pso-cache.py record --build ~/scratch-bm/Signal47-build \
  --out /absolutt/tom-bevismappe/pso --qualities high low ultra \
  --warmup 4 --hold 4 --resume
python3 Scripts/pso-cache.py expand --recordings /absolutt/tom-bevismappe/pso
bash Scripts/package-linux.sh
```

Record fryser samme cooks .shk før første tur og gjenbruker bare identiske hashkontrollerte PASS-blader. Expand avviser endrede/ekstra cachefiler og delvis matrise. Kontroller deretter cookens faktiske SPC-input, positive graphics-antall og runtime med samme fysiske fil/GUID/prosjektalias. `Scripts/pso-cache.py verify --help` beskriver beviskontrollen; runtime trenger `-LogCmds="LogRHI Verbose"`. Kald/varm oppstart og lengre High-prøver tas på ferdig cachepakke. Se unreal/LIGHTING.md for faktiske tall og sluttprøvenes resultater.
