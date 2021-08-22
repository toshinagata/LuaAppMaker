/*  win_fopen.c  */
/*  2019.8.31. Toshi Nagata  */
/*  Public Domain  */

#if _WIN32 || _WIN64
#include <Windows.h>
#include <stdio.h>
static int utf8towchar(const char *utf8, wchar_t **outbuf)
{
  size_t buflen = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, (void *)0, 0);
  wchar_t *buf = calloc(buflen + 1, sizeof(wchar_t));
  int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, buf, buflen);
  if (len == 0) {
    free(buf);
    return -1;
  }
  buf[len] = 0;
  *outbuf = buf;
  return 0;
}

FILE *
fopen(const char *path, const char *mode)
{
  wchar_t *wpath, *wmode;
  FILE *f;
  if (utf8towchar(path, &wpath) != 0 || utf8towchar(mode, &wmode) != 0)
    return NULL;
  f = _wfopen(wpath, wmode);
  free(wpath);
  free(wmode);
  return f;
}

HMODULE LoadLibraryExA_Patch(
  LPCSTR lpLibFileName,
  HANDLE hFile,
  DWORD  dwFlags
)
{
  wchar_t *wpath, *wmode;
  HMODULE retval;
  if (utf8towchar(lpLibFileName, &wpath) != 0) {
    return NULL;
  }
  retval = LoadLibraryExW(wpath, hFile, dwFlags);
  free(wpath);
  return retval;
}

HMODULE GetModuleHandleA_Patch(
  LPCSTR lpModuleName
)
{
  wchar_t *wname = NULL;
  HMODULE retval;
  if (lpModuleName != NULL && utf8towchar(lpModuleName, &wname) != 0) {
    return NULL;
  }
  retval = GetModuleHandleW(wname);
  if (wname)
    free(wname);
  return retval;
}

BOOL GetModuleHandleExA_Patch(
  DWORD   dwFlags,
  LPCSTR  lpModuleName,
  HMODULE *phModule
)
{
  wchar_t *wname = NULL;
  BOOL retval;
  if (dwFlags & GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS) {
    return GetModuleHandleExA(dwFlags, lpModuleName, phModule); /* Call the unpatched version */
  }
  if (lpModuleName != NULL && utf8towchar(lpModuleName, &wname) != 0) {
    return NULL;
  }
  retval = GetModuleHandleExW(dwFlags, wname, phModule);
  if (wname != NULL)
    free(wname);
  return retval;
}

DWORD GetModuleFileNameA_Patch(
  HMODULE hModule,
  LPSTR   lpFilename,
  DWORD   nSize
)
{
  wchar_t *wname;
  DWORD retval;
  wname = (wchar_t *)calloc(nSize + 1, sizeof(wchar_t));
  retval = GetModuleFileNameW(hModule, wname, nSize);
  if (retval > 0) {
    retval = WideCharToMultiByte(CP_UTF8, 0, wname, retval, lpFilename, nSize, NULL, FALSE);
    if (retval > 0)
      lpFilename[retval] = 0;
  }
  free(wname);
  return retval;
}

DWORD FormatMessageA_Patch(
  DWORD   dwFlags,
  LPCVOID lpSource,
  DWORD   dwMessageId,
  DWORD   dwLanguageId,
  LPSTR   lpBuffer,
  DWORD   nSize,
  va_list *Arguments
)
{
  DWORD retval;
  wchar_t *wBuffer;
  wBuffer = (wchar_t *)calloc(nSize + 1, sizeof(wchar_t));
  retval = FormatMessageW(dwFlags, lpSource, dwMessageId, dwLanguageId, wBuffer, nSize, Arguments);
  if (retval > 0) {
    retval = WideCharToMultiByte(CP_UTF8, 0, wBuffer, retval, lpBuffer, nSize, NULL, FALSE);
    if (retval > 0)
      lpBuffer[retval] = 0;
  }
  free(wBuffer);
  return retval;
}


#endif
