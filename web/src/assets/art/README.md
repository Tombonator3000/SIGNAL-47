# Grafikk til spillet

Bildefilene fra `web/ART_BRIEF.md` ligger her, i undermapper med nøyaktig navn fra briefen, for eksempel:

```
room/tex_floor_hextile.jpg
sky/sky_milkyway_equirect.jpg
yard/vane_b12.png
lab/sign_blank.png
```

Et bilde kommer med i spillet når det er importert i `src/core/art.ts`. Der står alle 22 bildene spillet bruker, og de lastes før verden bygges. Store PNG-er brukes som WebP-kopier fra `runtime/` (lag dem med `python3 tools/prepare_art.py`). Alt annet her, som `concept/`, `maps/`, `production/` og forhåndsvisninger, er arbeidsmateriale og havner aldri i bygget.

Ny fil: last den opp med riktig navn (på GitHub: Add file, Upload files, og skriv undermappa foran filnavnet hvis den ikke finnes), og be Claude koble den inn. Når endringen er på main, bygger Pages spillet på nytt i løpet av et par minutter.

Hvor hver fil brukes, står i ART_BRIEF.md under «Slik kommer bildene inn i spillet». Opphav, prompts og sjekksummer står i JSON-filene i hver mappe og i `web/THIRD_PARTY_NOTICES.md`.
