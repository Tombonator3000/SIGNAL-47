# Kinski synthetic parody: offline opplesning

`corpus.json` er AST-uttrekket fra spillets dokumentbeskrivelser, statiske papirtekster, Panels.talk-linjer og telefonmanuset `src/core/phoneScripts.ts`. Dokumenttekst deles ved tom linje, samtaler ved hver `lines[].text`, og whitespace normaliseres. Telefon bruker bare ordrette taletekster, uten talernavn, sitattegn eller spilleraside. Papirlesing fjerner bare linjeledende pencil-markør og rene separatorlinjer. Tall som -39 LY og kanontekst bevares. Titler, valg, status, svarmeldinger og radio er utelatt.

Omfanget er 337 unike lydtekster: 52 beskrivelsesavsnitt, 84 papiravsnitt, 192 samtalelinjer og ni nye telefonreplikker, totalt 4924 ord. Telefonbanken omfatter 24 tekster: ni nye fra Ward og Nora i kapittel 2 og 3, seks eksisterende fra feltlinjen i kapittel 4 og ni fra Ward-samtalen i kapittel 5. De første 244 tekstene og ID-ene ble beholdt da 84 papirklipp ble lagt til. Alle disse 328 MP3-filene er uendret etter telefonutvidelsen.

MP3-filene utgjør 11 805 932 byte og 1446,43 sekunder (24 minutter og 6 sekunder). De ni nye telefonreplikkene utgjør 271 500 byte og 33,14 sekunder.

`telefonmanus.md` viser alle 24 talereplikkene gruppert etter scene og aktør, med den ene stille spillerasiden i kapittel 2 uttrykkelig markert. Manusets replikker har identisk ordlyd med telefonbanken og korpuset.

Fotoenes klokker gjør beskrivelsene til frame01–04 utilgjengelige for full opplesning. Bortføringsfotoenes teller gjør det samme for away*. Papirtekstene s03log og printout har dynamiske klokker, mens telex bygges fra løpende meldinger. Disse tre papirtekstene leses ikke. Et helt forespurt dokument eller samtaleblokk må ha alle sine avsnitt/linjer i katalogen. Ingen tekst hoppes stille over.

`generation.json` oppdateres atomisk per fil og bevarer forespørsel, profil-/referansehash og lydmålinger. Generering kjører sekvensielt på den eksisterende lokale VoiceStudio-profilen. WAV-cache holdes utenfor repoet i brukerens lokale cachemappe og gjør jobben gjenopptakbar.

En første take av signaturavsnittet `T.` var taus og ble avvist. Bare synteseforespørselen bruker derfor uttalen `Tee.` med egen uttalehash i cachen; katalogens tekst, ID og kildeordlyd er fortsatt `T.`. Den første historikkreddede lydfilen ble generert på nytt med samme tekst og seed for å få en eksplisitt `dropped_chunks=0`-kvittering.

`provenance.json` beskriver stemmekilden, programvaren og lisensene fra den faktisk nedlastede modellen. VoiceStudio er AGPL-3.0-only. OmniVoice-koden er Apache-2.0, mens det nedlastede modellkortet oppgir CC-BY-NC for modellvektene. Kildelyd, modellvekter og referanseopptak følger ikke med runtime. Runtime består av MP3 og catalog.json i `src/assets/audio/kinski`.

Kjør `node tools/kinski/extract.mjs`, deretter `python3 tools/kinski/generate.py`. `--limit N` behandler høyst N nye klipp. Lyd med samme tekst, stemmereferansehash og innstillinger gjenbrukes. `python3 tools/kinski/verify.py --asr 6` kontrollerer hele katalogen og en liten spredt ASR-prøve. Verifisering skal kjøres etter generering, slik at ASR ikke konkurrerer med TTS om GPU-en.

Sluttkontrollen i `verification.json` er PASS for format, dekning, hash, varighet og lydenergi på alle 337 filer. Alle genereringer oppga `dropped_chunks=0`; ingen MP3 mangler eller er foreldreløse. Alle varigheter er positive, ingen dekodede samples klipper, og høyeste peak er −2,48 dBFS.

ASR-prøven bevarer seks spredte klipp fra første leveranse og legger til to nye telefonklipp. Fire har identiske ordtoken. To har avvik i tall-/klokkeslettskriving eller bindestrek. Den korte survey-linjen med A/B/C-labels (`c21188610bc7ce84`) har seks avvik i elleve token og bør lyttes gjennom. Den siste Nora-felttelefonlinjen (`c88b297e80f976bd`) har ett bøyningsavvik, «developed» gjenkjent som «develop», og bør også lyttes gjennom. Rapporten bevarer både forventet og automatisk gjenkjent tekst. Automatisk transkripsjon bekrefter ikke hørbar ordnøyaktighet. Klangslektskap, innlevelse og hørbar kvalitet er UNVERIFIED fordi det ikke er gjennomført menneskelig lytting.
