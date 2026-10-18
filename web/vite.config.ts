import { readFileSync } from 'node:fs';
import { defineConfig, type Plugin } from 'vite';
import { viteSingleFile } from 'vite-plugin-singlefile';

// The link preview and the icon (index.html; ART_BRIEF.md, round 15, brand/). Vite gives them
// hashed files with addresses relative to the page. On Pages the sites that show a shared link
// want the preview picture's whole address, so it is given here. The single file keeps its icon
// inside itself, and has no link preview to give (nobody shares it by its address).
const SITE = 'https://tombonator3000.github.io/SIGNAL-47/';
function brand(single: boolean): Plugin {
  return {
    name: 's47-brand',
    enforce: 'post',
    transformIndexHtml: {
      order: 'post',
      handler(html) {
        if (!single) return html;
        const icon = 'data:image/png;base64,' + readFileSync(new URL('./src/assets/art/brand/icon_512.png', import.meta.url)).toString('base64');
        return html.replace(/\s*<meta (?:property="og:image(?::\w+)?"|name="twitter:image") [^>]*>/g, '')
          .replace(/(<link rel="(?:icon|apple-touch-icon)"[^>]*? href=")[^"]*"/g, `$1${icon}"`);
      },
    },
    // the built page has the files' final addresses only here
    generateBundle(_o, bundle) {
      if (single) { for (const k of Object.keys(bundle)) if (/^(icon_512|og_cover)-.*\.(png|jpg)$/.test(k)) delete bundle[k]; return; }
      const page = bundle['index.html'];
      if (page?.type === 'asset' && typeof page.source === 'string')
        page.source = page.source.replace(/(<meta (?:property="og:image"|name="twitter:image") content=")\.\//g, `$1${SITE}`);
    },
  };
}

// "npm run build" gives the normal multi-file build in dist/, which GitHub Pages serves.
// It is split so a phone downloads what it needs: three.js in its own file (it rarely
// changes, so the browser keeps it between updates), the game, and each later chapter and
// area as a separate file that is fetched when the story gets there.
// "npm run build:single" gives one self-contained HTML file in dist-single/, for the tests
// and for sharing a build as a single file.
export default defineConfig(({ mode }) => ({
  base: './',
  plugins: mode === 'single' ? [viteSingleFile(), brand(true)] : [brand(false)],
  build: {
    outDir: mode === 'single' ? 'dist-single' : 'dist',
    target: 'es2020',
    chunkSizeWarningLimit: 2000,
    rollupOptions: mode === 'single' ? {} : {
      output: { manualChunks: (id) => (id.includes('node_modules/three') ? 'three' : undefined) },
    },
  },
}));
