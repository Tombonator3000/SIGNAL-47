# SIGNAL / 47 - designbibel v0.1

**Komplett førsteutkast til «Den slettede natten».** Ramme for et avgrenset 5–6 timers spill: tre steder, seks kapitler og epilog, 18 hovedoppgaver og to avslutninger. Spilletid er et mål som må prøves; nye historiedetaljer er forslag, ikke allerede godkjent kanon. Inneholder hele avsløringen.

## Les og bruk

- [Samlet PDF, med kart, konseptbilder og UI](output/pdf/SIGNAL47-Designbibel-v0.1.pdf)
- [Redigerbar designbibel](design-bible.md): hele historien, regler, personer, kapittelløp, bevis, UI/save/load, ressursbudsjett, roadmap og testporter.
- [Tekstgrunnlag for hovedsporene](story-material.md): ti dokumentutkast og sju grupper med avgjørende replikkutkast, på engelsk som nåværende spill.
- [Dokumentkontroll og avgrensning](Sources/design-review.json)

De viktigste forslagene å gjennomgå er trestedsrammen, Nora/Tomás-forløpet og den ulike kostnaden i de to avslutningene. Deretter kan historien fryses før nytt gameplay produseres. Første nye oppgave er papirprøven med original og korrigert protokoll, fulgt av trygg lagring/områdeovergang og en liten spillbar arkivsekvens.

## Kart

| Fil | Bruk |
| --- | --- |
| [Verdensruten](Visuals/map-01-world.svg) | SARO → målestasjon → motell → SARO |
| [SARO](Visuals/map-02-saro.svg) | Bevar fungerende kjerne; koble til ett arkivrom |
| [Målestasjon og motell](Visuals/map-03-local-areas.svg) | Små gangsløyfer og seks interiører totalt i spillet |
| [Oppgaveflyt](Visuals/map-04-evidence-flow.svg) | 18 oppgaver, lokale rekkefølgevalg og tidsbudsjett |

Kartene er redigerbare topologiske designskisser. De er ikke oppmålt geografi eller fasit for nåværende Unity-koordinater. Piler viser foreslått undersøkelsesflyt; vanlige gangforbindelser er ikke enveiskjørte.

## Konseptbilder

| Fil | Hensikt |
| --- | --- |
| [Målestasjonen](Visuals/concept-01-survey-station.png) | Fysisk rekonstruksjon, isolasjon og en lesbar siktelinje |
| [Nora i rom 6](Visuals/concept-02-nora-motel.png) | Et menneskelig møte rundt konkrete bevis |
| [Siste avlesning](Visuals/concept-03-final-reading.png) | Kjent SARO, fysisk protokollvalg og analogt kamera |

Alle tre er generert med det innebygde image_gen-verktøyet og inspisert som konsepter. En strålelignende himmeleffekt og et for moderne kamera i første finaleutkast ble rettet. De er ikke faktisk spillgrafikk, forhåndsgodkjent figurkvalitet eller spillerens fotografiske bevis. [Prompter](Sources/image-prompts.json) og [filidentitet](Sources/concept-files.json) følger med. Referansene fra konsept11 finnes i den eldre lokale Git-historikken; de er ikke en avhengighet for å åpne denne pakken.

## UI-skisser

| Fil | Tilstand |
| --- | --- |
| [Startmeny](Visuals/ui-01-start.svg) | Eksisterende sak og tydelig Fortsett |
| [Last sak](Visuals/ui-02-load.svg) | Tre saker og valg av checkpoint |
| [Pause / lagre](Visuals/ui-03-save.svg) | Tre manuelle plasser, lagring og retur |
| [Foto / saksmappe](Visuals/ui-04-evidence.svg) | Sammenligning uten forhåndsmarkert løsning |
| [Innstillinger](Visuals/ui-05-settings.svg) | Tekst, teksting og bevegelse |
| [Gjenoppretting](Visuals/ui-06-load-recovery.svg) | Lagringsfeil med tydelig valg av bevart backup |

UI-tekst er engelsk, som i dagens bygg; dokumentasjonen er norsk. Dette er redigerbare vektorskisser, ikke interaktive Unity-menyer. Saksdata og fotoflatene er illustrasjoner. Produktflyten, feiltilfellene og bevaring av originaler står i designbibelens del 11–14.

## Kilder og gjenbygging

Baseline er `main` på `f156a9f2f656d168356145dabbe0bc8fb4ef0ec9`, etter bekreftet merge av PR8. Unity-koden og spillpakkene er ikke endret av dette passet. Forslaget ligger på egen lokal gren `design/bible-13`; det er ikke pushet eller publisert som ny PR.

`Sources/build_visuals.py` lager de ti SVG-ene. `Sources/build_book.py` lager den samlede PDF-en fra Markdown, bilder og de samme vektortegningene. Bygget her bruker det tilgjengelige Python-miljøet med ReportLab og Pillow samt installerte DejaVu-fonter; PDF-kontroll bruker Poppler/pdfplumber. Designfilene kan leses uten å installere disse verktøyene. Renderte QA-sider ligger lokalt under `Artifacts/Design13/` og er ikke spillbevis.

Den leverte PDF-en har 43 sider. Alle sidene er rendret og visuelt gjennomgått; berørte skisser og tekstsider er også kontrollert i større visning. `Sources/check_package.py` kontrollerer oppgave-ID-er, foreslått avhengighetsmodell, tidsbudsjett, lokale lenker, bildefiler, vektorgrenser, PDF-geometri og at Unity/standardstarteren er uendret. Dette dokumenterer designpakken, ikke fungerende menyer eller målt spilletid. Filidentitetene er samlet i `Sources/package-manifest.json`.

Nøyaktig kilde-/referansestatus ligger i [source-status.json](Sources/source-status.json). Gjeldende spillstatus og neste avgrensede arbeid vedlikeholdes i [CURRENT_HANDOFF](../CURRENT_HANDOFF.md). Historiske og nye lisenser i [THIRD_PARTY_NOTICES](../THIRD_PARTY_NOTICES.md) skal bevares ved faktisk ressursproduksjon.
