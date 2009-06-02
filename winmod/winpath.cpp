/**
* @file    winpath.cpp
* @brief   ...
* @author  bbcallen
* @date    2009-03-07  15:52
*/

#include "stdafx.h"
#include "winpath.h"

#include <assert.h>
#include <winnetwk.h>
#include <shlobj.h>

#pragma comment(lib, "mpr.lib")

using namespace WinMod;


#define LEN_MID_ELLIPSES        4   // for "...\\"
#define LEN_END_ELLIPSES        3   // for "..."
#define MIN_CCHMAX              (LEN_MID_ELLIPSES + LEN_END_ELLIPSES)


HRESULT CWinPathApi::ExpandFullPathName(CString& strPathName)
{
    DWORD dwLen = ::GetFullPathName(strPathName, 0, NULL, NULL);
    if (0 == dwLen)
        return GetLastError() ? AtlHresultFromLastError() : E_FAIL;


    assert(dwLen <= WIN_PATH_MAX_UNICODE_PATH);
    if (dwLen > WIN_PATH_MAX_UNICODE_PATH)
        return E_UNEXPECTED;


    CString strFullPathName;
    dwLen = ::GetFullPathName(strPathName, dwLen, strFullPathName.GetBuffer(dwLen), NULL);
    if (0 == dwLen)
    {
        strFullPathName.ReleaseBuffer(0);
        return GetLastError() ? AtlHresultFromLastError() : E_FAIL;
    }

    strFullPathName.ReleaseBuffer();
    strPathName = strFullPathName;
    return S_OK;
}

HRESULT CWinPathApi::ExpandLongPathName(CString& strPathName)
{
    DWORD dwLen = ::GetLongPathName(strPathName, NULL, 0);
    if (0 == dwLen)
        return GetLastError() ? AtlHresultFromLastError() : E_FAIL;


    assert(dwLen <= WIN_PATH_MAX_UNICODE_PATH);
    if (dwLen > WIN_PATH_MAX_UNICODE_PATH)
        return E_UNEXPECTED;


    CString strLongPathName;
    dwLen = ::GetLongPathName(strPathName, strLongPathName.GetBuffer(dwLen), dwLen);
    if (0 == dwLen)
    {
        strLongPathName.ReleaseBuffer(0);
        return GetLastError() ? AtlHresultFromLastError() : E_FAIL;
    }


    strLongPathName.ReleaseBuffer();
    strPathName = strLongPathName;
    return S_OK;
}

void CWinPathApi::NormalizePathName(CString& strFullPathName)
{
    ExpandLongPathName(strFullPathName);
    strFullPathName.MakeLower();
}


BOOL CWinPathApi::ExpandSpecialFolderPathAtBeginning(CString& strPath, LPCWSTR lpPattern, int csidl)
{
    assert(lpPattern);

    if (0 != StrNCmpI(strPath, lpPattern, (int)wcslen(lpPattern)))
        return FALSE;

    CString strNormalizedPath;
    BOOL br = ::SHGetSpecialFolderPath(NULL, strNormalizedPath.GetBuffer(MAX_PATH), csidl, FALSE);
    if (!br)
    {
        strNormalizedPath.ReleaseBuffer(0);
        return FALSE;
    }
    strNormalizedPath.ReleaseBuffer();


    strNormalizedPath.AppendChar(L'\\');
    strNormalizedPath.Append((LPCWSTR)strPath + wcslen(lpPattern));
    strPath = strNormalizedPath;
    return TRUE;
}

BOOL CWinPathApi::ExpandPatternAtBeginning(CString& strPath, LPCWSTR lpPattern, LPCWSTR lpExpandAs)
{
    assert(lpPattern);
    assert(lpExpandAs);

    if (0 != StrNCmpI(strPath, lpPattern, (int)wcslen(lpPattern)))
        return FALSE;

    CString strOrigPath;
    strOrigPath.Append(lpExpandAs);
    strOrigPath.Append((LPCWSTR)strPath + wcslen(lpPattern));
    strPath = strOrigPath;
    return TRUE;
}

BOOL CWinPathApi::ExpandEnvironmentStrings(CString& strPath)
{
    DWORD dwLen = ::ExpandEnvironmentStrings(strPath, NULL, 0);
    if (0 == dwLen)
        return GetLastError() ? AtlHresultFromLastError() : E_FAIL;


    assert(dwLen <= WIN_PATH_MAX_UNICODE_PATH);
    if (dwLen > WIN_PATH_MAX_UNICODE_PATH)
        return E_UNEXPECTED;


    CString strLongPathName;
    dwLen = ::ExpandEnvironmentStrings(strPath, strLongPathName.GetBuffer(dwLen), dwLen);
    if (0 == dwLen)
    {
        strLongPathName.ReleaseBuffer(0);
        return GetLastError() ? AtlHresultFromLastError() : E_FAIL;
    }


    strLongPathName.ReleaseBuffer();
    strPath = strLongPathName;
    return S_OK;
}

BOOL CWinPathApi::ExpandAsAccessiblePath(CString& strPath)
{
    if (ExpandSpecialFolderPathAtBeginning(strPath, L"System32\\", CSIDL_SYSTEM))
    {
        return TRUE;
    }
    else if (ExpandSpecialFolderPathAtBeginning(strPath, L"\\SystemRoot\\", CSIDL_WINDOWS))
    {
        return TRUE;
    }
    else if (ExpandPatternAtBeginning(strPath, L"\\??\\", L""))
    {
        return TRUE;
    }

    return FALSE;
}

BOOL CWinPathApi::HasUnicodePrefix(LPCWSTR pszPath)
{
    if (!pszPath)
        return FALSE;

    if (0 != StrCmpNW(pszPath, L"\\\\?\\", 4))
        return FALSE;

    return TRUE;
}


LPCWSTR CWinPathApi::FindAfterAnyPrefix(LPCWSTR pszPath)
{
    if (!pszPath || !*pszPath)
        return pszPath;

    if (HasUnicodePrefix(pszPath))
    {
        pszPath += WIN_PATH_UNICODE_PATH_PREFIX;
        if (0 == StrCmpNIW(pszPath, L"UNC\\", 4))
        {
            pszPath += 4;
        }
    }
    else if (IsUNC(pszPath))
    {
        pszPath += 2;
    }

    return pszPath;
}

LPCWSTR CWinPathApi::FindAfterUnicodePrefix(LPCWSTR pszPath)
{
    if (!pszPath || !*pszPath)
        return pszPath;

    if (HasUnicodePrefix(pszPath))
    {
        pszPath += WIN_PATH_UNICODE_PATH_PREFIX;
    }

    return pszPath;
}

LPCWSTR CWinPathApi::FindFileName(LPCWSTR pszPath)
{
    if (!pszPath)
        return pszPath;

    if (L'\\' == pszPath[0] && L'\\' == pszPath[1] && L'\0' == pszPath[2])
        return pszPath;

    pszPath = FindAfterAnyPrefix(pszPath);
    LPCWSTR lpFound = pszPath;
    for (NULL; *pszPath; ++pszPath)
    {
        if (L'\\' == pszPath[0] ||
            L':'  == pszPath[0] ||
            L'/'  == pszPath[0])
        {
            if (L'\0' != pszPath[1] &&
                L'\\' != pszPath[1] &&
                L'/'  != pszPath[1])
            {
                lpFound = pszPath + 1;
            }
        }
    }

    return lpFound;
}

LPCWSTR CWinPathApi::FindExtension(LPCWSTR pszPath)
{
    if (!pszPath)
        return NULL;

    LPCWSTR lpExtension = NULL;
    for (NULL; *pszPath; ++pszPath)
    {
        switch(*pszPath)
        {
        case L'.':              // find last dot
            lpExtension = pszPath;
            break;

        case L'\\':
        case L' ':              // extensions can't have spaces
            lpExtension = NULL;
            break;
        }
    }

    return lpExtension ? lpExtension : pszPath;
}

HRESULT CWinPathApi::CreateLnkFile(LPCWSTR pszPath, LPCWSTR pszDesc, LPCWSTR pszLnkFilePath)
{
    assert(pszPath);
    assert(pszDesc);
    assert(pszLnkFilePath);

    CComPtr<IShellLink> spiShellLink;

    CString strFullPath = pszPath;
    HRESULT hr = ExpandFullPathName(strFullPath);
    if (FAILED(hr))
        return hr;


    hr = ::CoCreateInstance(
        CLSID_ShellLink,
        NULL, 
        CLSCTX_INPROC_SERVER,
        IID_IShellLink,
        (void**)&spiShellLink);
    if (FAILED(hr))
        return hr;


    CComPtr<IPersistFile> spiPersistFile;
    hr = spiShellLink.QueryInterface(&spiPersistFile);
    if (FAILED(hr))
        return hr;

    // Set the path to the shortcut target and add the description
    hr = spiShellLink->SetPath(strFullPath);
    if (FAILED(hr))
        return hr;

    hr = spiShellLink->SetDescription(pszDesc);
    if (FAILED(hr))
        return hr;


    // Write the shortcut to disk
    hr = spiPersistFile->Save(pszLnkFilePath, TRUE);
    if (FAILED(hr))
        return hr;


    return S_OK;

}

HRESULT CWinPathApi::ResolveLnkFile(LPCWSTR pszLnkFile, CString& strTargetPath, DWORD dwFlag)
{
    assert(pszLnkFile);


    CComPtr<IShellLink> spiShellLink;
    strTargetPath = L"";


    // Get a pointer to the IShellLink interface
    HRESULT hr = ::CoCreateInstance(
        CLSID_ShellLink,
        NULL, 
        CLSCTX_INPROC_SERVER,
        IID_IShellLink,
        (void**)&spiShellLink); 
    if (FAILED(hr))
        return hr;


    CComPtr<IPersistFile> spiPersistFile;
    hr = spiShellLink.QueryInterface(&spiPersistFile);
    if (FAILED(hr))
        return hr;


    // Open the shortcut file and initialize it from its contents
    hr = spiPersistFile->Load(pszLnkFile, STGM_READ); 
    if (FAILED(hr))
        return hr;


    // Try to find the target of a shortcut, even if it has been moved or renamed
    hr = spiShellLink->Resolve(NULL, dwFlag);
    if (FAILED(hr))
        return hr;



    // Get the path to the shortcut target
    WIN32_FIND_DATA findData;
    hr = spiShellLink->GetPath(
        strTargetPath.GetBuffer(32768), 
        32768,
        &findData,
        SLGP_RAWPATH);
    if (FAILED(hr))
    {
        strTargetPath.ReleaseBuffer(0);
        return hr;
    }
    strTargetPath.ReleaseBuffer(-1);


    return S_OK;
}

BOOL CWinPathApi::IsDots(LPCWSTR pszPath)
{
    if (!pszPath || !*pszPath)
        return FALSE;

    if (L'.' != pszPath[0])
        return FALSE;

    if (pszPath[1] == L'\0')
    {
        return TRUE;
    }
    else if (pszPath[1] == L'.' &&
             pszPath[2] == L'\0')
    {
        return TRUE;
    }

    return FALSE;
}

// copied from source code of win2k
BOOL CWinPathApi::IsDirectory(LPCWSTR pszPath)
{
    if (!pszPath || !*pszPath)
        return FALSE;


    if (IsUNCServer(pszPath))
    {
        return FALSE;
    }
    else if (IsUNCServerShare(pszPath))
    {
        union {
            NETRESOURCE nr;
            TCHAR buf[512];
        } nrb;

        LPTSTR lpSystem;
        DWORD dwRet;
        DWORD dwSize = sizeof(nrb);

        nrb.nr.dwScope = RESOURCE_GLOBALNET;
        nrb.nr.dwType = RESOURCETYPE_ANY;
        nrb.nr.dwDisplayType = 0;
        nrb.nr.lpLocalName = NULL;
        nrb.nr.lpRemoteName = (LPTSTR)pszPath;
        nrb.nr.lpProvider = NULL;
        nrb.nr.lpComment = NULL;

        dwRet = ::WNetGetResourceInformation(&nrb.nr, &nrb, &dwSize, &lpSystem);

        if (dwRet != WN_SUCCESS)
            goto TryGetFileAttrib;

        if (nrb.nr.dwDisplayType == RESOURCEDISPLAYTYPE_GENERIC)
            goto TryGetFileAttrib;

        if ((nrb.nr.dwDisplayType == RESOURCEDISPLAYTYPE_SHARE) &&
            ((nrb.nr.dwType == RESOURCETYPE_ANY) ||
            (nrb.nr.dwType == RESOURCETYPE_DISK)))
        {
            return TRUE;
        }
    }
    else
    {
        DWORD dwAttribs;
TryGetFileAttrib:

        dwAttribs = ::GetFileAttributes(pszPath);
        if (INVALID_FILE_ATTRIBUTES != dwAttribs)
            return (BOOL)(dwAttribs & FILE_ATTRIBUTE_DIRECTORY);
    }

    return FALSE;
}

BOOL CWinPathApi::IsRelative(LPCWSTR pszPath)
{
    if (!pszPath || !*pszPath)
        return TRUE;    // The NULL path is assumed relative

    if (L'\\' == pszPath[0])
        return FALSE;   // Does it begin with a slash ?

    if (L'\0' != pszPath[0] && L':' == pszPath[1])
        return FALSE;   // Does it begin with a drive and a colon ?

    return TRUE;
}

BOOL CWinPathApi::IsRoot(LPCWSTR pszPath)
{
    if (!pszPath || !*pszPath)
        return FALSE;

    // single '\'
    if (L'\\' == pszPath[0] && L'\0' == pszPath[1])
        return TRUE;

    if (IsUNC(pszPath))
    {
        pszPath = FindAfterAnyPrefix(pszPath);
        assert(pszPath);

        // nothing more
        if (L'\0' == pszPath[0])
            return TRUE;

        // prefix + "\"
        if (L'\\' == pszPath[0] && L'\0' == pszPath[1])
            return FALSE;

        LPCWSTR lpNextBackSlash = ::StrChrW(pszPath + 1, L'\\');
        if (!lpNextBackSlash)
            return TRUE;    // 只有服务器名

        // prefix + "\\"
        if (lpNextBackSlash == pszPath)
            return FALSE;

        // \\abc\?
        if (L'\0' == lpNextBackSlash[1])
            return FALSE;

        if (L'\\' == lpNextBackSlash[1])
            return FALSE;

        if (::StrChrW(lpNextBackSlash + 2, L'\\'))
            return FALSE; // too many backslash

        return TRUE;
    }
    else
    {   // "\\?\"
        pszPath = FindAfterUnicodePrefix(pszPath);
        assert(pszPath);
    }

    // for normal
    if (L'\0' != pszPath[0])
    {
        if (L':' == pszPath[1])
        {
            if (L'\\' == pszPath[2] && L'\0' == pszPath[3])
                return TRUE;
        }
    }


    return FALSE;
}

BOOL CWinPathApi::IsUNC(LPCWSTR pszPath)
{
    if (!pszPath || !*pszPath)
        return FALSE;

    if (CWinPathApi::HasUnicodePrefix(pszPath))
    {
        pszPath += WIN_PATH_UNICODE_PATH_PREFIX;
        if (0 == StrCmpNIW(pszPath, L"UNC\\", 4))
            return TRUE;

        return FALSE;
    }

    if (L'\\' == pszPath[0])
    {
        if (L'\\' == pszPath[1])
        {
            if (L'?' != pszPath[2])
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

BOOL CWinPathApi::IsUNCServer(LPCWSTR pszPath)
{
    if (!pszPath || !*pszPath)
        return FALSE;

    if (!IsUNC(pszPath))
        return FALSE;

    pszPath = FindAfterAnyPrefix(pszPath);
    pszPath = ::StrChrW(pszPath, L'\\');
    if (pszPath)
        return FALSE;

    return TRUE;
}

BOOL CWinPathApi::IsUNCServerShare(LPCWSTR pszPath)
{
    if (!pszPath || !*pszPath)
        return FALSE;

    if (!IsUNC(pszPath))
        return FALSE;

    pszPath = FindAfterAnyPrefix(pszPath);
    pszPath = ::StrChrW(pszPath, L'\\');
    if (!pszPath)
        return FALSE;

    pszPath = ::StrChrW(pszPath + 1, L'\\');
    if (pszPath)
        return FALSE;

    return TRUE;
}

BOOL CWinPathApi::IsFileExisting(LPCWSTR pszPath)
{
    if (!pszPath || !*pszPath)
        return FALSE;

    DWORD dwAttr = ::GetFileAttributes(pszPath);
    if (INVALID_FILE_ATTRIBUTES == dwAttr)
        return FALSE;

    return TRUE;
}

BOOL CWinPathApi::IsLnkFile(LPCWSTR pszPath)
{
    if (!pszPath)
        return FALSE;

    LPCWSTR lpszExtension = FindExtension(pszPath);
    if (0 != StrCmpIW(lpszExtension, L".lnk"))
        return FALSE;

    return TRUE;
}







void CWinPath::AddBackslash()
{
    if (m_strPath.IsEmpty())
        return;

    int nLen = m_strPath.GetLength();
    if (L'\\' == m_strPath[nLen - 1])
        return;  

    m_strPath.Append(L"\\");
}


BOOL CWinPath::AddExtension(LPCWSTR pszExtension)
{
    LPCWSTR lpOldExtension = CWinPathApi::FindExtension(m_strPath);
    assert(lpOldExtension);
    if (*lpOldExtension)
        return FALSE;   // extension already exist

    if (pszExtension == NULL)
        pszExtension = L".exe";

    m_strPath.Append(pszExtension);
    return TRUE;
}

BOOL CWinPath::Append(LPCWSTR pszMore)
{
    if (!pszMore)
        return FALSE;

    CWinPath pathResult;
    pathResult.Combine(m_strPath, pszMore);

    m_strPath = pathResult.m_strPath;
    return TRUE;
}

void CWinPath::BuildRoot(int iDrive)
{
    if (iDrive >= 0 && iDrive <= 26)
    {
        m_strPath = L"";
        m_strPath.AppendChar(L'A' + (WCHAR)iDrive);
        m_strPath.Append(L":\\");
    }
}

void CWinPath::Canonicalize()
{
    CString strResult;
    LPCWSTR lpszSource = (LPCWSTR)m_strPath;

    // copy prefix
    LPCWSTR lpAfterPrefix = CWinPathApi::FindAfterAnyPrefix(m_strPath);
    if (lpszSource != lpAfterPrefix)
    {
        strResult.Append(lpszSource, int(lpAfterPrefix - lpszSource));
        lpszSource = lpAfterPrefix;
    }


    while (lpszSource[0])
    {
        LPCWSTR lpPathNodeEnd = StrChrW(lpszSource, L'\\');
        if (!lpPathNodeEnd)
        {
            lpPathNodeEnd = (LPCWSTR)m_strPath + m_strPath.GetLength();
        }

        int cbPathNodeCount = int(lpPathNodeEnd - lpszSource);

        if (1 == cbPathNodeCount && L'.' == lpszSource[0])    
        {   // single dot
            lpszSource = lpPathNodeEnd;

            // skip backslash
            if (L'\\' == lpszSource[0])
                ++lpszSource;
        }
        else if (2 == cbPathNodeCount && L'.' == lpszSource[0] && L'.' == lpszSource[1])
        {
            if (!CWinPathApi::IsRoot(strResult))
            {
                int nPos = strResult.ReverseFind(L'\\');
                if (-1 != nPos)
                    strResult.Truncate(nPos + 1);
            }

            lpszSource = lpPathNodeEnd;

            // skip backslash
            if (L'\\' == lpszSource[0])
                ++lpszSource;
        }
        else
        {
            strResult.Append(lpszSource, cbPathNodeCount);
            lpszSource = lpPathNodeEnd;

            if (L'\0' != lpszSource[0])
            {
                strResult.AppendChar(lpszSource[0]);
                ++lpszSource;
            }
        }
    }


    
    int nLen = strResult.GetLength();
    if (strResult.IsEmpty())
    {   // empty path
        strResult = L'\\';
    }
    else if (L':' == strResult[nLen - 1])
    {
        LPCWSTR lpNewAfterPrefix = CWinPathApi::FindAfterAnyPrefix(strResult);
        if (L'\0' != lpNewAfterPrefix[0] && 
            L':'  == lpNewAfterPrefix[1] &&
            L'\0' == lpNewAfterPrefix[2])
        {   // prefix + L"X:"
            strResult.AppendChar(L'\\');
        }
    }
    else if (L'.' == strResult[nLen - 1])
    {
        while (nLen && L'.' == strResult[nLen - 1])
        {
            --nLen;
        }

        strResult.Truncate(nLen);
    }

    m_strPath = strResult;
}



void CWinPath::Combine(LPCWSTR pszDir, LPCWSTR pszFile)
{
    if (!pszDir && !pszFile)
        return;

    if (!pszDir)
        pszDir = L"";

    if (!pszFile)
        pszFile = L"";

    CWinPath pathResult;
    if (!*pszFile)
    {
        pathResult = pszDir;
    }
    else if (*pszDir && CWinPathApi::IsRelative(pszFile))
    {
        pathResult = pszDir;
        pathResult.AddBackslash();
        pathResult.m_strPath.Append(pszFile);
    }
    else if (*pszDir && L'\\' == *pszFile && !CWinPathApi::IsUNC(pszFile))
    {
        // BUGBUG: Note that we do not check that an actual root is returned;
        // it is assumed that we are given valid parameters
        pathResult = pszDir;
        pathResult.StripToRoot();
        if (!pathResult.IsRoot())
        {
            return;
        }

        pathResult.AddBackslash();
        pathResult.m_strPath.Append(pszFile + 1);
    }
    else 
    {
        pathResult.m_strPath.Append(pszFile);
    }


    pathResult.Canonicalize();
    m_strPath = pathResult.m_strPath;
}

BOOL CWinPath::CompactPathEx(UINT nMaxChars, DWORD dwFlags)
{
    UNREFERENCED_PARAMETER(dwFlags);
    if (m_strPath.IsEmpty())
        return TRUE;


    if (0 == nMaxChars)
    {
        m_strPath.Empty();
        return TRUE;
    }


    UINT uMaxLength = nMaxChars - 1;


    CString strResult;
    if (m_strPath.GetLength() < (int)uMaxLength)
    {
        return TRUE;
    }


    LPCWSTR pszFileName = CWinPathApi::FindFileName(m_strPath);
    UINT    uiFNLen     = UINT((LPCWSTR)m_strPath + m_strPath.GetLength() - pszFileName);


    // if the whole string is a file name
    if(pszFileName == (LPCWSTR)m_strPath && uMaxLength > LEN_END_ELLIPSES)
    {
        strResult.SetString(m_strPath, uMaxLength - LEN_END_ELLIPSES);
        strResult.Append(L"...");

        assert(0 == nMaxChars || strResult.GetLength() == int(uMaxLength));
        m_strPath = strResult;
        return TRUE;
    }


    // Handle all the cases where we just use ellipses ie '.' to '.../...'
    if ((uMaxLength <= MIN_CCHMAX))
    {
        strResult = L"";
        for (int n = 0; n < (int)uMaxLength - 1; ++n)
        {
            if ((n + 1) == LEN_MID_ELLIPSES)
            {
                strResult.AppendChar(L'\\');
            }
            else
            {
                strResult.AppendChar(L'.');
            }
        }

        assert(0 == nMaxChars || strResult.GetLength() == int(uMaxLength));
        m_strPath = strResult;
        return TRUE;
    }

    // Ok, how much of the path can we copy ? Buffer - (Length of MID_ELLIPSES + Len_Filename)
    int cchToCopy = uMaxLength - (LEN_MID_ELLIPSES + uiFNLen);

    if (cchToCopy < 0)
        cchToCopy = 0;


    strResult.SetString(m_strPath, cchToCopy);

    // Now throw in the ".../" or "...\"
    strResult.Append(L"...\\");

    //Finally the filename and ellipses if necessary
    if (uMaxLength > (LEN_MID_ELLIPSES + uiFNLen))
    {
        strResult.Append(pszFileName);
    }
    else
    {
        cchToCopy = uMaxLength - LEN_MID_ELLIPSES - LEN_END_ELLIPSES;

        strResult.Append(pszFileName, cchToCopy);
        strResult.Append(L"...");
    }

    assert(0 == nMaxChars || strResult.GetLength() == int(uMaxLength));
    m_strPath = strResult;
    return TRUE;
}

int CWinPath::FindExtension() const
{
    LPCWSTR lpszBuffer    = (LPCWSTR)m_strPath;
    LPCWSTR lpszExtension = NULL;

    lpszExtension = CWinPathApi::FindExtension(lpszBuffer);
    if(*lpszExtension == 0)
        return -1;
    else
        return int(lpszExtension - lpszBuffer);
}

int CWinPath::FindFileName() const
{
    LPCWSTR lpFound = CWinPathApi::FindFileName((LPCWSTR)m_strPath);
    if (!lpFound || !*lpFound)
        return -1;

    return int(lpFound - (LPCWSTR)m_strPath);
}

CString CWinPath::GetExtension() const
{
    return CWinPathApi::FindExtension(m_strPath);
}

BOOL CWinPath::IsDirectory() const
{
    return CWinPathApi::IsDirectory(m_strPath);
}

BOOL CWinPath::IsRelative() const
{
    return CWinPathApi::IsRelative(m_strPath);
}

BOOL CWinPath::IsRoot() const
{
    return CWinPathApi::IsRoot(m_strPath);
}

BOOL CWinPath::IsUNC() const
{
    return CWinPathApi::IsUNC(m_strPath);
}

BOOL CWinPath::IsUNCServer() const
{
    return CWinPathApi::IsUNCServer(m_strPath);
}

BOOL CWinPath::IsUNCServerShare() const
{
    return CWinPathApi::IsUNCServerShare(m_strPath);
}

void CWinPath::RemoveArgs()
{
    for (int i = 0; i < m_strPath.GetLength(); ++i)
    {
        switch (m_strPath[i])
        {
        case L'\"': // 不移除空格中的引号
            return;
        case L' ' :  // 遇到空格截断
            m_strPath.Truncate(i + 1);
            m_strPath.TrimRight();
            return;
        }
    }
}

void CWinPath::RemoveBackslash()
{
    if (m_strPath.IsEmpty())
        return;

    if (IsRoot())
        return;

    int nLen = m_strPath.GetLength();
    if (L'\\' == m_strPath[nLen - 1])
        m_strPath.Truncate(nLen - 1);
}

void CWinPath::RemoveExtension()
{
    int nPos = FindExtension();
    if (-1 == nPos)
        return;

    m_strPath.Truncate(nPos);
    m_strPath.TrimRight();
}

BOOL CWinPath::RemoveFileSpec()
{
    if (m_strPath.IsEmpty())
        return FALSE;


    LPCWSTR lpszPathBegin = CWinPathApi::FindAfterAnyPrefix(m_strPath);
    LPCWSTR lpszPathEnd   = (LPCWSTR)m_strPath + m_strPath.GetLength();
    assert(lpszPathBegin <= lpszPathEnd);


    // find after root
    if (L'\0' == lpszPathBegin[0])
    {
        NULL;
    }
    else if (L'\\' == lpszPathBegin[0] && L':' == lpszPathBegin[1])
    {
        if (L'\\' == lpszPathBegin[2])
        {
            if (L'\0' == lpszPathBegin[3])
            {
                lpszPathBegin += 2;     // prefix + "\:\"       <- strange implement in vista
            }
            else
            {
                lpszPathBegin += 3;     // go through "\:\"     <- strange implement in vista
            }
        }
    }
    else if (L':' == lpszPathBegin[1])
    {
        if (L'\0' == lpszPathBegin[2])
        {
            return FALSE;               // prefix + "C:"
        }
        else if (L'\\' == lpszPathBegin[2])
        {
            if (L'\0' == lpszPathBegin[3])
            {
                return FALSE;           // prefix + "C:\"
            }
            else
            {
                lpszPathBegin += 3;     // go through "C:\"
            }
        }
        else
        {

            lpszPathBegin += 2;         // go through "C:"
        }
    }


    LPCWSTR lpszRemoveBegin = StrRChrW(lpszPathBegin, lpszPathEnd, L'\\');
    if (!lpszRemoveBegin)
    {
        lpszRemoveBegin = lpszPathBegin;
    }
    else if ((LPCWSTR)m_strPath == lpszRemoveBegin)
    {
        ++lpszRemoveBegin;      // "\" + others
    }
    else if (lpszPathBegin < lpszRemoveBegin &&
             '\\' == *(lpszRemoveBegin - 1))
    {
        --lpszRemoveBegin;      // ..."\\"...   eat 2 backslash
    }


    if (L'\0' == lpszRemoveBegin)
        return FALSE;   // stripped nothing


    int nOldLength = m_strPath.GetLength();
    int nNewLength = int(lpszRemoveBegin - m_strPath);
    if (nOldLength == nNewLength)
        return FALSE;


    m_strPath.Truncate(nNewLength);
    return TRUE;
}

void CWinPath::StripPath()
{
    LPCWSTR lpFound = CWinPathApi::FindFileName(m_strPath);
    m_strPath = lpFound;
}

BOOL CWinPath::StripToRoot()
{
    // for vista
    if (L'\\' == m_strPath[0] &&
        L'\0' == m_strPath[1])
    {
        return TRUE;        // "\"
    }


    LPCWSTR lpAfterAnyPrefix = CWinPathApi::FindAfterAnyPrefix(m_strPath);
    assert(lpAfterAnyPrefix);


    // for vista
    if (L'\0' == lpAfterAnyPrefix[0])
    {
        NULL;
    }
    else if (L'\\' == lpAfterAnyPrefix[0] && L':' == lpAfterAnyPrefix[1])
    {   // strange implement in vista
        m_strPath.Truncate(int(lpAfterAnyPrefix + 1 - (LPCWSTR)m_strPath));
        return TRUE;            // prefix + "\:" + others
    }
    else if (L':' == lpAfterAnyPrefix[1])
    {
        if (L'\\' == lpAfterAnyPrefix[2])
        {
            m_strPath.Truncate(int(lpAfterAnyPrefix + 3 - (LPCWSTR)m_strPath));
            return TRUE;        // prefix + "X:\" + others
        }
        else
        {
            m_strPath.Truncate(int(lpAfterAnyPrefix + 2 - (LPCWSTR)m_strPath));
            return TRUE;        // prefix + "X:" + others
        }
    }


    if (IsUNC())
    {
        int nFirstBackSlashAfterUNC = m_strPath.Find(L'\\', int(lpAfterAnyPrefix - (LPCWSTR)m_strPath));
        if (-1 == nFirstBackSlashAfterUNC)
            return TRUE;        // "\\Server"


        if (L'\0' == m_strPath[nFirstBackSlashAfterUNC + 1])
        {
            m_strPath.Truncate(nFirstBackSlashAfterUNC);
            return TRUE;        // "\\Server\"
        }


        int nSecondBackSlashAfterUNC = m_strPath.Find(L'\\', nFirstBackSlashAfterUNC + 1);
        if (-1 == nSecondBackSlashAfterUNC)
            return TRUE;        // "\\Server\\foo"


        if (nSecondBackSlashAfterUNC == nFirstBackSlashAfterUNC + 1)
        {
            m_strPath.Truncate(nFirstBackSlashAfterUNC);
            return TRUE;        // "\\Server\\\\foo"
        }


        m_strPath.Truncate(nSecondBackSlashAfterUNC);
        return TRUE;
    }

    while(!IsRoot())
    {
        if (!RemoveFileSpec())
        {
            return FALSE;
        }
    }

    return TRUE;
}

void CWinPath::UnquoteSpaces()
{
    m_strPath.Trim();

    if (m_strPath.GetLength() < 2)
        return;

    if (L'\"' == m_strPath[m_strPath.GetLength() - 1] && L'\"' == m_strPath[0])
    {
        m_strPath.Truncate(m_strPath.GetLength() - 1);
        m_strPath = (LPCWSTR)m_strPath + 1;
    }
}







BOOL CWinPath::IsExisting() const
{
    return CWinPathApi::IsFileExisting(m_strPath);
}

void CWinPath::RemoveSingleArg()
{
    m_strPath.Trim();  // 先清除多余的空格
    for (int i = m_strPath.GetLength(); i > 0; --i)
    {
        switch (m_strPath[i - 1])
        {
        case L'\"': // 不移除空格中的引号
            return;
        case L' ':  // 遇到空格截断
            m_strPath.Truncate(i);
            m_strPath.TrimRight();
            return;
        }
    }
}









BOOL CWinPath::HasUnicodePrefix() const
{
    return CWinPathApi::HasUnicodePrefix(m_strPath);
}

void CWinPath::RemoveUnicodePrefix()
{
    if (CWinPathApi::HasUnicodePrefix(m_strPath))
    {
        LPCWSTR lpszAfterUnicodePrefix = (LPCWSTR)m_strPath + CWinPathApi::WIN_PATH_UNICODE_PATH_PREFIX;
        if (0 == StrCmpNIW(lpszAfterUnicodePrefix, L"UNC\\", 4))
        {   // remove 'UN', and set 'C' as '\\'
            m_strPath.SetAt(CWinPathApi::WIN_PATH_UNICODE_PATH_PREFIX + 2, L'\\');
            m_strPath = lpszAfterUnicodePrefix + 2;
        }
        else
        {
            m_strPath = lpszAfterUnicodePrefix;
        }
    }
}

void CWinPath::AddUnicodePrefix()
{
    if (HasUnicodePrefix())
        return;

    CString strResult;
    if (IsUNC())
    {
        assert(m_strPath.GetLength() >= 2);
        strResult = L"\\\\?\\UNC\\";
        strResult.Append((LPCWSTR)m_strPath + 2);
    }
    else
    {
        strResult = L"\\\\?\\";
        ExpandFullPathName();
        ExpandLongPathName();
        strResult.Append(m_strPath);
    }

    m_strPath = strResult;
}

CWinPath CWinPath::GetPathWithoutUnicodePrefix() const
{
    if (!HasUnicodePrefix())
        return (LPCWSTR)m_strPath;

    CWinPath pathResult = (LPCWSTR)m_strPath;
    pathResult.RemoveUnicodePrefix();
    return pathResult;
}

HRESULT CWinPath::ExpandFullPathName()
{
    return CWinPathApi::ExpandFullPathName(m_strPath);
}

HRESULT CWinPath::ExpandLongPathName()
{
    return CWinPathApi::ExpandLongPathName(m_strPath);
}
