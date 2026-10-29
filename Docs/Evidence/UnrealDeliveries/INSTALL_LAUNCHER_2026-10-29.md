# Lokal levering til spillstarter, 29. oktober 2026

**Status:** Referanse. **Gjelder:** Farm R1 på Linux/Vulkan, installert etter Toms bestilling. **Sist kontrollert:** 29. oktober 2026.

Farm R1 er faktisk installert i `/home/tom/Games/Signal47/Linux`. Begge eksisterende spillstartere bruker fortsatt `Signal47.sh` i denne mappen. Kilde `c301ead9313510125d037a4cc020d667cd9c5661` er runtimeidentisk med kontrollert main `094a67cb3e1ba68e34521fb19add28726609e89c` gjennom PR223. Dette er en lokal levering; offentlig nedlasting er uendret. Senere HUM-01/ART-01-arbeid inngår ikke.

[Portabel kvittering](INSTALL_LAUNCHER_2026-10-29.json) binder resultatene til binær, payload og lokale originalkvitteringer. [STATUS](../../Active/STATUS.md) fører dagens leveringsnivåer.

## Innhold og kontroll

Pakken viderefører grafikkmenyen og tidligere lokale leveranser, samt den nye rolige kvelden/nattrekkefølgen, SC01-endringene og Dales dørstemme, SARO-dialogretting, Linux-minnetiltak, TB2, trafikksignal, trafofelt, Caliche-boliger/skilt, motellrettinger, nye dispatchopptak, meteorpassasje og gård med stengt port. Tidligere målrettede bevis beholder sine egne grenser; fullmatrisen er ingen menneskelig gjennomspilling av alle disse endringene.

| Kontroll | Faktisk resultat |
| --- | --- |
| Kilde-/pakkefrys | 13 465 runtime-inputs og åtte kjernepayloads kontrollert mot originalmanifest. Alle 68 payloadnoder er uendret gjennom testen og identiske i installasjonen. |
| Samlet kanonisk matrise | 111/111 PASS fra main-kontrakten, inkludert 1720 selvtestkontroller. Bare absolutte guard-stier og offscreen/unattended er tilpasset miljøet. Ingen registrert tungjobbkonkurranse. |
| Pakket meteor, Low og High | 18 kontroller hver, phase8/complete/PASS; maksimalt to/tre samtidige hoder. Hendelsestid, frysing og engangsavspilling består. Kapittel 1-state skrevet til disk og gjenopprettet gjennom Flow gir ingen replay, også med eldre lagring. Dette er kontraktens testfil, ikke ordinær WriteSave/Continue. 12/13 PNG-er er integritetskontrollert. |
| Visuell kandidatkontroll | Alle 111 bilder sett i fire merkede oversikter; rom og spillinnstillinger åpnet i full størrelse. Åtte meteororiginaler åpnet: naturlig vindu, A med streker, B med skjulte streker og etterfase på begge kvaliteter. High-skyene kan bevege seg mellom A/B; bakgrunnen hevdes ikke pikselidentisk. |
| Faktisk installert grafikkmeny | Skrivekontroll 47 og fersk separat leseprosess 15 PASS fra installasjonens `Signal47.sh`, med isolert profil. |
| Faktisk installert Fortsett | Ordinær Fortsett fra en fersk fysisk kopi av personlige Saved består. Riktig sted og tidspunkt er visuelt kontrollert. Ingen Case/Jump-overstyring; normal autosave tillates bare i den isolerte kopien. |
| Bevaring og sluttintegritet | 119 Saved-noder / 92 filer, begge launchere og kandidat er identiske med fersk førsnapshot. COW-tilbakekopi av tidligere installasjon er verifisert. Sluttstatus PASS_INSTALLED_FINAL. |

## Installasjon, bevaring og opprydding

Installasjonen brukte eksisterende prosjektverktøy med fersk snapshot, reflink-tilbakekopi og checksum-synkronisering som utelater Saved. Etterkontrollen omfatter bytes, metadata og launcherbindinger. Prosjektbibliotekets generelle EngineWire-treff var UNVERIFIED og ble ikke gjenbrukt som implementasjon.

Originalkvitteringer og tilbakekopi er lokale i `/home/tom/scratch-bm/Signal47-install-launcher-20261029/`. Tilbakekopien er `previous-installed/Linux`; den inneholder payload, uten personlige Saved. Private lagringssnapshots, personlige skjermbilder og lisensierte pakker publiseres ikke i repoet.

Kanonisk prune ble faktisk kjørt med `--apply` etter gjennomsett plan: null pensjonerte pakker/filer. Gjeldende Farm R1 og de to nyeste eldre Dispatch R1 / Motell R1 beholdes. Midlertidig beskyttet i tillegg er denne installasjonens grafikk-R2-tilbakekopi, FIX01-minnekandidaten og den historiske Ultra-komparatoren for åpen startup-OOM. Ingen antakelse om bare tre fysiske pakker eller frigjorte byte. Kilder, Saved, logger og bildebevis er bevart. Beskyttelsene må vurderes når rollback-/diagnosebehovet opphører.

Seriens reservasjon ble frigitt 29. oktober kl. 10:20:44 Europe/Oslo etter normal avslutning. Andre utviklere kan bruke guard-vinduet videre; ingen fremmede prosesser ble stoppet.

## Kjente grenser

Det installerte Fortsett-bildet viser fortsatt den kjente, ikke-kritiske meldingen om RT always-resident geometry: 80,842 MiB mot 20 prosent av 400 MiB. Dette er ingen ny krasj, men advarselen er ikke rettet av denne installasjonen. Første historiske startup-OOM og den uidentifiserte E04-klumpen ved bassenget står åpne.

Fysisk input/gamepad, subjektiv lytting, menneskelig full gjennomspilling, FPS og andre maskiner er UNVERIFIED. Oversiktsbildene er ikke individuell typografikontroll av alle 111 originalene. Hele SC03 til SC13 og UFO-finalen er ikke ferdigmeldt.
