import type { AudioSys } from './Audio';
import { RadioVoice, type EnsembleVoice } from './radioVoice';
import { RecordedDialogue, type DialogueLine } from './recordedDialogue';

export type CharacterVoice = Exclude<EnsembleVoice, 'price'>;
export const CHARACTER_SPEAKERS: Record<CharacterVoice, string> = {
  stanton: 'THE ATTENDANT', stack: 'THE DEPUTY', lynch: 'THE SOUVENIR MAN', bacall: 'N. VEGA',
};
export const CHARACTER_LABELS: Record<EnsembleVoice, string> = {
  stanton: 'Harry Dean Stanton', stack: 'Robert Stack', lynch: 'David Lynch', bacall: 'Lauren Bacall', price: 'Vincent Price',
};
export function characterLines(voice: EnsembleVoice, lines: DialogueLine[]) {
  return lines.filter((line) => line.who === (voice === 'price' ? 'THE LINE' : CHARACTER_SPEAKERS[voice])
    || voice === 'bacall' && line.who === 'THE LINE');
}

/** Roles are selected by the authored interaction, never inferred from arbitrary text. */
export class NPCDialogue extends RecordedDialogue<{ voice: EnsembleVoice; lines: DialogueLine[] }> {
  constructor(audio: AudioSys) {
    super(audio, new RadioVoice('ensemble'), ({ voice, lines }, bank) => {
      const speech = characterLines(voice, lines);
      if (!speech.length) return null;
      const clips = speech.map((line) => bank.resolve(line.text, 0, line.who === 'THE LINE' ? 'price' : voice));
      return clips.every((found) => found !== null) ? clips.flatMap((found) => found!) : null;
    }, (clip) => clip.voice === 'price');
  }
}
