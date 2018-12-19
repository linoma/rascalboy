#define INITGUID
//---------------------------------------------------------------------------
#include <windows.h>
#pragma hdrstop

#include "dragdrop.h"
#include <shlobj.h>

//---------------------------------------------------------------------------
LDataObject::LDataObject()
{
	m_nCount = 0;
   m_ArrFormatEtc = new LList();
   m_StgMedium = new LList();
}
//---------------------------------------------------------------------------
LDataObject::~LDataObject()
{
   DWORD dwPos;
   LPVOID p;

   if(m_ArrFormatEtc != NULL)
       delete m_ArrFormatEtc;
   m_ArrFormatEtc = NULL;
   if(m_StgMedium != NULL){
       p = m_StgMedium->GetFirstItem(&dwPos);
       while(p != NULL){
           ReleaseStgMedium((STGMEDIUM *)p);
           p = m_StgMedium->GetNextItem(&dwPos);
       }
       delete m_StgMedium;
   }
   m_StgMedium = NULL;
}
//---------------------------------------------------------------------------
STDMETHODIMP LDataObject::SetData(LPFORMATETC pformatetc,LPSTGMEDIUM pmedium,BOOL fRelease)
{
   if(pformatetc == NULL || pmedium == NULL)
       return E_INVALIDARG;

	FORMATETC* fetc = new FORMATETC;
	STGMEDIUM* pStgMed = new STGMEDIUM;
   if(fetc == NULL || pStgMed == NULL)
       return E_OUTOFMEMORY;
	ZeroMemory(fetc,sizeof(FORMATETC));
	ZeroMemory(pStgMed,sizeof(STGMEDIUM));
	*fetc = *pformatetc;
	m_ArrFormatEtc->Add((LPVOID)fetc);
   if(fRelease)
       *pStgMed = *pmedium;
   else
		CopyMedium(pStgMed, pmedium, pformatetc);
	m_StgMedium->Add((LPVOID)pStgMed);
   return S_OK;
}
//---------------------------------------------------------------------------
STDMETHODIMP LDataObject::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	if(riid == IID_IUnknown || riid == IID_IDataObject){
       AddRef();
		*ppvObj = this;
		return NOERROR;
   }
	*ppvObj = NULL;
	return ResultFromScode(E_NOINTERFACE);
}
//---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) LDataObject::AddRef()
{
	return ++m_nCount;
}
//---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) LDataObject::Release()
{
   if(--m_nCount == 0){
		delete this;
		return 0;
	}
	return m_nCount;
}
//---------------------------------------------------------------------------
STDMETHODIMP LDataObject::EnumDAdvise(LPENUMSTATDATA *ppenumAdvise)
{
   return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}
//---------------------------------------------------------------------------
STDMETHODIMP LDataObject::GetCanonicalFormatEtc(LPFORMATETC pformatetc,LPFORMATETC pformatetcOut)
{
   if(pformatetcOut == NULL)
		return E_INVALIDARG;
	return DATA_S_SAMEFORMATETC;
}
//---------------------------------------------------------------------------
STDMETHODIMP LDataObject::DUnadvise(DWORD dwConnection)
{
   return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}
//---------------------------------------------------------------------------
STDMETHODIMP LDataObject::DAdvise(FORMATETC *pFormatetc, DWORD advf,LPADVISESINK pAdvSink, DWORD *pdwConnection)
{
   return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}
//---------------------------------------------------------------------------
STDMETHODIMP LDataObject::QueryGetData(LPFORMATETC petc)
{
   LPFORMATETC p;
   DWORD dwPos;
	HRESULT hr = DV_E_TYMED;

	if(petc == NULL || m_ArrFormatEtc == NULL || m_ArrFormatEtc->Count() == 0)
		return E_INVALIDARG;
	if(!(DVASPECT_CONTENT & petc->dwAspect))
       return DV_E_DVASPECT;
   p = (LPFORMATETC)m_ArrFormatEtc->GetFirstItem(&dwPos);
   while(p != NULL){
       if(petc->tymed & p->tymed){
           if(petc->cfFormat == p->cfFormat)
               return S_OK;
		    else
			    hr = DV_E_CLIPFORMAT;
	    }
	    else
		    hr = DV_E_TYMED;
       p = (LPFORMATETC)m_ArrFormatEtc->GetNextItem(&dwPos);
   }
	return hr;
}
//---------------------------------------------------------------------------
STDMETHODIMP LDataObject::EnumFormatEtc(DWORD dwDirection,LPENUMFORMATETC *ppenumFormatEtc)
{
   if(ppenumFormatEtc == NULL || m_ArrFormatEtc == NULL || m_ArrFormatEtc->Count() == 0)
       return E_POINTER;
	*ppenumFormatEtc=NULL;
	switch (dwDirection){
       case DATADIR_GET:
           *ppenumFormatEtc= new LEnumFormatEtc(m_ArrFormatEtc);
		    if(*ppenumFormatEtc == NULL)
			    return E_OUTOFMEMORY;
           (*ppenumFormatEtc)->AddRef();
       break;
	    case DATADIR_SET:
       default:
		    return E_NOTIMPL;
   }
   return S_OK;
}
//---------------------------------------------------------------------------
void LDataObject::CopyMedium(STGMEDIUM* pMedDest, STGMEDIUM* pMedSrc, FORMATETC* pFmtSrc)
{
   switch(pMedSrc->tymed){
       case TYMED_HGLOBAL:
           pMedDest->hGlobal = (HGLOBAL)OleDuplicateData(pMedSrc->hGlobal,pFmtSrc->cfFormat, NULL);
       break;
		case TYMED_GDI:
           pMedDest->hBitmap = (HBITMAP)OleDuplicateData(pMedSrc->hBitmap,pFmtSrc->cfFormat, NULL);
       break;
		case TYMED_MFPICT:
           pMedDest->hMetaFilePict = (HMETAFILEPICT)OleDuplicateData(pMedSrc->hMetaFilePict,pFmtSrc->cfFormat, NULL);
       break;
		case TYMED_ENHMF:
           pMedDest->hEnhMetaFile = (HENHMETAFILE)OleDuplicateData(pMedSrc->hEnhMetaFile,pFmtSrc->cfFormat, NULL);
       break;
		case TYMED_FILE:
			pMedSrc->lpszFileName = (LPOLESTR)OleDuplicateData(pMedSrc->lpszFileName,pFmtSrc->cfFormat, NULL);
       break;
		case TYMED_ISTREAM:
			pMedDest->pstm = pMedSrc->pstm;
			pMedSrc->pstm->AddRef();
       break;
		case TYMED_ISTORAGE:
			pMedDest->pstg = pMedSrc->pstg;
			pMedSrc->pstg->AddRef();
       break;
		case TYMED_NULL:
		default:
			break;
   }
	pMedDest->tymed = pMedSrc->tymed;
	pMedDest->pUnkForRelease = NULL;
	if(pMedSrc->pUnkForRelease != NULL){
       pMedDest->pUnkForRelease = pMedSrc->pUnkForRelease;
		pMedSrc->pUnkForRelease->AddRef();
   }
}
//---------------------------------------------------------------------------
STDMETHODIMP LDataObject::GetData(LPFORMATETC pformatetcIn,LPSTGMEDIUM pmedium)
{
   DWORD dwPos,i;
   LPFORMATETC p;

   if(pformatetcIn == NULL || pmedium == NULL || m_ArrFormatEtc == NULL || m_ArrFormatEtc->Count() == 0)
		return E_INVALIDARG;
	pmedium->tymed = TYMED_NULL;
	pmedium->pUnkForRelease = NULL;
	pmedium->hGlobal = NULL;
   p = (LPFORMATETC)m_ArrFormatEtc->GetFirstItem(&dwPos);
   i = 1;
   while(p != NULL){
       if((pformatetcIn->tymed & p->tymed) && pformatetcIn->cfFormat == p->cfFormat && pformatetcIn->dwAspect == p->dwAspect){
           CopyMedium(pmedium,(LPSTGMEDIUM)m_StgMedium->GetItem(i),p);
			return S_OK;
       }
       p = (LPFORMATETC)m_ArrFormatEtc->GetNextItem(&dwPos);
       i++;
   }
	return DV_E_FORMATETC;
}
//---------------------------------------------------------------------------
STDMETHODIMP LDataObject::GetDataHere(LPFORMATETC pformatetc,LPSTGMEDIUM pmedium)
{
   return E_NOTIMPL;
}
//---------------------------------------------------------------------------
LEnumFormatEtc::LEnumFormatEtc()
{
   m_cRefCount = 0;
   m_iCur = 0;
   m_pFmtEtc = new LList();
}
//---------------------------------------------------------------------------
LEnumFormatEtc::LEnumFormatEtc(LList *pList) : m_cRefCount(0),m_iCur(0)
{
   DWORD dwPos;
   LPFORMATETC p;

   m_pFmtEtc = new LList();
   p = (LPFORMATETC)pList->GetFirstItem(&dwPos);
   while(p != NULL){
       m_pFmtEtc->Add((LPVOID)p);
       p = (LPFORMATETC)pList->GetNextItem(&dwPos);
   }
}
//---------------------------------------------------------------------------
STDMETHODIMP LEnumFormatEtc::QueryInterface(REFIID refiid, void FAR* FAR* ppv)
{
   *ppv = NULL;
   if(IID_IUnknown==refiid || IID_IEnumFORMATETC==refiid)
       *ppv=this;
   if(*ppv != NULL){
       ((LPUNKNOWN)*ppv)->AddRef();
       return S_OK;
   }
   return E_NOINTERFACE;
}
//---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) LEnumFormatEtc::AddRef(void)
{
   return ++m_cRefCount;
}
//---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) LEnumFormatEtc::Release(void)
{
   long nTemp = --m_cRefCount;
   if(nTemp == 0)
       delete this;
   return nTemp;
}
//---------------------------------------------------------------------------
STDMETHODIMP LEnumFormatEtc::Next(ULONG celt,LPFORMATETC lpFormatEtc, ULONG FAR *pceltFetched)
{
   if(pceltFetched != NULL)
       *pceltFetched=0;
   ULONG cReturn = celt;
   if(celt <= 0 || lpFormatEtc == NULL || m_iCur >= m_pFmtEtc->Count())
       return S_FALSE;
   if(pceltFetched == NULL && celt != 1)
       return S_FALSE;
	while (m_iCur < m_pFmtEtc->Count() && cReturn > 0){
		CopyMemory(lpFormatEtc++,m_pFmtEtc->GetItem(m_iCur+1),sizeof(FORMATETC));
       m_iCur++;
		--cReturn;
	}
   if (pceltFetched != NULL)
       *pceltFetched = celt - cReturn;
   return (cReturn == 0) ? S_OK : S_FALSE;
}
//---------------------------------------------------------------------------
STDMETHODIMP LEnumFormatEtc::Skip(ULONG celt)
{
   if((m_iCur + celt) >= m_pFmtEtc->Count())
       return S_FALSE;
   m_iCur += celt;
	return S_OK;
}
//---------------------------------------------------------------------------
STDMETHODIMP LEnumFormatEtc::Reset(void)
{
   m_iCur = 0;
   return S_OK;
}
//---------------------------------------------------------------------------
STDMETHODIMP LEnumFormatEtc::Clone(IEnumFORMATETC FAR * FAR*ppCloneEnumFormatEtc)
{
  if(ppCloneEnumFormatEtc == NULL)
      return E_POINTER;
  LEnumFormatEtc *newEnum = new LEnumFormatEtc(m_pFmtEtc);
  if(newEnum == NULL)
		return E_OUTOFMEMORY;
  newEnum->AddRef();
  newEnum->m_iCur = m_iCur;
  *ppCloneEnumFormatEtc = newEnum;
  return S_OK;
}
//---------------------------------------------------------------------------
STDMETHODIMP LDropSource::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
	if(riid == IID_IUnknown || riid == IID_IDropSource){
       AddRef();
		*ppvObj = this;
		return NOERROR;
   }
	*ppvObj = NULL;
	return ResultFromScode(E_NOINTERFACE);
}
//---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) LDropSource::AddRef()
{
	return ++m_nCount;
}
//---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) LDropSource::Release()
{
   if(--m_nCount == 0){
		delete this;
		return 0;
	}
	return m_nCount;                               
}
//---------------------------------------------------------------------------
STDMETHODIMP LDropSource::QueryContinueDrag(BOOL fEscapePressed,DWORD grfKeyState)
{
   if(fEscapePressed)
       return ResultFromScode(DRAGDROP_S_CANCEL);
	else if(!(grfKeyState & MK_LBUTTON))
		return ResultFromScode(DRAGDROP_S_DROP);
	else
		return NOERROR;
}
//---------------------------------------------------------------------------
STDMETHODIMP LDropSource::GiveFeedback(DWORD dwEffect)
{
	return ResultFromScode(DRAGDROP_S_USEDEFAULTCURSORS);
}
