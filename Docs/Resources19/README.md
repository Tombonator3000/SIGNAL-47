# Resources19 — gratis lydressurser og skala-/antennepass

13. september 2026. Brukeren ba om gratis ressurser, mer lyd/musikk, et mer detaljert radioteleskop og en telefon i korrekt størrelse. Dette er et avgrenset pass i det eksisterende Unity-spillet. Det endrer ingen historie, frekvenser eller lagringsformat.

## Ressurser som er klare til vurdering

Åpne [lydprøven](lydprove.html) lokalt i en nettleser. Den har 15 individuelle avspillere uten automatisk start. Filene følger repoet og trenger ikke innlogging. [Manifestet](audio-manifest.json) har kilde, lisens, varighet, format, bearbeiding og SHA-256 for hver fil. [Krediteringen](CREDITS.md) skal følge filene. Tiltenkt bruk nedenfor bygger på kildebeskrivelsene, ikke en gjennomført subjektiv lyttetest.

| Ressurs | Lisens og utvalg | Foreslått bruk / beslutning |
| --- | --- | --- |
| [Echoes of the Past — isaiah658](https://opengameart.org/content/echoes-of-the-past) | CC0; ett 48-sekunders spor | Undersøkelse av arkivmateriale; loopkandidat |
| [The World Fell Silent — Tsorthan Grove](https://opengameart.org/content/the-world-fell-silent) | CC0; Outpost loop og Dirty Rain loop | Avsides stasjon og korte overganger med uro |
| [12 Ambient Machine Sounds](https://opengameart.org/content/12-ambient-machine-sounds) | CC BY 3.0; alle tolv opptak | Vifte, kompressor, elektrisk støy og motorer; velg passende opptak og lag løkker etter lytting |
| [Kenney RPG Audio](https://kenney.nl/assets/rpg-audio) | CC0; 50 filer på kildesiden | Reserve for fottrinn og håndtering av bøker/metall. Ikke lastet ned i dette passet |
| [Relé fra en flipperspillmaskin fra 1970-tallet — bassmosphere](https://freesound.org/people/bassmosphere/sounds/384698/) | CC0; 0,189 sekunder | Relevant alternativ for mottakerens brytere. Originalnedlasting krever Freesound-innlogging; ikke hentet |
| [Sonniss GameAudioGDC](https://sonniss.com/gameaudiogdc/) | Egen gratis kommersiell lisens | Reservebibliotek. [V2-vilkårene](https://sonniss.com/gdc-bundle-license/) tillater bruk i spillet, men ikke offentlig distribusjon av lydfilene som assetpakke. Ingen Sonniss-filer lastet ned eller lagt i det offentlige repoet |
| [NASA 70 meter dish](https://science.nasa.gov/3d-resources/70-meter-dish/) | NASA/Ames; se [kreditering og vilkår](CREDITS.md) | GLB på 2,22 MB lastet ned lokalt. 14 852 trekanter, én mesh og to bilder ifølge GLB-strukturen. Kandidat/referanse; ikke integrert |

NASA-modellen er en stor DSN-antenne. [NRAOs VLA](https://www.vla.nrao.edu/) har 25-meters antenner, mens SAROs eksisterende skål er 8,4 meter før instansskalering. Disse er ikke samme konstruksjon. Vi beholder SAROs spillbare avstander og originale modell, og legger til egen detaljgeometri. En gratis lavpoly Cisco-telefon funnet hos Poly Pizza ble avvist fordi typen ikke passer 1986; en ny tilfeldig telefonmodell ville heller ikke løst scenens skala automatisk.

De fem opprinnelige nedlastingene ligger lokalt under `Artifacts/Resources19/Incoming/`. [Nedlastingsregisteret](downloads.json) beholder eksakte URL-er og hash. Gjenskap med `python3 Automation/fetch-resources19.py`, deretter `python3 Automation/prepare-resources19-audio.py`. Musikk og SFX er fullengde Vorbis q5 med −3 dB nivåmargin. Alle 15 er dekodet og målt; høyeste dekodede sampletopp er −2,08 dBFS. Dette beviser format og nivåmargin, ikke fravær av opprinnelig forvrengning, sømløse løkker eller en god subjektiv miks. Den aktive Unity-miksen og tidligere Scott Buckley-kreditering er bevart.

## Modellendringen

**Telefon:** Unity målte huset til 0,680 m bredde før endringen. Uniform skala 0,30 gir huset 0,204 m bredde, omtrent 0,227 m over røret og 0,144 m total høyde. Ledningen gir større total bredde. Dette er en realistisk bordskala for den eksisterende stiliserte telefonen, ikke en måleriktig replika av en bestemt Western Electric-/Cortelco-modell. Original Blender/FBX er beholdt. Treffboksen følger den nye størrelsen; rørløftet beholdes i verdensmeter, med en synlig ledningsforbindelse under løftet. Kaffekoppen ved siden av var også for stor og er skalert til 0,35, inkludert skår og interaksjonsvolum.

**Antenner:** to nye originale Blender/FBX-modeller med panelinndeling, kantprofil, 16 bakre ribber med triangulering, bakring, stige, motorhus, ventiler og lagerdetaljer. 10 828 nye trekanter og seks renderere per antenne; 119 108 trekanter for elleve instanser. Skåldetaljene følger eksisterende elevasjons- og azimutpivoter. Den opprinnelige skålen, materen og støttene er beholdt. Eksisterende bærebjelke er senket 60 cm i lokal plassering etter at spillbildet viste at den skar gjennom skålen. Lagerkappene ble også senket, og stigen ble lagt på baksiden etter kontroll i begge antennevinkler. En geometrikontroll fant at den gamle bærebjelken fortsatt trengte 7,8 cm gjennom skålen i sluttstillingen etter første senking; endelig plassering kontrolleres mot parabelen i elleve posisjoner per antenne. Ingen nye kollisjonskomponenter; 154 totalt og to opprinnelige lightmaps er beholdt. Telefonens treffboks er tilsiktet endret.

Kilde: `Unity/Blender/Source/resources19_dish.py`, redigerbar `Unity/Blender/SIGNAL47_resources19_dish.blend`, importer i `Unity/Assets/Signal47/Art/Resources19/`. `Resources19.Apply` kjøres også ved senere scenebygging. Bygg den bevarte hovedscenen med `bash Automation/build-resources19-linux.sh`.

## Verifikasjon og videre arbeid

[Bevis og identitet](Evidence/verification.json) knytter sluttresultatet til faktisk bygg og tester. Originale runtime-bilder er tatt i Unity-spilleren fra faste inspeksjonskameraer; de er ikke konseptbilder eller native tastatur-/museprøver. Første kjøring besto 37 API-kontroller, men ble likevel korrigert etter visuelle funn i skål og bordskala. Den kjøringen er ikke sluttbeviset.

Ny lyd må fortsatt lyttes til, få valgte inn-/utpunkter og mikses mot dialog og signaler. Bruk romlyd nær maskinene og spar musikk til meningsfulle overganger; ikke fyll hele den planlagte 5–6-timersopplevelsen med kontinuerlig musikk. Native brukerreise, subjektiv lyd, nye ytelsesmålinger og andre oppløsninger er fortsatt UNVERIFIED. Visual10 forblir standardstarter. Dokumentlesbarhet og kobling til P04 er neste tidligere avtalte innholdssteg.

**Sluttresultat:** 42 spillkontroller og 50 lagringskontroller består gjennom samme utpakkede starter. 173 pakkefiler og begge originale foto er kontrollert. Lokal kandidat: `Artifacts/Releases/Resources19-bea744ade4d9/Start-SIGNAL47.sh`. Seks inspiserte [spillbilder](Evidence/Runtime/) følger rapporten. Ingen ny lyd er aktivert i spillmiksen.
