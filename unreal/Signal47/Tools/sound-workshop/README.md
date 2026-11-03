# Lydverksted

**Status:** Gjeldende verktøy. **Gjelder:** Lokal lydutvikling med S47SoundMixer. **Sist kontrollert:** 3. november 2026.

Motoren kombinerer opptak i lag med individuelle nivåer, hastigheter, høypass/lavpass, utsnitt og mottaksstyrte lydkurver. `recipes.json` inneholder startpunkter for arraysøk, låst arraysignal, VHS-sporing og PC-overføring. Fire/sju-puls styres av motorens felles signalklokke.

Fra `unreal/Signal47`:

```sh
python3 Scripts/sound-workshop.py --sources-root '/home/tom/Documents/ChatGPT/Signal 47/lydreferanser'
python3 Scripts/sound-workshop.py --check
python3 Scripts/sound-workshop.py --self-test
```

Dette forbereder seks korte PCM16-opptak i mono / 48 kHz og `Saved/AudioWorkshop/bank.json`. Originalene bevares. Forberedelsen bruker eksisterende FFmpeg og Python-standardbiblioteket. Kjør igjen etter redigering av oppskriftene. `--output` kan velge en annen, separat lokal arbeidsmappe.

Send `-S47SoundWorkshop=/absolutt/sti/til/bank.json` til prosjektets Unreal-kjøring for fire 12-sekunders forhåndslyttfiler fra den faktiske lydgrafen. Kjøringen avsluttes automatisk og skriver `previews/*.wav` og `previews/results.json` ved banken. Bruk en separat `-UserDir` som ved andre utviklerkjøringer. Argumentet laster den lokale referansebanken; opptakene importeres ikke automatisk i spillet eller Content. Bankens kategorier er `receiver`, `vhs` og `pc_transfer`. Modemkilden er merket `modem_only` og avvises i de to andre kategoriene. Korte modemopptak kan bare dekke sin faktiske lengde; lange overføringer må styre videre aktivitet separat.

For lokal utprøving i mottakeren: start med `-S47SoundBank=/absolutt/sti/til/bank.json`. Da bruker SARO-mottakeren oppskriften `array_locked`, med faktisk mottakskvalitet, eksisterende fire/sju-klokke og samme volumkontroll som før. Uten en valgt bank beholdes den prosedyriske lyden. Båndprosessoren og øvrige apparater er ikke automatisk byttet til referanseklippene.

Andre apparater bruker `US47Audio::PlaySoundMix(Key, Recipe, Use, Errors, Destination)`, med egen lydnode eller den vanlige effektbussen. `Use` må samsvare med oppskriften. Deretter kalles `SoundMixFrame(Key, Clarity, Interference, Activity, T)` fra apparatets oppdatering og `StopSoundMix(Key)` ved stopp. PC-overføring bruker `ES47SoundUse::PcTransfer`; modemlyden styres av faktisk aktivitet. Ingen ny PC-overføringshandling legges til av selve lydmotoren.

Oppskriftene velger opptak, nivå i dB, avspillingshastighet, høypass/lavpass, utsnitt, løkke og kontrollkurve per lag. `pulse` bruker fire/sju-signaturen, `signal` vokser med klarhet, `static` følger forstyrrelse og lav klarhet, `activity` følger apparatets aktivitet, og `constant` er kontinuerlig. Motoren glatter nivåendringer og løkkeskjøter og begrenser utgangens amplitude. Endre `recipes.json` og kjør forberedelsen igjen for å lage nye varianter.

Nivåer i oppskriftene er forsiktige utgangspunkter. Referanseopptakene har ikke dokumentert distribusjonsklarering og forblir utenfor Git og automatisk pakking. Godkjente originaler kan senere erstatte filene uten at oppskriftenes grensesnitt endres.
