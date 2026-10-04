import type { SavedCase } from './Prologue';
import type { SaveCard } from '../core/saves';
import { clockText } from './time';

// What a save holds. The checkpoint says where the story resumes (the prologue has two
// safe points, every chapter resumes from its own state); the case carries the chapters'
// progress and photographs; area and pose put the player back where they stood.
export interface GameState {
  v: 2;
  checkpoint: string;   // 'residual' | 'chapter1' | 'chapter2' | ...
  case: SavedCase | null;
  area: string;         // 'saro' | 'station01' (a save is never made on the road)
  pose: { x: number; z: number; yaw: number; pitch: number } | null;
}

const CHAPTERS: Record<string, string> = {
  residual: 'Prologue: Night Shift',
  chapter1: 'Chapter 1: The Second Exposure',
  chapter2: 'Chapter 2: The Amended Record',
  chapter3: 'Chapter 3: The Survey Station',
};

// The first web version saved one case and the name of a checkpoint. It becomes the
// newest autosave of case 1 (core/saves.ts).
export function migrateOldSave(checkpoint: string, oldCase: unknown): { state: GameState; card: SaveCard } | null {
  if (!CHAPTERS[checkpoint]) return null;
  const c = checkpoint === 'residual' ? null : (oldCase as SavedCase | null);
  if (checkpoint !== 'residual' && !c) return null;
  return {
    state: { v: 2, checkpoint, case: c, area: 'saro', pose: null },
    card: { chapter: CHAPTERS[checkpoint], place: 'SARO', clock: c ? clockText(c.clock, false) : '02:13', playtime: 0, thumb: null },
  };
}
