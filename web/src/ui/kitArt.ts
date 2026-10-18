// The drawings beside the things in the notebook's Kit tab (ART_BRIEF.md, round 15, kit/): ink
// and watercolour, as if drawn in the notebook's margin. Shown as pictures in the page, so the
// browser fetches them when the tab is first opened. The keys' leather tag is blank in the
// drawing; the tab writes SARO 07 on it.
import notebook from '../assets/art/runtime/kit_notebook.webp';
import jacket from '../assets/art/runtime/kit_jacket.webp';
import camera from '../assets/art/runtime/kit_camera.webp';
import flashlight from '../assets/art/runtime/kit_flashlight.webp';
import keys from '../assets/art/runtime/kit_keys.webp';
import papers from '../assets/art/runtime/kit_papers.webp';
import nickel from '../assets/art/runtime/kit_nickel_1947.webp';

export const KIT_ART = { notebook, jacket, camera, flashlight, keys, papers, nickel } as const;
export type KitArt = keyof typeof KIT_ART;
