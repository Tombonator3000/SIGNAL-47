# Dørstudie for Signal 47

Selvstendig C++17-referansemodell uten Unreal-avhengighet. Den er ikke hentet fra Greyhills kildekode og er ikke integrert i spillets Source.

`DoorPolicy.hpp` har lokal eierskap, faseoverganger, gyldige animasjons-tickets, stabil checkpoint-policy og kontrollert lagringsmelding. `DoorPolicyTests.cpp` har 16 scenarioer. Modellen eier ikke navigasjon, syn, geometri, lyd eller disk-I/O.

MSVC 14.44 kompilering og linking med `/std:c++17 /EHsc /W4 /WX` består. Kjøring er UNVERIFIED: Windows nektet å starte det native testprogrammet med «Ingen tilgang». Ingen omgåelse er forsøkt. Det er 0 kjørte testscenarioer.

I en konfigurert C++17-kompilatorterminal kan testprogrammet bygges med `cl /std:c++17 /EHsc /W4 /WX DoorPolicyTests.cpp` og deretter kjøres som `DoorPolicyTests.exe`. Dette er kilde for senere testing, ikke instruksjon om å omgå maskinens kjøreregler.

Varsel-/timeout-verdiene er eksempelverdier for denne modellen, ikke målte tall fra Greyhill eller Signal 47. `MotionComplete` må få ticket fra den konkrete animasjonshendelsen. Hosten må beholde completion over pause og sette mesh/kollisjon ved restore før aktører slippes løs.

## Kjørt på Linux, 28. oktober 2026

Claude bygget og kjørte testene på Kubuntu med clang 20.1.8 fra Unreal 5.8.4 sin Linux-verktøykjede og medfølgende libc++ (`-std=c++17 -Wall -Wextra -Werror`, statisk lenket). Alle 16 scenarioer består. Modellen er fortsatt ikke integrert i spillet; den er tenkt brukt for dørene i SC04 og SC11.

```sh
C=~/.local/opt/UnrealEngine-5.8.4/Engine/Extras/ThirdPartyNotUE/SDKs/HostLinux/Linux_x64/v26_clang-20.1.8-rockylinux8/x86_64-unknown-linux-gnu
$C/bin/clang++ -std=c++17 -Wall -Wextra -Werror -O1 --sysroot=$C -nostdinc++ -isystem $C/include/c++/v1 -L$C/lib64 -static -o /tmp/DoorPolicyTests DoorPolicyTests.cpp -lc++ -lc++abi -lpthread && /tmp/DoorPolicyTests
```
