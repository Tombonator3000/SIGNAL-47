const pad = (n: number, w = 2) => String(Math.floor(n)).padStart(w, '0');
export { pad };
/** An antenna's angle as the readouts print it: whole degrees padded to w figures and the tenth
 *  (truncated, as the encoders do), with a minus sign below zero (a dish below the horizon). */
export function angleText(v: number, w = 2) {
  const a = Math.abs(v), t = Math.floor(a * 10 + 1e-6) / 10;
  return `${v < 0 && t > 0 ? '-' : ''}${pad(Math.floor(t), w)}.${Math.round((t % 1) * 10) % 10}`;
}

// Clock text from seconds after midnight; wraps at 24 h.
export function clockText(sec: number, withSeconds = true) {
  const s = ((sec % 86400) + 86400) % 86400;
  const h = Math.floor(s / 3600), m = Math.floor(s / 60) % 60;
  return withSeconds ? `${pad(h)}:${pad(m)}:${pad(s % 60)}` : `${pad(h)}:${pad(m)}`;
}
