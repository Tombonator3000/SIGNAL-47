# Freeform: nattkurve, gjetning og koblede hendelser

**Status:** Under arbeid. Punkt 1 og 2 er implementert 4. november; resten er design. **Gjelder:** Freeform. **Sist kontrollert:** 4. november 2026.

Tom bestilte en forbedring av gameplay-løkken og hendelsene. Dette bygger på [saksløkken](FREEFORM_CASE_LOOP.md) og regelen om at kvelden først er en vanlig jobb.

## 1. Nattkurve (implementert)

Director lager én plan per spilldag i stedet for uavhengige trekk (`FS47FreeformDirector::PhaseAt`, `PhaseAllows`, `IntervalFor`).

| Fase | Klokke | Tillatt |
| --- | --- | --- |
| Dag | 06–19 | Alt unntatt nærvær og overflyvninger |
| Kveld | 19–01:30 | Instrument, vedlikehold, små hint som kan forklares bort (takskritt). Ingen nærvær, spor eller trusler. |
| Topp | 01:30–04:30 | Alt, også trussel (`service_presence`) og hovedhendelsen (`close_pass`) |
| Gry | 04:30–06 | Instrument, vedlikehold og spor etter natta (landingsspor, tom bil, lys som er tent) |

Nattplan per dag (dag 1 alltid vanlig): **vanlig** 50 %, **rolig** 25 % (bare instrument og vedlikehold, ingen trussel eller hovedhendelse, lengre pauser) og **stigende** 25 % (minst to undersøkelseshendelser, tettere topp). Planen lagres (`nightPlan`) og logges som `S47: Freeform night plan day N`. Tvungne hendelser i tester omgår planen.

## 2. Gjetning før forsøk (implementert)

På saksbrettet planlegger spilleren et forsøk ved å velge hvilken forklaring hen mistenker («Plan: Generator power / I suspect Feeder»). Resultatet viser «YOUR HUNCH / …: expected … The result KEPT / RULED OUT your hunch.» på brettet og i tilbakemeldingen. Lagres som `plannedHunch` per sak og `hunch` per resultat. Gamle lagringer uten gjetning virker som før. Utviklervisning: `-S47Freeform -S47CaseBoardShot` (temp-profil) tar bilde før og etter.

## 3. Feltforsøk i verden for sak 3 (implementert)

`FS47CaseFieldTests` gjør sak 3 («Visitors around the base») mulig å løse. Før dette fantes forsøkene bare i saksmotoren.

- **Sikre takbeslaget:** stige ved kontrollrommets nordfasade. Fest platen, og lytt så inne i kontrollrommet ett minutt i neste nattopp (01:30–04:30, minst to timer etter festing). Er den skjulte årsaken ikke vind, høres fire slag over taket likevel. Ingen lytter i vinduet gir et ugyldig forsøk.
- **Kontrollert adkomst:** forsegl porten til antennefeltet med tape, og legg sporstriper av sand. Etter neste nattopp kan du inspisere dem: tapen er revet og fotspor krysser stripene, eller alt er urørt.
- Forberedelsen lagres som funn (`<jobb>-prep-<klokke>`), så den overlever Lagre/Fortsett.
- Forsøk 3 (to samtidige målesteder) trenger feltloggeren og er ikke iscenesatt ennå.
- Utviklerprøve: `-S47Freeform -S47CaseFieldTest` (temp-profil). Den går gjennom begge forsøkene via de ekte handlingene og tar bilder i `Saved/CaseFieldShots`.

## 4–7. Videre (design)

4. **Håndarbeid på benkene:** bytt relé og les av RT-12; mikroskop mot referanse; utlånslogg i delelageret der noe kan mangle.
5. **Verden reagerer:** åpen rulleport gir sand og spor; Jeepen står flyttet; gjenstander ligger annerledes. Mønster: `roomlight_return`. Foreslått eier: Codex.
6. **Hverdagslig tvilling** for hver uhyggelig hendelse (løst takbeslag, glemt bil).
7. **Mer på spill:** slitasje, drivstoff, strømbrudd som ødelegger forsøk, tillit hos ledelsen.

## Kontroll

Selvtest 2321; `-S47FreeformTest` 247/0; `-S47CaseFieldTest` 11/0; `-S47EastWingDriveTest` 24/0.
