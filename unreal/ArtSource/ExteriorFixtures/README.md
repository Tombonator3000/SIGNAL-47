# SARO armaturer, original Blender-kilde

Tre kompakte armaturer med støpt bronsehus, avrundede kanter, linsepakning,
prismatisk diffusor, skruer og monteringsplater. Ingen LED-striper eller
tredjepartsmodeller. PBR-kartene er originalt generert av kildefila.

`build_fixtures.py` skriver `SARO_ExteriorFixtures.blend`, ni FBX-filer,
materialkart og `model_contract.json`. Kjør med Blender 5.2 og to CPU-tråder
etter prosjektets jobbvern. `verify_source.py` åpner den lagrede fila på nytt.
`preview.py` lager to Blender-forhåndsvisninger, ikke bilder fra spillet.
Monteringsveggen og soklene i disse bildene er kun kontekst og eksporteres ikke.

Alle modeller har tre håndskrevne LOD-er. LOD1 reduserer segmenter, skruehoder
og glassribber. LOD2 bevarer huset, linserammen og monteringen, mens små
fastemidler og fysiske ribber erstattes av samme normaltekstur. Detaljerte
originaldeler beholdes i egne `EDIT_LOD`-collections i Blender-fila.

Koordinater: Three `(x,y,z)` i meter, Blender `(x,-z,y)`, målt Unreal-import
`(x,z,y)*100`. Ingen vilkårlig pivotkorreksjon. WallPack berører veggen ved
`z=0`, Canopy berører taket ved `y=0` og går nedover, SignWash står på `y=0`
med hodet vinklet 45 grader opp. Eksakte bounds, optiske normaler og
emitterposisjoner står i kontrakten. Integrasjonen skal følge disse.

Den separate materialslotten `M_ExteriorFixtures_Lens` er ugjennomsiktig
prismatisk/opalt glass. `EmissiveFactor` styrer lysfargen og multipliserer
diffusorteksturen; `(0,0,0)` er slukket. `TextureTint` har standard `(1,1,1)`.
Normalteksturen er DirectX; Blender-grafen vender grønn kanal for visning.
Ingen global eksponering, himmel eller lysstyring endres av kildeskriptene.

Native import og spillintegrasjon eies av `Signal47/Tools/exteriorfixtures`
og runtime-koden. Kildekontrollen her dokumenterer ikke disse stegene.
