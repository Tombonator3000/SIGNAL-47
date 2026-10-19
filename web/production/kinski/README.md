# Kinski synthetic parody: offline opplesning

`corpus.json` er et AST-uttrekk fra spillets engelske tekst. Omfanget er 1555 unike tekster og 18 791 ord. De 337 tidligere telefon-, dokument- og samtaleklippenes tekster, ID-er og MP3-filer bevares. Ingen nye dialogreplikker er skrevet. Kartet har 22 nye beskrivelsesfraser som formulerer de eksisterende kartlabelene og synlige markeringene som lesbar tekst.

Beskrivelsene omfatter undersøk-tekster, toast og notater, dokumentbeskrivelser og papirtekst, panelenes forklaringer og spørsmål, bevisbordets innhold og hint, notatbokens oppgaver og notater, Kit- og People-opplysninger, bildetekster med sted og tid, kartbeskrivelse og kapitteloppsummeringer. 1293 katalogtekster har en beskrivelseskilde; kategoriene overlapper med den opprinnelige leveransen. Knappelabels, utviklermeny, rulletekst og radioens egne programmoduler er utelatt. Radio- og CB-avspilling er uendret; synlige sitater i kapittel- og hendelsesbeskrivelser kan likevel leses manuelt.

Hele normaliserte tekster foretrekkes. Dokumenter og vanlige beskrivelser deles ved tom linje. Bare papirlesing fjerner linjeledende blyantmarkør og rene separatorlinjer. Endelige betingede varianter tas med fra kilden. Dynamiske templates gir ordrette statiske fragmenter. Resolveren krever en komplett kjede og avviser hele blokken hvis et ord mangler.

35 korte tallord dekker faktiske runtime-klokker, tellere og målinger. 27 identifikatoropptak dekker A til Z og `frame` for strengt gjenkjente spill-ID-er. Tallordene endrer ikke verdiene som står i spillet. Initial stor bokstav i bildetekster kan bruke samme opptak som liten bokstav. Ingen vilkårlig tekst syntetiseres i nettleseren, og ingen ord hoppes stille over.

`generation.json` oppdateres atomisk per fil og bevarer forespørsel, profil- og referansehash, respons og lydmålinger. Generering kjører sekvensielt mot eksisterende lokal VoiceStudio. WAV-cache ligger utenfor repoet og gjør arbeidet gjenopptakbart. `--limit N` behandler høyst N nye klipp. Uttaleoverstyringer som `T.` til `Tee.` og `S` til `Ess.` er dokumentert i forespørselen og har egen cacheidentitet; katalogens tekst og kildeordlyd beholdes.

Kjør `node tools/kinski/extract.mjs`, deretter `python3 tools/kinski/generate.py --limit 100`. `python3 tools/kinski/verify.py` kontrollerer format, dekning, hash, varighet og lydenergi. ASR skal først kjøres etter all TTS, slik at jobbene ikke konkurrerer om GPU-en. Den historiske ASR-prøven bevares ved utvidet sluttkontroll med `--keep-asr`; `--asr-new 3 --asr-text zero --asr-text twenty --asr-text six` velger tre nye langtekster og tre tallord.

`provenance.json` beskriver stemmen, programvaren og de faktisk nedlastede modellvilkårene. VoiceStudio er AGPL-3.0-only. OmniVoice-koden er Apache-2.0, mens det nedlastede modellkortet oppgir CC-BY-NC for vektene. Kildelyd og modellvekter følger ikke spillet. Runtime består av MP3 og `catalog.json`; lokal API, profil og private filstier brukes bare under produksjon.

`telefonmanus.md` viser de 24 eksisterende telefonreplikkene etter scene og aktør. Spillerasiden i kapittel 2 er markert stille.

Sluttkontrollen er PASS for alle 1555 MP3-filer: 45 411 332 byte og 5541,30 sekunder (1 time, 32 minutter og 21 sekunder). Alle oppga `dropped_chunks=0`; ingen filer mangler eller er foreldreløse, og ingen dekodede samples klipper. Alle 337 tidligere MP3-filer og tekster er faktisk sammenlignet med Gitbase `95719a4` og er uendret. Filvise SHA-256-bevis ligger i `descriptions-qa/asset-audit.json`.

Åtte tidligere ASR-prøver er beholdt, og seks nye omfatter tre lange tekster og tallordene zero, twenty og six. De lange tekstene har avvik rundt ID-er, homofoner, tall og WASD. Twenty skrives 20. Six ble alene gjenkjent som Fix; i en tapsfri sammensetting av de ferdige bankfilene gjenkjennes «Zero, twenty, six!». Det korte S-et ble ikke skrevet av ASR. Disse ASR-prøvenes lydfiler er uendret. Feltportens beskrivelse og nulltestpanelets papirtekst ble deretter lagt til etter to konkrete hull i nettlesertesten. Begge er inkludert i den fullstendige format- og PCM-kontrollen uten ny ASR.

Fem tause takes ble avvist og erstattet: el-fragmentet, five og bokstavene A/H/X. De endelige opptakene har taleenergi og egne request-/cacheidentiteter. Automatisk ASR er en ufullkommen ordkontroll, ingen lytting eller test av stemmelikhet. Klang, sinne og hørbar kvalitet er UNVERIFIED fordi menneskelig lytting ikke er gjennomført.
