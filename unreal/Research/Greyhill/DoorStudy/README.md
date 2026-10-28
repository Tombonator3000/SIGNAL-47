# Dørstudie for Signal 47

Selvstendig C++17-referansemodell uten Unreal-avhengighet. Den er ikke hentet fra Greyhills kildekode og er ikke integrert i spillets Source.

`DoorPolicy.hpp` har lokal eierskap, faseoverganger, gyldige animasjons-tickets, stabil checkpoint-policy og kontrollert lagringsmelding. `DoorPolicyTests.cpp` har 16 scenarioer. Modellen eier ikke navigasjon, syn, geometri, lyd eller disk-I/O.

MSVC 14.44 kompilering og linking med `/std:c++17 /EHsc /W4 /WX` består. Kjøring er UNVERIFIED: Windows nektet å starte det native testprogrammet med «Ingen tilgang». Ingen omgåelse er forsøkt. Det er 0 kjørte testscenarioer.

I en konfigurert C++17-kompilatorterminal kan testprogrammet bygges med `cl /std:c++17 /EHsc /W4 /WX DoorPolicyTests.cpp` og deretter kjøres som `DoorPolicyTests.exe`. Dette er kilde for senere testing, ikke instruksjon om å omgå maskinens kjøreregler.

Varsel-/timeout-verdiene er eksempelverdier for denne modellen, ikke målte tall fra Greyhill eller Signal 47. `MotionComplete` må få ticket fra den konkrete animasjonshendelsen. Hosten må beholde completion over pause og sette mesh/kollisjon ved restore før aktører slippes løs.
