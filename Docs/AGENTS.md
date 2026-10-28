# Dokumentregler for SIGNAL / 47

Denne fila gjelder dokumentasjonen under Docs. Rotens prosjektregler gjelder fortsatt, og spill-/byggearbeid følger unreal/AGENTS.md.

## Én kilde per formål

- `Docs/README.md` er inngangen. `Docs/Active/STATUS.md` er eneste samlede leveringsstatus, `TODO.md` eneste aktive hovedarbeidsliste og `DECISIONS.md` gjeldende beslutningsoversikt.
- `unreal/OPPGAVER.md` fører eier, filansvar, kandidater og tunge kjørevinduer. `unreal/log.md` er datert append-only historikk. Ikke kopier leveringsstatus og todo inn i begge eller gjenopprett lange `unreal/memory.md`-/`todo.md`-samlinger.
- Ved en endring oppdateres den hovedfilen som faktisk berøres. README og korte pekere trenger bare endring når navigasjonen endres. Ikke lag en ny datert «gjeldende» fil etter hver chat.

## Klassifisering og plassering

Nye dokumenter skal ha synlig **Status**, **Gjelder** og **Sist kontrollert**. Bruk Gjeldende, Under arbeid, Referanse, Erstattet eller Uavklart. Erstattede dokumenter peker til etterfølgeren; de overstyrer ikke nyere beslutninger.

- Active: samlet status, aktive oppgaver, beslutninger og relevante guider/åpne funn.
- Design: historie, verdenskilder, visuell regi og planlagt innhold. Design er ikke ferdig runtime.
- Research: undersøkelser og ferdigheter med kilde, kompatibilitet og faktisk teststatus.
- Evidence: bilder, kvitteringer og avgrensede leveringsrapporter. Navngi hvilket bygg resultatet gjelder.
- Archive: Unity, web og erstattede planer/statusinnganger. Gamle uboksete oppgaver er uavklart historikk til dagens kode er kontrollert.

Alder alene avgjør ikke klassifiseringen. Ikke slett eller ferdigmeld uavklarte punkter for å få en kortere liste. Planenes gjeldende overstyringsrekkefølge står i DECISIONS.

## Bevar kilder og lenker

Bevar binærfiler, originalbilder, PDF, logger, JSON-kvitteringer og hashbundne snapshots byteidentisk. Ikke legg statusheader inn i et originalbevis; klassifiser gjennom katalog, nærmeste inngang eller sidecar. DesignBible-2026-10-27, LocalDeliveries-2026-10-27, DesignBible13, WorldCase22 og VisualTargets beholder originalstier. Gamle proveniensstier i manifest er kildedata, ikke tekst som skal massesubstitueres.

Ved flytting: oppdater levende dokumentlenker og faktiske verktøystier, før gammel/ny sti i `Archive/document-map.json`, og bevar nødvendige kompatibilitetslenker. Ikke endre Unity-spillkode eller runtime for å rydde docs. Maskinlokale filstier kan være kvitteringsinformasjon, men aktive hoveddokumenter skal bruke portable repo-/nettlenker.

## Kontroll uten motorjobb

Kjør fra repoets rot:

```sh
python3 Tools/docs/check_docs.py --baseline Docs/Archive/link-baseline.json --map-json Docs/Archive/document-map.json
```

Baselinen registrerer kjente historiske mangler fra før ryddingen; nye feil skal rettes. Ikke oppdater baseline bare for å skjule nye feil. `git diff --check`, relevante fil-/hashkontroller og lenkekontroll er riktig omfang for en dokumentleveranse. Ingen Unreal-, Blender- eller spillmatrise kreves av denne rutinen.
