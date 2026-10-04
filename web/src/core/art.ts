import * as THREE from 'three';
import floor from '../assets/art/room/tex_floor_hextile.jpg';
import ceiling from '../assets/art/room/tex_ceiling_tile.jpg';
import wall from '../assets/art/room/tex_wall_paint.jpg';
import desk from '../assets/art/room/tex_desk_laminate.jpg';
import concrete from '../assets/art/ext/tex_concrete.jpg';
import desert from '../assets/art/ext/tex_desert_ground.jpg';
import asphalt from '../assets/art/ext/tex_asphalt_wet.jpg';
import cabinet from '../assets/art/yard/tex_cabinet_metal.jpg';
import listen from '../assets/art/runtime/poster_listen.webp';
import saro from '../assets/art/runtime/poster_saro.webp';
import map from '../assets/art/runtime/map_new_mexico.webp';
import logo from '../assets/art/runtime/logo_saro.webp';
import yard from '../assets/art/runtime/sign_service_yard.webp';
import procedure from '../assets/art/runtime/label_s03_procedure.webp';
import sky from '../assets/art/sky/sky_milkyway_equirect.jpg';
import sierra from '../assets/art/runtime/sign_sierra_on.webp';
// Round 3 (PR #31): text-free surfaces. The game draws every label on them in code.
import vane from '../assets/art/yard/vane_b12.png';
import bars from '../assets/art/yard/board_r07.png';
import frame from '../assets/art/runtime/floor_paint_frame.webp';
import fieldMap from '../assets/art/lab/map_field_yard.png';
import sign from '../assets/art/runtime/sign_blank.webp';
import paper from '../assets/art/lab/tex_paper_card.jpg';

// Static Vite imports work both under /SIGNAL-47/ and in the offline single file.
// Images are decoded before constructing the world or caching Low materials.
// Only files imported here reach the game: concept/, maps/, production/ and QA
// pictures in the same folder tree are never bundled.
const urls = { floor, ceiling, wall, desk, concrete, desert, asphalt, cabinet,
  listen, saro, map, logo, yard, procedure, sky, sierra,
  vane, bars, frame, fieldMap, sign, paper };
export type ArtId = keyof typeof urls;
const images = new Map<ArtId, HTMLImageElement>();
const textures = new Map<string, THREE.Texture>();
const sources = new Map<ArtId, THREE.Source>();

// Observation only, for the existing S47 diagnostics and offline-load checks.
export function artStatus() {
  return { expected: Object.keys(urls).length, loaded: [...images.keys()].sort(),
    textures: [...textures.values()].map(t => ({ name: t.name, width: t.image.width, height: t.image.height,
      colorSpace: t.colorSpace, repeat: t.repeat.toArray() })) };
}

export async function loadArt() {
  await Promise.all((Object.keys(urls) as ArtId[]).map(id => new Promise<void>((resolve, reject) => {
    const img = new Image();
    const timeout = setTimeout(() => { img.src = ''; reject(new Error(`Artwork timed out: ${id}`)); }, 30000);
    img.onload = () => { clearTimeout(timeout); images.set(id, img); resolve(); };
    img.onerror = () => { clearTimeout(timeout); reject(new Error(`Artwork could not load: ${id}`)); };
    img.src = urls[id];
  })));
}

export function artImage(id: ArtId) {
  const image = images.get(id);
  if (!image) throw new Error(`Artwork requested before loading: ${id}`);
  return image;
}

export function artTexture(id: ArtId, repeat?: [number, number]): THREE.Texture {
  const key = `${id}:${repeat?.join(',') ?? 'clamp'}`;
  let texture = textures.get(key);
  if (texture) return texture;
  let source = sources.get(id);
  let image: HTMLImageElement | HTMLCanvasElement = artImage(id);
  // Keep the 4K source for export; one 2K sampler is enough for the mobile sky.
  if (!source && id === 'sky') {
    const canvas = document.createElement('canvas'); canvas.width = 2048; canvas.height = 1024;
    canvas.getContext('2d')!.drawImage(image, 0, 0, 2048, 1024); image = canvas;
  }
  if (!source) { source = new THREE.Source(image); sources.set(id, source); }
  texture = new THREE.Texture();
  // Repeat transforms stay independent while compatible samplers share GPU data.
  texture.source = source;
  texture.name = `art/${id}`;
  texture.colorSpace = THREE.SRGBColorSpace;
  texture.anisotropy = 4;
  if (repeat) { texture.wrapS = texture.wrapT = THREE.RepeatWrapping; texture.repeat.set(...repeat); }
  if (id === 'sky') {
    texture.wrapS = THREE.RepeatWrapping;
    texture.generateMipmaps = false; texture.minFilter = THREE.LinearFilter;
  }
  texture.needsUpdate = true;
  textures.set(key, texture);
  return texture;
}
