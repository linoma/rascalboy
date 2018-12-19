#include <windows.h>
#include "list.h"

//---------------------------------------------------------------------------
#ifndef serviceH
#define serviceH
//---------------------------------------------------------------------------
struct ServiceInterface
{
public:
	virtual HRESULT CreateService(REFIID iid,LPVOID *pUnk) PURE;
};
//---------------------------------------------------------------------------
class LServiceInterface : public ServiceInterface
{
public:
	LServiceInterface();
   ~LServiceInterface();
	HRESULT CreateService(REFIID iid,LPVOID *pUnk);
protected:
};
#endif
