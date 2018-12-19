#include <windows.h>
#include <ole2.h>
#include "list.h"

//---------------------------------------------------------------------------
#ifndef dragdropH
#define dragdropH
//---------------------------------------------------------------------------
class LDropSource : public IDropSource
{
public:
	LDropSource() {m_nCount = 0;};
	~LDropSource() {} ;
	STDMETHODIMP QueryInterface (REFIID riid, LPVOID *ppv);
	STDMETHODIMP_(ULONG) AddRef ();
	STDMETHODIMP_(ULONG) Release ();
	STDMETHODIMP QueryContinueDrag (BOOL fEscapePressed, DWORD grfKeyState);
	STDMETHODIMP GiveFeedback (DWORD dwEffect);
private:
	int m_nCount;
};
//---------------------------------------------------------------------------
class LEnumFormatEtc : public IEnumFORMATETC
{
public:
   LEnumFormatEtc();
	LEnumFormatEtc(LList *pList);
     //IUnknown members
   STDMETHOD(QueryInterface)(REFIID, void FAR* FAR*);
   STDMETHOD_(ULONG, AddRef)(void);
   STDMETHOD_(ULONG, Release)(void);

     //IEnumFORMATETC members
   STDMETHOD(Next)(ULONG, LPFORMATETC, ULONG FAR *);
   STDMETHOD(Skip)(ULONG);
   STDMETHOD(Reset)(void);
   STDMETHOD(Clone)(IEnumFORMATETC FAR * FAR*);
private:
   ULONG m_cRefCount,m_iCur;
   LList *m_pFmtEtc;
};
//---------------------------------------------------------------------------
class LDataObject : public IDataObject
{
public:
   LDataObject();
   ~LDataObject();
	STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();
	STDMETHODIMP DAdvise(FORMATETC *pFormatetc, DWORD advf,LPADVISESINK pAdvSink, DWORD *pdwConnection);
	STDMETHODIMP DUnadvise(DWORD dwConnection);
	STDMETHODIMP EnumDAdvise( LPENUMSTATDATA *ppenumAdvise);
	STDMETHODIMP EnumFormatEtc(DWORD dwDirection,LPENUMFORMATETC *ppenumFormatEtc);
   STDMETHODIMP GetCanonicalFormatEtc(LPFORMATETC pformatetc,LPFORMATETC pformatetcOut);
	STDMETHODIMP GetData(LPFORMATETC pformatetcIn,LPSTGMEDIUM pmedium );
	STDMETHODIMP GetDataHere(LPFORMATETC pformatetc,LPSTGMEDIUM pmedium);
	STDMETHODIMP QueryGetData(LPFORMATETC petc);
   STDMETHODIMP SetData(LPFORMATETC pformatetc,LPSTGMEDIUM pmedium,BOOL fRelease);
private:
   void CopyMedium(STGMEDIUM* pMedDest, STGMEDIUM* pMedSrc, FORMATETC* pFmtSrc);
	int m_nCount;
   LList *m_ArrFormatEtc;
   LList *m_StgMedium;
};

#endif
