# Linux-distribusjon 2. november 2026

**Status:** Gjeldende. **Gjelder:** Linux x86_64, UE 5.8.4, offentlig testutgave. **Sist kontrollert:** 2. november 2026.

## Bygg og rettelse

Distribusjonen gjenbruker pakket runtime `bfa1c26dd913d1b9d46b0db1ce732619eee5fe63`, samme spillbygg som ble installert kl. 07:03. Ingen nye gameplay- eller shaderendringer inngår i distribusjonsarbeidet. Launcher, dokumentasjon og snarvei leveres gjennom [PR344](https://github.com/Tombonator3000/SIGNAL-47/pull/344).

RTX 50-serien velger automatisk `r.Vulkan.Bindless.PreferredExtension=0` (descriptor buffer). Andre kort beholder motorens tidligere standard. `--descriptor-buffer` og `--descriptor-heap` gir eksplisitt valg. X11/Wayland velges ikke automatisk om. Mottakeren bekreftet fungerende oppstart av forrige testpakke med buffer-valget på RTX 5090, CachyOS/niri og NVIDIA 615.78.08; det er ikke en full gjennomspilling eller en mottakerprøve av dette nye arkivet. Lokal RTX 3080 beholdes på tidligere standard; buffer-valget skal ikke påtvinges alle kort.

`Play.sh`, `Play-Low.sh`, `Install-Shortcut.sh`, ikon, generisk systemkontroll og kravliste følger pakken. `Spill-SIGNAL47.sh` henter og hashkontrollerer tre arkiver. Manuell nedlasting krever hovedarkiv, Data1 og Data2 i samme mappe. Snarveien peker til den utpakkede mappen, krever ikke sudo og bevarer redigerte snarveier. Det følger ingen NVIDIA-driver med spillet.

## Kontroller

Tolv lette testgrupper består: argumenter, grafikkvalg, åtte GPU-deteksjonsfixtures, split-data/hashkontroll, feilretur/logg og snarveier med spesialtegn eller eksisterende personlige filer. Pakken bygger på 1997 beståtte selvtester og installert Fortsett. Alle 112 pakkede matrisescenarioer består. Glimpse-forventningen ble oppdatert til native Roswell-gruppe; dette og ett manglende fieldbook-scenario ble kjørt separat. Nattkontrakten består i editorens game-modus med 106 kontroller og åtte disk/Flow-gjenopprettinger. Bare testdriveren er tilpasset dagens fysiske arbeidsflyt; produksjonskoden er den samme som i pakken. De tre faktiske ZIP-arkivene er utpakket og startet gjennom Play.sh: sammensetting/hashkontroll, selvtest og tittelskjerm består på lokal RTX 3080 med standard descriptorvalg. Tittelskjermen er gjennomsett med fri QUIT-knapp. Alle 49 arkivoppføringer er lest tilbake; kjøremodus og SHA256 stemmer. Arkivene er cirka 1,475 / 1,472 / 1,475 GB og ligger under grensen per GitHub-fil.

Personlige Saved er urørt. Testene bruker egne profiler. Private modeller, redigerbare kilder og debugfiler skal ikke ligge som løse filer i spillarkivene. Lisenskreditering og nødvendige kjøretidsfiler beholdes.

## Avgrensning

Matrisen er skriptede scenarioer med skjermbilder. Nattkontrakten bruker eksisterende SC01/SC02-fixtures og produksjons-API-er; den er ikke en full manuell natt eller måling av hele spillets lengde. WORLD01s terrengstriper, WORLD04s SARO-vegger, motellkontorets vegger og den eldre SARO-renderstansen står fortsatt åpne i [TODO](../../Active/TODO.md).

Nedlastingsadresse etter publisering: [Linux-testutgave 2. november](https://github.com/Tombonator3000/SIGNAL-47/releases/tag/playtest-linux-2026-11-02). Den nye pakkens RTX 5090-mottakerprøve gjenstår.
