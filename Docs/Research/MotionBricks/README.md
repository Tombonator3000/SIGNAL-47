# MotionBricks i animasjonsarbeidsflyten

**Status:** Referanse, lokal integrasjon UNVERIFIED. **Gjelder:** kandidat for animasjonsproduksjon til Blender/Tripo3D og SIGNAL /47 i UE5.8.4/Linux. **Sist kontrollert:** 31. oktober 2026.

Tom ba om å legge MotionBricks i repoet og prosjektbiblioteket. Dette er registrering med festet kilde, konkret bruksplan og lisensgrenser. Modellvekter, demoklipp, Python-miljø og sanntidsintegrasjon er ikke installert, og ingen av spillets leverte animasjoner er erstattet her.

## Kilder og faktisk dekning

- [Offisiell kode](https://github.com/NVlabs/motionbricks/tree/54fc563aeaaf819a753d72cc9fdc6713989cdd43), festet til `54fc563aeaaf819a753d72cc9fdc6713989cdd43`.
- [NVIDIAs modellpakker](https://huggingface.co/nvidia/MotionBricks) og [prosjektside](https://nvlabs.github.io/motionbricks/).
- [Kildemanifest](upstream.json): kontrollerte dokument-/lisensfiler ved den festede koderevisjonen, med SHA-256 og Git-blob-ID. Dokumentkopiene er bevart i prosjektbiblioteket.
- [Prosjektbibliotekets vurdering](https://github.com/Tombonator3000/prosjektbibliotek/blob/main/docs/MOTIONBRICKS.md) og [søkbart løsningskort](https://github.com/Tombonator3000/prosjektbibliotek/blob/main/losninger/grafikk/motionbricks-animasjon-til-egen-rigg.md).

README, Runtime API, Scene Preview, pyproject og relevante kode-/modell-/eksempeldata-vilkår er lest. Dette bekrefter hva kildepakken beskriver, ikke at den er prøvd på vår rigg, vår GPU eller UE5.8.4.

## Foreslått bruk for Roswell

MotionBricks lager korte bevegelser mellom fire nylige positurer og opptil fire målpositurer. SOMA-modellen bruker menneskeproporsjoner og 30 modellledd. Den forstår ikke fritekst eller scenens geometri. Den eksisterende Roswell-riggen trenger derfor kartlegging og retargeting, ikke bare direkte innlasting av modellvektene.

Første prøve er **stå, luske, titte rundt et hjørne og flykte**. Bruk egne/eide start- og målpositurer, generer bevegelse, overfør til en separat kopi av Roswell-riggen i Blender, rett føtter/skuldre/hender og bak ordinære klipp til Unreal. En konverterings-/eksportbro må lages og prøves; ferdig FBX-eksport og ferdig Unreal-plugin er ikke verifisert. Fingre og særegen alien-/krabbebevegelse trenger fortsatt egen regi.

Produksjonsforslaget er ferdigbakte klipp, slik at NVIDIA-inferens ikke blir et krav hos spilleren. Den medfølgende Python/TensorRT-demoen trenger NVIDIA-GPU. Opphavet oppgir Linux/Windows x86-64 og RTX4090, men dette er ikke vår lokale kompatibilitetstest.

Eksisterende `S47RoswellBrain`, `S47RoswellWorld`, native kollisjon og kontakt-/lydsamordning beholdes under prøven. MotionBricks erstatter ikke verdensregler eller jaktlogikk. Bevar originale hudvekter, rigg, modell/LOD, skala, fotpivot og fungerende animasjon. Sammenlign samme sekvens i Blender og ekte Unreal-runtime for fotglidning, root motion, loop-sømmer, dørklaring og lesbarhet før produksjonsbytte.

## Lisens og innhenting

Koden oppgis under Apache-2.0, NVIDIA-dokumentasjon/media under CC-BY-4.0 og modellvekter/ONNX under NVIDIA Open Model Agreement. BONES-demobevegelsene og oppførte avledninger har separat NVIDIA Sample Data Evaluation License for intern evaluering/testing av NVIDIA-teknologi. De skal ikke automatisk kopieres til spillet eller offentlig Git, og generering fra dem antas ikke å fjerne vilkårene. Avhengigheter og robotassets har egne vilkår. Se de festede kildene og bibliotekets vurdering.

Ingen automatisk nedlasting/installasjon er knyttet til denne referansen. Den åpne produksjonsprøven føres som **ANIM-01** i [TODO](../../Active/TODO.md), uten å flytte de åpne krasj-/progresjonsfeilene ned i prioritet. Installert spill og personlige Saved er uendret av registreringen.
