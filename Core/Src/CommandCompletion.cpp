#include "CorePrivate.h"


// list, result of completion

class CCompletedItem : public CStringItem
{
public:
	unsigned flags;
};

class CCompletedList : public CMemoryChain, public TList<CCompletedItem>
{
public:
	char *completedString;
	void AddItem (const char *name, unsigned flags);
	//?? int numItems;
	//?? void Refine() -- reduce list by known few next chars
};


// worker, which can complete some kind of commands

class CCmdCompletorWorker
{
public:
	virtual void Complete (const char *partial, CCompletedList &list);
};


// main completion system

class CCommandCompletion
{
private:
	CCmdCompletorWorker *workers[256];
public:
	void RegisterWorker (CCmdCompletorWorker &worker);
	CCompletedList *CompleteCommand (const char *partial);
};


// CCommandCompletion

void CCommandCompletion::RegisterWorker (CCmdCompletorWorker &worker)
{
	for (int i = 0; i < ARRAY_COUNT(workers); i++)
	{
		if (workers[i] == &worker) return;	// already registered
		if (!workers[i])
		{
			// register worker
			workers[i] = &worker;
			return;
		}
	}
	//!! check this:
	appWPrintf ("Cannot register completor: \"%s\"\n", appSymbolName ((address_t)&worker));
}

CCompletedList *CCommandCompletion::CompleteCommand (const char *partial)
{
	CCompletedList *list = new CCompletedList;


	return list;
}
