import source from '../assets/audio/serling/catalog.json?raw';
import openingSource from '../assets/audio/serling-intro/catalog.json?raw';
import type { ReadingBank, ReadingClip } from './readAloud';

const FILES = import.meta.glob<string>('../assets/audio/serling/*.mp3', { eager: true, query: '?url', import: 'default' });
const OPENING_FILES = import.meta.glob<string>('../assets/audio/serling-intro/*.mp3', { eager: true, query: '?url', import: 'default' });
const clips = (JSON.parse(source) as { clips: ReadingClip[] }).clips;
const openingClips = (JSON.parse(openingSource) as { clips: ReadingClip[] }).clips;
export const serlingBank: ReadingBank = { clips: [...clips, ...openingClips], label: 'Synthetic Rod Serling parody',
  urls: Object.fromEntries([
    ...clips.map((clip) => [clip.file, FILES[`../assets/audio/serling/${clip.file}`]]),
    ...openingClips.map((clip) => [clip.file, OPENING_FILES[`../assets/audio/serling-intro/${clip.file}`]]),
  ]) };
