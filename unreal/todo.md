# Todo for unreal/

## Prøven av prologen

- [x] Prosjektet bygger, modellene er importert, og den pakkede builden starter med kontrollrommet, gården og himmelen (2026-10-21).
- [x] Alle fjorten testkjøringene gjennom med verdenen fra nettversjonen (2026-10-21).
- [ ] Tom går rundt i rommet og ut på gården og sier om prøven holder: lys, stemning, modellenes skala og retning.
- [x] VT323 og Special Elite som i nettversjonen (2026-10-21).
- [x] Verdenen hentes fra nettversjonen (Tools/webexport), så modellene står som der (2026-10-21).
- [ ] Bildet sammenlignet med nettversjonen fra de samme stedene: rommet, gården, antennene, vinduet, racket.

## Hvis prøven holder (hele konverteringen)

- [ ] Prologen spilt av et menneske fra 23:41 til sluttkortet.
- [ ] Historien som JSON felles for web og Unreal, slik at tekst og valg ikke må skrives to steder.
- [ ] Feltkameraet og fotolaben.
- [x] Lyd og musikk fra web/public (2026-10-21).
- [x] Lagring ved kontrollpunkter, tittel, pause og notatbok (2026-10-22, `S47Saves`, `S47Flow`).
- [ ] Kapittel 1 til 6 (web/src/story), kjøringen (web/src/drive) og områdene. Verdenene er med i eksporten (AS47Web::Enter).
- [ ] Glødepunktene blinker som i glow.ts.

## Etter lagring og flyt

- [ ] Kjør screenshots.sh med den pakkede builden (title, pause) og se tittelkameraet over gården og miniatyrbildene i lagringene med verdenen på plass.
- [ ] Saksbrettets oppsett i lagringen: sett `M->Flow->SaveBoard` og `LoadBoard` til CaseBoard sin Save()/Load().
- [ ] Feltkameraet og rullen i Kit-fanen: sett `M->Flow->HaveCamera` og `RollText` når kapittel 1 og albumet er portert.
- [ ] Musikk- og radionivået (Music, Radio) inn i lydmodulen når den har egne busser; volumet virker allerede.
- [ ] Worn VHS (bildet `heavy`) i M_WebTone; nå bare Clean og VHS.
- [ ] Fortelleren (Serling) til åpningskortet og kapittelkortet når stemmene er portert.
