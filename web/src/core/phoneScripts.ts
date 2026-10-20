/** Exact authored speech; actor labels are captions, never spoken into the clone. */
export interface PhoneCue { text: string; by?: string; aside?: boolean; pause?: number }

/** These existing CB words share Nora's character voice and small channel preload. */
export const NORA_CB_TEXTS = [
  'SARO seven, this is Sierra. You hear me?',
  'Pull in at the north end, by the office. Not down past the rooms.',
  'Room six. The door is open. Bring the prints.',
];

export const PHONE_CALLS: Record<'ward' | 'nora' | 'fieldNora', PhoneCue[]> = {
  ward: [
    { by: 'WARD', text: 'That is enough to hold the morning series. It is not enough to tell us why it happened.' },
    { by: 'WARD', text: 'Check the reference record. If the instrument inherited the fault, the record may tell us from where.' },
  ],
  nora: [
    { by: 'N. VEGA', text: 'Sierra Motor Court.' },
    { text: 'You give your name and SARO, and read her the survey number from the sleeve.', aside: true, pause: 3.4 },
    { by: 'N. VEGA', text: 'Station One. Nobody has asked me for that key in a long time.' },
    { by: 'N. VEGA', text: 'Don\'t send me copies. Go out to the station first and look at the cable, where it was cut. Then bring me the originals.' },
  ],
  fieldNora: [
    { by: 'N. VEGA', text: 'So you went out there.' },
    { by: 'N. VEGA', text: 'This line still rings in the motel office. It has since 1947. Nobody ever took it down.' },
    { by: 'N. VEGA', text: 'You have seen the cable now. It did not break by itself.' },
    { by: 'N. VEGA', text: 'Room 6. Bring the original record and your film, developed. Then I will tell you the rest.' },
  ],
};

// Only this small bank is decoded with AudioSys's ordinary sounds. The larger
// document/conversation library remains lazy in ReadAloud. All 24 are literal
// source strings so the offline generator can extract them without running game code.
export const PHONE_TEXTS = [
  'That is enough to hold the morning series. It is not enough to tell us why it happened.',
  'Check the reference record. If the instrument inherited the fault, the record may tell us from where.',
  'Sierra Motor Court.',
  'Station One. Nobody has asked me for that key in a long time.',
  'Don\'t send me copies. Go out to the station first and look at the cable, where it was cut. Then bring me the originals.',
  'So you went out there.',
  'This line still rings in the motel office. It has since 1947. Nobody ever took it down.',
  'You have seen the cable now. It did not break by itself.',
  'Room 6. Bring the original record and your film, developed. Then I will tell you the rest.',
  'Where does that phone go?',
  'To the office. At night the office line is patched through to this room, and the other end of it is the field hut. It has been since 1947.',
  'You left the receiver on out there. Listen.',
  'Tomás. Where did you leave the spare key?',
  'Reference west. No. East. Hold the last reading.',
  'That is what he said then. Not an answer to what I asked now.',
  'Ward.',
  'It is Reyes. I am at the diner on the old road.',
  'I read the telex. All of it.',
  'Hondo called. So did Site 11.',
  'What did they say?',
  'Enough. I am not doing this on a pay phone.',
  'I am checking the old road first.',
  'Then check it, and come back. I want you on the console before the morning series.',
  'Six o\'clock, Reyes.',
];
