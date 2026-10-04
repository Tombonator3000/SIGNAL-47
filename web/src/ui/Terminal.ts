import type { UI } from './UI';

// A green 80-column terminal over the game, like a document: the operations system on the
// west desk (story/nightshift.ts). Commands are chosen from buttons; on a keyboard they
// can also be typed. Keys typed into the line stay in the terminal (the game's own keys,
// E, P and the rest, are stopped at the input), Escape closes it.

export interface TerminalSpec {
  header: () => string;
  run: (cmd: string) => string;
  buttons: () => string[];
  touch: boolean;
  onCommand?: (cmd: string) => void;
}

const esc = (s: string) => s.replace(/[&<>"]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]!));

export function openTerminal(ui: UI, t: TerminalSpec, onClose?: () => void) {
  const el = document.createElement('div');
  el.className = 'termview';
  el.innerHTML = `
    <div class="term" role="log" aria-live="polite">
      <pre class="term-out"></pre>
      <label class="term-line"><span>$&nbsp;</span><input type="text" spellcheck="false" autocomplete="off" aria-label="Command"${t.touch ? ' readonly' : ''}></label>
    </div>
    <div class="term-cmds"></div>
    <div class="term-foot"><button data-a="close"><kbd>Esc</kbd>Log out</button></div>`;
  const out = el.querySelector('.term-out') as HTMLPreElement;
  const input = el.querySelector('input') as HTMLInputElement;
  const cmds = el.querySelector('.term-cmds') as HTMLDivElement;
  let text = t.header() + '\n';
  const draw = () => { out.textContent = text; out.parentElement!.scrollTop = out.parentElement!.scrollHeight; };
  const buttons = () => {
    cmds.innerHTML = t.buttons().map((b) => `<button data-c="${esc(b)}">${esc(b)}</button>`).join('');
  };
  const exec = (cmd: string) => {
    const c = cmd.trim();
    if (/^(LOGOUT|EXIT|LO)$/i.test(c)) { ui.close(); return; }
    const res = t.run(c);
    text += '$ ' + c.toUpperCase() + '\n' + (res ? res + '\n' : '');
    // keep the screen from growing without end
    const lines = text.split('\n');
    if (lines.length > 400) text = lines.slice(-400).join('\n');
    draw(); buttons();
    t.onCommand?.(c.toUpperCase());
  };
  cmds.addEventListener('click', (e) => {
    const b = (e.target as HTMLElement).closest('button[data-c]') as HTMLButtonElement | null;
    if (b) exec(b.dataset.c!);
  });
  input.addEventListener('keydown', (e) => {
    e.stopPropagation();
    if (e.key === 'Enter') { exec(input.value); input.value = ''; e.preventDefault(); }
    else if (e.key === 'Escape') ui.close();
  });
  input.addEventListener('keyup', (e) => e.stopPropagation());
  el.querySelector('[data-a=close]')!.addEventListener('click', () => ui.close());
  el.addEventListener('click', (e) => { if (e.target === el) ui.close(); });
  draw(); buttons();
  ui.open(el, onClose);
  if (!t.touch) setTimeout(() => input.focus(), 0);
}
