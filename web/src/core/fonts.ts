import vt323 from '../assets/fonts/vt323.woff2';
import specialElite from '../assets/fonts/specialelite.woff2';
import reenieBeanie from '../assets/fonts/reeniebeanie.woff2';
import oswald from '../assets/fonts/oswald.woff2';

// Fonts are registered from bytes with the FontFace API instead of CSS @font-face.
// A published page's content-security policy may refuse data: font URLs, but it
// does not apply to fonts built from an ArrayBuffer. Same result offline and online.
const FONTS: [string, string, FontFaceDescriptors?][] = [
  ['VT323', vt323],
  ['Special Elite', specialElite],
  ['Reenie Beanie', reenieBeanie],
  ['Oswald', oswald, { weight: '200 700' }],
];

function bytesOf(dataUrl: string): ArrayBuffer {
  const bin = atob(dataUrl.slice(dataUrl.indexOf(',') + 1));
  const out = new Uint8Array(bin.length);
  for (let i = 0; i < bin.length; i++) out[i] = bin.charCodeAt(i);
  return out.buffer;
}

export async function loadFonts() {
  await Promise.all(FONTS.map(async ([family, url, desc]) => {
    try {
      const buf = url.startsWith('data:') ? bytesOf(url) : await (await fetch(url)).arrayBuffer();
      const face = new FontFace(family, buf, desc);
      await face.load();
      document.fonts.add(face);
    } catch {
      // the CSS fallback stacks take over (Courier New, Arial Narrow, cursive)
    }
  }));
}
