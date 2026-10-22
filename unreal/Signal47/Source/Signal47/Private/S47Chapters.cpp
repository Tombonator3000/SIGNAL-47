#include "S47Chapters.h"

#include "S47Chapter.h"
#include "S47Chapter1.h"
#include "S47Chapter2.h"
#include "S47Main.h"
#include "S47Story.h"

namespace
{
	template <class T>
	T* Make(AS47Story* Host, US47Main* M)
	{
		T* C = NewObject<T>(Host);
		C->Bind(M, Host);
		Host->Chapters.Add(C);
		return C;
	}
}

void S47RegisterChapters(AS47Story* Host, US47Main* M)
{
	Host->Chapters.Reset();
	// (one line per ported chapter, in the order Prologue.ts builds them)
	Make<US47Chapter1>(Host, M)->Register(); // also Host->Jumps["chapter1"]
	US47Chapter2::Register(Make<US47Chapter2>(Host, M));
}
