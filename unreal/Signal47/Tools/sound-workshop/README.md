# Lydverksted

**Status:** Gjeldende verktøy. **Gjelder:** Lokal lydutvikling med S47SoundMixer. **Sist kontrollert:** 3. november 2026.

Motoren kombinerer opptak i lag med individuelle nivåer, hastigheter, høypass/lavpass, utsnitt og mottaksstyrte lydkurver. `recipes.json` inneholder startpunkter for arraysøk, låst arraysignal, truende arraysignal, VHS-sporing, svak menystøy og PC-overføring. Fire/sju-puls styres av motorens felles signalklokke.

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

Startskjermens `title_vhs` ligger betydelig lavere enn VHS-effekten i verdenen. Menyen starter én tekstur, lar den få et kort sporingspust omtrent hvert 19. sekund og fader den ut når spilleren forlater startskjermen. Teksturen følger SFX/menuvolum og bruker egen av/på-tilstand uavhengig av musikk. Uten eksplisitt lokal lydfilbank brukes et originalt syntetisk sus, svak nettbrum og langsom wow. Derfor fungerer menystøy også i standardspillet uten de lokale referanseopptakene.

`array_ominous` er et mørkere klangtrinn: langsommere opptak, et svakt lavt trykklag og smalere, ustabil radiotekstur. Det beholder fire/sju-portingen og prøver å skape uro gjennom klang fremfor bare høyere volum. Hendelsen velger hvilket trinn som passer; vanlige radiosignaler bruker ikke automatisk dette trinnet. Modem er fortsatt bare tilgjengelig i `pc_transfer`.


## Adaptiv musikk og lyddesign

Startmenyen får en svak VHS-tekstur etter første tast/klikk, sammen med den eksisterende tittellyden. Den stoppes når menyen forlates eller åpningens stillhet tar over. En eksplisitt lokal bank bruker `title_vhs`; uten banken lages sus, svakt brum og langsom båndustabilitet i lydgrafen. Arrayets prosedyriske anomali har et svakt ustabilt basslag mellom fire/sju-gruppene. `array_ominous` er en separat mørkere opptaksoppskrift for videre klangarbeid. Modemreservasjonen gjelder fortsatt.

Musikkens dirigent bygger på prinsippene i [Morbidiums musikkmotor ved 729219c](https://github.com/Tombonator3000/morbidium/blob/729219ceb3e77119179deadb9fbfcd25bd8b2a4a/src/06_musikk.js): felles lydklokke, rask opptrapping og langsommere nedtrapping. SIGNAL bruker to overlappende mediestrømmer med 2 til 4 sekunders overgang. Rolig utforskning, uro, action og etterspill kan styres gjennom `RequestMood`; faktisk synlig nær fare kobles inn fra spillet i både Story og Freeform. Skjult hendelsesplan eller usett fiende gir ikke automatisk farevarsel. Viktige lytteøyeblikk og radio/film beholder musikkens hold-funksjon. Etter action holdes tilstanden i seks sekunder og etterspillet i åtte; vanlige områdeskifter har to sekunders stabilisering.

De 17 eksisterende musikksporene har ikke kontrollerte tempo, toneart eller separate instrumentlag. Dagens overganger er derfor frie crossfades. `SetTrackTiming(track,bpm,beatsPerBar,phraseBars)` aktiverer slag-/taktplanlegging for et kontrollert spor; det gjør ikke ukjente filer automatisk musikalsk kompatible. Morbidiums harmoniske broer og instrumentlag er ikke portert som nye musikalske assets. Slike overganger trenger komponerte eller klippede fraser eller kompatible separate stemmer.

Workshop-kjøringen skriver også `adaptive_music.wav` (45 sekunder fra ro via uro/action til etterspill og stillhet), `title_synthetic.wav` og `array_synthetic.wav`. Dette er faktiske lydgrafopptak til lytting, ikke dokumentasjon på menneskelig godkjent klang eller en installert spillutgave.


Første kuratering holder roligere `solitude`/`cylTwo` adskilt fra actionens `phantom`; områdets eksisterende drone-/urospor brukes fortsatt til spenning. Dette er et startvalg for lytting. Komponistens [In Search of Solitude](https://www.scottbuckley.com.au/library/in-search-of-solitude/) beskrives som langsomt og mystisk; [The Old Ones](https://www.scottbuckley.com.au/library/the-old-ones/) og [Decoherence](https://www.scottbuckley.com.au/library/decoherence/) er ambient uro. [Phantom from Space](https://remote.incompetech.com/music/royalty-free/index.html?Search=Search&isrc=USUAN1500038) har perkusjon og oppgitt tempo 109 BPM. Publisert tempo alene brukes ikke som bevis på hvor første taktslag ligger i vår importerte fil. Ingen nye spor er lastet ned eller lisensierte stemmer lagt i Git.


## Bevarte NASA- og Nebula-kilder

Forberedelsen tar også med de fem NASA-kandidatene fra `~/scratch-bm/Signal47-array-sounds-20261028/wav` og Nebula-opptaket fra den lokale Ultimate SFX-pakken når de finnes. `--observatory-root` og `--nebula-file` velger andre plasseringer. Manglende valgfrie standardkilder varsles og hoppes over; en eksplisitt ugyldig sti avvises.

NASA-banknøklene er `nasa_cassini_jupiter`, `nasa_saturn_eerie`, `nasa_saturn_bizarre`, `nasa_saturn_lightning` og `nasa_earth_chorus`. Kildefiler og opphav/kreditering fra kandidatbankens SOURCES.md bevares i metadata. Disse er radiomålinger/plasmabølger gjort hørbare; presetene `nasa_saturn_scan` og `nasa_jupiter_scan` spiller hvert sitt utsnitt med hastighet 1 uten løkke. Nebula er `space_nebula`, en kjøpt designet atmosfære fra Ultimate SFX, og har ikke NASA-proveniens. Den får oppskriften `nebula_ambience`.

Med alle lokale kilder tilgjengelige blir banken 12 kilder og 9 oppskrifter. Originalene røres ikke, og de lokale klippene følger ikke offentlig Git. Den eksisterende spillbanken har fortsatt `obsPulsarPulse`, `obsArraySlew`, `obsArrayDrive`, `obsBroadband` og `obsChartPrinter` i S47Observing. Disse fem er egne observatorieopptak; de fem NASA-kandidatene var ikke automatisk importert som erstatninger for dem. Endringen gjør kildene tilgjengelige for mikseren, uten å bytte installerte spillhendelser.
