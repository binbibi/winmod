/**
* @file    wintrustverifier.cpp
* @brief   ...
* @author  bbcallen
* @date    2009-02-11  17:22
*/

#include "stdafx.h"
#include "wintrustverifier.h"

#include <assert.h>
#include <imagehlp.h>
#include <atlfile.h>
#include <atlstr.h>

#pragma comment(lib, "imagehlp.lib")

using namespace WinMod;


DWORD CWinTrustVerifier::VerifyFile(LPCWSTR pwszFileFullPath)
{
    assert(pwszFileFullPath);

    CAtlFile hFile;
    HRESULT  hr = hFile.Create(
        pwszFileFullPath,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        OPEN_EXISTING);
    if (FAILED(hr))
        return hr;

    return VerifyFile(pwszFileFullPath, hFile);
}

DWORD CWinTrustVerifier::VerifyFile(LPCWSTR pwszFileFullPath, HANDLE hFile)
{
    assert(hFile && hFile != INVALID_HANDLE_VALUE);
    DWORD dwRet = 0;

    dwRet = VerifyWinTrustFile(pwszFileFullPath, hFile);
    if (ERROR_SUCCESS == dwRet)
        return ERROR_SUCCESS;

    //dwRet = VerifyDriverFile(pwszFileFullPath, hFile);
    //if (ERROR_SUCCESS == dwRet)
    //    return ERROR_SUCCESS;

    dwRet = VerifyCatalogSignature(pwszFileFullPath, hFile);
    if (ERROR_SUCCESS == dwRet)
        return ERROR_SUCCESS;

    return dwRet;
}

DWORD CWinTrustVerifier::VerifyFileFast(LPCWSTR pwszFileFullPath)
{
    assert(pwszFileFullPath);
    CAtlFile hFile;
    HRESULT  hr = hFile.Create(
        pwszFileFullPath,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        OPEN_EXISTING);
    if (FAILED(hr))
        return hr;

    return VerifyFileFast(pwszFileFullPath, hFile);
}

DWORD CWinTrustVerifier::VerifyFileFast(LPCWSTR pwszFileFullPath, HANDLE hFile)
{
    assert(hFile && hFile != INVALID_HANDLE_VALUE);
    DWORD dwRet = 0;

    // 判断数字签名是否被包含在PE文件内
    if (HasEmbeddedSignature(hFile))
    {
        dwRet = VerifyWinTrustFile(pwszFileFullPath, hFile);
        if (ERROR_SUCCESS == dwRet)
            return ERROR_SUCCESS;

        // 如果有嵌入的数字签名，仍然有可能包含cat方式的数字签名
        dwRet = VerifyCatalogSignature(pwszFileFullPath, hFile);
        if (ERROR_SUCCESS == dwRet)
            return ERROR_SUCCESS;
    }
    else
    {
        dwRet = VerifyCatalogSignature(pwszFileFullPath, hFile);
        if (ERROR_SUCCESS == dwRet)
            return ERROR_SUCCESS;
    }

    return dwRet;
}

DWORD CWinTrustVerifier::VerifyWinTrustFile(LPCWSTR pwszFileFullPath, HANDLE hFile)
{
    assert(hFile && hFile != INVALID_HANDLE_VALUE);
    GUID WinTrustVerifyGuid = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    return VerifyEmbeddedSignature(pwszFileFullPath, hFile, WinTrustVerifyGuid);
}

//DWORD CWinTrustVerifier::VerifyDriverFile(LPCWSTR pwszFileFullPath, HANDLE hFile)
//{
//    assert(hFile && hFile != INVALID_HANDLE_VALUE);
//    GUID DriverVerifyGuid = DRIVER_ACTION_VERIFY;
//    return VerifyEmbeddedSignature(pwszFileFullPath, hFile, DriverVerifyGuid);
//}


// modified from MSDN
DWORD CWinTrustVerifier::VerifyEmbeddedSignature(LPCWSTR pwszFileFullPath, HANDLE hFile, GUID& policyGUID)
{
    assert(pwszFileFullPath);
    assert(hFile && hFile != INVALID_HANDLE_VALUE);

    // Load wintrust.dll
    HRESULT hRet = TryLoadDll();
    if (FAILED(hRet))
        return (DWORD)hRet;



    // Initialize the WINTRUST_FILE_INFO structure.

    WINTRUST_FILE_INFO FileData;
    memset(&FileData, 0, sizeof(FileData));
    FileData.cbStruct       = sizeof(WINTRUST_FILE_INFO);
    FileData.pcwszFilePath  = pwszFileFullPath;
    FileData.hFile          = hFile;
    FileData.pgKnownSubject = NULL;

    /*
    WVTPolicyGUID specifies the policy to apply on the file
    WINTRUST_ACTION_GENERIC_VERIFY_V2 policy checks:
    
    1) The certificate used to sign the file chains up to a root 
    certificate located in the trusted root certificate store. This 
    implies that the identity of the publisher has been verified by 
    a certification authority.
    
    2) In cases where user interface is displayed (which this example
    does not do), WinVerifyTrust will check for whether the  
    end entity certificate is stored in the trusted publisher store,  
    implying that the user trusts content from this publisher.
    
    3) The end entity certificate has sufficient permission to sign 
    code, as indicated by the presence of a code signing EKU or no 
    EKU.
    */


    // Initialize the WinVerifyTrust input data structure.
    GUID          WVTPolicyGUID = policyGUID;
    WINTRUST_DATA WinTrustData;
    memset(&WinTrustData, 0, sizeof(WinTrustData));
    WinTrustData.cbStruct = sizeof(WinTrustData);
    WinTrustData.pPolicyCallbackData    = NULL;             // Use default code signing EKU.
    WinTrustData.pSIPClientData         = NULL;             // No data to pass to SIP.
    WinTrustData.dwUIChoice             = WTD_UI_NONE;      // Disable WVT UI.
    WinTrustData.fdwRevocationChecks    = WTD_REVOKE_NONE;  // No revocation checking.
    WinTrustData.dwUnionChoice          = WTD_CHOICE_FILE;  // Verify an embedded signature on a file.
    WinTrustData.dwStateAction          = 0;                // Default verification.
    WinTrustData.hWVTStateData          = NULL;             // Not applicable for default verification of embedded signature.
    WinTrustData.pwszURLReference       = NULL;             // Not used.
    WinTrustData.dwProvFlags            = WTD_SAFER_FLAG;   // Default.
    WinTrustData.dwUIContext            = 0;                // Not used.
    WinTrustData.pFile                  = &FileData;        // Set pFile.

    // WinVerifyTrust verifies signatures as specified by the GUID 
    // and Wintrust_Data.
    LONG lStatus = m_modWinTrust.WinVerifyTrust(
        NULL,
        &WVTPolicyGUID,
        &WinTrustData);

    return lStatus;
}


DWORD CWinTrustVerifier::VerifyCatalogSignature(LPCWSTR pwszFileFullPath, HANDLE hFile)
{
    assert(pwszFileFullPath);
    assert(hFile && hFile != INVALID_HANDLE_VALUE);

    // Load wintrust.dll
    CWinModule_wintrust modWinTrust;
    HRESULT hRet = modWinTrust.LoadLib(L"wintrust.dll");
    if (FAILED(hRet))
        return (DWORD)hRet;


    // Create verify context
    GUID      WVTPolicyGUID = DRIVER_ACTION_VERIFY;
    HCATADMIN hCatAdmin = NULL;
    BOOL bRet = modWinTrust.CryptCATAdminAcquireContext(
        &hCatAdmin,
        &WVTPolicyGUID,
        0);
    if (!bRet)
        return GetLastError() ? AtlHresultFromLastError() : E_FAIL;


    // Calculate hash of file
    BYTE  pbyHashBuf[BUFSIZ];
    DWORD dwHashSize = BUFSIZ;
    bRet = modWinTrust.CryptCATAdminCalcHashFromFileHandle(
        hFile,
        &dwHashSize,
        pbyHashBuf,
        0);
    if (!bRet)
    {
        modWinTrust.CryptCATAdminReleaseContext(hCatAdmin, 0);
        return GetLastError() ? AtlHresultFromLastError() : E_FAIL;
    }



    // Guess member tag
    CString strMemberFilePath = pwszFileFullPath;
    strMemberFilePath.MakeLower();
    LPCWSTR lpFileName = ::PathFindFileName(strMemberFilePath);


    // Initialize the WINTRUST_CATALOG_INFO structure.
    WINTRUST_CATALOG_INFO CatalogData;
    memset(&CatalogData, 0, sizeof(CatalogData));
    CatalogData.cbStruct                = sizeof(WINTRUST_CATALOG_INFO);
    CatalogData.dwCatalogVersion        = 0;                    // optional: Catalog version number
    CatalogData.pcwszCatalogFilePath    = NULL;                 // required: path/name to Catalog file
    CatalogData.pcwszMemberTag          = lpFileName;           // optional: tag to member in Catalog
    CatalogData.pcwszMemberFilePath     = strMemberFilePath;    // required: path/name to member file
    CatalogData.hMemberFile             = hFile;                // optional: open handle to pcwszMemberFilePath
    CatalogData.pbCalculatedFileHash    = pbyHashBuf;           // optional: pass in the calculated hash
    CatalogData.cbCalculatedFileHash    = dwHashSize;           // optional: pass in the count bytes of the calc hash
    CatalogData.pcCatalogContext        = NULL;                 // optional: pass in to use instead of CatalogFilePath.



    // Look up catalog
    HCATINFO hPrevCat = NULL;
    HCATINFO hCatInfo = modWinTrust.CryptCATAdminEnumCatalogFromHash(
        hCatAdmin,
        pbyHashBuf,
        dwHashSize,
        0,
        &hPrevCat);
    while (hCatInfo)
    {
        CATALOG_INFO CatInfo;
        memset(&CatInfo, 0, sizeof(CatInfo));
        CatInfo.cbStruct = sizeof(CATALOG_INFO);


        // retrieve catalog info
        bRet = modWinTrust.CryptCATCatalogInfoFromContext(hCatInfo, &CatInfo, 0);
        if (bRet)
        {
            CatalogData.pcwszCatalogFilePath = CatInfo.wszCatalogFile;


            // Initialize the WinVerifyTrust input data structure.
            WINTRUST_DATA WinTrustData;
            memset(&WinTrustData, 0, sizeof(WinTrustData));
            WinTrustData.cbStruct = sizeof(WinTrustData);
            WinTrustData.pPolicyCallbackData    = NULL;                         // Use default code signing EKU.
            WinTrustData.pSIPClientData         = NULL;                         // No data to pass to SIP.
            WinTrustData.dwUIChoice             = WTD_UI_NONE;                  // Disable WVT UI.
            WinTrustData.fdwRevocationChecks    = WTD_REVOKE_NONE;              // No revocation checking.
            WinTrustData.dwUnionChoice          = WTD_CHOICE_CATALOG;           // Verify signature embedded in a file.
            WinTrustData.dwStateAction          = 0;                            // Default verification.
            WinTrustData.hWVTStateData          = NULL;                         //
            WinTrustData.pwszURLReference       = NULL;                         // Not used.
            WinTrustData.dwProvFlags            = WTD_SAFER_FLAG;               // Default.
            WinTrustData.dwUIContext            = 0;                            // Not used.
            WinTrustData.pCatalog               = &CatalogData;                 // Set pCatalog.


            LONG lStatus = modWinTrust.WinVerifyTrust(
                NULL,
                &WVTPolicyGUID,
                &WinTrustData);


            if (ERROR_SUCCESS == lStatus)
            {   // we're done here
                modWinTrust.CryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfo, 0);
                modWinTrust.CryptCATAdminReleaseContext(hCatAdmin, 0);
                return ERROR_SUCCESS;
            }
        }


        // look up next catalog
        hPrevCat = hCatInfo;
        hCatInfo = modWinTrust.CryptCATAdminEnumCatalogFromHash(
            hCatAdmin,
            pbyHashBuf,
            dwHashSize,
            0,
            &hPrevCat);
    }

    if (hCatInfo)
        modWinTrust.CryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfo, 0);

    modWinTrust.CryptCATAdminReleaseContext(hCatAdmin, 0);
    return GetLastError() ? AtlHresultFromLastError() : E_FAIL;
}


BOOL CWinTrustVerifier::HasEmbeddedSignature(HANDLE hFile)
{
    assert(hFile && hFile != INVALID_HANDLE_VALUE);

    // Get Certificate Header
    WIN_CERTIFICATE CertHead;   
    CertHead.dwLength  = 0;   
    CertHead.wRevision = WIN_CERT_REVISION_1_0;   
    if (!::ImageGetCertificateHeader(hFile, 0, &CertHead))   
    {   
        return FALSE;
    }   

    return TRUE;
}

BOOL CWinTrustVerifier::IsPEFile(LPCWSTR pwszFileFullPath)
{
    assert(pwszFileFullPath);

    CAtlFile hFile;
    HRESULT  hr = hFile.Create(
        pwszFileFullPath,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        OPEN_EXISTING);
    if (FAILED(hr))
        return hr;

    return IsPEFile(hFile);
}

BOOL CWinTrustVerifier::IsPEFile(HANDLE hFile)
{
    assert(hFile && hFile != INVALID_HANDLE_VALUE);

    // 定位MZ头
    DWORD nNewPos = ::SetFilePointer(hFile, 0, 0, FILE_BEGIN);
    if (nNewPos == INVALID_SET_FILE_POINTER)
        return FALSE;

    // 读取MZ头
    IMAGE_DOS_HEADER DosHeader;
    DWORD dwBytesRead;
    BOOL br = ::ReadFile(hFile, &DosHeader, sizeof(DosHeader), &dwBytesRead, NULL);
    if (!br || dwBytesRead != sizeof(DosHeader))
        return FALSE;

    // 判断MZ特征
    if (IMAGE_DOS_SIGNATURE != DosHeader.e_magic)
        return FALSE;

    // 定位PE头
    nNewPos = ::SetFilePointer(hFile, DosHeader.e_lfanew, 0, FILE_BEGIN);
    if (nNewPos == INVALID_SET_FILE_POINTER)
        return FALSE;

    // 读取PE头
    DWORD dwPeSign = 0;
    br = ::ReadFile(hFile, &dwPeSign, sizeof(DWORD), &dwBytesRead, NULL);
    if (!br || dwBytesRead != sizeof(DWORD))
        return FALSE;
    
    // 判断PE特征
    if (IMAGE_NT_SIGNATURE != dwPeSign)
        return FALSE;

    return TRUE;
}

BOOL CWinTrustVerifier::IsWinTrustRetCode(DWORD dwRetCode)
{
    if (ERROR_SUCCESS == dwRetCode)
        return TRUE;

    if (dwRetCode & 0x30000000) // Customer code flag
        return FALSE;

    if (HRESULT_FACILITY(dwRetCode) == FACILITY_WIN32)
    {   // win32 error
        switch (HRESULT_CODE(dwRetCode))
        {
        case ERROR_NOT_FOUND:
            return TRUE;
        }
    }
    else if (HRESULT_FACILITY(dwRetCode) == FACILITY_CERT)
    {   // Cert error
        return TRUE;
    }

    return FALSE;
}

HRESULT CWinTrustVerifier::TryLoadDll()
{
    if (m_modWinTrust)
        return S_OK;

    return m_modWinTrust.LoadLib(L"wintrust.dll");
}