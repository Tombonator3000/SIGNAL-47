> **Status: Referanse** · Klassifisert 28.10.2026. Gjelder det dokumenterte omfanget; [gjeldende leveringsstatus](../../Active/STATUS.md).

# Valgfri kveldsmat, besøksparkering og avstandsvisning

Bestilling 25. oktober 2026: kveldsmat skal gi en vanlig hverdagsaktivitet, SARO-parkeringen må ikke låse progresjonen, og småobjekter skal bruke mindre arbeid på avstand.

## Endring

Matkjøp var allerede valgfritt i I11_SARO_ARRIVAL og D01-overleveringen. Den reelle parkeringsporten brukte derimot bare midtplassen og krevde nordvendt bil. Alle tre malte besøksplasser godtas nå med bilen forlengs eller baklengs. Hele bilens fotavtrykk må fortsatt ligge innenfor en plass med samme lille sidetoleranse som før. Serviceplassen og gangstripen er ikke besøksparkering.

Tre vanlige VISITOR PARKING-skilt identifiserer plassene før bilen stopper. HUD skifter til parkeringsveiledning ved SARO og tilbake til kjøreveiledning dersom spilleren snur. Dineren omtales som et frivillig stopp. Ingen mat-, klokke-, telefon- eller lagringsflagg fabrikkeres for å komme videre.

Den fjerne gården øst for hovedveien er eksisterende landskapsdekorasjon med stengt port. Ingen ny besøksoppgave eller historieadgang er lagt til.

Floraens eksisterende gradvise shaderkrymping beholdes. Instanser som allerede er helt krympet utelates fra opplastingen med 8 m bevegelsesmargin. Native culling ligger etter full krymping. Skalerte/tiltede plantegrupper beholder gammel løype. Highway-auditen logger både vurderte, utelatte, opplastede og faktiske instansantall.

Hovedstrømlinjens spenn økes fra 45 til 65 m. SARO-kildestolpens eksakte posisjon og faste avgreninger beholdes. Dette er en reduksjon av geometri, ikke en dokumentert FPS-forbedring.

## Bibliotek

Prosjektbibliotek og START_HER.md ble lest. To behovssøk ga ingen kompatibel, dokumentert løsning for dette arbeidet. Ingen bibliotekskode eller nye assets ble gjenbrukt. Eksisterende Unreal 5.8.4-systemer ble tilpasset.

## Kontroll og levering

Bevis lagres i /home/tom/scratch-bm/Signal47-visitor-fix.

- Unreal 5.8.4 editor og Linux-pakking PASS. 1405 selvtester PASS.
- Samlet P1 består elleve fokuserte pakkede forløp: tre parkerings-/diskreloadkontroller, ankomstbilder, to virkelige områdeskifter, hele direkte kjøreruten uten diner og Dales overlevering, matmeny, SARO-, motell- og Rom6-bilder samt kapittel3-pilot. Direkteruten kjører 2622,8 m og fullfører på 201,1 s i automatiseringen; dette er ikke målt menneskelig spilletid.
- P2 endrer bare Fleets målte rendertrim og SaroSites trekantaudit. Alle øvrige kilde-/config-/CPU-asset-hash er kontrollert identiske med P1. P2 består native SARO-audit, tolv dag/skumring/nattbilder, vanlig innkjøring, faktisk gange til inngangen, diskreload og 1405 selvtester. Tidligere eksponerte144 hjulrester er borte; hjulstoppene beholdes.
- 36 relevante runtimebilder er vurdert: P2s SARO-bilder og P1s uendrede ankomst-, motell- og Rom6-bilder. Dette er spillbilder, ikke Blender-render.
- Veikontrollen registrerer 57 hovedstolper mot tidligere 82, og 3096 wiresegmenter mot 4296. En kontrollposisjon utelater 3359 av 6730 vurderte planteinstanser og laster opp 3371. Dette er geometri-/instansmåling, ikke en FPS-garanti.
- Oppryddingsverktøyets sikkerhetsprøver PASS. Etter levering er gjeldende pakke og akkurat to eldre komplette testpakker beholdt. To eldre payloadkopier er pensjonert; kilder, Saved, logger og bilder er bevart.
- Samme testede P2 er installert i Games/Signal47/Linux: 40 payloadfiler hashkontrollert og 40 Saved-filer byteidentiske. Kort oppstart av den installerte utgaven med isolert testprofil består; Saved er fortsatt identisk etter testen.

Full 110-scenariorunde, fysisk Xbox, menneskelig gjennomspilling/lytting og FPS-sammenligning er ikke utført. Ingen ny hovedgren-/releasepublisering inngår; kildeintegrasjon går til unreal-port-core.

## Tillegg: østfløy, motell og lagringsplass

Østfløyens eksisterende fasade beholdes. Forplassen får asfalt med sammenheng til serviceveien, en tydelig gangkant og tre personalfelt. Rød sedan flyttes fra x27 til x35, og det eksakte ganghinderet flyttes med bilen. Pickupen blir stående i eget personalfelt. Besøksfeltet vest for hovedinngangen er ledig for spilleren. Ingen ny historieadgang til østfløyen er implementert.

Asfalt og gammel fysisk bakke er atskilt: overflaten tegnes 24 mm over eksisterende gulv uten å heve gang- eller kjøregulvet. De samme avgrensningene utelukker ny prosedyreflora. Kun 230 målte brushDry/grassDry-trekanter i fasadens gamle kant og 144 gamle dekk-/hjulbrønnrester fjernes fra dressingklonen; 10328 øvrige trekanter, materialatlas, UV-er og takdetaljer beholdes. Kjør Tools/saroforecourt/prepare_cpu.py i editorens Python-commandlet før cook i en annen arbeidskopi. Dette beholder CPU-data på akkurat LegacyTrim-kildeasseten. Den opprinnelige geometrien endres ikke.

Installert F3-logg fra 25. oktober kl. 11:49 lokal tid bekrefter at bygningssettet faktisk lastes: seks familier, 28 native deler, 32 sammensatte patcher og 60 erstattede gamle tegneflater. Motellfamilien omfatter fløy, resepsjon, kontor og Rom 6 med bad. Dette er ikke bare nye skilt. Nye kontrollerte spillbilder føres i sluttresultatet nedenfor.

AGENTS.md har nå en varig grense på gjeldende verifiserte bygg pluss maksimalt to eldre. Scripts/prune-test-builds.py planlegger som standard, beskytter gjeldende/aktive kandidater og pensjonerer bare generert pakkepayload. Saved, kilder, logger og bilder bevares. Sikkerhetsprøver med midlertidige filer bekrefter antallsgrense, prosessblokkering, symlink-eksklusjon og at lagringer/kilder/bevis beholdes.
