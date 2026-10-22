#pragma once

class AS47Story;
class US47Main;

// Makes every ported chapter and hands it to the host (Prologue.ts's constructor builds them all
// at once). One line per chapter in S47Chapters.cpp.
void S47RegisterChapters(AS47Story* Host, US47Main* M);
