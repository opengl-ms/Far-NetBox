#pragma once

//---------------------------------------------------------------------------
#define _int64 __int64
//---------------------------------------------------------------------------

#ifndef MPEXT
#define MPEXT
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#define MPEXT_NO_ZLIB
#define MPEXT_NO_GSS
#define MPEXT_NO_SFTP
#define MPEXT_NO_IDENT
#define MPEXT_NO_CACHE
#define MPEXT_NO_SPEED_LIM_RULES
#define MPEXT_NO_SSLDLL
#define _AFX_NOFORCE_LIBS
#define _MPT(T) _T(T)
#define _MPAT(T) T
//---------------------------------------------------------------------------
#if defined(__BORLANDC__)
#define GetOption(OPTION) GetInstanceOption(this->m_pApiLogParent, OPTION)
#define GetOptionVal(OPTION) GetInstanceOptionVal(this->m_pApiLogParent, OPTION)
#endif
//---------------------------------------------------------------------------
#define LENOF(x) ( (sizeof((x))) / (sizeof(*(x))))
//---------------------------------------------------------------------------
#define _ATL_MIN_CRT
#define _ATL_NO_DEFAULT_LIBS

#include "afxpriv.h"
#include "afxole.h"
#include "..\src\mfc\oleimpl2.h"
#include "..\src\mfc\afximpl.h"

#include <afx.h>
#include "wtypes.h"
#include <afxmt.h>

//STL includes
#include <list.h>
#include <map.h>
#include <vector.h>
//#include <deque>
#include <set.h>
#include <algorithm>
//---------------------------------------------------------------------------
class CFileFix;
#define CFile CFileFix
//---------------------------------------------------------------------------
#ifdef _MSC_VER
#include <FzApiStructures.h>
#else
#include "MFC64bitFix.h"
#include <ApiLog.h>
#include <FileZillaOpt.h>
#include <Options.h>
#include <Crypt.h>
#include <TextsFileZilla.h>
#endif
//---------------------------------------------------------------------------
#include <oleauto.h>
#include <afxdisp.h>
#include <afxconv.h>
//---------------------------------------------------------------------------
#define _strlwr strlwr
#ifndef USEDPARAM
#if defined(__BORLANDC__)
#define USEDPARAM(p) ((p) == (p))
#else
#define USEDPARAM(p) (void)(p);
#endif
#endif
//---------------------------------------------------------------------------
#ifdef _MSC_VER
extern HINSTANCE HInst;
#endif

//---------------------------------------------------------------------------
const int FILEEXISTS_ASK = -1;
const int FILEEXISTS_OVERWRITE = 0;
const int FILEEXISTS_OVERWRITEIFNEWER = 1;
const int FILEEXISTS_RESUME = 2;
const int FILEEXISTS_RENAME = 3;
const int FILEEXISTS_SKIP = 4;
const int FILEEXISTS_RESUME_ASKONFAIL = 5; // Used by queue for automatic resuming. If APPE failes, ask what to do instead.
const int FILEEXISTS_COMPLETE = 6;
//---------------------------------------------------------------------------
class t_ffam_statusmessage
{
public:
  CString status;
  int type;
  BOOL post;
};
//---------------------------------------------------------------------------
typedef struct
{
  __int64 bytes;
#ifdef MPEXT
  __int64 transfersize;
#endif
  int percent;
  int timeelapsed;
  int timeleft;
  int transferrate;
  BOOL bFileTransfer;
} t_ffam_transferstatus;
//---------------------------------------------------------------------------
#undef CFile
//---------------------------------------------------------------------------
class CFileFix : public CFile
{
public:
  explicit CFileFix() : CFile() {}
  explicit CFileFix(HANDLE hFile) : CFile(hFile) {}
  void SetCloseOnDelete(BOOL Value) { m_bCloseOnDelete = Value; }
  // MFC CFile::Read does not include file name into error message
  UINT Read(void * lpBuf, UINT nCount)
  {
    ASSERT_VALID(this);
    ASSERT(m_hFile != hFileNull);

    if (nCount == 0)
    {
      return 0;   // avoid Win32 "null-read"
    }

    ASSERT(lpBuf != NULL);
    ASSERT(AfxIsValidAddress(lpBuf, nCount));

    DWORD dwRead;
    if (!::ReadFile(m_hFile, lpBuf, nCount, &dwRead, NULL))
    {
      // The only change from MFC CFile::Read is m_strFileName
      CFileException::ThrowOsError((LONG)::GetLastError(), m_strFileName);
    }

    return (UINT)dwRead;
  }

  // MFC allocates CObject (ancestor of CFile) with new, but deallocates with free,
  // what codeguard dislikes, this is fix, not sure if it is necessary for
  // release version, but probably causes no harm
  /*void operator delete(void* p)
  {
    delete p;
  }*/
};
//---------------------------------------------------------------------------
#define CFile CFileFix
//---------------------------------------------------------------------------
