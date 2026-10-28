# Grafikkvalg for svakere maskiner

Tom ba 28. oktober 2026 om flere innstillinger for Unreal-effekter og belastning. Kandidaten legger til **Settings > Graphics**. Picture beholder skjermmodus, oppløsning, bildefrekvensgrense, V-sync, filmfilter, lysstyrke og synsvinkel. Menytekst er engelsk.

## Valgene

Fem startpunkter: Very Low, Low, Medium, High og Ultra. Individuelle endringer vises som Custom. High beholder det tidligere standardutseendet; gamle Low/High/Ultra-innstillinger migreres. En knapp gjenoppretter High uten å endre skjermmodus, filmfilter eller tilgjengelighetsvalg.

| Valg | Faktisk virkning |
| --- | --- |
| Render scale | 50 til 100 prosent av 3D-bildets bredde og høyde; tekst og menyer beholder skjermoppløsningen. |
| Shadows | Av, Low, High, Ultra; skyggegruppe, VSM-prøver og lokale skyggebudsjetter. |
| Indirect lighting | Av eller to kvaliteter av Lumen-lysrefleksjon mellom flater. |
| Reflections | Av, Screen Space eller to Lumen-kvaliteter. |
| Lumen tracing | Software eller Hardware if available; motorens grunnleggende ray-tracing-/plattformstøtte endres ikke. |
| Streamed textures | Fire nivåer for mip-tillatelse og filtrering. Fastoppløste assets påvirkes ikke; dette er ikke en grense for samlet VRAM. |
| Anti-aliasing | Tre kvaliteter av eksisterende temporær rekonstruksjon. |
| Volumetric fog | Av, Standard eller High; vanlig avstandståke beholdes. |
| Volumetric clouds | Av med fotografisk himmelfallback, Standard eller High. Historiens himmelhendelser beholdes. |
| Effect detail | Redusert eller full mengde for støttede gnister og valgfrie lufteffekter. Historietriggere beholdes. |
| Scenery detail | Redusert, Standard eller Dense for eksisterende vegetasjons-/dekorbudsjett. Interaksjoner og historieobjekter fjernes ikke. |

Lumen-refleksjoner uten indirekte Lumen-lys er ikke en bærbar kombinasjon med software tracing. Kandidaten bruker derfor Screen Space når indirekte lys er av, viser **SSR (GI OFF)** og beholder brukerens Lumen-valg til indirekte lys slås på igjen.

Ingen uvirksom bloom-bryter er lagt til: prosjektet har allerede native bloom-intensitet null. Ingen endring av `sg.EffectsQuality`, materialkvalitet eller den globale PSO-policyen. Teksturvalgene bruker mip-tillatelse 2/1/0/0 og anisotrop filtrering 2/4/4/8 uten å endre streamingpoolen. Med motorens eksisterende per-teksturpolicy skjer detaljreduksjon under minnepress.

## Lagring og skjermendringer

`FS47GraphicsSettings` har versjonert, typekontrollert JSON i den eksisterende enhetsfilen `Saved/SaveGames/settings.json`. Manglende eller ugyldige felt faller tilbake til tidligere kvalitetsvalg. Grafikkvalgene er ikke historiefakta og legges ikke i sakslagringer. Et eksplisitt `-S47Quality=` overstyrer bare den aktuelle oppstarten.

Grafikkvalg brukes og lagres med en gang. Skjermoppløsning/-modus beholder Apply og automatisk tilbakerulling. Unreal sin `UGameUserSettings::ApplySettings` skriver også skaleringsgrupper; de individuelle valgene brukes derfor på nytt etter både Apply og rollback.

## Kontroll og status

Arbeidsgren `codex/graphics-settings`, fra main `d5206af5`. Bevismappe: `/home/tom/scratch-bm/Signal47-graphics-settings-20261028`.

Kildearbeidet, editorbygg og Linux-pakking er gjennomført. Faktisk menybilde, ny prosess med innstillingsreload, fullmatrise, fletting og installasjon gjenstår. Ingen FPS-forbedring eller støtte for alle svake GPU-er hevdes. Dette er justeringer innenfor prosjektets eksisterende renderer og plattformkrav.

## Kildegrunnlag

Prosjektbibliotekets eksakte søk ga ingen treff; ett bredere søk fant [scenario-unreal-performance](https://github.com/scenario-labs/skills/blob/f0adb5b5ebc1fc8f7e333554d2aeb767c5558d6b/skills/game-engines/unreal/scenario-unreal-performance/SKILL.md). Dets runtime-skript er merket uprøvd og er ikke installert. Brukt som referanse for å skille faktiske målinger fra ytelsesantakelser.

[Epics skaleringsreferanse](https://dev.epicgames.com/documentation/en-us/unreal-engine/scalability-reference-for-unreal-engine) og lokal UE 5.8.4-kilde er brukt for CVars, prioritet, Lumen-kombinasjoner, skaleringsgrupper og `UGameUserSettings`. Faktiske kontroller i målprosjektet avgjør leveringsstatus.
