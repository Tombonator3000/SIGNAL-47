# Xbox-styring i Unreal

Arbeidsgren `codex/xbox-controller`, 24. oktober 2026. Tom bestiller styring med Xbox-kontroller, commit og push. Eksisterende tastaturbindinger beholdes separat. Kilde er implementert; bygg, pakket kontroll og installasjon venter på at det kjørende spillet avsluttes. Ikke runtime-PASS ennå.

| Handling | Xbox |
|---|---|
| Gange / styre bilen | Venstrestikk |
| Se rundt | Høyrestikk |
| Bruk, start nøkkel, gå inn/ut | A |
| Andre valg / tilbake i paneler | B |
| Feltkamera | X |
| Hopp | Y |
| Løp, hold | Klikk venstrestikk |
| Nærmere blikk / kameraets lukker | LT / RT |
| Gass / brems og revers i bil | RT / LT |
| Lykt / bilradio | D-pad opp / ned |
| Kart / les beskrivelse | D-pad venstre / høyre |
| Notatbok / saksbrett | View eller LB / RB |
| Pause | Menu |
| Menyvalg og bekreftelse | D-pad eller venstrestikk, A |
| Sider og faner | LB / RB |
| Peker i paneler, klikk og dra | Høyrestikk, hold A |
| Rulle paneler under pekeren | LT / RT |

Stikkene har radial dødsone på 18 prosent, og ganghastigheten er proporsjonal. Høyrestikkets blikk bruker sekunder og en responskurve for presis sikting. Kontrollerhastighet og invertert Y lagres separat fra musen i settings.json. Knappetekst følger siste brukte enhet. Frakobling og tapt vindusfokus nullstiller input. Overgangen fra panel til spill krever at holdte knapper slippes og aktive stikker/triggere returnerer til nøytral. Avbrutt pekerdrag kanselleres før syntetisk museknapp slippes.

Linux på denne maskinen viser et virtuelt Sunshine Xbox Series-endepunkt. Testharnessen oppretter en separat, midlertidig Xbox 360-enhet gjennom uinput og endrer ikke Sunshine-enheten. Den nøytrale enhetstesten består; dette er foreløpig ikke bevis for kernel til SDL til Unreal eller en fysisk Xbox-kontroller.

`-S47GamepadLog` logger faktisk bevegelse, kamera og inputtilstand. Den isolerte plattformtesten bruker `-RenderOffscreen -S47GamepadBackgroundTest`, prosesslokal SDL-filtrering og egen UserDir. Testunntaket tillater bakgrunnsinput kun med begge flaggene; pekeren i denne testen rutes logisk uten å flytte skrivebordsmusen. Vanlig spill krever aktivt vindu.

Bevis og testharness ligger i `~/scratch-bm/Signal47-xbox`. Fysisk kontrollerergonomi, klienttransport, vibrasjon og Windows/XInput er UNVERIFIED. Ingen standardinstallasjon endret av dette arbeidet ennå.
