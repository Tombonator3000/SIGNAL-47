# Produksjonssystem med Claude

**Status:** Gjeldende. **Gjelder:** Lokale produksjonsverktøy for SIGNAL / 47,
Python 3.11+ på Linux. **Sist kontrollert:** 29. oktober 2026.

Bruk [produksjonskoordinatorens veiledning](../../../Automation/production/README.md)
for oppsett, oppgavekontrakter, kommandoer og grenser. Systemet samler eksisterende
Blender-/Unreal-verktøy rundt en varig SQLite-kø, lokale kildeoppslag, Claude Code
og hashbundne kontrollkvitteringer. Ingen ny Python-avhengighet eller API-nøkkel
kreves med eksisterende Claude-innlogging.

Claude har bidratt med adapteren og rettelser. Raske regresjoner dekker kø,
filansvar, avhengigheter, forsøk/budsjett, kildeproveniens, ekte prosessfamilier,
reservasjon gjennom prosesspauser, CLI og avvisning av uriktig PASS. Se
[kontrollkvitteringen](../../Evidence/ProductionCoordinator/verification.json).

Den faktiske Claude-proben traff kontoens femtimersgrense. Denne feilen blir
bevart som ventende kvotestatus med backoff. En vellykket normal modelljobb
gjennom worker og en faktisk tung Blender-/Unreal-serie er derfor ikke hevdet
live-verifisert i leveransen. De tilhørende kontrolløypene har syntetiske tester.
Ingen spillinstallasjon, offentlig spillrelease eller gameplay-aksept inngår.

Systemet planlegger og kontrollerer produksjonsoppgaver. Fortsatt spillets
[STATUS](../STATUS.md), [TODO](../TODO.md) og [DECISIONS](../DECISIONS.md) avgjør
hva som faktisk er levert og hvilke spillerregler som gjelder.

Material-/lysarbeid for Blender, Tripo og Unreal har en separat [kjørbar lookdev-pakke](../../../unreal/Signal47/Tools/lookdev/README.md) med kildevern, PBR-baking og egne dag-/nattprofiler. Bruk eksisterende tungjobbvern; en lookdev-prøve er ikke automatisk installert spillinnhold.
