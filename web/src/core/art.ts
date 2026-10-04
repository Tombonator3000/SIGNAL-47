import * as THREE from 'three';

// Graphics made from ART_BRIEF.md (by ChatGPT or anyone else) go in src/assets/art, under
// the exact file names the brief gives, for example src/assets/art/room/tex_floor_hextile.jpg.
// Vite picks up whatever is there when the game is built. A file that exists replaces the
// texture drawn in code; a missing file leaves the code texture as it is. Nothing else
// has to change, so a picture uploaded on GitHub is in the game after the next Pages build.

const found = import.meta.glob('../assets/art/**/*.{png,jpg,jpeg,webp}', { eager: true, query: '?url', import: 'default' }) as Record<string, string>;
const urls: Record<string, string> = {};
for (const [path, url] of Object.entries(found)) urls[path.slice('../assets/art/'.length)] = url;

export const artFiles = () => Object.keys(urls).sort();
export const hasArt = (name: string) => name in urls;

const images = new Map<string, Promise<HTMLImageElement | null>>();
export function artImage(name: string): Promise<HTMLImageElement | null> | null {
  if (!hasArt(name)) return null;
  let p = images.get(name);
  if (!p) {
    p = new Promise((resolve) => {
      const im = new Image();
      im.onload = () => resolve(im);
      im.onerror = () => { console.warn('art file could not be read', name); resolve(null); };
      im.src = urls[name];
    });
    images.set(name, p);
  }
  return p;
}

// Put the art file's picture into an existing texture (it keeps wrap, repeat and colour space).
export function swapIn<T extends THREE.Texture>(name: string, tex: T): T {
  // dispose first: the picture usually has other dimensions than the texture already on the GPU
  artImage(name)?.then((im) => { if (im) { tex.dispose(); tex.image = im; tex.needsUpdate = true; } });
  return tex;
}

// A texture straight from an art file, or null when there is none. For surfaces that have
// no texture today (desk laminate, cabinet metal, desert ground, the sky panorama).
export function artTexture(name: string, repeat?: [number, number]): THREE.Texture | null {
  if (!hasArt(name)) return null;
  // a small grey stand-in until the picture has loaded, so nothing renders black
  const c = document.createElement('canvas');
  c.width = c.height = 2;
  const g = c.getContext('2d')!; g.fillStyle = '#808080'; g.fillRect(0, 0, 2, 2);
  const t = new THREE.Texture(c);
  t.needsUpdate = true;
  t.colorSpace = THREE.SRGBColorSpace;
  t.anisotropy = 4;
  if (repeat) { t.wrapS = t.wrapT = THREE.RepeatWrapping; t.repeat.set(repeat[0], repeat[1]); }
  return swapIn(name, t);
}
