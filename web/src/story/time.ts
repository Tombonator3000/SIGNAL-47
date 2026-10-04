const pad = (n: number, w = 2) => String(Math.floor(n)).padStart(w, '0');
export { pad };

// Clock text from seconds after midnight; wraps at 24 h.
export function clockText(sec: number, withSeconds = true) {
  const s = ((sec % 86400) + 86400) % 86400;
  const h = Math.floor(s / 3600), m = Math.floor(s / 60) % 60;
  return withSeconds ? `${pad(h)}:${pad(m)}:${pad(s % 60)}` : `${pad(h)}:${pad(m)}`;
}
