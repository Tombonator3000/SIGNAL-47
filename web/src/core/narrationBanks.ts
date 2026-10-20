import source from '../assets/audio/serling/catalog.json?raw';
import type { ReadingBank, ReadingClip } from './readAloud';

const FILES = import.meta.glob<string>('../assets/audio/serling/*.mp3', { eager: true, query: '?url', import: 'default' });
const clips = (JSON.parse(source) as { clips: ReadingClip[] }).clips;
export const serlingBank: ReadingBank = { clips, label: 'Synthetic Rod Serling parody',
  urls: Object.fromEntries(clips.map((clip) => [clip.file, FILES[`../assets/audio/serling/${clip.file}`]])) };
