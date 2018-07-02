/////////////////////////////////////////////////////////////////
// drumsandbass.h: drums beats, basslines and chord progressions
// for Pentasynth
//

uint8_t drum_pattern_n = 4;
uint8_t bass_pattern_n = 4;
uint8_t chord_prog_n = 7;

// drum beats
char drum_notes[] = {
  0,
  36,42,36,42,
  36,42,36,42,
  36,42,36,42,36,42,36,42
};
char drum_durations[] = {
  32,
  0,8,16,24, // 8,8,8,8
  0,8,20,24, // 8,12,4,8
  0,4,8,12,16,20,26,28
};
char drum_n[] = {
  0,
  1,
  5,
  9,
  17
};

// basslines
char bass_notes[] = {
  0,
  0,0,0,12,0,0,0,0,0,7,12,0,
  0,4,7,12,0,4,7,12,
  0,4,7,12,0,4,7,12,0,4,7,12,0,4,7,12,
};
char bass_durations[] = {
  32,
  0,4,6,8,12,14,16,20,22,24,28,30, // 4,2,2,4,2,2,4,2,2,4,2,2
  0,4,8,12,16,20,24,28, // 4,4,4,4,4,4,4,4
  0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30
};
char bass_n[] = {
  0,
  1,
  13,
  21,
  37
};

// Chord progressions
char chord_progressions[] = {
  0,0,0,0,
  0,-2,0,-2,
  0,-2,-4,-2,
  0,3,5,0,
  0,3,5,7,
  0,5,7,0,
  0,-2,7,5
};

