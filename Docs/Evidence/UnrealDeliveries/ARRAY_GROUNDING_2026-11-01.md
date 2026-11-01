# Antennefundamenter ved SARO

**Status:** Levert lokalt. **Gjelder:** Unreal Linux/Vulkan, kilde `5ea2803d`. **Sist kontrollert:** 1. november 2026.

Toms spillbilder viste et gap under antennenes betongfundamenter. Den opprinnelige native antennemodellen fra23. oktober har underside vedY=0; feltets terreng er gradert tilY=-0.62. Hver av de27 antennene har fire fundamenter.

Rettelsen forlenger fundamentene ned i terrenget med det eksisterende M_Array_Concrete-materialet. Underkant ligger under det graderte terrengplanet, med minst åtte centimeters innstikk. Overkant overlapper opprinnelig betong svakt. Native modell, LOD-er, antennerot, azimut-/elevasjonspivoter, tracking, beacons og serviceankre beholdes. Forlengelsene er108 instanser i én komponent; ingen nye modellkilder eller kjøpte assets.

Editorbygg, pakking,1957 selvtester og målrettet runtime-kontroll av108 fundamentforlengelser består. Det pakkede dagslysbildet er åpnet og visuelt kontrollert. [Toms førbilde](ArrayGrounding-2026-11-01/before.png), [pakket etterbilde](ArrayGrounding-2026-11-01/after.png), [pakket resultat](ArrayGrounding-2026-11-01/packaged.json). Installert kl.20:31 CET fra5ea2803d. Installert Fortsett består; alle158 personlige Saved-elementer er uendret. [Installasjonsresultat](ArrayGrounding-2026-11-01/installed.json). Ingen ny rydderunde; arbeidskopi og byggcache beholdes til gjenbruk/neste samlede kjøring. Lys og felles Story-/Freeform-betjening er egne åpne oppgaver i TODO, ikke del av denne runtime-rettelsen.

Prosjektbibliotekets relevante treff var generelle terreng-/lysreferanser og et three.js bloom-kort. Ingen ekstern kode eller materiale ble tatt inn; eksisterende SaroLand og antennebetong er gjenbruksgrunnlaget.
