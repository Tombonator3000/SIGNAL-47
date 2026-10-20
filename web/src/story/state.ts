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
  doors?: Record<string, boolean>;   // open doors at SARO (saves from before 4 October 2026 have none)
  flashlight?: boolean;   // taken off the truck's seat (saves from before 15 October 2026 have none)
  truck?: string | null;  // where the truck was left ('motel': on the motel's lot; older saves have none)
  truckPose?: TruckPose | null;   // exactly how it stands there (World.truckSave; saves before 18 October 2026 have none)
  abduct?: import('./abduction').AbductSave;   // the times away (story/abduction.ts; older saves have none)
  town?: string[];        // what the notebook has from Caliche (world/Town.ts, story/caliche.ts; older saves have none)
  album?: import('../core/album').AlbumSave;   // the free frames (core/album.ts; older saves have none)
  motelTv?: { id: string; url: string };   // the witnessed office frame, in the shared photo store
  seen?: import('./seen').SeenEntry[];   // what the night did and whether it was seen (story/seen.ts; older saves have none)
  curve?: import('./curve').CurveSave;   // where the night's one curve was (story/curve.ts; older saves have none)
}

/** Where the truck was left and how it stands: the place (World.truckAt), its position and its
 *  heading, with the tilt of the ground it stands on (rotation x and z, order YXZ). */
export interface TruckPose { at: string; x: number; y: number; z: number; h: number; p?: number; r?: number }

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
