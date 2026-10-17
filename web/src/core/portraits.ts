// The faces of the people of the night (ART_BRIEF.md, round 13: painted by ChatGPT, ordered by
// Tom on 16 October). Each is a file in assets/art/people/ by the name the brief gives; the game
// takes whichever are there when it is built, so a portrait shows up as soon as its file is in
// the repository, and until then the person is a name with an empty frame (as before). They are
// shown in the conversation panels (ui/Panels.ts) and in the notebook's People tab (main.ts).

const FILES = import.meta.glob<string>('../assets/art/people/portrait_*.jpg', { eager: true, import: 'default' });

export type PersonId = 'nora' | 'waitress' | 'driver' | 'ward' | 'dale' | 'tomas_1947'
  | 'deputy' | 'host' | 'attendant' | 'radio_woman' | 'souvenir_man';

/** The picture of a person, or null while there is none. */
export function portrait(id: PersonId): string | null {
  return FILES[`../assets/art/people/portrait_${id}.jpg`] ?? null;
}

// the names the conversations and calls give the speakers
const BY_NAME: Record<string, PersonId> = {
  'N. VEGA': 'nora', 'NORA': 'nora', 'THE WAITRESS': 'waitress', 'THE DRIVER': 'driver',
  'E. WARD': 'ward', 'WARD': 'ward', 'DALE': 'dale',
};
/** The picture of whoever speaks under this name, or null. */
export function portraitOf(who: string): string | null {
  const id = BY_NAME[who.trim().toUpperCase()];
  return id ? portrait(id) : null;
}
/** How many portraits the build has (the developer menu, the tests). */
export const portraitCount = () => Object.keys(FILES).length;
