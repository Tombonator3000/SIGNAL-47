# Stemmeensemble og Serling-forteller

Dette er ferdiggenererte engelske spillreplikker og beskrivelser fra seks referansebaserte syntetiske parodistemmer. Spilleren trenger ingen lokal modell eller VoiceStudio. Originalopptak, referanse-WAV, modellvekter og privat lydcache følger ikke spillet.

## Omfang

Ensemblebanken har 100 klipp: 18 for bensinstasjonsbetjenten med Harry Dean Stanton, 49 for Nora med Lauren Bacall, 19 for visesheriffen med Robert Stack, 13 for suvenirmannen med David Lynch og én avgrenset båndfrase med Vincent Price. Price-klippet gjenbrukes på begge eksisterende avspillingssteder. Noras bank omfatter direkte samtaler, telefon, CB og de allerede skrevne korte svarene. Spillerens replikker og stille handlinger inngår ikke.

Serlingbanken har 1402 klipp med eksisterende beskrivelser, avsnitt og statiske fragmenter, komplette tall- og ID-banker og alle feltene i de fem mellomkapitlene. Alle betingede sammendrag er med. Klokke, tall og identifikatorer settes sammen av den samme komplette resolveren som Kinski-bankens beskrivelser. Ukjente ord gir utilgjengelig opplesning; teksten hoppes aldri stille over.

I Settings er **Character voices** og **Rod Serling narrator** egne lagrede valg, begge På som standard. Karakterene snakker når samtalen åpnes og ved nye svar; **Replay voice** og **Stop voice** styrer gjentakelse og avbrudd. Hold Zoom på et objekt for opplesning etter et stabilt nærblikk, eller bruk **Read description**. V gjentar beskrivelsen på PC. Serling leser alle fem mellomkapittelfelt automatisk, og kortet venter til talen er ferdig. Kinski er fortsatt et separat valg for papirer og eldre telefonroller, og kan lese beskrivelser når Serling er Av. Art Bell-valget gjelder fortsatt både radioen og Earl i Caliche.

Gamle Kinski-, Art Bell- og radiocast-filer og profiler bevares. Nye banker har egne kataloger. Tekstnormalisering er sammenfolding av mellomrom og trimming. Serling-ID er de første 16 heksadesimale tegnene i SHA-256 av teksten. Ensemble-ID inkluderer rollen, et NUL-tegn og teksten før samme hashing. Katalogene inneholder bare manus, ID, filnavn og varighet, samt rolle i ensemblebanken.

## Kilder og forbehold

`profiles.json` er den eneste offentlige filen med ett kort ordrett kildeutdrag per skuespiller, høyst 25 ord. Der står også kilde-URL, originalfilens hash, valgt intervall, den faktiske profil-ID-en og referansehash. Korte utdrag er valgt inne i aktørens svar eller solorapport. Sammenheng og publikasjonsopplysninger underbygger hvem som taler. Bacalls kilde har også et transkript med talermerking. Stanton og Stack er vurdert med spørsmål og svar i opptaket; vi har ikke et uavhengig talermerket transkript for dem.

Offentlig tilgang er ikke en dokumentert tillatelse til stemmekloning eller distribusjon. Ingen lisens, samtykke eller anbefaling fra personene eller rettighetshaverne hevdes. Arkivopplasterens merking av Serling-opptaket som offentlig eiendom er ikke selvstendig verifisert. Originalmaterialet ligger utenfor Git og runtime.

VoiceStudio 0.5.6 og de allerede installerte OmniVoice- og Whisper Small-modellene er gjenbrukt. VoiceStudio er AGPL-3.0-only. OmniVoice-koden er Apache-2.0, mens modellkortet for de festede vektene oppgir CC-BY-NC. Se `provenance.json` og prosjektets tredjepartsnotiser før videre distribusjon.

## Produksjon og kontroll

Uttrekket er AST-basert og kjører ikke spillkoden. `extract.mjs` skriver ensemblekorpuset. `extract-serling.mjs` skriver et separat Serling-korpus og endrer ikke Kinski-korpuset. Alle variantene beholdes. `generate.py` bruker faktiske clone-profiler med festede referansehasher, én GPU-forespørsel om gangen og en lokal gjenopptakbar cache utenfor repoet.

Grunninnstillingene er engelsk, OmniVoice, 32 steg, seed 42 og naturlig hastighet 1. Uttalevarianter for bokstavnavn, initialer og forkortelser bevarer katalogens originale tekst og ID. Målrettede omtak for ordsvikt logges separat med innstillinger, gammel lydhash og begrunnelse. Kapittelkortenes store bokstaver og separatorer gjengis som vanlig tale med pauser. Korte ID-er uttales med bokstavnavn og positive sifre; frittstående negative tall får eksplisitt «minus». Én kort Stanton-frase er fortsatt tvetydig i automatisk ordkontroll etter begrensede forsøk og kontroller uten fasitprompt. Dette er ført som uavklart ASR-/uttaleusikkerhet og menneskelig ordkontroll UNVERIFIED, ikke som perfekt hørt tale.

WAV blir MP3 med mono, 24 kHz og 64 kbit/s. Ett konstant gain per klipp søker aktiv RMS omkring -20 dBFS og begrenser dekodet topp til høyst -2 dBFS. Ingen tidsstrekking eller stemmehøydejustering brukes. Alle filer kontrolleres med ffprobe, dekodet varighet, aktiv lyd, topper, klipping, filhash og null tapte genereringsbiter.

`asr.json` har ordkontroll av alle 100 ensembleklipp. `serling/asr.json` har det faktiske stratifiserte utvalget, alle mellomkapittelfelt og sammenføyninger fra runtime med tall, klokker og ID-er. Strenge ordavvik teller også tallskriving, forkortelser og egennavn; vurderte forskjeller oppgis eksplisitt. Automatisk transkripsjon er ikke menneskelig lytting. Klang, likhet, bakgrunnsmusikk og innlevelse er **UNVERIFIED**.

Serling har 137 kontrollerte filer og 17 sammenføyninger: det gjeldende stratifiserte utvalget på 105, alle 18 town-varianter og bevart kontrollhistorikk med fortsatt aktuelle lydhasher. Av filene har 44 strengt ordsamsvar og 93 vurderte forskjeller. Tjueto klipp og ni sammenføyninger har fortsatt konkrete ASR-/uttaleusikkerheter. Dette gjelder blant annet bokstavene i E09A/P13, enkelte E-prefikser, klokkeslettet 02:17:00, siste siffer i sammensatte 48/47 og noen navn eller tekniske ord. Disse står med egne ID-er, faktiske transkripsjoner og begrunnelser i `serling/asr.json` og `serling/word-review-notes.json`. De er ikke merket som ordrett hørt tale. De kontrollerte negative fortegnene er representert. Den tidligere nevnte Stanton-frasen har også en uttrykkelig uavklart ordstatus.

Åtte konkrete tekstklipp ble forbedret med ett avgrenset uttaleomtak: tekniske overskrifter, negative-range-setninger, dag 19 i en dato og «Chapter one». Et eget ninety-omtak beholdes. Ett forsøk med innledende bærersetninger for korte bokstaver og sifre ble forkastet fordi sammenføyningskontrollen ga nye ekstraord. Ingen endelige klipp bruker bærerord eller klipping fra denne metoden; original, kort fonetisk bank beholdes. `serling/carrier-word-controls.json` og `corrections.json` bevarer forsøket og det faktiske tilbakevalget. Fil-/PCM-resultat **PASS** betyr teknisk kontroll med gjennomgåtte ordresultater, og er ikke en påstand om perfekt uttale eller likhet.

En faktisk nettleserprøve avdekket 13 manglende town-undersøkelser. Caliche registrerer dem i det fjerde argumentet til `add`, som den første ekstraktoren ikke fulgte. Det målrettede argumentsinket tar nå med alle 18 faktiske varianter, også andre tilstander for patruljebil, studio, fjernsyn og personer. Alle 18 har ASR. To nye skilt fikk ett avgrenset omtak for «County sheriff» og «Closed»; disse ordene gjenkjennes nå. Navnet Caliche på det siste skiltet er fortsatt usikkert i ASR. De tidligere 1 484 valgte MP3-filene er byteidentiske. `serling/town-addendum.json` inneholder før-hasher, de 18 nye ID-ene og CPU-bevis for resolverdekningen.

Endelige tall og resultat står i `verification.json`, per bank i `bank-verification.json` og `serling/bank-verification.json`. `preservation.json` har hashbevis for de gamle filene, 12 gamle profiler, seks ulike nye referanser og fravær av byteidentiske Serling/Kinski-opptak. `qa/tts-ready.json` skrives først etter at alt GPU-arbeid og hele lydkontrollen er avsluttet.

<!-- production-counts:start -->

## Faktiske produksjonstall

Teknisk kontrollstatus: PASS. Ordstatus: CHECKED_WITH_UNRESOLVED_ASR_AMBIGUITY. Tallene under gjelder filene i arbeidskopien.

| Bank | Ferdige klipp | Byte | Varighet i sekunder |
| --- | ---: | ---: | ---: |
| ensemble | 100 / 100 | 3224048 | 394.29 |
| serling | 1402 / 1402 | 56267960 | 6913.71 |

<!-- production-counts:end -->
