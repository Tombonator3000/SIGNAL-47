# SIGNAL / 47 — aktuell overlevering

Kontrollert 10. september 2026 ved gjenopptakelse. Les denne sammen med FIELD_CAMERA_08.md. Referansekopien START_HER_SIGNAL_47.md fra 79c0742 er eldre enn aktuell kode; synkroniserte filer under sources/ skal fortsatt være urørt.

## Bekreftet nå

- Kanonisk mappe: /home/tombonator3000t/.codex/.chatgpt-projects/g-p-6aa05604a4708191a65139c1d5f89363/SIGNAL-47.
- Lokal main og GitHubs HEAD var begge 8a6b4d9f72b222d4cc8e124553d64b84770cd0e3. Arbeidsmappen var ren før denne dokumentasjonsrettelsen.
- PR 1–5 er MERGED, kontrollert gjennom GitHub. Ingen fletting ble utført ved denne gjenopptakelsen.
- Kontrollrommets CRT-er, tastaturer, stoler og første målte lydbalanse er gjennomført i pass06. Servicegårdens tur/retur og S-03-spor er gjennomført i pass07. Feltkamera, faktisk fotografering og sammenligning med motorloggen er gjennomført i pass08.
- Seneste tidligere verifikasjon: Docs/Evidence/FieldCamera08/final-af5a85d/final-verification.json. Rapporten dokumenterer 34 native tastatur-/musekontrollpunkter, 74,987 fps over 185,832 sekunder, p95 15,875 ms, p99 16,181 ms og ingen intervaller over 50 ms på Intel ARL / OpenGLCore / 1280×800 Ultra. Dette er tidligere kjøringer, ikke nye tester i denne oppgaven.
- GPU-tid, subjektiv lytting, andre maskiner og ferdig rom-/utegrafikk er fortsatt ikke verifisert.

## Riktig spillpakke

Start Artifacts/Releases/FieldCamera08-af5a85d/Signal47.x86_64, eller pakk ut Artifacts/Releases/SIGNAL47-FieldCamera08-Linux.tar.gz i en ny mappe. Lokal README pekte tidligere på en eldre GauntletLinux-utgave; dette er nå rettet. Leveransene ligger lokalt og er ikke inkludert i en vanlig kildekloning.

Ved denne kontrollen ble alle 171 filer fra Artifacts/MobileReview/worktree/Artifacts/GauntletLinux sammenlignet byte for byte med FieldCamera08-af5a85d: ingen avvik. Bygginnholdets SHA-256, med den opprinnelige manifestmetoden og filsettet, er 672a8a768f0069dc1e06e561c50ed496cb1bca7d8ca3772374aeea88bdb8a020. Leveransemappen har i tillegg LES_MEG.txt; hashing av hele mappen inklusive dette tillegget gir derfor en annen verdi. Ingen spillfiler ble endret eller nye kjøretidsresultater hevdet.

## Neste avgrensede steg

Pass08 peker på fysisk filmframkalling og et nytt spor som må tolkes visuelt. Dette er planlagt, ikke implementert. Nåværende FieldCamera.FinishPhoto gjør bildet umiddelbart tilgjengelig i notatboken; framkalling krever en eksplisitt skillelinje mellom eksponert bilde og framkalt bevis.

Foreslåtte kriterier for neste implementeringsrunde:

1. Spilleren tar bildet ute, returnerer fysisk og framkaller det ved en tydelig arbeidsstasjon før det kan undersøkes som bevis.
2. Et nytt spor må finnes i det faktiske motivet og kunne underbygges av bilde og logg. Ingen tidlig forklaring av årsaken.
3. Gjentatt bruk dupliserer ikke bevis; avbrudd, notatbok og omstart beholder en sammenhengende tilstand. Eksporterte bilder fra tidligere runder bevares.
4. Den utvidede reisen kjøres med faktisk tastatur/mus, originale spillbilder inspiseres, og berørte tester samt ytelse måles på samme sluttbygg og preset.

Før implementering må motivet/sporet konkretiseres fra tilgjengelige fortellingskilder. Bevar Unity/Blender, originale ressurser og lisenskreditering, 1419.900 / 1420.110 / 1420.405 MHz, 4/7, -39 LY og hendelsen etter 47 sekunder. Ikke flett eller publiser nye endringer uten relevant autorisasjon.
