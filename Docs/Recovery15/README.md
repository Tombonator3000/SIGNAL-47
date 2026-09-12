# Recovery15 — åpne tidligere nattevakter

12. september 2026. Avgrenset neste steg i designbibelens lagringsflyt etter [PR9](https://github.com/Tombonator3000/SIGNAL-47/pull/9), som flettet designbibel og Menu14. **Testkandidat**; Visual10 er fortsatt standardutgaven. Dette passet lar spilleren bruke checkpoint-kopiene Menu14 allerede bevarer, gjennom startmenyen.

## Spillerflyt

Startmeny → **Previous Shifts** → velg checkpoint → les bekreftelsen → fortsett. Listen viser seks oppføringer per side, lokal lagringsdato og om en oppføring bruker recovery-kopi. Uleselige saker vises deaktivert. Tilbake avbryter uten å bytte sak. Det finnes ingen sletteknapp. Bytte er bare tilgjengelig fra startmenyen, ikke fra en pågående nattevakt med mulig ulagret fremgang.

Før åpning valideres filen på nytt og sammenlignes med fingeravtrykket fra forhåndsvisningen. En endret fil krever nytt valg. Gjeldende checkpoint og backup kopieres sammen til en ny unik PreviousCases-mappe før den valgte saken aktiveres. Det valgte arkivet og originale foto bevares.

Listen sorteres etter arkivmappenes synkende tids-ID, altså når checkpoint ble arkivert; datoen på knappen er datoen inne i selve checkpointet. Flere arkiver kan derfor vise samme lagringsdato. Dette er ikke tre navngitte manuelle save-plasser, og det er ingen automatisk opprydding av gamle arkiver.

## Feil og avgrensninger

En gyldig backup kan brukes når valgt primærfil er skadet. Etter bevaring fjernes gjeldende aktive backup før primærfilen byttes atomisk, slik at senere gjenoppretting ikke velger en annen sak ved en feil. Ved feil under dette byttet kan den gamle backupen ligge bare i PreviousCases, mens gammel primærfil fortsatt er aktiv. Begge filer er bevart sammen før forsøket. Det er ingen garanti om en samlet flermappetransaksjon ved strømbrudd.

Format v1, checkpoint-regler og eksisterende fotoreferanser beholdes. Kopiering av en hel profil til en annen maskin med absolutte fotostier er fortsatt et eget arbeid. Prologlagring, områdeovergang, tre samtidige saksplasser og nye historieområder er ikke implementert. Recovery15 lukker derfor ikke hele M2.

## Verifikasjon

Endelig resultat og identitet registreres i [Evidence/verification.json](Evidence/verification.json). Testene kjører i faktisk Unity-spiller med en egen merket, midlertidig profil. Testoppsettet bruker både bevisst skadede filer og en kopi av en eldre fullført v1-sak. Fotostiene i kopien flyttes til testprofilen og envelope-hash oppdateres. Den valgte tidligere saken får i tillegg en annen savedUtc-verdi for å kunne kontrollere forhåndsvisning og filbytte. Originale JPEG-bytes og kildeprofilen kontrolleres uendret.

Runtime-kontrollene dekker Menu14-regresjon, sidegrenser, avbryt, ugyldig ID, uleselig sak, backup-valg, endring mellom valg og bekreftelse, blokkert backup-destinasjon, feil ved primærskriving, bevaring av begge gamle filer og gjenoppretting av fullført sak med to foto etter faktisk scenelasting. De samme kontrollene kjøres gjennom launcheren fra et nyutpakket arkiv.

PNG-bevis er uendrede Unity-captures av API-oppsatte tilstander, ikke konseptbilder eller dokumenterte museklikk. Visuell kontroll gjelder 1280×800. Native muse-/tastaturreise, fokus/Enter/Tab, andre skjermstørrelser, ytelse og himmel under vanlig gange er fortsatt **UNVERIFIED** mens skrivebordet er låst. M1s blindprøve og spilletid på 5–6 timer er også åpne.

## Kjør igjen

```bash
bash Automation/build-menu14-linux.sh
bash Automation/run-menu14-checks.sh "/sti/til/Start-SIGNAL47.sh" "/sti/til/fullført-testprofil" --signal47-recovery15-checks
python3 Automation/package-chapter09.py --label Recovery15 --candidate --expect-source SHA256_FRA_BYGGMANIFEST
```

De eksisterende build-/testscriptnavnene gjenbrukes. Bygging bevarer gjeldende scene og lysbaking. Testene sender ingen input til skrivebordet og kjører bare ved eksplisitt flagg og profilmarkør. Pakking med `--candidate` beholder standardstarteren. Spillpakken er en lokal leveranse og følger ikke et vanlig Git-klon.

## Lokal kandidat

[Start Recovery15](../../Artifacts/Releases/Recovery15-bda088dfe638/Start-SIGNAL47.sh) eller pakk ut [Linux-arkivet](../../Artifacts/Releases/SIGNAL47-Recovery15-bda088dfe638-Linux.tar.gz). Behold hele pakkemappen. Kildehash `bda088dfe6387a55b039f17f2c3fb6b881269abdd3bb6f9a36e23806001add53`; øvrig identitet ligger i verifikasjonsrapporten.

Utpakking med Python sitt `data`-filter normaliserer enkelte filrettigheter. Den første for strenge likhetskontrollen ble derfor avvist. Sluttkontrollen sammenligner alle filnavn, bytes, pakkehash og lagrede tar-rettigheter, og kontrollerer kjørerettighetene separat. Normaliseringene er dokumentert i [extraction.json](Evidence/extraction.json). Et mellomliggende direkte spillerløp regnes ikke som test av utpakket launcher.
