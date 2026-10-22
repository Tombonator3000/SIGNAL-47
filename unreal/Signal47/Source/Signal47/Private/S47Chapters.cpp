#include "S47Chapters.h"

#include "S47Chapter.h"
#include "S47Chapter1.h"
#include "S47Chapter2.h"
#include "S47Chapter3.h"
#include "S47Chapter4.h"
#include "S47Chapter6.h"
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
	US47Chapter1* C1 = Make<US47Chapter1>(Host, M);
	C1->Register(); // also Host->Jumps["chapter1"]
	US47Chapter2* C2 = Make<US47Chapter2>(Host, M);
	US47Chapter2::Register(C2);
	US47Chapter3::Register(Make<US47Chapter3>(Host, M));
	US47Chapter4* C4 = Make<US47Chapter4>(Host, M)->Register(); // also Host->Jumps["chapter4"]
	Make<US47Chapter6>(Host, M)->Register(); // chapter five is integrated separately

	// main.ts: the chapters' save hooks ask for an autosave; chapter one's end is chapter two's card
	C1->SaveHook = [M] { M->RequestAutosave(false); };
	C2->OnSave = [M] { M->RequestAutosave(false); };
	C4->OnSave = [M] { M->RequestAutosave(false); };
	TWeakObjectPtr<US47Chapter2> W2 = C2;
	C1->OnEnd = [W2](const FString& Method) { if (W2.IsValid()) W2->AfterChapterOne(Method); };
}
