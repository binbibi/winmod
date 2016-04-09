#Bug of atlmincrt in vc2005

# Introduction #

This bug was found by my friend Gdier. http://xiaoxzh.blogbus.com/logs/43415176.html

It's also be methoned at http://news.rsdn.ru/forum/atl/82885.1.aspx

# Details #

atexit() in atlinit.cpp, can be found at %ProgramFiles%\Microsoft Visual Studio 8\VC\atlmfc\src\atl\atlmincrt\

```
    // ...
    nCurrentSize = _msize(__onexitbegin);
    if((nCurrentSize+sizeof(_PVFV)) < ULONG(((const BYTE*)__onexitend-
        (const BYTE*)__onexitbegin)))
    { //< not enough onexit entries
        _PVFV* pNew;

        pNew = (_PVFV*)_recalloc(__onexitbegin, 2,nCurrentSize); //< recalloc here
        if(pNew == NULL)
        {
            LeaveCriticalSection(&g_csInit);	  
            return(-1);
        }
    }

    //< then pNew is lost
    *__onexitend = pf;
    __onexitend++; //< __onexitend is moved to the hell -_-b...
    // ...
    // ...
    // ...
    // something bad ...
```

This bug occurs when there are more than 16 atexit entries (destructor of global/static C++ object) are registered

Error dialog may pop up and show message below:

```
heap corruption detected:
after normal block(#xxx) at 0x xxxxxxxx
crt detected that the application wrote to menory after end of heap buffer
```