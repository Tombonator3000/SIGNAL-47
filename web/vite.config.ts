import { defineConfig } from 'vite';
import { viteSingleFile } from 'vite-plugin-singlefile';

// "npm run build" gives a normal multi-file build in dist/.
// "npm run build:single" gives one self-contained HTML file in dist-single/
// (used for the claude.ai artifact link and quick sharing to mobile).
export default defineConfig(({ mode }) => ({
  base: './',
  plugins: mode === 'single' ? [viteSingleFile()] : [],
  build: {
    outDir: mode === 'single' ? 'dist-single' : 'dist',
    target: 'es2020',
    chunkSizeWarningLimit: 2000,
  },
}));
