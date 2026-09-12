# Art direction and asset production

Use the project's existing art direction and available tooling. These steps apply to 3D where relevant; for 2D/UI use the same concept → authored source → actual product inspection principle and skip meshes/rigs.

## Establish a direction you can inspect

Use a small set of references that communicate palette, silhouette, material treatment, lighting, density, typography and camera feel. Extract concrete traits rather than copying another game's layout or assuming a named style is a complete brief. Keep source attribution and distinguish inspiration from assets licensed for reuse.

Generate concepts when the task needs new bitmap art and an appropriate tool is available. Inspect them for period, anatomy, equipment, narrative and functional contradictions. Reuse approved references for later passes. Store prompts/reference IDs and mark generated art as concept, never as runtime evidence or a player's original capture.

Build a representative asset scene before an extensive library: a few high-use objects, real scale, representative lighting, the intended camera and enough context to judge readability. Inspect both isolated forms and their in-game use.

## Choose the production method per object

| Asset | Useful starting method | Acceptance concern |
| --- | --- | --- |
| Repeated architectural/technical props | Parameterized Blender scripts or reusable authored modules | Scale, silhouettes, pivots, topology and consistent variants |
| A distinctive character/organic object | Authored or licensed asset, or concept-to-3D candidate | Face, topology, deformation, licensing and cleanup cost |
| Flat UI/maps/icons | Existing vector/code-native system when suitable | Actual text, hierarchy, states, contrast and sizing |
| Background illustration/texture | Image generation or an appropriate licensed source | Detail consistency, tiling/UV needs and runtime appearance |

A generated mesh is an input candidate, not an engine-ready asset. Verify the real capabilities, export formats, rights and costs of the available service. Tools mentioned by the user, including Magnific, Meshy or other generators, are options rather than assumed working integrations. Do not buy credits or install bridges just to satisfy a workflow diagram.

For Blender, CLI/Python can be a reproducible route when installed and executed successfully; an MCP bridge is not inherently required. Use the engine's supported asset format and existing import conventions. A GLB pipeline proven in a browser project does not automatically fit another engine.

When `blender-mcp` is available, its skill provides a tested saved-file MCP route and version-specific fallbacks for scene/API/dependency inspection. Use it where those capabilities help; retain the project's Blender version and authored export recipes. Its Blender render is authoring evidence, and integration still needs the engine checks below.

## Prove the complete route once

Record the source/reference, generator or authoring recipe, output, import settings and runtime consumer. Prove save/export/import in the actual target before producing many variants. Preserve authored meshes, textures, metadata, licenses and baked lighting; distinguish compiling a scene from regenerating or rebaking it.

Choose polygon, material, texture, transparency and draw-call budgets from the target device, camera distance and asset role. Do not copy a social post's triangle count as a budget. Test decimation on the actual silhouette and deformation, especially faces, hands, garment boundaries and contacts.

For 3D acceptance inspect the relevant subset:

- Front/side/rear views and a simple clay/material view for silhouette and mesh defects.
- Close and normal gameplay distances, scale relative to hands/furniture and pivot placement.
- Normals, UVs, shading, seams and collision/interactable bounds after import.
- For animation, the actual action extremes: reach, grip, bend, throw/fall or other project-specific poses; check clipping and contact points.
- The in-engine material, light and post-processing result, not only a Blender beauty render.

Use repeatable capture scripts where they pay for themselves. Tie each review image to an asset revision and pose/camera. State what was inspected. Preserve original runtime captures; put annotations or comparisons in separate clearly labeled files.

## Polish that serves the interaction

Prioritize informative response: readable state changes, sound timing, material contact, motion cues and transitions. Then add a few distinctive details that reinforce the setting. Weather, animated crowds and cinematic cameras are optional production costs, not a universal checklist.

Give sound its own runtime review: loops, attenuation, mix, timing and accessibility cues. A visual inspection does not certify subjective audio balance. Recheck core behavior after integrating art and animation; polished presentation must not mask a broken interaction.

When a method repeatedly produces the same defect, compare the remaining cleanup effort with a simpler model, licensed asset or alternative authoring route. Stop optional variants once the asset meets its role and budget.
