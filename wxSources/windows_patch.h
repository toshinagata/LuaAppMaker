/*
 *  windows_patch.h
 *
 *  Created by Toshi Nagata on 2021/8/21.
 *  Copyright 2021 Toshi Nagata. All rights reserved.
 *
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation version 2 of the License.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 */

/*  This header file overrides Windows.h, to hook the system calls that
    use the CP_ACP encoding  */

#ifndef __WINDOWS_PATCH_H
#define __WINDOWS_PATCH_H

/* #warning "Windows_patch.h is included instead of Windows.h" */

#include_next "Windows.h"

#define LoadLibraryExA LoadLibraryExA_Patch
#define FormatMessageA FormatMessageA_Patch
#define GetModuleFileNameA GetModuleFileNameA_Patch
#define GetModuleHandleA GetModuleHandleA_Patch
#define GetModuleHandleExA GetModuleHandleExA_Patch

#ifdef __cplusplus
extern "C" {
#endif

extern HMODULE LoadLibraryExA_Patch(
  LPCSTR lpLibFileName,
  HANDLE hFile,
  DWORD  dwFlags
  );

extern HMODULE GetModuleHandleA_Patch(
  LPCSTR lpModuleName
  );

extern DWORD GetModuleFileNameA_Patch(
  HMODULE hModule,
  LPSTR   lpFilename,
  DWORD   nSize
  );

extern DWORD FormatMessageA_Patch(
  DWORD   dwFlags,
  LPCVOID lpSource,
  DWORD   dwMessageId,
  DWORD   dwLanguageId,
  LPSTR   lpBuffer,
  DWORD   nSize,
  va_list *Arguments
  );

extern BOOL GetModuleHandleExA_Patch(
  DWORD   dwFlags,
  LPCSTR  lpModuleName,
  HMODULE *phModule
  );

#ifdef __cplusplus
}
#endif

#endif  /* __WINDOWS_PATCH_H  */
