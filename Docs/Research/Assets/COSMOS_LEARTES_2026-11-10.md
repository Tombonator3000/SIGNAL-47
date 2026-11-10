# Cosmos by Leartes: oversikt for SIGNAL / 47

**Status: Referanse** · **Gjelder:** Toms Cosmos-inventar (78 oppføringer) og abonnementsmåneden vurdert mot SIGNAL / 47; ingen pakke er lastet ned eller testet i Unreal 5.8 ennå. **Sist kontrollert:** 10. november 2026. Inngang: [README](README.md).

Kun offentlige sider (cosmos.leartesstudios.com, Fab, Gumroad, Epic-forum, nerdydeals). Ingen innlogging, ingen nedlasting. Cosmos-filer ligger aldri i Git (se lisens under); bare denne oversikten gjør det.

Viktig begrensning: Cosmos-produktsidene (/environments/..., /assets/..., /free, /bundles) rendres i nettleseren og gir ingen innhold uten JavaScript. Pakkeinnhold er derfor hentet fra Fab- og Gumroad-oppføringene (samme utgiver) og fra søkeresultater. Det er markert der noe ikke er bekreftet. Meshantall er utgiverens egne tall og avviker mellom oppføringer.

## 1. Kort oppsummering

- Eierskap: Tom eier mest fantasy, middelalder, cyberpunk og post-apo. Det som faktisk passer New Mexico 1986: kirken (Colonial Convent Church), 1900s Office (SARO-rom), Diner Booth Seating, Coffee Shop, VFX Smoke, SFX Warzone og Footsteps/Movement, 1950s-dressmenn (menn fra staten), Police Character (deputy), Spanish Cottage, Post Apo Playground, Container Port og skyttergrav-/gjerdedetaljer fra favela og Middle Eastern Town.
- Lisens: Studio License tillater kommersielt spill. Ingen klausul nevner offentlige Git-repoer, men kontrakten forbyr å gjøre assets tilgjengelige "as standalone files" for tredjeparter. Rå assets bør derfor IKKE ligge i det offentlige repoet. Dette stemmer med Toms eksisterende regel.
- Abonnement: standardabonnementet (Individual/Studio) dekker 3D-assets, lyd, VFX og materialer, men IKKE environments og tools ("must be paid for individually"). Premium-kreditter gjelder environments. Det er uklart om nedlastede assets forblir lisensiert etter abonnementet. FAQ sier at gratisprodukter fra premium beholdes. Last ned og behold alt som trengs, og eksporter kvittering/lisensbevis.
- Last ned først (på Standard): (1) lyd: Post Apo Diner SFX, SFX Retro Police Station og andre lydpakker, (2) VFX-biblioteket (støv, røyk, gnister, elektrisitet), (3) Vintage-kategorien (kassetter, TV-bord, telefon, radioklokke), (4) materialer (sand, adobe, asfalt, rust). Environments (gassstasjon, diner, Western Town) kjøpes enkeltvis, eller sjekk om de allerede ligger i Humble-pakkene (se pkt. 4).
- Humble-pakkene: innholdet per tier lot seg ikke bekrefte offentlig. Bruk Cosmos-siden /redeem og kontoen til å se hva som er innløst.

## 2. Lisens

### 2.1 Studio License og Individual License

Kilde: https://cosmos.leartesstudios.com/legal/digital-asset-licensing-contract (Digital Asset Licensing Contract), https://cosmos.leartesstudios.com/frequently-asked-questions

- Individual: "designated for individuals operating independently or as sole proprietors" med "annual revenues not exceeding $100,000 USD". "Grants permission for use in both commercial and non-commercial projects."
- Studio: "intended for studios or independent developers with annual revenues of up to $2 million USD" "or those employing between 1 to 11 personnel."
- Enterprise: over $2 million eller 12 eller flere ansatte. Student: kun ikke-kommersielt.
- Seksjon 2.1: Licensee kan "use, copy, adapt, modify, and create derivative works based upon the Licensed Content" "for incorporation into various projects, including ... game development", "for both personal and commercial purposes". Fortale (B): formålet er "developing and commercializing interactive digital media contents and games."
- Kontrakten bruker ikke ordet "shipped", men inkludering i et utgitt kommersielt spill dekkes av 2.1.
- Teamstørrelse: hvis teamet vokser over nivået etter kjøpet, "there shall be no obligation to upgrade the product licenses or incur additional fees" (Art. 5). FAQ: Individual-lisens kan ikke deles mellom to personer.
- Videresalg/deling: forbudt å "Redistribute, resell, lease, license, sub-license, or offer the Licensed Content on a standalone basis" (2.2). Art. 5: "shall not make the Digital Assets available in a manner that allows third parties to download, extract, or access the assets as standalone files."
- Offentlig Git-repo: kontrakten nevner verken offentlige repoer, versjonskontroll eller open source (bekreftet ved lesing). Nærmeste klausul er Art. 5 over, og Terms of Use VII.4(a): "Redistribution of Assets, either for free or for a fee to third parties, is expressly forbidden." (https://cosmos.leartesstudios.com/legal/terms-of-use). Et offentlig repo lar tredjeparter laste ned rå filer. Vurdering (ikke juridisk råd): ikke tillatt i praksis. Hold rå assets utenfor Git mens repoet er offentlig, slik prosjektregelen allerede sier. Spør Leartes (https://cosmos.leartesstudios.com/contact) hvis du vil ha skriftlig svar.
- Humble-pakker: "Humble Bundle's Bundles are licensed under Studio License." (FAQ). Tom: "Studio License" på de fleste oppføringene stemmer.
- AI/ML: 2.2 forbyr å bruke innholdet som "training data or reference material for AI/ML systems, neural networks, or any automated content generation tools". Terms of Use VII.3(b) og FAQ gjentar det. Konsekvens for Tom: ikke mat Cosmos-assets inn i AI-verktøy som referanse eller treningsdata (Codex/Claude som bildereferanse, Tripo/bilde-til-3D med Cosmos-modeller som input osv.). Å bruke dem som vanlige assets i Unreal er greit. Hva som regnes som "reference material" er ikke definert.
- Salg av spillet/overdragelse: lisensrettigheter kan overføres til ny eier, men Cosmos må varsles formelt per e-post (FAQ).
- Tilbakekalling: Art. 9, "All rights granted to the Licensee under this Contract shall immediately cease" ved opphør av kontrakten (utdraget viser ikke fullt vilkår; sannsynligvis brudd på kontrakten).

### 2.2 Abonnementet

Kilder: https://cosmos.leartesstudios.com/legal/subscription-agreement-for-leartes-studios, https://cosmos.leartesstudios.com/legal/subscription-agreement-preliminary-information-form, https://cosmos.leartesstudios.com/frequently-asked-questions

- Nivåer i avtalen (2.2, 2.3): Free (ingen kreditter), Individual $2,99/mnd, Studio $9,99/mnd, Enterprise $99,99/mnd; Premium Individual $14,99, Premium Extra Individual $49,99, Premium Studio $149,99, Premium Enterprise $1499,99 per måned. Individual/Studio inkluderer "3D, sound, VFX, and material assets" og teknisk støtte.
- "Standard" finnes ikke som navn i avtalen. Skjemaet nevner "Standard Subscription Members earn 3% cashback in Cosmos Credits on environment asset purchases" (Premium: 5%). En Gumroad-oppføring "Cosmos One month Subscription" (https://leartesstudios.gumroad.com/l/ftviv) skal ha pris $19,99 ifølge søkeutdrag (ikke bekreftet, siden siden ga bare tittel). Avtalens priser kan være utdaterte (FAQ nevner 50 % rabatt for de første 1000 abonnentene og dobling etterpå). Sjekk hvilken lisens (Individual eller Studio) abonnementet faktisk gir Tom.
- Nedlastingsgrenser: avtalen setter ingen nedlastingsgrenser for abonnementsassets (ingen grense omtalt). Kreditter: Premium Individual 1 per måned, Extra/Studio/Enterprise 3 per måned, til environments, tools eller bundles. Kredittkostnad etter pris: under $99 = 1, $99 til $198,99 = 2, ... $499+ = 6 (FAQ). Standardnivået har ingen kreditter ("Cosmos Credits" gis ikke på Free; for Standard er det ikke bekreftet).
- Environments og tools er ikke inkludert: "Environments and tools are separately licensed digital assets and must be paid for individually" (2.5).
- Etter at abonnementet slutter: IKKE avklart. Avtalen 5.4: abonnenten beholder tilgang "until the end of the current subscription period"; 5.5: tilgang opphører ved opphør. 6.4 henviser til lisenskontrakten, som ikke har abonnementsvilkår og ikke setter noen utløpsdato på lisensen. Terms of Use VI/XI: tilgang til nettstedet forsvinner. FAQ: "If I cancel my premium subscription, will I keep the free products I received with my premium subscription? Yes, You will continue to have these products even after your subscription ends." Det gjelder uttrykkelig produkter mottatt med premium (kreditter). For vanlige abonnementsassets er det uklart. Anbefaling: last ned det du trenger i løpet av måneden, lagre lisens/kvittering (skjermbilder av kontoen, ordrenummer) og spør support skriftlig før lansering.
- Avbestilling: automatisk fornyelse, kanselleres via Xsolla (FAQ). Ingen refusjon av ubrukt tid. Slå av fornyelse med en gang hvis det er en enkeltmåned.

## 3. Levering og Unreal

- Nedlasting: lenker legges automatisk på kontoen; zip fra nettsiden (FAQ: "The download links for your purchased products are automatically added to your account").
- Unreal assets: pakk ut zip, kopier mappen "Rocket" fra Unreal-mappen til prosjektets Content-mappe. Environments: bruk Unreals Migrate fra kildeprosjektet (FAQ). Formater: OBJ/FBX, PNG/JPEG, UASSET, WAV/MP3.
- Rocket-plugin: gratis plugin på Fab for å dra og slippe Cosmos-assets, VFX, lyd og materialer, og laste ned environments (https://forums.unrealengine.com/t/leartes-studios-cosmos-rocket-plugin-drag-and-drop-free-plugin/2461577). Motorversjon og plattform (Linux) står ikke på siden: IKKE bekreftet. Gumroad selger samme plugin for $4,99 (https://leartesstudios.gumroad.com/l/Rocketplugin). Fab-plugin og Epic-launcher: Epic Games Launcher finnes ikke på Linux, og Fab-pluginen har kjente problemer på Linux for plugin-kjøp (https://forums.unrealengine.com/t/fab-on-linux/2149608). Praktisk løsning på Linux: nettside-zip og manuell kopiering/Migrate. Cosmos-zipene er vanlig Unreal-innhold, ikke Windows-spesifikt (Sourwood-oppføringen sier "Windows only" som plattform, men det er innholdsflagget, ikke noe som stopper Linux-editor; ikke testet).
- Unreal-versjoner: Colonial Convent Church er oppført med UE 5.0 til 5.8 på Cosmos (søkeutdrag av https://cosmos.leartesstudios.com/environments/colonial-convent-church), Gas Station 5.3 til 5.8 (https://cosmos.leartesstudios.com/environments/gas-station). Sci-fi Cargo Ship krever 5.7+ (https://cosmos.leartesstudios.com/environments/cargo-ship). Eldre pakker (1900s Office: UE 4.22 til 4.27, UE5, 5.1 til 5.3 per Gumroad; Sourwood 5.0 til 5.5+; SUV 01 UE 4.19 til 4.27 og UE5) er ikke merket for 5.8, men innholdet er vanlig statiske meshes og lar seg normalt oppgradere. Biler (Blueprint) og VFX bør testes. Ingen pakke er testet her.
- Mange pakker følger med ULAT (Ultimate Level Art Tool, oppgitt verdi $100, hentes via support-ticket) og Lumen/Nanite fra 5.0.

## 4. Eide pakker: innhold og vurdering

Skala: A = bruk direkte, B = løft enkeltelementer, C = la ligge. "Ubekreftet" betyr at innholdet er utledet fra tittel eller tagger, ikke fra en detaljert liste.

| Pakke | Innhold (kilde) | Vurdering for SIGNAL / 47 |
|---|---|---|
| Colonial Convent Church Environment | 116 til 126 unike meshes, meksikansk/spansk koloni, UE 5.0 til 5.8, tekstur 1024 til 4096, Nanite, Lumen, ferdig scene. Ikke itemisert (Fab: https://www.fab.com/listings/14d933c1-c66a-4313-9e3b-f0966d07ca01) | A. Caliche-kirken: adobe/kalkede vegger, trebenker, buer, dører, korsfeste og klokketårn er sannsynlige deler (ubekreftet; sjekk i editor). Kandidat til hele kirkebygget. |
| Diner Booth Seating | Ingen enkeltside funnet; ligner båssofa-sett fra Rino's Diner. Individual License | A for diner (bås, vinylseter). Ubekreftet innhold. |
| Coffee Shop | Realistisk, 42 unike meshes, UE 4.25+ (Gumroad: https://leartesstudios.gumroad.com/l/CoffeeShopEnvironment) | B. Disk, kaffemaskin, kopper og stoler til diner/bensinstasjon. Moderne preg, så velg bort lyse espresso-detaljer. |
| Post Apo Coffee Shop | 275 unike meshes, gritty, Nanite/Lumen UE 5.0+ (https://leartesstudios.gumroad.com/l/post-apoc-coffee-shop) | B. Skrap og rusk til forlatt motell/butikk. Mye er "post-apo", ikke 1986. |
| 1900s Office | 109 unike meshes, 4096/2048/1024 tekstur, 39 materialer, UE 4.22 til 4.27, UE5, 5.1 til 5.3 (https://leartesstudios.gumroad.com/l/1900soffice) | A/B. Skrivebord, arkivskap, lamper, mapper til SARO kontrollrom og arkivrom og kommunekontor. For gammelt (1900-tallet), så velg treverk og kontorrekvisitter; sjekk om skrivemaskin finnes. |
| Victorian Interior | Viktoriansk møbler/interiør (ubekreftet innhold) | B. Stoler, lamper, bokhyller, teppe, speil til huset. Fremstår gammelt, passer eldre beboer (Twin Peaks-preg). |
| Abandoned / Post-Apocalyptic (environment) | Del av stort post-apo-utvalg (2000+ meshes i megabundle, https://leartesstudios.gumroad.com/l/Abadnonedpostapomegabundle) | B. Forlatt gate, rusk, rust, dekk, containere, skilt; nyttig for nedslitt Caliche. |
| Post Apocalyptic Playground (karusell, sklie og kuppel, huskestativ) | Rustne lekeplassmeshes, Vol 1 til 3 (https://www.fab.com/listings/4f00db78-4fd8-4262-bf27-cad5d1cdd12a) | B/A. Lekeplass ved kirke/skole i Caliche eller på gården; rust passer ørken. |
| Forgotten Echoes | Ikke funnet/ukjent | C inntil bekreftet. |
| Grand Ossuary | Benhus, skjeletter (ubekreftet) | C (B for lysestaker/steinvegg i en sjelden hendelse). |
| Gothic Cathedral | Katedral-interiør (https://cosmos.leartesstudios.com/environments/church-cathedral, 147 meshes for Old Cathedral) | B. Benker, lysestaker, glassmaleri-detaljer, kors, alterduk til kirken. Selve bygget er for stort. |
| Fantasy Gothic Castle (med kirkegård) | Kirkegård, gravsteiner (Toms opplysning; ikke bekreftet av oppføring) | B. Gravsteiner, jerngjerde, kors til kirkegård; velg slitte, enkle. |
| Colonial Convent Church | se øverst | A |
| Spanish Cottage | 106 meshes (Gumroad oppgir uklart/kopiert tekst: https://leartesstudios.gumroad.com/l/fdnij) | A/B. Hvite/okerfargede vegger, trehyller, tak til adobehus (verifiser i editor). |
| Sourwood Trees | 5 trær, Nanite, 3 LOD, UE 5.0 til 5.5+ (https://leartesstudios.gumroad.com/l/sourwood-trees) | B. Noen løvtrær ved huset/gården; ikke ørken. Svært tunge (38K til 301K trekanter). |
| Middle Eastern Town Gigapack | Flate tak, murvegger, dører, gater (ubekreftet innhold) | B. Adobe-vegger, flate tak, trapper, ledninger til hus og skur. |
| South American Slums / Favela | https://cosmos.leartesstudios.com/environments/south-american-slums-favela | B. Bølgeblikk, stolper, tak, ledninger, vanntanker til trailere og skur. |
| Medieval Russian Village | Trehus, gjerder, brønn (ubekreftet) | B. Gjerder, trehus/lade, brønn til gården. |
| Medieval Tavern, Medieval Castle Interior, Dwarven Dungeons, Merlin's Cave, Frozen Harbor Fortress | Fantasy/middelalder | C. (Merlin's Cave og stein fra Dwarven kan brukes som grotte under mesaen: B.) |
| Container Port | Containere, kraner (ikke funnet i søk) | B. Containere, paller, kjettinggjerde, lasterampe til SARO-garasje, regjeringens kjøretøyområde. |
| Underwater Art Deco Hotel | Art deco-hotell | C (B for lobbydisk/motellskilt med retrostil). |
| Cyberpunk Apartment, Night Club, Neo Tokyo | Neon, skjermer, elektronikk | C (B: tavler, kabler, CRT-lignende skjermer og små neonskilt, hvis de finnes). |
| Stylized Village, Eastern Province, Ancient Ruins, Ancient Athens, Lost Civilization (StylArts) | Tegneserieaktig stil | C (stil passer ikke). |
| Minipoly Series Modern Island | Lavpoly | C. |
| 1950s Businessman / Mafia Character og 1950s Mafia Character / NPC | Rigget til UE-mannequin-skjelett, kun dressmann (https://www.fab.com/listings/bba4384a-5165-4fc8-929c-bff8544ab3bb) | A. Menn fra regjeringen: svart dress, hatt, slips. 1950-tallsstil kan være litt gammeldags for 1986, men det passer "menn i svart"-stemningen. |
| Police Character / NPC | Rigget politi-NPC (https://www.fab.com/listings/879f876d-0c63-4a23-8e86-7f12cbce874c) | A/B. County-deputy; sjekk uniformfarge og skiltet. |
| Minotaur Statue, Roman Statues Vol 2 | Statuer | C (B for én marmorstatue i et rom). |
| Sedan Car, SUV 01, SUV 04, HatchBack | Drivbare, animerte, Blueprint, dører åpnes med Q/E/R/F, 8 til 26 meshes (https://www.fab.com/listings/603e54ee-cdc5-4709-8287-87ee3b6065fe) | B. Karosseri ser moderne ut (årsmodell står ikke oppgitt), trolig for nye for 1986; bruk som "bakgrunnskjøretøy" for biler i bakgrunnen eller bytt materialer. Ikke til spillerens pickup. |
| Driveable/Animated Excavator | Gravemaskin | B. Veiarbeid ved strømlinjene eller gården. |
| Driveable SteamPunk Car, Cyberpunk Sports Hover, City Scooter, Military Drone | Fantasy | C. |
| SFX Character Footsteps | Ikke funnet som egen Leartes-oppføring; Leartes har "Hyper Footstep System V4" (UE 5.7, per overflate, uten lyder) som verktøy (https://cosmos.leartesstudios.com/tools/gbh-footstep-system) | A hvis pakken har overflater (grus, tre, betong). Ubekreftet hvilke overflater. |
| SFX Character Movement | Ikke funnet | B. Tøyraslinger, pust, fall. |
| SFX Medieval Armor & Body Impact | Rustning/slag | C (B for kroppsfall/slag). |
| SFX Cyberpunk Guns, SFX Mobile Game, SFX Cyberpunk | Sci-fi våpen, UI | C (B: elektroniske pip, UI-klikk til CRT og radio). |
| SFX Warzone | WW2-tema, én ambience-loop, kamp og ødeleggelser (https://leartesstudios.gumroad.com/l/Sfxwarzone) | B. Eksplosjoner, glass, metallslag til generator, UFO-hendelse. |
| VFX Smoke | Tåke, skorsteinsrøyk, giftig røyk, UE 5.0 (https://leartesstudios.gumroad.com/l/VFXSMOKE; Niagara/Cascade ubekreftet) | A. Skorsteinsrøyk, støv, tåke over playaen, røyk fra generatoren. |
| VFX Explosion | Eksplosjoner (ubekreftet) | B. Generatorsvikt, transformator. Lys og gnister. |
| FPS 4K Custom Modern Shotgun, Handgun, Western Guns Vol 1 og 2 | Våpen i førsteperson (https://www.fab.com/listings/349b7fd8-ee53-467a-a61d-d0fb709f1994 for Vol 2) | B. Hagle, revolver og rifle til gårdbruker og sheriff. Bare dersom spillet trenger våpen. |
| Bright Eye (PulsarDev) | Editorverktøy for arbeidslys i viewport | A som arbeidsverktøy; ikke en spillpakke. Linux-støtte ubekreftet. |
| QA Agent: Crash Reporting, Project Optimization Tool | Verktøy | B. Prøv Optimization-verktøyet på LOD/Nanite; krasjrapportering etter behov. |

Vurdering av biler: Cosmos oppgir ikke produksjonsår. Fab-teksten sier bare "four different sedans". Se miniatyrbildene før bruk. Vi vet ikke om noen av dem er boxy 80-talls.

## 5. Humble-pakkene

Bekreftet:
- "The Ultimate Cyber November Game Dev Bundle" (Humble, Leartes Studios): tre betal-selv-nivåer, $5 / $15 / $30 med 5 / 12 / 74 elementer (https://www.nerdydeals.net/bundle-deals/, oppdatert 9. november 2026; "$7,785 worth", ca. 17 dager igjen). Cosmos-bundlesiden lister Tier 1 til 3 med tallene 9, 72 og 12 (uklart hva de betyr; https://cosmos.leartesstudios.com/bundles).
- Innholdslister per tier: IKKE bekreftet. Humble-siden er blokkert for oss, og Cosmos-bundlesidene krever JavaScript.
- Tidligere Leartes-bundler (Super Game Assets Bundle ENCORE Tier 1 til 3: https://cosmos.leartesstudios.com/bundles/hb-tier-1 osv., Giga Bundle Tier 3) har samme struktur, og viser at disse inneholder bl.a. 1950s Mafia/Businessman og Police Character. Toms liste (78 oppføringer) ser ut til å blande flere bundler. Dermed kan duplikater skyldes flere tier-nivåer.
- Hvordan sjekke uinnløst: Humble-bibliotek (Keys) og Cosmos /redeem. FAQ: "Humble Bundle's Bundles are licensed under Studio License".

## 6. Ønskeliste (søk i katalogen)

Alle prisene nedenfor er Gumroad/Fab-listepris (ikke nødvendigvis Cosmos-pris) hvis oppgitt. "I abonnement" = 3D-assets, lyd, VFX og materialer; environments og tools er betalt enkeltvis (Subscription Agreement 2.5). Cosmos-prisen og om en gitt pakke er "gratis" er IKKE bekreftet fordi /free og /bundles ikke vises uten JavaScript. Ingen gratispakke er identifisert ved navn (Victorian Decayed Alley var gratis en måned i fortiden, https://80.lv/articles/download-leartes-new-environment-asset-pack-for-unreal-engine-for-free).

Prioritert (høyest først):

1. The Gas Station Environment, 91 til 93 meshes, UE 5.3 til 5.8, 50x50 m spillområde. Betalt (Gumroad $89,99). https://cosmos.leartesstudios.com/environments/gas-station
2. Rino's Diner Environment, 1950-talls amerikansk diner (båser, skranke, jukebox, neon; kun tagger bekreftet). Betalt. https://www.fab.com/listings/ebc98fa6-5c25-4b45-953d-29bd1781b4ff
3. Post-Apo American Diner (447 meshes, rene og overgrodde varianter, modulær, Route 66). Betalt; gir gratis "Post Apo Diner SFX" via ticket. https://www.fab.com/listings/55b0fa5c-89bb-4e7b-9cd1-3e83c145319b
4. Western Town / Village Pack (ca. 10 bygg: butikker, hus, gårdsbygg, kabler som spline-Blueprint). Betalt. Bra for Caliche-gaten (1800-tall, må stilsettes om). https://www.fab.com/listings/ffff78e6-f9a0-40ac-b5e0-f020fac237a0
5. Post Apocalyptic Desert Environment (87 meshes). Betalt. Ørkenbakke, stein, kjøretøy? (ubekreftet). https://leartesstudios.gumroad.com/l/kjori
6. Wasteland (168 meshes; tagger "pole" og "fence"). Betalt. https://cosmos.leartesstudios.com/environments/wasteland
7. Vintage-kategorien (kassetter, TV-bord, telefon-tilbehør; 4 kreditter per stk). Sannsynlig abonnements-/kredittbasert. https://cosmos.leartesstudios.com/product-category/3d-assets/realistic/vintage/ og Radio Clock https://cosmos.leartesstudios.com/assets/cs1-radio-clock
8. Retro Police Station Environment + SFX Retro Police Station (lyd, Gumroad). Deputy-kontor, celle, retro-interiør. Betalt/ lyd kan ligge i abonnement. https://leartesstudios.gumroad.com/l/RetroPoliceEnvironment og https://leartesstudios.gumroad.com/l/SFXPolice
9. Post Apocalyptic NYC (8 drivbare kjøretøy; ukjent hvilke) og Auto Repair Garage / Garage Warehouse (verksted til SARO-garasje). Betalt. https://cosmos.leartesstudios.com/environments/post-apocalyptic-nyc, https://cosmos.leartesstudios.com/environments/sb-auto-repair-garage
10. Hyper Footstep System V4 (verktøy, UE 5.7, lyder følger ikke med; pris ukjent). https://cosmos.leartesstudios.com/tools/gbh-footstep-system
11. Vegetation Pack (45 meshes, UE 5.1 til 5.4+). https://leartesstudios.gumroad.com/l/vegetation-pack
12. Stylized Wild West (stylArts, 249 assets) og Stylized Desert Town (67 meshes): stil passer neppe. https://cosmos.leartesstudios.com/environments/stylized-wildwest
13. Medieval Farmstead Builder (CropCraft, ca. 330 meshes, UE 5.5 til 5.8; åkre og gjerder, middelalderstil). https://cosmos.leartesstudios.com/environments/ccs-medieval-farmstead-builder
14. Retropunk Saloon (146 meshes). https://cosmos.leartesstudios.com/bot/environments/retropunk-saloon
15. American Suburban House Environment (Barrel Bark, ikke Leartes; postkasse, innkjørsel, gjerde, UE 5.0+). Fab, betalt. https://www.fab.com/listings/8807c029-cf26-4082-a976-40907d7a5798

Ikke funnet på Cosmos (søk ga ingenting): strømstolper/kraftlinjer, radioteleskop/parabolantenner, pickup fra 70- og 80-tallet, CRT-TV og -datamaskin, kassettopptaker, telefonkiosk, generator, motell, tilhenger/trailer, åker/avling, steinformasjoner/mesa, UFO- og stråle-VFX, kirkegård som eget produkt. Alle disse må løses utenfra (Fab/Quixel/Sketchfab/Poly Haven) eller modelleres selv. Fab-pakker for strålevfx finnes (f.eks. https://www.fab.com/listings/d87cd9d5-e56a-40ac-bfd6-66a7401ee676 Energy Beam Pack, 26 Niagara-effekter), men er ikke fra Leartes.

## 7. Åpne spørsmål

1. Hva som faktisk er inkludert i standardabonnementet (Individual eller Studio?) og om hvert 3D-asset koster kreditter (Vintage-sidene viser "4 Credits").
2. Om assets nedlastet under abonnementet fortsatt er lisensiert etter at det slutter. Svar fra support bør innhentes skriftlig.
3. Om rå assets kan ligge i et offentlig repo (ingen uttrykkelig klausul, men Art. 5 og Terms VII.4(a) taler imot).
4. Innholdet i de tre Humble-nivåene og hva som er innløst.
5. Rocket-pluginens støtte for UE 5.8 og Linux; Cosmos-eksporter for 5.8 (flere pakker er merket kun 5.0 til 5.5).
6. Eksakt innhold i en rekke eide pakker (Diner Booth Seating, Forgotten Echoes, Grand Ossuary, VFX Explosion, Footsteps): se i editor.
7. Er bilene tidsriktige for 1986 (årsmodeller ikke oppgitt).
