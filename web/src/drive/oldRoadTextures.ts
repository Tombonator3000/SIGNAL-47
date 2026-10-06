import * as THREE from 'three';
import { canvasTex, rng } from '../core/textures';
import { artImage, artLoaded } from '../core/art';

// Canvas textures for the old Roswell road (chapter six). The asphalt and desert art from
// earlier rounds are the base; the paint, the numbers on the mile posts and the stamp on
// the survey bolt are drawn here. Codex's round 11 (ART_BRIEF.md) will replace the drawn
// mile post, witness post and brass disk under the same text.

const PX = 256 / 8.8; // the road texture is 8.8 m across: 6.4 m of asphalt and 1.2 m of shoulder each side

/** Old two-lane county asphalt, 24 m per repeat: bleached, chip seal showing through, tar
 *  snakes, and a faded broken yellow centre line (a 3 m dash every 12 m). */
export function oldAsphaltTex() {
  return canvasTex(256, 768, (g, w, h) => {
    const r = rng(1986);
    const own = artLoaded('oldAsphalt'), desert = artImage('desert');
    if (own) {
      // Codex's old county asphalt (round 11), four tiles of about 6.6 m down the 24 m repeat
      const tile = artImage('oldAsphalt'), t = h / 4;
      for (let y = 0; y < h; y += t) g.drawImage(tile, w / 2 - t / 2, y, t, t);
    } else {
      const asphalt = artImage('asphalt');
      for (let y = 0; y < h; y += 128) for (let x = 0; x < w; x += 128) g.drawImage(asphalt, x, y, 128, 128);
      g.fillStyle = 'rgba(150,146,138,.32)'; g.fillRect(0, 0, w, h);          // bleached by forty summers
      for (let i = 0; i < 5000; i++) {                                          // chip seal
        const v = r() < 0.5 ? 70 + r() * 40 : 150 + r() * 50;
        g.fillStyle = `rgba(${v},${v * 0.97},${v * 0.92},${0.25 + r() * 0.4})`; g.fillRect(r() * w, r() * h, 1 + r() * 1.5, 1 + r() * 1.5);
      }
    }
    // the wheel tracks are darker and smoother
    for (const m of [-1.6, 1.6]) for (const k of [-0.75, 0.75]) {
      const cx = w / 2 + (m + k) * PX, gr = g.createLinearGradient(cx - 0.35 * PX, 0, cx + 0.35 * PX, 0);
      gr.addColorStop(0, 'rgba(0,0,0,0)'); gr.addColorStop(0.5, 'rgba(25,24,24,.18)'); gr.addColorStop(1, 'rgba(0,0,0,0)');
      g.fillStyle = gr; g.fillRect(cx - 0.35 * PX, 0, 0.7 * PX, h);
    }
    // shoulders: packed caliche and gravel, a ragged edge where the asphalt breaks up
    for (const x0 of [0, w - 1.2 * PX]) {
      g.drawImage(desert, 0, 0, 128, 512, x0, 0, 1.2 * PX, h);
      g.fillStyle = 'rgba(120,100,80,.25)'; g.fillRect(x0, 0, 1.2 * PX, h);
    }
    for (let i = 0; i < 700; i++) {
      const left = r() < 0.5, x = left ? 1.2 * PX + (r() - 0.3) * 7 : w - 1.2 * PX + (r() - 0.7) * 7;
      g.fillStyle = `rgba(${r() < 0.5 ? '70,66,60' : '150,136,116'},.8)`; g.fillRect(x, r() * h, 1 + r() * 3, 1 + r() * 3);
    }
    // cracks across and along, sealed with tar long ago (the picture has its own)
    g.strokeStyle = 'rgba(12,12,12,.7)'; g.lineCap = 'round';
    for (let i = 0; i < (own ? 0 : 12); i++) {
      let x = 1.3 * PX + r() * (w - 2.6 * PX), y = r() * h;
      g.lineWidth = 1.5 + r() * 2; g.beginPath(); g.moveTo(x, y);
      for (let k = 0; k < 9; k++) { x += (r() - 0.5) * 30; y += (r() - 0.35) * 26; g.lineTo(Math.min(w - 1.3 * PX, Math.max(1.3 * PX, x)), y); }
      g.stroke();
    }
    // the centre line: yellow, mostly gone
    for (const y0 of [0, h / 2]) {
      g.fillStyle = 'rgba(200,160,60,.55)'; g.fillRect(w / 2 - 0.05 * PX, y0 + 8, 0.1 * PX, h / 8);
    }
    g.globalCompositeOperation = 'destination-out';
    for (let i = 0; i < 300; i++) { g.fillStyle = `rgba(0,0,0,${0.3 + r() * 0.5})`; g.fillRect(w / 2 - 3 + r() * 6, r() * h, 1 + r() * 3, 1 + r() * 6); }
    g.globalCompositeOperation = 'source-over';
  }, [1, 1]);
}

// ---------- mile posts: one card per number in an atlas, 128 x 320 each ----------
export const POST_NUMBERS = [1, 2, 3, 4, 5, 6, 7, 8, 9];
const ATLAS_W = POST_NUMBERS.length * 128;
export const postRect = (i: number): [number, number, number, number] => [i * 128 / ATLAS_W, 0, (i + 1) * 128 / ATLAS_W, 1];
export function milepostAtlas() {
  return canvasTex(ATLAS_W, 320, (g) => {
    const own = artLoaded('milepostBlank');
    POST_NUMBERS.forEach((n, i) => {
      const x = i * 128, r = rng(40 + n);
      if (own) g.drawImage(artImage('milepostBlank'), x, 0, 128, 320);   // Codex's blank plate (round 11)
      else {
        g.fillStyle = '#1f5a3c'; g.fillRect(x, 0, 128, 320);
        g.strokeStyle = '#e8eee6'; g.lineWidth = 5;
        g.beginPath(); g.roundRect(x + 7, 7, 114, 306, 9); g.stroke();
      }
      g.fillStyle = '#ecefe4'; g.textAlign = 'center'; g.textBaseline = 'middle';
      g.font = '500 30px Oswald'; g.fillText('MILE', x + 64, 64, 96);
      g.font = '600 132px Oswald'; g.fillText(String(n), x + 64, 196, 96);
      // dust at the bottom, sun-faded spots, a dent or two
      const gr = g.createLinearGradient(0, 230, 0, 320);
      gr.addColorStop(0, 'rgba(150,120,90,0)'); gr.addColorStop(1, `rgba(150,120,90,${own ? 0.2 : 0.45})`);
      g.fillStyle = gr; g.fillRect(x, 230, 128, 90);
      for (let k = 0; k < (own ? 40 : 160); k++) { g.fillStyle = r() < 0.6 ? 'rgba(120,100,80,.3)' : 'rgba(240,240,230,.25)'; g.fillRect(x + r() * 128, r() * 320, 1 + r() * 2, 1 + r() * 2); }
      if (n === 8) {   // the one by the line: a bullet hole, like most of them out here
        g.fillStyle = 'rgba(110,55,20,.5)'; g.beginPath(); g.arc(x + 92, 258, 8, 0, 7); g.fill();
        g.fillStyle = '#121212'; g.beginPath(); g.arc(x + 92, 258, 3.5, 0, 7); g.fill();
      }
    });
  });
}

// ---------- the survey bolt: a brass disk in a little concrete top ----------
/** label: STA 01 / C / 1947 on C; BM 6 on the bolt by the six-mile post. */
export function boltTex(label: string[]) {
  return canvasTex(256, 256, (g, w) => {
    const r = rng(label.join('').length * 7);
    const c = w / 2, R = 78;
    if (artLoaded('surveyDisk')) {
      // Codex's brass disk in its concrete collar (round 11): only the stamp is drawn here
      g.fillStyle = '#8c877c'; g.fillRect(0, 0, w, w);
      g.drawImage(artImage('surveyDisk'), 0, 0, w, w);
      g.fillStyle = 'rgba(40,28,10,.88)'; g.textAlign = 'center'; g.textBaseline = 'middle';
      g.font = '600 24px "Special Elite", monospace';
      label.forEach((s, i) => g.fillText(s, c, c - (label.length - 1) * 15 + i * 30, 120));
      if (label.length > 1) {   // the arrow along C, east
        g.fillRect(c - 16, c + 46, 26, 3); g.beginPath(); g.moveTo(c + 18, c + 47.5); g.lineTo(c + 8, c + 41); g.lineTo(c + 8, c + 54); g.fill();
      }
      return;
    }
    g.fillStyle = '#8c877c'; g.fillRect(0, 0, w, w);                      // concrete
    for (let i = 0; i < 900; i++) { const v = 100 + r() * 90; g.fillStyle = `rgba(${v},${v},${v * 0.95},.5)`; g.fillRect(r() * w, r() * w, 2, 2); }
    const gr = g.createRadialGradient(c - 20, c - 25, 6, c, c, R);
    gr.addColorStop(0, '#c9a35a'); gr.addColorStop(0.7, '#9a7838'); gr.addColorStop(1, '#5d4b26');
    g.fillStyle = gr; g.beginPath(); g.arc(c, c, R, 0, 7); g.fill();
    g.strokeStyle = 'rgba(70,110,85,.6)'; g.lineWidth = 6; g.beginPath(); g.arc(c, c, R - 2, 0, 7); g.stroke();  // verdigris
    g.strokeStyle = 'rgba(40,30,15,.7)'; g.lineWidth = 2; g.beginPath(); g.arc(c, c, R - 16, 0, 7); g.stroke();
    // stamped letters round the rim and in the middle
    g.fillStyle = 'rgba(45,32,12,.85)'; g.font = '600 15px "Special Elite", monospace'; g.textAlign = 'center'; g.textBaseline = 'middle';
    const rim = 'SURVEY MARK  DO NOT DISTURB  ';
    for (let i = 0; i < rim.length; i++) {
      const a = -Math.PI / 2 + (i / rim.length) * Math.PI * 2;
      g.save(); g.translate(c + Math.cos(a) * (R - 9), c + Math.sin(a) * (R - 9)); g.rotate(a + Math.PI / 2); g.fillText(rim[i], 0, 0); g.restore();
    }
    g.font = '600 22px "Special Elite", monospace';
    label.forEach((s, i) => g.fillText(s, c, c - (label.length - 1) * 13 + i * 26, 110));
    g.fillStyle = 'rgba(30,22,10,.9)'; g.beginPath(); g.arc(c, c + 52, 3, 0, 7); g.fill();       // the punch mark
  });
}

/** The witness post's face: faded orange, a white reflective band, SURVEY MARK down it. */
export function witnessTex() {
  return canvasTex(64, 384, (g, w, h) => {
    const r = rng(11);
    if (artLoaded('witnessPost')) {
      // Codex's post (round 11), with the words stencilled down it
      g.drawImage(artImage('witnessPost'), 0, 0, w, h);
      g.save(); g.translate(w / 2, 230); g.rotate(-Math.PI / 2);
      g.fillStyle = 'rgba(30,22,14,.9)'; g.font = '600 24px Oswald'; g.textAlign = 'center'; g.textBaseline = 'middle';
      g.fillText('SURVEY MARK', 0, 0, 250); g.restore();
      return;
    }
    g.fillStyle = '#c4672a'; g.fillRect(0, 0, w, h);
    g.fillStyle = '#e9e6dc'; g.fillRect(0, 18, w, 54);
    for (let i = 0; i < 400; i++) { g.fillStyle = r() < 0.5 ? 'rgba(90,60,40,.3)' : 'rgba(250,230,200,.25)'; g.fillRect(r() * w, r() * h, 1 + r() * 2, 1 + r() * 2); }
    g.save(); g.translate(w / 2, 230); g.rotate(-Math.PI / 2);
    g.fillStyle = '#231a12'; g.font = '600 26px Oswald'; g.textAlign = 'center'; g.textBaseline = 'middle';
    g.fillText('SURVEY MARK', 0, 0, 270); g.restore();
    const gr = g.createLinearGradient(0, 300, 0, h);
    gr.addColorStop(0, 'rgba(120,90,60,0)'); gr.addColorStop(1, 'rgba(120,90,60,.6)');
    g.fillStyle = gr; g.fillRect(0, 300, w, h - 300);
  });
}

/** Old weathered wood for the survey stakes along C. */
export const STAKE = new THREE.Color(0.36, 0.3, 0.24);
