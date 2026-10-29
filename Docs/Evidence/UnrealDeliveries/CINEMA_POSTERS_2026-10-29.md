# To utvendige Plaza-plakater, ART01

**Status:** Under arbeid. Målrettet native import og editorprøve består; pakket kontroll er UNVERIFIED. **Gjelder:** To eksisterende utvendige plakatskap på Plaza, UE 5.8.4 / Linux Vulkan, Low, 1280×720 og Picture off. **Sist kontrollert:** 29. oktober 2026.

The Desert Sky og Friday Matinee er bundet til kinoens to utvendige trykkflater. [PR228](https://github.com/Tombonator3000/SIGNAL-47/pull/228) leverer kilden. Begge består faktisk material-/UV-/mål-/synlighetskontroll, dag/skumring/nattbilder og en sammenhengende fysisk fotrute med original Closed-interaksjon og full disk/title/Continue. Kinoen er fortsatt stengt. [Portabel editorkvittering](../../../unreal/Evidence/CINEMAPOSTERS-2026-10-29/editor-r3-receipt.json) fører komplette kilde-, native- og guardbindinger, faktiske avlesninger og råfilhashes. [Fem originale PNG-er](../../../unreal/Evidence/CINEMAPOSTERS-2026-10-29/README.md) er kopiert byteidentisk.

| Bevisnivå | Faktisk kilde og resultat |
| --- | --- |
| Importoppskrift | `33ae8dc94912d0283ac5163762bf4f34033558fb`. Native R3 består 168 kontroller og en separat fersk leser består 169. Begge prosesser gir exit 0. Native editorleser var bygget fra `a79db0ac109834acef97c248e8eadb702b581fb5`. |
| Fem native assets | LFS-commit `42d63427e51fee4f258959c9b210b9799489bb03`. Alle fem er byteidentiske med de bevarte R2-filene. 293 beskyttede TB2-/master-/Engine-filer er uendret. Aktiv bindingsfil har SHA256 `c940872585dab6c2ed0051689a0b00da11d41bb5a65291646f229c976758b212`. |
| Faktisk editorprøve | `05a49e8a90f2122f0e27f34f5251205fdb1567b9`. Editorbygg R3 gir exit 0 på 9,64 s. Ni produksjonsbilder og begge fysiske kontrakter gir exit 0, uten tungjobbkonkurranse eller guard-stopp. |
| Pakket kandidat | UNVERIFIED. Planlagt som felles kandidat med HUM01 når den er klar. Ingen installasjon eller offentlig release er utført av denne leveransen. |

Git-referansen er operatørført. Råkvitteringene gir ikke alene en kryptografisk attestasjon fra den kjørte binærfilen til Git-commit. Kvitteringen binder de relevante kildefilene ved angitt head og de faktiske bygg-/motorloggene, og holder dette atskilt fra native filhashes og bildene.

Originalene er de to ferdige 1024×1536-JPG-ene i [EnvironmentGraphics](../../../unreal/ArtSource/EnvironmentGraphics/README.md). [Kildekontrakt og importoppskrift](../../../unreal/Signal47/Tools/cinemaposters/README.md) fører identitet og koordinater. TB2s målte trykkflater er 1,13×1,61 m, med sentre i Three `(7985.162,10.497393474578857,2169)` og `(7985.162,10.497393474578857,2155)`. Hvert motiv er innskrevet som 1,073333333×1,61 m, uten beskjæring eller strekk, 1 mm foran originaltrykket og 87 mm bak eksisterende glass. Sideforholdet er 2:3.

Nye flater bruker en egen CPU-lesbar kopi av Engine Plane, egne materialinstanser og eksisterende delt materialmaster. De har NoCollision og ingen skygge. Original TB2-geometri, glass, PrintAtlas, materialbindinger og kinoens proxy 1514 med rekkevidde 2,6 m er bevart. Ingen originalflate skjules. Hele paret valideres før det eksponeres; Off beholder originalene. Ingen interiøradgang, nye quests eller integrasjon av de fire interiørplakatene inngår.

Første native R1 ble avvist etter 35 kontroller fordi kildegruppens ID var behandlet som global materialslot og kompaktert renderseksjon ved LOD2. To riktige trekanter var funnet. Den avgrensede editorleseren måler nå faktisk PolygonGroup-importnavn, globalt import-/displaynavn, renderseksjonens MaterialIndex og SectionInfoMap hver for seg. Samme materialnavn og faktiske materialasset må stemme; kravet om to trekanter, kildeplassering og alle tre LOD-er er beholdt.

R2 gav en PASS-rapport med 178 kontroller og lagret fem assets, men commandleten avsluttet med exit 1. Lasting av de fraværende egne filene hadde logget ERROR før korrekt oppretting. R2 ble ikke akseptert, og filene ble bevart. Resume-import R3 bruker eksistenssjekk før lasting og avviser fortsatt feil eller manglende assets i verify. Fersk leser sammenligner hele kandidaten og alle fem filhashes før PASS-binding publiseres.

Første RHI-forsøk gav null PNG-er fordi materialparet ble avvist. Diagnostikken fra `631ad73e` viste korrekte stier, Map, forelder, scalarer, vektorer, sRGB og bindings-ID. Bare minimumsstørrelsen feilet: teksturen var en faktisk 32×32-editorplassholder med ressurs, IsCompiling=true og IsAsyncCacheComplete=false. I `05a49e8a` fullfører `FTextureCompilingManager::FinishCompilation` bare de to egne teksturene under WITH_EDITOR. Faktisk før-/etteravlesning i produksjonsloggen viser **begge** fra 32×32/kompilerende til 2048×2048/ferdige. Samme Ready-krav består etterpå. Ingen global kompileringstømming eller assetlagring er lagt til; pakket runtime bruker den ordinære valideringen.

| Målrettet editorprøve | Faktisk resultat |
| --- | --- |
| Produksjonsbilder | Ni komplette originale PNG-er, tre kameraer i hvert av dag-, skumrings- og nattlys. Ni auditer av paret og 18 individuelle plakatauditer består. Alle ni er gjennomsett og kontrollert med PNG-signatur, CRC/IEND, SHA og bildedekoding. |
| Normal fysisk rute | 1720 selvtester, 87 kontroller, 441 Walker-prøver og 37,384956 m. Begge plakater nås utenfra. Faktisk kameraray/fokus og ordinær Player.OnUse treffer den originale kino-interaksjonen to ganger. |
| Off | 1720 selvtester, 69 kontroller, 442 Walker-prøver og 37,403985 m. Null nye plakater; original Shell/Glass/proxy, Closed og samme diskreise består. Ingen audit av ny plakatkunst hevdes i Off. |
| Lagring og levetid | Begge ruter kopierer et uendret komplett case0-snapshot til isolert case1, bruker fersk diskleser og ordinær title/Continue gjennom OpenSave/StartGame, og skriver en vanlig lagring etterpå. Area-synlighet, originale komponent-/materialbindinger, dører, klokke og quest-/dialogtilstand kontrolleres. |

Fotprøven har ett uttrykkelig kapittel2-utgangspunkt ved `(7991,2175)`. Resten av tilnærmingen bruker vanlig Walker. Den særskilte World.Enter-runden til SARO og tilbake kontrollerer synlighet; den er ikke en fysisk reise mellom områdene. Ingen bilkjøring, naturlig ankomst eller menneskelig input hevdes. Produksjonsbildene er separate statiske kamerafixtures. De har logget fot/gulv/øye med tre desimaler og kildebestemt mål/nærbilde-FOV; faktisk CameraManager-FOV er avlest i de fysiske capture-kvitteringene, ikke i produksjonssuitens bildelogger.

De fysiske råresultatene var PASS. Første eksterne etterkontroll antok feilaktig at den registrerte interaksjonsetiketten måtte være tom mens SARO var vist. Etterkontrollen ble rettet til faktisk synlighet og ikke-pickbarhet; runtime og råbevis ble ikke endret. Begge gjeldende acceptance-kvitteringer er PASS og hashbundet her.

Original Closed-tekst er beholdt: «Closed. The doors are chained. The posters behind the glass have faded to the same blue.» Det vedlagte Closed-bildet viser både «E THE PLAZA», denne teksten og eksisterende kapittel2-/02:55-UI. UI-et følger den erklærte fotfixturen og er ikke omskrevet eller fjernet fra bildet.

Dagsnærbildene viser riktige titler og orientering. Nattbildet er mørkt; leveransen innfører ingen ny lamperegi. Alle ni lysbilder er faktisk gjennomsett av Codex, men maskinprøven gir ingen menneskelig lesbarhetsaksept. Den eksisterende advarselen om RT-geometri som alltid er resident, 83,942 MiB av 400 MiB, er beholdt i bildene. Dette er ikke FPS-, samlet GPU-minne- eller fullmatriseaksept. Samlet ART01, menneskelig spillinput og pakket kontroll står fortsatt åpne. Hele private testlagringer og maskinlokale absolutte stier er utelatt fra den portable oppsummeringen; råbevisene er bevart privat og hashbundet.
