# Minne for unreal/

Beslutninger og fallgruver. Dagbok hører hjemme i log.md.

## Oppsett (2026-10-21)

- Prosjektet heter Signal47 og ligger i unreal/Signal47. UE 5.8.4 (binærutgaven for Linux) i ~/.local/opt/UnrealEngine-5.8.4, Vulkan SM6, Lumen i programvare, Nanite av for de importerte modellene.
- Pakkede bygg legges utenfor repoet, i ~/scratch-bm/Signal47-build, så Artifacts-regelen i rotens .gitignore ikke berøres.
- Akser: X nord, Y øst, Z opp, cm. Nettversjonens (x, y, z) i meter er (Y, Z, -X) ganger 100. Vinduet i nettversjonen ligger ved z -4.65 (nord), altså X +465 her.
- Rommet er 1200 (Y) x 900 (X) x 300 cm, vindu fra Z 95 til 275 langs hele nordveggen med sprosser hver 180 cm, dør mot sør ved Y -290 til -190 og dør mot øst ved X -210 til -110.
- Modellene importeres med det gamle FBX-importeren (FbxImportUI), uten materialer; hvert materialspor får en MaterialInstanceConstant av M_S47Flat med fargen fra Unity-materialfilen med samme navn (_BaseColor, 1 - _Smoothness, _Metallic, _EmissionColor).
- Fonten VT323 finnes bare som woff2 i web/. Konvertering krever fontTools med brotli, som ikke er installert. Reserve: JetBrains Mono fra THE BLACK MAP (OFL).
- Stjernehimmelen er en tekstur laget ved oppstart (UTexture2D::CreateTransient) på innsiden av en kule med skala 400; fiktiv himmel uten katalog.
- Eksponeringen er manuell (bias 0, fysisk kamera av) så natta blir mørk med vilje. Lamper i candela: taklys 7 til 9, skrivebordslampe 3, gårdslampe 14. Månen er et retningslys på 0,9 lux.

## Prologen (2026-10-21)

- AS47Story er en direkte port av Prologue.ts. Spilltid (GameTime) stopper mens et papir, konsollen, terminalen eller notatboken er åpen (HUD-en setter Story->SetPaused). Tidsplanen (After) går på spilltid.
- Historien når tingene i rommet bare gjennom AS47Saro sine settere og PropLocation(id). Nye ting får Tag(actor, id) og en linje i Label() og Use() i S47Story.
- De fire CRT-ene er render targets på 512 x 384 som HUD-en tegner tolv ganger i sekundet med FWidgetRenderer (SS47Crt). Skjermflaten er en tynn boks med M_S47Screen (Glow 0 når skjermen er av).
- Lyder slås opp ved navn i /Game/Licensed/Sfx, så /Game/Audio/Sfx, så /Game/Audio/Synth. Mangler en lyd, logges «no sound named» én gang og spillet går videre.
- De kjøpte glb-modellene importeres av Interchange til /Game/Licensed/Models/<pakke>/StaticMeshes/<mesh> (crt_terminal har crt_terminal og crt_keyboard).
- Et medlem og en konstant kan ikke hete det samme (RelayFirst): clang tar medlemsfunksjonen.

## Lagring og flyt (2026-10-22)

- Lagringene ligger i Saved/SaveGames: save-<sak>-<auto|manual>-<plass>.json med kortets felt og `state` (GameState v2 som i nettversjonen), bildene i photos/<nøkkel>.txt og referert som `idb:<nøkkel>` som i nettversjonen, active.json er saken som ble spilt sist, settings.json er innstillingene (main.ts store).
- Det som ennå ikke er portert (bilen, lommelykten, bortføringen, albumet, kurven, Caliche, ns.ln, ns.array, ns.hondo, kapitler uten C++-port) leses fra lagringen som åpnes og skrives tilbake uendret (`US47Flow::Carry`).
- Pausemenyen stopper verden med SetGamePaused, og lyden pauses med den. Verdenens tidtakere står da stille, så røyktestens tidtakere går på FTSTicker.
- Ethvert -S47-flagg unntatt -S47Picture og -S47Title starter rett i spill som før; uten flagg starter spillet på tittelen.
- I UE 5.8 har FJsonObject::Values nøkler av typen UE::TSharedString: `const auto&` i løkker, `FString(*Key)`.
- AActor har en virtuell Reset(); `AS47Story::Reset` er Prologue.reset og overstyrer den.
- Uten den genererte verdenen kan prosjektet prøves med UnrealEditor <uproject> -game -RenderOffscreen (tom verden): menyene, notatboken og lagringen virker.
