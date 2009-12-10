/**
* @file    winmodinethttpconnection.cpp
* @brief   ...
* @author  zhangrui
* @date    2009-08-07 15:14
*/

#include "stdafx.h"
#include "winmodinethttpconnection.h"

using namespace WinMod;

HRESULT CInetHttpConnection::HttpRequest(
    /* [in ] */ LPCTSTR         lpObject,
    /* [in ] */ DWORD           dwTimeout,
    /* [in ] */ LPCTSTR         lpszContentType,
    /* [in ] */ const CStringA& strCommand,
    /* [out] */ CStringA*       pstrResponse,
    /* [out] */ DWORD*          pdwStatusCode,
    /* [in ] */ LPCTSTR         lpszSpecHostName)
{
    if (!m_h)
        return E_HANDLE;

    if (!lpObject || !lpszContentType)
        return E_POINTER;

    SetConnectTimeOut(dwTimeout);
    SetSendTimeOut(dwTimeout);
    SetReceiveTimeOut(dwTimeout);


    CInetHttpFile hFile;
    hFile.Attach(OpenRequest(L"POST", lpObject));
    if (!hFile)
        return GetLastError() ? AtlHresultFromLastError() : E_FAIL;


    HRESULT hr = E_FAIL;
    if (lpszSpecHostName)
    {
        CString strHost;
        strHost.Format(L"Host: %s", lpszSpecHostName);
        hr = hFile.AddRequestHeaders(strHost,  strHost.GetLength(), HTTP_ADDREQ_FLAG_REPLACE);
    }


    CString strContentLength;
    strContentLength.Format(L"Content-Length: %d", strCommand.GetLength());
    CString strContentType = L"Content-Type: ";
    strContentType.Append(lpszContentType);

    hr = hFile.AddRequestHeaders(strContentLength,  strContentLength.GetLength());
    hr = hFile.AddRequestHeaders(strContentType,    strContentType.GetLength());



    INTERNET_BUFFERS inetBuf;
    ::ZeroMemory(&inetBuf, sizeof(inetBuf));
    hr = hFile.SendRequest(NULL, 0, (LPVOID)(LPCSTR)strCommand, (DWORD)strCommand.GetLength());
    if (FAILED(hr))
        return hr;


    DWORD dwStatusCode = HTTP_STATUS_SERVER_ERROR;
    hr = hFile.QueryInfoStatusCode(dwStatusCode);
    if (FAILED(hr))
        return hr;

    
    if (pdwStatusCode)
        *pdwStatusCode = dwStatusCode;


    if (HTTP_STATUS_OK != dwStatusCode)
        return E_FAIL;



    // 如果不需要返回response,则直接返回
    if (!pstrResponse)
        return S_OK;


    DWORD dwContentLength = 400;
    hr = hFile.QueryInfoContentLength(dwContentLength);
    if (FAILED(hr))
        return hr;

    // 简单的长度限制
    if (dwContentLength > 0x100000)
        return E_FAIL;


    DWORD dwBytesRead = 0;
    hr = hFile.Read(pstrResponse->GetBuffer(dwContentLength), dwContentLength, dwBytesRead);
    pstrResponse->ReleaseBuffer(dwBytesRead);
    if (FAILED(hr))
        return hr;


    return S_OK;
}



HRESULT CInetHttpConnection::HttpDownload(
    /* [in ] */ IInetHttpDownloadFile*      piDownloadFile,
    /* [in ] */ IInetHttpDownloadProgress*  piCallback,
    /* [in ] */ LPCTSTR                     lpObject,
    /* [in ] */ DWORD                       dwTimeout,
    /* [out] */ DWORD*                      pdwStatusCode,
    /* [in ] */ LPCTSTR                     lpszSpecHostName)
{
    if (!m_h)
        return E_HANDLE;

    if (!lpObject || !piDownloadFile)
        return E_POINTER;

    if (pdwStatusCode)
        *pdwStatusCode = 0;


    SetConnectTimeOut(dwTimeout);
    SetSendTimeOut(dwTimeout);
    SetReceiveTimeOut(dwTimeout);


    CInetHttpFile hHttpFile;
    hHttpFile.Attach(OpenRequest(L"GET", lpObject));
    if (!hHttpFile)
        return GetLastError() ? AtlHresultFromLastError() : E_FAIL;


    HRESULT hr = E_FAIL;
    if (lpszSpecHostName)
    {
        CString strHost;
        strHost.Format(L"Host: %s", lpszSpecHostName);
        hr = hHttpFile.AddRequestHeaders(strHost,  strHost.GetLength(), HTTP_ADDREQ_FLAG_REPLACE);
    }



    INTERNET_BUFFERS inetBuf;
    ::ZeroMemory(&inetBuf, sizeof(inetBuf));
    hr = hHttpFile.SendRequest();
    if (FAILED(hr))
        return hr;



    // 查询http状态码
    DWORD dwHttpStatusCode = 0;
    hr = hHttpFile.QueryInfoStatusCode(dwHttpStatusCode);
    if (FAILED(hr))
        return hr;


    if (pdwStatusCode)
        *pdwStatusCode = dwHttpStatusCode;


    // 检查http状态码
    if (HTTP_STATUS_OK != dwHttpStatusCode)
        return MAKE_WINMOD_HTTP_ERROR(dwHttpStatusCode);


    // 查询内容长度
    DWORD dwContentLength = 0;
    hr = hHttpFile.QueryInfoContentLength(dwContentLength);
    if (FAILED(hr))
        return hr;




    hr = piDownloadFile->SetSize(dwContentLength);
    if (FAILED(hr))
        return hr;


    // 开始下载数据
    DWORD dwTotalSize       = dwContentLength;
    DWORD dwTransferedSize  = 0;


    hr = piDownloadFile->Seek(0, FILE_BEGIN);
    if (FAILED(hr))
        return hr;


    while (dwTransferedSize < dwTotalSize)
    {
        BYTE  byBuffer[4096];
        DWORD dwBytesRead = 0;
        DWORD dwBytesLeft = dwTotalSize - dwTransferedSize;
        DWORD dwToRead = min(sizeof(byBuffer), dwBytesLeft); 


        hr = hHttpFile.Read(byBuffer, dwToRead, dwBytesRead);
        if (FAILED(hr))
            return hr;


        if (0 == dwBytesRead)
            break;


        // 回调进度
        if (piCallback)
        {
            hr = piCallback->OnReceiveData(
                dwTotalSize,
                dwTotalSize - dwBytesLeft,
                dwBytesRead,
                byBuffer);
            if (FAILED(hr))
                return hr;
        }


        hr = piDownloadFile->Write(byBuffer, dwBytesRead);
        if (FAILED(hr))
            return hr;


        dwTransferedSize += dwBytesRead;
    }


    // 结束文件的传输
    hr = piDownloadFile->Flush();
    if (FAILED(hr))
        return hr;


    hr = piDownloadFile->SetEndOfFile();
    if (FAILED(hr))
        return hr;


    return S_OK;
}