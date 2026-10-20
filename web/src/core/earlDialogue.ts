import type { AudioSys } from './Audio';
import { RadioVoice } from './radioVoice';
import { RecordedDialogue, type DialogueLine, type DialogueState } from './recordedDialogue';

export type EarlState = DialogueState;
export type EarlLine = DialogueLine;

/** Earl's public API and bank remain independent of the other characters. */
export class EarlDialogue extends RecordedDialogue<EarlLine[]> {
  constructor(audio: AudioSys) {
    super(audio, new RadioVoice('earl'), (lines, bank) => {
      const speech = lines.filter((line) => line.who === 'EARL');
      if (!speech.length) return null;
      const clips = speech.map((line) => bank.resolve(line.text, 0));
      return clips.every((found) => found !== null) ? clips.flatMap((found) => found!) : null;
    });
  }
}
