def banfunc():
    dangerous_functions = [{("strcpy", "wcscpy", "_tcscpy", "_mbscpy", "StrCpy", "StrCpyA", "StrCpyW", "lstrcpy", "lstrcpyA", "lstrcpyW", "strcpyA", "strcpyW", "_tccpy", "_mbccpy"):"strcpy_s"},
                       {("strcat", "wcscat", "_tcscat", "_mbscat", "StrCat", "StrCatA", "StrCatW", "lstrcat", "lstrcatA", "lstrcatW", "StrCatBuffW", "StrCatBuff", "StrCatBuffA","StrCatChainW", "strcatA", "strcatW", "_tccat", "_mbccat"):"strcat_s"},
                       {("wnsprintf", "wnsprintfA", "wnsprintfW", "sprintfW", "sprintfA", "wsprintf", "wsprintfW", "wsprintfA", "sprintf", "swprintf", "_stprintf"):"sprintf_s"},
                       {("_snwprintf", "_snprintf", "_sntprintf", "nsprintf"):"_snprintf_s or _snwprintf_s"},
                       {("wvsprintf", "wvsprintfA", "wvsprintfW", "vsprintf", "_vstprintf", "vswprintf"):"_vstprintf_s"},
                       {("vsnprintf", "_vsnwprintf", "_vsntprintf", "wvnsprintf", "wvnsprintfA", "wvnsprintfW"):"vsntprintf_s"},
                       {("strncpy", "wcsncpy", "_tcsncpy", "_mbsncpy", "_mbsnbcpy", "StrCpyN", "StrCpyNA", "StrCpyNW", "StrNCpy", "strcpynA", "StrNCpyA", "StrNCpyW", "lstrcpyn", "lstrcpynA", "lstrcpynW", "_fstrncpy"):"strncpy_s"},
                       {("strncat", "wcsncat", "_tcsncat", "_mbsncat", "_mbsnbcat", "StrCatN", "StrCatNA", "StrCatNW", "StrNCat", "StrNCatA", "StrNCatW", "lstrncat", "lstrcatnA", "lstrcatnW", "lstrcatn", "_fstrncat"):"strncat_s"},
                       {("strtok", "_tcstok", "wcstok", "_mbstok"):"strtok_s"},
                       {("Makepath", "_tmakepath", "_makepath", "_wmakepath"):"_makepath_s"},
                       {("_splitpath", "_tsplitpath", "_wsplitpath"):"_splitpath_s"},
                       {("scanf", "wscanf", "_tscanf", "sscanf", "swscanf", "_stscanf"):"sscanf_s"},
                       {("snscanf", "snwscanf", "_sntscanf"):"_snscanf_s"},
                       {("_itoa", "_itow", "_i64toa", "_i64tow", "_ui64toa", "_ui64tot", "_ui64tow", "_ultoa", "_ultot", "_ultow"):"_itoa_s, _itow_s"},
                       {("gets", "_getts", "_gettws"):"gets_s"},
                       {("strlen", "wcslen", "_mbslen", "_mbstrlen", "StrLen", "lstrlen"):"strnlen_s"},
                       {("ChangeWindowMessageFilter",):"ChangeWindowMessageFilterEx"},
                       {("alloca", "_alloca"):"SafeAllocA"},
                       {("CharToOem", "CharToOemA", "CharToOemW", "OemToChar", "OemToCharA", "OemToCharW", "CharToOemBuffA", "CharToOemBuffW"):"WideCharToMultiByte"},
                       {("IsBadWritePtr", "IsBadHugeWritePtr", "IsBadReadPtr", "IsBadHugeReadPtr", "IsBadCodePtr", "IsBadStringPtr"):" Rewrite the code to avoid using these functions."},
                       {("memcpy", "RtlCopyMemory","CopyMemory", "wmemcpy", "memmove"):"memcpy_s, wmemcpy_s, memmove_s"}]
    return dangerous_functions
