# Freeform-start fra hovedmenyen

**Status:** Levert lokalt. **Gjelder:** Unreal Linux/Vulkan, kilde `4d38c76f`. **Sist kontrollert:** 1. november 2026.

Tom meldte at konsekvensvalget lukket Freeform-dialogen til hovedmenyen. Den eksakte feilen ble ikke gjenskapt: den gamle dialogen startet både en tom profil og en isolert kopi av alle personlige lagringsplasser. Tidligere Freeform-prøver startet modusen direkte og dekket ikke denne menyreisen.

Den generelle dokumentdialogen er erstattet med en fokusbar startmeny. Bakgrunnsklikk avbryter ikke; lokal gjenoppretting, død/lasting og tilbake er eksplisitte valg. Tastatur og gamepad har navigasjon og bekreftelse. Ved fulle plasser beholder lagringsmenyen Freeform-navnet og krever bekreftelse før en sak erstattes. Den parallelle QUIT-rettelsen og Linux-startdiagnostikken fra PR315 er integrert.

Editorbygg og automatisert Slate-pointerprøve med fulle lagringsplasser består. Prøven klikker den synlige startporten, NEW FREEFORM, konsekvensvalget, CASE 1 og START NEW FREEFORM. MouseDown og MouseUp ligger på forskjellige frames. Aktiv Freeform, dag 1, riktige konsekvenser og tre oppgaver kontrolleres. Original Saved brukes aldri som testprofil.

Installert kl.19:58 CET fra4d38c76f. Pakking,1957 selvtester, begge konsekvensvalg, full-lagringsflyt ved1666×898 og QUIT ved1280×720 består med automatisert Slate-pointerinput i vanlige grafiske vinduer. Installert Fortsett består;155 personlige Saved-elementer er uendret. [Installasjonsresultat](FreeformMenu-2026-11-01/installed.json), [lokal recovery](FreeformMenu-2026-11-01/packaged-recovery.json), [død/lasting](FreeformMenu-2026-11-01/packaged-lethal.json), [fulle plasser](FreeformMenu-2026-11-01/packaged-full.json), [QUIT](FreeformMenu-2026-11-01/packaged-quit.json).

Én prøve uten synlig vindu stoppet med VK_ERROR_DEVICE_LOST; den erstatter ikke de beståtte vanlige grafiske prøvene og er ikke forklart av menyendringen. Ingen menneskelig spillaksept eller bevist opprinnelig rotårsak hevdes. Ved fortsatt returfeil trengs observasjon fra den nye startmenyen.

Ingen ny rydderunde. Arbeidskopien kan gjenbrukes eller avvikles ved neste samlede kjøring; byggcache beholdes. Biblioteksøket ga generelle Slate-referanser, ingen ekstern implementasjon ble gjenbrukt.
