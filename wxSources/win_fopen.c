/*  win_fopen.c  */
/*  2019.8.31. Toshi Nagata  */
/*  Public Domain  */

#if _WIN32 || _WIN64
#include <Windows.h>
#include <stdio.h>
static int utf8towchar(const char *utf8, wchar_t **outbuf)
{
  size_t buflen = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, (void *)0, 0);
  wchar_t *buf = calloc(buflen, sizeof(wchar_t));
  if (MultiByteToWideChar(CP_UTF8, 0, utf8, -1, buf, buflen) == 0) {
    free(buf);
    return -1;
  }
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
#endif
