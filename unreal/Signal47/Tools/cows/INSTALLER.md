# Lokal installasjon av ku-kandidaten

`install.py` endrer aldri Git, starter ingen prosesser og stopper ingen spill. Standard `install` validerer bare. `--write` kreves for backup og faktisk payloadkopiering til `/home/tom/Games/Signal47/Linux`. Kjør først etter bestått native import, fersk assetkontroll, pakket runtime og faktisk gjennomgåtte spillbilder.

Frys den komplette kandidatpakken og den faktiske prosjektmappen som ble bygget. Source, Config, Content, Shaders og `.uproject` tas med som SHA256-inventar. Bruk isolert profil utenfor kandidaten under test, slik at kandidaten aldri får Saved-noder.

```sh
python3 Tools/cows/install.py snapshot \
  --candidate /home/tom/scratch-bm/Signal47-cows-20261025/candidate-cows-r1/Linux \
  --source-root /home/tom/scratch-bm/Signal47-cows-20261025/project/unreal/Signal47 \
  --out /home/tom/scratch-bm/Signal47-cows-20261025/candidate-snapshot.json
```

Etter tester lager root en faktisk sluttproof. Følgende viser bare formatet, ikke utførte prøver:

```json
{
  "schema": "s47-cows-install-v1",
  "status": "PASS_READY_TO_INSTALL",
  "package_sha256": "EXACT_PACKAGED_EXECUTABLE_SHA256",
  "snapshot": {"path": "candidate-snapshot.json", "sha256": "ACTUAL_SHA256"},
  "native_import": {"path": "native-import.json", "sha256": "ACTUAL_SHA256"},
  "native_verify": {"path": "native-verify.json", "sha256": "ACTUAL_SHA256"},
  "runtime_cases": [
    {
      "name": "selftest",
      "returncode": 0,
      "command": ["/ABS/CANDIDATE/Linux/Signal47.sh", "-S47SelfTest", "-S47SmokeTest"],
      "package_sha256": "EXACT_PACKAGED_EXECUTABLE_SHA256",
      "snapshot_sha256": "ACTUAL_SNAPSHOT_FILE_SHA256",
      "log": {"path": "selftest.log", "sha256": "ACTUAL_SHA256"},
      "required": ["self test OK", "smoke test OK"]
    }
  ],
  "visual_review": {"path": "visual-review.json", "sha256": "ACTUAL_SHA256"}
}
```

Runtime-listen må også ha unike `pasture`, `pasturemissing` og `ninth`. De bruker henholdsvis `-S47Pasture=still`, `-S47Pasture=missing` og `-S47Ninth` eller `-S47Ninth=found`. Alle fire må fullføre smoke, ha faktisk ku-logg fra alle tre native assetstiene og riktig særskilt case-markør. Kommandoen må starte nettopp kandidatens binær eller shellstarter. Flere ferdige scenarioer kan føyes til. Hver logg, prøve og bildetolkning må gjelde den samme frosne pakken.

Native-rapportene må ha nøyaktig de tre avtalte meshnavnene og stiene. Fersk verify må referere til SHA256 av den bundne importrapporten og ha samme LOD-/materialmetadata.

Visuell rapport bruker dette formatet, med minst ett faktisk runtimebilde av hver av `alive` og `abducted`:

```json
{
  "status": "PASS",
  "package_sha256": "EXACT_PACKAGED_EXECUTABLE_SHA256",
  "images": [
    {"path": "pasture.png", "sha256": "ACTUAL_SHA256", "subject": "alive", "reviewed": true, "verdict": "PASS"},
    {"path": "ninth.png", "sha256": "ACTUAL_SHA256", "subject": "abducted", "reviewed": true, "verdict": "PASS"}
  ]
}
```

Bilder og rapporter kan ligge utenfor bevismappen ved absolutte stier, men aldri inne i kandidaten eller installasjonen. Dette skriptet kontrollerer bytes og erklærte bindinger; selve bildekvaliteten må faktisk undersøkes av root før PASS settes.

```sh
python3 Tools/cows/install.py install \
  --proof /ABS/final-install-proof.json --proof-sha256 ACTUAL_SHA256 \
  --work /home/tom/scratch-bm/Signal47-cows-20261025
```

Etter vellykket validering kjøres samme kommando med `--write`. Ingen gamle PR-/mergekrav fra T02 er innført. Før skriving kreves streng Unreal-idle uten installed-game-grace. Hver payloadfil får en tvungen reflinkbackup med `cp -a --reflink=always`; alle bytes, filtyper, modusbits, eier/gruppe, lenkemål, tommapper og utvidede attributter kontrolleres før skriving. Backupen inneholder ingen Saved-noder.

Kopiering og rollback bruker `rsync -aX --checksum --delete --exclude=Saved`. Checksummen hindrer at filer med samme størrelse og samme sekunds mtime blir hoppet over. Alle Saved-noder, inkludert tommapper, lenker, modes og xattrs, må være nøyaktig uendret. Ved feil etter skriving forsøkes rollback av bare payload under ny idle-kontroll. Saved blir aldri overskrevet eller gjenopprettet. En ekstern endring gir FAIL og beholdes. Installasjonsrapport og full rollbackbackup beholdes i en unik `installation-cows-*`-mappe; skriptet rydder ingenting.

Mønsteret bygger på de lokale gjennomgåtte `install_field_models.py` og `install_t02_r1.py`. Bibliotekssøk med ett bredere forsøk ga ingen mer relevant installasjonsløsning. Den isolerte btrfs-fixturen bestod reflink, nøyaktig utskifting ved lik størrelse/mtime, Saved-bevaring og rollback. Dette er forberedelsesbevis, ikke en faktisk spillinstallasjon.
