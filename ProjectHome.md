# Introduction #

win32 api encapsulation

compatiable for `_ATL_MIN_CRT`

[winmod class list](WinModClasses.md)

# Quick Example #

CWinTrustVerifier
```
#include "winmod\wintrustverifier.h"

...

WinMod::CWinTrustVerifier hVerifier;

HRESULT hRetCode = hVerifier.VerifyFileFast(L"c:\\windows\\system32\\cmd.exe");
if (FAILED(hRetCode))
    wprintf(L"%s\n", L"failed to verify trust of cmd.exe");
else
    wprintf(L"%s\n", L"succeeded to verify trust of cmd.exe");
```


CWinFileFindDepthFirst
```
#include "winmod\winfilefinddepthfirst.h"

...

WinMod::CWinFileFindDepthFirst hFileFinder;

BOOL bFound = hFileFinder.FindFirstFile(L"c:\\windows\\system32");
while (bFound)
{
    if (hFileFinder.IsDirectory())
        wprintf(L"[dir] %s\n", (LPCWSTR)hFileFinder.GetFullPath());
    else
        wprintf(L"      %s\n", (LPCWSTR)hFileFinder.GetFullPath());

    bFound = hFileFinder.FindNextFile();
}
```


# Extra #
  * [Bug of atlmincrt in VC2005](BugOfAtlMinCrtInVC2005.md)
  * [理解ReadDirectoryChangesW](UnderstandingReadDirectoryChangesW_CN.md)