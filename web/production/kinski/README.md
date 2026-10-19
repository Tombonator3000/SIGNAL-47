# Kinski synthetic parody: offline opplesning

`corpus.json` er AST-uttrekket fra spillets dokumentbeskrivelser, statiske papirtekster og Panels.talk-linjer. Dokumenttekst deles ved tom linje, samtaler ved hver `lines[].text`, og whitespace normaliseres. Papirlesing fjerner bare linjeledende pencil-markør og rene separatorlinjer. Tall som -39 LY og kanontekst bevares. Titler, valg, status, svarmeldinger og radio er utelatt.

Omfanget er 328 unike lydtekster: 52 beskrivelsesavsnitt, 84 papiravsnitt og 192 samtalelinjer, totalt 4798 ord. De første 244 tekstene og ID-ene ble beholdt da 84 papirklipp ble lagt til. MP3-filene utgjør 11 534 432 byte og 1413,29 sekunder (23 minutter og 33 sekunder).

Fotoenes klokker gjør beskrivelsene til frame01–04 utilgjengelige for full opplesning. Bortføringsfotoenes teller gjør det samme for away*. Papirtekstene s03log og printout har dynamiske klokker, mens telex bygges fra løpende meldinger. Disse tre papirtekstene leses ikke. Et helt forespurt dokument eller samtaleblokk må ha alle sine avsnitt/linjer i katalogen. Ingen tekst hoppes stille over.

`generation.json` oppdateres atomisk per fil og bevarer forespørsel, profil-/referansehash og lydmålinger. Generering kjører sekvensielt på den eksisterende lokale VoiceStudio-profilen. WAV-cache holdes utenfor repoet i brukerens lokale cachemappe og gjør jobben gjenopptakbar.

En første take av signaturavsnittet `T.` var taus og ble avvist. Bare synteseforespørselen bruker derfor uttalen `Tee.` med egen uttalehash i cachen; katalogens tekst, ID og kildeordlyd er fortsatt `T.`. Den første historikkreddede lydfilen ble generert på nytt med samme tekst og seed for å få en eksplisitt `dropped_chunks=0`-kvittering.

`provenance.json` beskriver stemmekilden, programvaren og lisensene fra den faktisk nedlastede modellen. VoiceStudio er AGPL-3.0-only. OmniVoice-koden er Apache-2.0, mens det nedlastede modellkortet oppgir CC-BY-NC for modellvektene. Kildelyd, modellvekter og referanseopptak følger ikke med runtime. Runtime består av MP3 og catalog.json i `src/assets/audio/kinski`.

Kjør `node tools/kinski/extract.mjs`, deretter `python3 tools/kinski/generate.py`. `--limit N` behandler høyst N nye klipp. Lyd med samme tekst, stemmereferansehash og innstillinger gjenbrukes. `python3 tools/kinski/verify.py --asr 6` kontrollerer hele katalogen og en liten spredt ASR-prøve. Verifisering skal kjøres etter generering, slik at ASR ikke konkurrerer med TTS om GPU-en.

Sluttkontrollen i `verification.json` er PASS for format, dekning, hash, varighet og lydenergi på alle 328 filer. Alle genereringer oppga `dropped_chunks=0`; ingen MP3 mangler eller er foreldreløse. Alle varigheter er positive, ingen dekodede samples klipper, og høyeste peak er −2,48 dBFS.

ASR-prøven omfatter seks spredte klipp. Tre har identiske ordtoken. To har avvik i tall-/klokkeslettskriving eller bindestrek. Den korte survey-linjen med A/B/C-labels (`c21188610bc7ce84`) har seks avvik i elleve token og bør lyttes gjennom. Rapporten bevarer både forventet og automatisk gjenkjent tekst. Automatisk transkripsjon bekrefter ikke hørbar ordnøyaktighet. Klangslektskap, innlevelse og hørbar kvalitet er UNVERIFIED fordi det ikke er gjennomført menneskelig lytting.
