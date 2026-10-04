import { defineConfig } from 'vite';
import { viteSingleFile } from 'vite-plugin-singlefile';

// "npm run build" gives the normal multi-file build in dist/, which GitHub Pages serves.
// It is split so a phone downloads what it needs: three.js in its own file (it rarely
// changes, so the browser keeps it between updates), the game, and each later chapter and
// area as a separate file that is fetched when the story gets there.
// "npm run build:single" gives one self-contained HTML file in dist-single/, for the tests
// and for sharing a build as a single file.
export default defineConfig(({ mode }) => ({
  base: './',
  plugins: mode === 'single' ? [viteSingleFile()] : [],
  build: {
    outDir: mode === 'single' ? 'dist-single' : 'dist',
    target: 'es2020',
    chunkSizeWarningLimit: 2000,
    rollupOptions: mode === 'single' ? {} : {
      output: { manualChunks: (id) => (id.includes('node_modules/three') ? 'three' : undefined) },
    },
  },
}));
