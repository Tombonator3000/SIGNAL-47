# Dales stemme ved besøksinngangen

**Status:** Gjeldende for den navngitte kilden. **Gjelder:** Unreal-kilde og én målrettet editor/game-kontroll på Linux. **Sist kontrollert:** 29. oktober 2026.

Når de fire observasjonene er arkivert, høres Dales første telefonreplikk fra utsiden av den lukkede besøksinngangen. Døren åpner mot en tom trapp, og hendelsen føres i notatboken. Teksten og fremdriften beholdes når Character voices er slått av. Hendelsen lagres som doorVoice0/1/2 og hint64.

Eksisterende opptak D01_HANDOVER_001 brukes på fysisk lydrute. Ingen ny take eller lisensiert binærfil er lagt til repoet. Stop og Begin rydder køen ved avslutning eller bytte av sak. Den vanlige talebryteren avbryter også denne køen mens lyden er suspendert; det å slå tale på igjen spiller ikke hendelsen om igjen.

## Kontrollgrunnlag

Kilden er 7a29935e796d6b036692a53d9f2b5712993ccf17, med main a9db4175 integrert. Hvert spor i Source og Config er hashbundet før og etter bygg/kjøring; modulhash kontrolleres ved launch og exit. Kjørevinduet holdes av én HeavyReservation over bygg, opphold og test. Bibliotekssøket ga generelle EngineWire/Epic MCP-referanser med uverifisert kompatibilitet. Ingen bibliotekskode er gjenbrukt; eksisterende build-editor, guard og SC01-forløp brukes.

Editorbygg PASS. 1709 selvtester og SC01 PASS med 185 kontroller, 19 disk/Flow-restores og 273.08m native gange. Sluttkontrollen er targeted-voice-5; de første tilstandsjekkene alene var utilstrekkelige til å bevise posisjon. R3 ble derfor avvist og oppslaget rettet fra bare Inter.Get til også Web.Item/ObjAt. Den godkjente målingen har 108544 kildesamples og 217088 romlige stereosamples, RMS0.025464, via SfxBus. Katalogalias D61_FIXED_FRAGMENT_001 peker til samme D01_HANDOVER_001.wav, bekreftet med identisk takehash. [Portabel kvittering](DOOR_VOICE_2026-10-29.json). Råbevis ligger lokalt under /home/tom/scratch-bm/Signal47-claude-handoff-20261029/targeted-voice-5. Første forsøk ble avvist av SC01 fordi profilen lå utenfor /tmp. Andre forsøk startet ingen tungjobb fordi annen diagnostikk brukte maskinen. Begge er bevart; de er ikke akseptgrunnlaget for sluttkilden.

## Avgrensning

SC01 bruker faktiske produksjonsinteraksjoner, kameramål og native gangkontroll. Stage25 har i tillegg uttrykkelig isolerte livssyklusfixtures for talebryter, Stop og Begin(saved), med opprinnelig hendelsesstate og talevalg gjenopprettet før ordinær videreføring. Disse er ikke en menneskelig meny-/inputprøve eller en separat prosessrestart.

SDL dummybackend brukes. Målt PCM gjennom den fysiske ruten er teknisk lydbevis, ikke fysisk høyttalerlytting eller subjektiv lydaksept. Ingen ny pakking, fullmatrise, installasjon eller offentlig spillutgave inngår. Installasjonen og personlige lagringer beholdes.

Det nye bildet door-voice-corridor.png er faktisk åpnet og vurdert i1280x720. Besøksdøren, vanlig interaksjonsprompt og hele Dales tekst er synlige og lesbare. Andre bildehash i kvitteringen er integritetskontroll, ikke en påstand om ny visuell gjennomgang av alle bildene.
