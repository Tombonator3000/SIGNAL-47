# Tools/fx: lysbue og gnister ved Caliche-trafoen

Skriptene bygger egne materialer og ett CPU-Niagara-system under `/Game/Signal47/FX`. Kildegrunnlaget er Claudes uferdige `Tools/fx` i grenen `claude/niagara-sparks`. Den opprinnelige arbeidskopien er bevart. Her er ballistikk, én sprett, lysbuemateriale og strengere tilbakekontroll lagt til.

```sh
bash unreal/Signal47/Tools/fx/import.sh
S47_FX_VERIFY_ONLY=1 bash unreal/Signal47/Tools/fx/import.sh  # kun eksisterende assets
S47_FX_MATERIALS_ONLY=1 bash unreal/Signal47/Tools/fx/import.sh  # kun de to materialene
```

Importen bruker UE 5.8.4s `NiagaraToolsets` og prosjektets `unreal_guard.py`. Rapporten skrives til `S47_FX_WORK` (standard `~/scratch-bm/Signal47-fx`). En kontrollfeil, uferdig kompilering eller manglende rapport gir feilstatus. Native import og visuell kontroll må gjennomføres før effekten regnes som verifisert.

## Originale materialer

- `M_S47_FxSpark`: unlit/additiv, rund UV-maske, partikkelfarge, alfa og levetidsfalming. Ingen tekstur. Bruksflagget for Niagara-sprites er satt.
- `M_S47_FxArc`: unlit/additivt meshmateriale med `ArcColor` og `ArcIntensity`, samt flagg for instanced static meshes. Ingen partikkel-, tids- eller teksturnoder. C++ bygger den forgrenede blåhvite lysbuen og styrer intensiteten.

Systemet lages uten malsystem. Emitteren starter fra Niagaras `Minimal`; motorens moduler brukes ved referanse med egne verdier. Ingen ferdig Epic-effekt eller innkjøpt asset kopieres.

## Gnistenes bane

`arc_trajectory.py` lager egne SetParameters-uttrykk: lagre startposisjon/-hastighet, beregn første kontakt med grusplanet analytisk, beregn én dempet sprett, og avslutt ved andre kontakt. Ingen kollisjonsmodul, scene query, GPU-simulering eller force-solver brukes.

Koordinatene er lokale centimeter. Komponenten står stille ved isolatoren, med identisk verdensrotasjon gjennom hele partikkelhalen. `User.GroundZ = -777` angir grusen 7,77 m under isolatoren. `User.Direction` er en lokal vektor, som tilsvarer verdensretningen når komponentrotasjonen er identisk. Det skal ikke legges til `Engine.Owner.Position` i grusplanet.

1100–1500 cm/s, kjegle på 18 grader og sprettfaktor 0,14 gir fall og sprett innen levetiden på 1,3 s ved den avtalte retningen. Tangentiell hastighet reduseres til 35 % ved støtet. Tilfeldigheten er deterministisk. C++ velger 20 gnister på Low / 40 på High, færre ved CalmFlash. Lysbuens 1,2 s konvolutt og gnistenes naturlige hale er separate.

## Kontroll og C++-kontrakt

`verify_fx.py` leser tilbake materialegenskaper, konkrete HLSL-uttrykk i assignment-modulene, modulrekkefølge, levetid, kjegle, User-parametre, bounds, renderer og kompileringsresultater. Pending, stale, ukjent status eller compile-feil gir aldri PASS. `UNiagaraSystem::PreSave` venter på kompilering. Verifikatoren tvinger derfor en lagring av det eksisterende systemet (også når pakken ikke er dirty) før kompileringsstatus leses. Dette gjelder også kontroll uten regenerering: grafen endres ikke, men pakken lagres for å fullføre eventuell kompilering fra innlasting. Den etterfølgende statuslesingen må faktisk være ferdig.

`S47SubstationArc.cpp` setter `User.SparkCount`, `User.SparkColor`, `User.Direction` og `User.GroundZ`, frøoffset og starter systemet. Runtime slår av ray tracing, skygger, kollisjon og distansefeltlys, og avbryter effekten ved relevante gjenopprettinger/sceneoverganger. `-S47FxOff` deaktiverer effekten. Hendelses- og lagringsregler eies av SC02-integrasjonen i C++.

Materialmodus bygger materialgrafene på eksisterende asset-objekter, slik at Niagara-referansen bevares. Niagara-systemet lastes, regenereres eller lagres ikke i denne modusen. Spark-alfa kobles direkte fra `ParticleColor`-utgangen `A`; standardutgangen er RGB og kan ikke alfa-maskeres. Materialkontroll i NullRHI er kun graf-/egenskapskontroll. Vulkan-shaderkompilering må bekreftes av cook/pakking.
