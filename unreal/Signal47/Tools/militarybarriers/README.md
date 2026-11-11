# Military Barriers-gjerdet ved SARO (D45-14)

Toms funn i d45: gjerdet mellom SAROs vindusside og antennefeltet skal være det bedre «wired fence» fra Cosmos/Leartes-pakken Military Barriers (`~/Desktop/MilitaryBarriers.zip`). Pakken er kjøpt og lisensiert. Ingen av filene skal i Git, Git LFS eller en patch. Cosmos-bilder åpnes ikke og gis ikke til AI-verktøy.

Pakken er et UE 4.22-prosjekt med rot `/Game/MilitaryBarriers`. Den roten er ikke ignorert i Signal47, så flyttingen skjer i et kasserbart UE 5.8-prosjekt. Bare resultatet, `Content/Licensed/MilitaryBarriers`, kopieres inn i Signal47. Den mappen er ignorert (`unreal/.gitignore`: `Signal47/Content/Licensed/`), og `DefaultGame.ini` koker hele `/Game/Licensed`.

De nødvendige filene er allerede pakket ut i `~/scratch-models/militarybarriers/`: 14 gjerdemesher, 4 materialinstanser, `MM_Basic_PBR` med fem materialfunksjoner og 24 teksturer, til sammen omtrent 55 MB. Kart, blueprints og betongsperrer er ikke med.

`import_fence.py` gjør dette i det kasserbare prosjektet:

- flytter `/Game/MilitaryBarriers` til `/Game/Licensed/MilitaryBarriers` og etterlater ingen redirector;
- slår av Nanite på alle meshene;
- kompilerer `MM_Basic_PBR` for instansierte mesher;
- lagrer alle pakker på nytt i 5.8-format;
- skriver `verification.json` med bounds for hver mesh og rammekontrollene som runtime krever.

Skriptet nekter å kjøre i `Signal47.uproject`.

## Kjøring

Kjør jobbene etter tur i hver sin ferske prosess, gjennom prosjektets motorreservasjon. Ingen annen Unreal-jobb skal gå samtidig.

```sh
UE=$HOME/.local/opt/UnrealEngine-5.8.4
SRC=$HOME/scratch-models/militarybarriers
WORK=$HOME/scratch-models/fence58
REPO=/home/tom/SIGNAL-47/unreal/Signal47
df -h "$HOME"                      # kopien er ~60 MB; minst 100 GB skal være ledig etterpå
[ -e "$WORK" ] && { echo "fence58 finnes fra før: avvikle den kontrollert først"; exit 1; }
mkdir -p "$WORK" && cp -a --reflink=auto "$SRC/Content" "$WORK/"
cat > "$WORK/Fence58.uproject" <<'EOF'
{ "FileVersion": 3, "EngineAssociation": "UE_5.8", "Category": "", "Description": "",
  "Plugins": [ { "Name": "PythonScriptPlugin", "Enabled": true }, { "Name": "EditorScriptingUtilities", "Enabled": true } ] }
EOF
python3 "$REPO/Scripts/unreal_guard.py" require-idle
S47_FENCE_MODE=import nice -n 19 "$UE/Engine/Binaries/Linux/UnrealEditor-Cmd" "$WORK/Fence58.uproject" \
  -run=pythonscript -script="$REPO/Tools/militarybarriers/import_fence.py" -unattended -nullrhi -nosplash -NoSound -NoLiveCoding
S47_FENCE_MODE=verify nice -n 19 "$UE/Engine/Binaries/Linux/UnrealEditor-Cmd" "$WORK/Fence58.uproject" \
  -run=pythonscript -script="$REPO/Tools/militarybarriers/import_fence.py" -unattended -nullrhi -nosplash -NoSound -NoLiveCoding
```

Begge kjøringene må gi `S47 MILITARYBARRIERS FENCE … PASS` i loggen og `"status": "PASS"` i `verification.json`. Kopier deretter resultatet inn og kontroller at Git ikke ser det:

```sh
mkdir -p "$REPO/Content/Licensed" && cp -a --reflink=auto "$WORK/Content/Licensed/MilitaryBarriers" "$REPO/Content/Licensed/"
git -C /home/tom/SIGNAL-47 check-ignore -v unreal/Signal47/Content/Licensed/MilitaryBarriers/Meshes/SM_MilitaryBase_WiredFence_b.uasset
git -C /home/tom/SIGNAL-47 status --porcelain -- unreal/Signal47/Content/Licensed/MilitaryBarriers   # skal være tom
```

`check-ignore` skal vise regelen fra `unreal/.gitignore:9`. Når kopien i Signal47 er kontrollert, kan `fence58` ryddes ved neste samlede opprydding. `verification.json` inneholder bare mål og navn, ingen lisensierte filer, og kan ligge i Git.

## Runtime

`S47ArrayFence::Build` kjøres rett etter `S47StreetLamps::Build`, fordi begge klipper den samme stolpebatchen (node 35, g26). Med pakken på plass og målt som her:

- De tre ulyste flatene fra eksporten skjules på nodenivå, slik at de holder seg skjult når området byttes. Flatene finnes etter forelder, plass og bredde, aldri etter indeks.
- De gamle 5 cm-stolpene klippes ut av batchen med `S47StreetLamps::CutBatch`.
- `WiredFence_b` legges som instanser, ett panel per 2,5 m. Hvert panel har sin egen stolpe og strekkes høyst ±10 % langs lengden: 21 + 7 + 24 paneler.
- Hver løpe lukkes med en bar stolpe (`EndCap_c`).
- `Wire_b` gir piggtråd langs de skrå armene, uten NATO-spiral.
- Armene peker sørover mot gården og veien.
- Hvert panel står på bakken som er målt under midten av panelet.
- Gjerdet har ingen kollisjon. Gangen og bilen beholder 2D-linjene.
- Materialene er belyst PBR, så SAROs strøm demper dem ikke.
- Grindbladet blir `Gate_R` som barn av hengselet (`world.field.gateHinge`) og svinger med historien. Låsen og kjettingen blir værende.

Hvis noe avviker, står det gamle gjerdet.

`-S47ArrayFenceOld` eller `-S47PropsOff` gir det gamle gjerdet. `-S47ArrayFenceNoWire` fjerner piggtråden.

Kontroller:

- `-S47SelfTest` kjører `S47ArrayFence::SelfTest` (oppsettet av panelene) og `Audit` (gjerdet slik det ble bygget).
- F10 «Looks (M6)» har «Array fence: as built», «… by day» og «… at night».
