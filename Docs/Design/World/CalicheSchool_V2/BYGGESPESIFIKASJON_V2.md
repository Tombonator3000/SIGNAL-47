# Caliche Public School V2 – undersøkelse, sniking, jakt og flukt

Skolekonsept og revidert plansje er laget med innebygd ImageGen. Dette er et nytt konseptforslag til Unreal, ikke en allerede bygget eller testet level.

## Bruk bildene
school_concept.png: fasade, materialer og stemning for skolen.
school_build_sheet_v2.png: visuell oversikt over etasjer og byggemoduler.

Plansjen er AI-generert og skjematisk. Tabellen og forbindelsene nedenfor er styrende ved avvik. Plansjens snitt viser et tverrsnitt gjennom bredden, selv om overskriften sier nord–sør. Dens utvendige kjellertrapp er ikke riktig orientert; bruk plasseringen nedenfor. Juster fasadevinduene etter rommene, særlig toalettene, og behold skolebildets stil.

## V2 gjelder foran tidligere plansje og beskrivelse
Tom ba om flere forbindelsesdører og en fast regel for mulig jakt, skjul og flukt. Hovedvolum, romfordeling oppe, etasjehøyder og skolens utseende beholdes. Nordlige kjellerrom utvides sørover slik at de får egne korridordører før trappesjakten; skilleveggen i kjellervingene flyttes fra Y=1800 til Y=1600. Bruk koordinatene i dette dokumentet ved avvik i plansjen. V1 er bevart som historikk. Plansjen er ikke tegnet i konsekvent målestokk; bruk de oppgitte centimeterkoordinatene for blokkering. Den nederste snittillustrasjonen er fortsatt skjematisk, ikke et presist nord–sør-snitt gjennom delkjelleren.

### Nye direkte forbindelsesdører
Alle koordinater er dørsentre i centimeter og følger de etablerte aksene. Frie åpninger 110x220 cm; sentrer på oppgitt punkt og trekk fra veggen. Dører skal kunne åpnes fra begge sider, med kollisjon og fri plass utenfor svingbuen.

| ID | Forbindelse | Dørsenter X,Y | Etasje Z |
|---|---|---|---:|
| D1 | Kontor ↔ lærerrom | 300,400 | 0 |
| D2 | Lærerrom ↔ klasserom 1 | 800,800 | 0 |
| D3 | Klasserom 1 ↔ klasserom 3 | 300,1600 | 0 |
| D4 | Klasserom 2 ↔ klasserom 4 | 2400,1600 | 0 |
| D5 | Lager ↔ fyrrom | 300,1600 | -340 |
| D6 | Verksted ↔ arkiv | 2400,1600 | -340 |

Behold alle separate korridordører. Klasserom 3 og 4 må ha sine korridordører ved Y=1700 før trappesonen Y=1800. Ikke bygg en åpning gjennom selve trappen. Toalettene får ikke gjennomgang til undervisningsrom.

### Utganger
E1: hovedinngangen, (1400,0,0), til sørplassen.
E2: klasserom 3 gjennom vestveggen ved (0,2100,0), med trapp/repos til terreng.
E3: klasserom 4 gjennom østveggen ved (2800,2100,0), med trapp/repos til terreng.
E4: arkiv gjennom østveggen ved (2800,2100,-340), til separat utvendig kjellertrapp S2 langs østfasaden. E3 og E4 ligger i ulike høyder; gi dem fysisk adskilte repos og gangareal. Terrengtrappen for E3 skal ikke blokkere kjellertrappens øvre repos eller ligge i dens frihøyde. Om nødvendig før E3s repos sørover langs fasaden og S2 nordover.
S1: eksisterende interne U-trapp mellom hovedkorridor og kjellerkorridor. S2: uavhengig kjellertrapp til uteområdet. Begge etasjene skal være tilgjengelige via S1; S2 skal også fungere fra uteområdet.

### Rundløyper og spillbar situasjon
- Vest oppe: korridor → klasserom 1 → D3 → klasserom 3 → korridor. Lengre variant via kontor → D1 → lærerrom → D2 → klasserom 1.
- Øst oppe: korridor → klasserom 2 → D4 → klasserom 4 → korridor.
- Vest nede: kjellerkorridor → lager → D5 → fyrrom → kjellerkorridor.
- Øst nede: kjellerkorridor → verksted → D6 → arkiv → kjellerkorridor.
- Utvendig omvei: E2 → vestgård → sørplass eller bakgård → østgård → E3/E4. Ingen dekor eller gjerde uten åpning skal bryte den planlagte omveien.

Eksempel, ikke et nytt fast historieoppdrag: spilleren undersøker et spor i klasserom eller arkiv. En trussel sperrer hovedruten. Spilleren kan krysse til naborom, bryte sikt bak reoler og velge ny korridordør eller sideutgang. Sporet krever ikke et bestemt jaktutfall; sikret undersøkelsesfremdrift bør bevares ved lokal retry.

### Sikt, skjul og møte med trussel
Lag forskjøvede, ugjennomsiktige bokreoler i klasserommene og arkivet, kles-/skapnisjer ved lærerrommet og en skjermet rute bak teknisk utstyr. Minst én lesbar siktbrytende rute i hver klasseromsving og hver kjellerving. Reoler kan være 180–210 cm høye, men skal ikke skape fastkiling eller dekke begge dørene til samme område. Bruk minst 120 cm fri ganglinje som utgangspunkt og sjekk mot faktisk spiller og fiende. Et H-symbol betyr et foreslått skjul, ikke implementert interaksjon eller garantert trygghet. Ikke baser ruter på kryping, hopping, ventiler eller flyttbare møbler uten bekreftet støtte i spillet.

### Kontroll før detaljering
Romforbindelsene er kontrollert som en graf med rundløyper. Hvert viktig undersøkelsesrom skal kunne nå uteområdet når enten hovedinngangen E1 eller hovedtrappen S1 er stengt. Kontroller også blokkert direkte korridordør fra hvert undersøkelsesrom. Dette er designkontroll, ikke dokumentert spilltest. Bygg først gråboks, gå rutene med spillerkapsel, og kontroller trapp, dørslag, reell sikt og trusselens navigasjon før detaljering.


## Koordinater og hovedmål
Alle tall nedenfor er centimeter. Bruk X mot øst, Y mot nord, Z opp. Sørvestre ytterhjørne ved (0,0,0). Sør er hovedfasade og inngang. Ingen motorendringer i andre prosjektkopier er del av denne leveransen.

Ytre bygningskropp: X=0..2800, Y=0..2400. Én hovedetasje.
Delvis kjeller: X=0..2800, Y=1200..2400, altså under nordre halvdel.
Hovedetasjens ferdige gulv Z=0; kjellergulv Z=-340.
Takunderside/himling hovedetasje Z=360; takdekke topp Z=400.
Parapet topp Z=460; sentralt inngangsparti topp Z=580.
Utvendig terreng ved hovedinngang omtrent Z=-60. Lag 4 opptrinn à 15 cm og inngangsrepos.
Yttervegger 30 cm innenfor ytterkonturen; skillevegger 15 cm. Etasjeskille 25 cm under Z=0. Kjellerhimling Z=-60 gir 280 cm fri romhøyde.
Grid for blokkering: 10/50/100 cm. Gjentakende fasademoduler 200 cm; egne avslutningsmoduler der rommene krever det.

## Hovedetasje – brutto romsoner
Dette er sonegrenser for blokkering, ikke frie innvendige rommål. Delte skillevegger sentreres på grensene og trekkes fra romarealet; yttervegger ligger innenfor konturen.

| Rom | X fra–til | Y fra–til |
|---|---:|---:|
| Kontor / resepsjon | 0–1200 | 0–400 |
| Lærerrom med liten kjøkkenbenk | 0–1200 | 400–800 |
| Klasserom 1 | 0–1200 | 800–1600 |
| Klasserom 3 | 0–1200 | 1600–2400 |
| WC A | 1600–2800 | 0–400 |
| WC B | 1600–2800 | 400–800 |
| Klasserom 2 | 1600–2800 | 800–1600 |
| Klasserom 4 | 1600–2800 | 1600–2400 |
| Korridor / vestibyle | 1200–1600 | 0–1800 |
| Lukket U-trapp til kjeller | 1200–1600 | 1800–2400 |

Dobbelt hovedinngang sentrert ved X=1400, Y=0. Åpning 200x240 cm.
Alle åtte rom har egen dør til korridoren. Omtrentlige Y-posisjoner for dørsentre: 200, 600, 1100 og 1700, på begge sider. Dørene til bakre klasserom ligger før trappesonen. Korridorens dør til trapp ved X=1400,Y=1800.
Enkeltdører: fri åpning 110x220 cm. Modellér karm og veggåpning større ved behov. Kontroller kollisjon med spillerkapselen.
Fire klasserom: omtrent 20 elevplasser, lærerbord, tavle, skap. Store vinduer på ytterveggene. Toaletter: tre avlukker, vaskerekke og ett romsligere avlukke i hver gruppe; høye frostede vinduer. Kontor: resepsjon og arkivskap. Lærerrom: bord, sofa og kjøkkenbenk.

## Kjeller – brutto romsoner
| Rom | X fra–til | Y fra–til |
|---|---:|---:|
| Lager | 0–1200 | 1200–1600 |
| Fyrrom / teknisk | 0–1200 | 1600–2400 |
| Verksted | 1600–2800 | 1200–1600 |
| Arkiv | 1600–2800 | 1600–2400 |
| Korridor | 1200–1600 | 1200–1800 |
| U-trapp, samme sjakt som oppe | 1200–1600 | 1800–2400 |

Korridor har separate dører til lager og verksted ved Y=1400, og til fyrrom og arkiv ved Y=1700, før trappesonen starter ved Y=1800. I tillegg skal lager og fyrrom forbindes direkte gjennom veggen Y=1600 ved X=300 (D5), og verksted og arkiv gjennom samme Y-vegg ved X=2400 (D6). Begge kjellervingene får dermed en innvendig rundløype. Ingen dører går gjennom trappeløp.
Ekstra utgang fra arkiv gjennom østveggen ved X=2800,Y=2100, via repos og åpen betongtrapp langs østfasaden opp til terreng. Denne ligger utenfor hovedfotavtrykket. Avsett en ca. 200x700 cm utvendig sone for repos, trapp og lysgrav, og juster etter terrenget. Ikke plasser denne under den sørlige halvdelen uten kjeller.
Hovedtrapp: U-form, to løp, totalt 20 opptrinn à 17 cm = 340 cm. 10 opptrinn per løp, ca. 28 cm inntrinn og 150 cm løpsbredde. Mellomrepos ved Z=-170. Øvre og nedre adkomst ligger i sørenden av sjakten ved Y=1800. Verifiser minst 220 cm fri høyde i trapp og åpne etasjeskillet over hele nødvendige trappesonen.
Fyrrom: kjele, varmtvannstank, ventiler og synlige rør. Lager: hyller, kasser, skolemateriell. Verksted: arbeidsbenk, verktøytavle, servicevask. Arkiv: metallskap og dokumenthyller. Ingen ekte merkenavn.

## Utseende og byggerekkefølge
Slitt rød tegl, kremfarget puss rundt Art Deco-inngangen, mørke stålrammer, malt treverk, linoleum oppe og betong nede. Skilt: CALICHE PUBLIC SCHOOL. Flatt tak med parapet, takventiler og værslitasje. Bruk diskrete variasjoner i slitasje og materialtiling.
1. Blokker volum, etasjer, kjeller, rom og trapp etter tallene.
2. Gå gjennom alle dører og begge kjellerruter med spillfigur. Kontroller trapp, fri høyde og terrengoverganger.
3. Bygg fasademoduler, vinduer, karmer og tak slik at de følger rommene.
4. Legg på materialer, møbler, tekniske installasjoner, lys og kollisjon.
5. Ta utvendig bilde, bilde langs korridor, klasserom og kjeller før videre detaljering.

## Genereringsbrief
Trailer: én separat værslitt single-wide i off-white korrugert metall med falmet rød stripe, liten treveranda og støvete Caliche-tomt, realistisk varm ettermiddag, ingen andre bygninger.
Skole: én eldre rød teglskole i amerikansk sørvest, kremfarget Art Deco-inngang, én etasje og delvis kjeller, 28x24 m konsept, realistisk varm ettermiddag, ingen trailer eller vanntårn.
Plansje: skolebildet som referanse, separate hovedetasje- og kjellerplaner, fire klasserom, kontor, lærerrom, to WC-grupper, lager/fyrrom/verksted/arkiv, mål, snitt, fasader og Unreal-modulnotater.
Prosjektbiblioteket ble undersøkt med «Unreal skole modulbygg romplan» og «Unreal modulbygg»; ingen relevante treff ble funnet. Ingen biblioteksløsning er gjenbrukt.
