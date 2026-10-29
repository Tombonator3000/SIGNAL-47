> **Status: Under arbeid** · Klassifisert 28.10.2026. Gjelder det dokumenterte omfanget; [gjeldende leveringsstatus](../../Active/STATUS.md).

# Niagara: meteorregn ved det stjerneløse feltet

28. oktober 2026. Tom ønsker det tettere meteorregnet ved hendelsen der et punkt på himmelen mangler stjerner. Dette er registrert designretning og kildevurdering for senere produksjon. Ingen ny Niagara-effekt, installasjon eller spilltest inngår i denne leveransen.

## Regi ved stjernehullet

Det korte meteorregnet knyttes til den eksisterende hendelsen ved **azimut 026, elevasjon 32**. Resten av natten beholder de sjeldne stjerneskuddene. En kort opptrapping omkring hendelsen etterfulgt av ro skal gjøre det tomme stjernefeltet lettere å oppdage.

Forslag til første visuelle prøve, ikke fastsatte spillkonstanter:

1. Noen få streker blir til en kort, tettere passasje i nabosektoren til feltet. Bruk smale, raske streker, en liten lys kjerne og en kort hale som dør ut.
2. Passasjen avtar idet stjernehullet begynner å tre frem. La det tomme feltet være lesbart uten flammer, lysring eller partikler som fyller det.
3. Etterpå vender himmelen tilbake til ro og vanlig lav meteorhyppighet. Kontrasten er poenget; effekten skal ikke bli et vedvarende fyrverkeri.

Et utgangspunkt kan være 8 til 12 sekunders passasje med høyst 3 til 6 samtidige meteorhoder, men varighet, lysstyrke og tetthet må vurderes i faktisk nattbilde på High og Low. Regien skal tåle at spilleren ser en annen vei eller er inne. Ingen tvungen kamerabevegelse, ny fotokvote eller automatisk journalobservasjon.

## Eksisterende kode og koblingspunkt

Kontrollert mot `main` ved `3456d029`:

- `Signal47/Source/Signal47/Public/S47Sky.h`: `STAR_HOLE = {26, 32, 3.4, 4.6, 10, 100}`. De siste to verdiene er forsinkelse etter hendelsen og 100 sekunders gradvis fremkomst.
- `S47WorldLife.cpp`: `HoleFrom` settes fra den faktiske historiehendelsens `EventClock`/`EventClockSec()` pluss `STAR_HOLE.After`, først etter `Turning`. En ny effekt skal følge denne hendelsen, ikke en ny hardkodet klokke eller tidspunktet da spilleren oppdager/fotograferer feltet.
- `S47Sky.cpp`: `MeteorNow` og `MeteorStep` gir ett eksisterende materialtegnet stjerneskudd av gangen, med 18 til 40 sekunders intervall om natten. Flere samtidige meteorer krever en egen effekt eller utvidelse; det oppnås ikke bare ved å kalle `MeteorNow` oftere.
- `S47StarHole.cpp`: faktisk blikk/foto, bevis og oppdagelsesflagg er egne mekanismer. De skal fortsatt bygge på det spilleren faktisk har sett.

Ved senere implementering skal engangsfasen beregnes fra lagret hendelsestid og gjeldende spilltid. Pause og åpne paneler skal fryse passasjen slik annen hendelsesregi gjør. Lasting midt i eller etter passasjen skal gjenopprette riktig fase uten ny oppstart, opphopning av emittere eller replay ved dørpassering. Bevar komet, stjernehullets geometri/fade, eksisterende observasjonsflagg og dag-/nattkurver. Ny regissert tetthet er en uttrykkelig utvidelse av den tidligere uavhengige meteorbakgrunnen.

Dette tillegget gjelder meteorregnets plassering i regien. Det flytter ikke SC08, antennenes retur til 026 eller den nyere [SC02 til SC13-revisjonen](../Story/HISTORIE_SC02_SC13_REVISJON.md).

## Implementert som FX-01, 29. oktober 2026

[PR222](https://github.com/Tombonator3000/SIGNAL-47/pull/222) produserer passasjen som en utvidelse av det eksisterende materialtegnede stjerneskuddet (`M_WebMeteor`, samme strimmel og shader), ikke som Niagara-system. Himmelen tegnes i egne himmelkoordinater rundt øyet, og shaderen maskerer allerede stjernehullet og dagslyset. Noen få smale streker trengte derfor ingen ny asset, import eller CPU-/GPU-simulering. Effekten følger `S47Fx::Off()` og forhåndslastes med `S47Fx::Precache` som lysbuen.

- Tid: fra `HoleFrom - 6` (den faktiske hendelsen + 4 s, ett sekund etter at antennene begynner å dreie) i 11 s pluss 0,2 s hale. Siste strek dør ut idet hullet begynner å tre frem; vanlige stjerneskudd holdes borte fra 3 s før til 60 s etter.
- Tetthet: 14 streker på High (høyst 3 samtidig), 9 på Low (høyst 2), styrt av grafikkmenyens Effects som lysbuen. Hver strek starter 9 til 17 grader fra 026 / 32, kommer aldri nærmere enn 9,6 grader (hullets kant er 4,6) og holder seg over 20 grader.
- Engangsavspilling: fasen beregnes fra historieklokka mot hendelsestiden, så pause og åpne paneler fryser den og lasting gjenoppretter den uten opphopning. `meteorPassage.done` lagres med saken. En lagring uten feltet regnes som avspilt når klokka er forbi passasjen. I vanlig spill kan det ikke lagres under hendelsen (`GameSaveBlock`), så neste gjenopptakspunkt er kapittel 1.
- Bevis: `-S47MeteorPassageContract` med `-S47Jump=event` PASS på Low og High; bilder i `~/scratch-bm/Signal47-fx01-20261029/`. Kontrakten stopper vinduskameraet og stiller blikket mot hullet bare i testen. Pakket spill, ekte input og ytelsesmåling er UNVERIFIED.

## Ressursene fra bildene

Alle fire GitHub-kilder var offentlige og ikke arkivert ved kontroll. **UNVERIFIED** betyr her at ingen av dem er importert, kompilert eller kjørt i SIGNAL / 47. Katalogisering er ikke en fungerende integrasjon.

| Kilde | Kontrollert revisjon og versjon | Nytte og begrensning |
|---|---|---|
| [filliperomero/NiagaraVFX](https://github.com/filliperomero/NiagaraVFX/tree/b6de4ad30bb83232eada194e3ad85424bb8273a4) | `b6de4ad30bb83232eada194e3ad85424bb8273a4`, UE 5.3 | Har `Content/Niagara/NiagaraSkills/MeteorAttack/NS_MeteorAttack.uasset`. Et meteorangrep med bakketreff er en visuell referanse, ikke et ferdig himmelregn. Ingen lisens/README funnet; ElectronicNodes er aktivert, og Infinity_Blade/Infinity_VFX/MSPresets finnes i treet. Ikke migrer hele prosjektet eller assets uten avklart opphav. |
| [Packt: Build Stunning Real-time VFX](https://github.com/PacktPublishing/Build-Stunning-Real-time-VFX-with-Unreal-Engine-5/tree/32f190016d07ee3c344050037ac1f23a3e08c7c0) | `32f190016d07ee3c344050037ac1f23a3e08c7c0`, UE 5.1 | God læringsreferanse for `EventDispatcher`, Blueprint-kontroll, Scratch Modules og emitterkommunikasjon. Repository-lisensen er MIT, copyright Packt 2023. Dette gjør ikke boka eller alle medfølgende tredjepartsassets MIT. Prøv selektiv migrering i en separat UE 5.8-kopi før gjenbruk. |
| [ue-vfx-niagara-profiler](https://github.com/buihuuloc/universal-ue-skills/blob/28c12a50ee8dc3b5ff2da91e7fba7a9b55a31347/skills/ue-vfx-niagara-profiler/SKILL.md), vist på [skills.rest](https://skills.rest/skill/ue-vfx-niagara-profiler) | `buihuuloc/universal-ue-skills@28c12a50ee8dc3b5ff2da91e7fba7a9b55a31347`, ingen eksplisitt motorversjon i skillens metadata | En Markdown-sjekkliste for partikkeltall, overdraw, minne og måling, ikke en egen profiler. Budsjettallene er ikke målte SIGNAL 47-krav. Huli/S2-stier og `FNiagaraAnalyzer::AnalyzeFolder` er ikke bekreftet som standard UE-verktøy. Ingen lisens for denne skillen funnet. |
| [kevinpbuckley/unreal-engine-skills](https://github.com/kevinpbuckley/unreal-engine-skills/tree/b22b11d0a46bac2d55eae8d3273d2ef22c08a95d) | `b22b11d0a46bac2d55eae8d3273d2ef22c08a95d`, UE 5.8 | Mest direkte relevant fagreferanse: `ue-niagara-vfx`, `ue-niagara-sequenced-effects`, renderers/materials og profiling/optimization under `skills/core/`. Handler om C++, User Parameters, levetid og tidsstyrte effekter. Ingen lisensfil funnet; ikke anta lisensen til VibeUE eller andre skillrepoer. APIer må kontrolleres mot lokal UE 5.8.4. |

## CodeLikeMe og teknisk avgrensning

[Meteor Shower Particle Effect with Niagara](https://www.youtube.com/watch?v=3Rpi-ky2fTI), publisert 26. mars 2022, er den målrettede oppskriften fra det første bildet. Videobeskrivelsen bekrefter to emittere: sprite for meteorhodet og ribbon for halen, koblet med Location Events. Beskrivelse og publiseringsmetadata er lest; full video og prosjektfiler er ikke gjennomgått. Prosjektfilene omtales som Patreon-materiale, uten avklart gjenbrukslisens her.

Den klassiske [Location Event-mekanismen](https://dev.epicgames.com/documentation/unreal-engine/events-and-event-handlers-in-niagara-effects-for-unreal-engine) krever CPU-simulering og Persistent IDs. Det er en egnet første kandidat for noen få meteorhoder. **Ribbons generelt krever ikke CPU**: nyere Unreal har også GPU-ribbons, dokumentert blant annet i [UE-189522](https://issues.unrealengine.com/issue/UE-189522). Profiler-skillens generelle CPU/ribbon-regel skal derfor ikke overtas. Skill også GPU-simulering fra GPU-generering av ribbon-geometri.

Før produksjon: velg én smal hode-/haleeffekt med egne materialer og teksturer, bind den til eksisterende hendelsestid, og mål faktisk CPU/GPU-tid og transparens mot et sammenlignbart bilde uten passasjen. Bruk [Epics Niagara-veiledning om skalering](https://dev.epicgames.com/documentation/unreal-engine/scalability-and-best-practices-for-niagara) og prosjektets faktiske målmaskin fremfor generiske partikkelbudsjetter. Kontroller High/Low, okklusjon, pause, foto, lasting før/midt/etter, ingen gjentakelse og fortsatt lesbart stjernehull. Ordinær bygge-/pakkekontroll og full spillmatrise gjelder når effekten implementeres.

## Bibliotek og kontrollstatus

Prosjektbiblioteket ble søkt før vurderingen. Profilen `quodsoler/unreal-engine-skills` fra forrige gjennomgang og de eksisterende Niagara-/MCP-treffene er kun referanser; ingen kode eller tredjepartskill er tatt i bruk. Denne gjennomgangen registrerer de fire nye kildene, profilerens katalogside og CodeLikeMe-videoen i prosjektbiblioteket med festede GitHub-revisjoner, norske behovsord og lisens-/testforbehold. Tekstkilder er bevart som referanser, og ingen tredjepartsassets eller installasjon følger med spillrepoet.

PASS: kildeidentitet, tekst-/metadatagjennomgang og dokumentlenker. UNVERIFIED: native Niagara-grafer, UE 5.8-import, nytt meteorregn, visuell kvalitet og ytelse i spillet. Spillkode og installert spill er uendret.
