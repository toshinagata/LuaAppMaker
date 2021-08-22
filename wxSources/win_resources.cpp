/*
 *  win_resources.cpp
 *
 *  Created by Toshi Nagata on 2019/09/13.
 *  Copyright 2019 Toshi Nagata. All rights reserved.
 *
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation version 2 of the License.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 */

#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "win_resources.h"

/*  The code below is taken from CodeProject article 30644.
  https://www.codeproject.com/Articles/30644/Replacing-ICON-resources-in-EXE-and-DLL-files
  Author: Maria Nadejde
  Copyright 2008 Maria Nadejde.  All rights reserved.
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation version 3 of the License.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
*/

//#include "resource.h"
#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <iostream>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <strsafe.h>
#include <string>

#define MAX_NR_BUNDLES 1000

/*
char *types[25]=
{
    "NULL","RT_CURSOR","RT_BITMAP","RT_ICON","RT_MENU","RT_DIALOG","RT_STRING","RT_FONTDIR","RT_FONT",
    "RT_ACCELERATORS","RT_RCDATA","RT_MESSAGETABLE","RT_GROUP_CURSOR","NULL",
    "RT_GROUP_ICON","NULL","RT_VERSION","RT_DLGINCLUDE","NULL","RT_PLUGPLAY","RT_VXD","RT_ANICURSOR", //21 de la 0
    "RT_ANIICON","RT_HTML","RT_MANIFEST"
};
*/

WORD nMaxID=0;
HANDLE hFile, hFileEx;
HINSTANCE currentUI;
UINT currentLangId;

//CHAR *szReplaceFile={"Replace.txt"};

//id-urile bundleurilor GROUP_ICON
typedef struct
{
    UINT nBundles;
    UINT nLangId;
} BUNDLES, *PBUNDLES;
int cBundles=0;

BUNDLES pBundles[MAX_NR_BUNDLES];


HWND hWndMain = 0;

// These next two structs represent how the icon information is stored
// in an ICO file.
typedef struct
{
    BYTE    bWidth;               // Width of the image
    BYTE    bHeight;              // Height of the image (times 2)
    BYTE    bColorCount;          // Number of colors in image (0 if >=8bpp)
    BYTE    bReserved;            // Reserved
    WORD    wPlanes;              // Color Planes
    WORD    wBitCount;            // Bits per pixel
    DWORD    dwBytesInRes;         // how many bytes in this resource?
    DWORD    dwImageOffset;        // where in the file is this image
} ICONDIRENTRY, *LPICONDIRENTRY;
typedef struct
{
    WORD            idReserved;   // Reserved
    WORD            idType;       // resource type (1 for icons)
    WORD            idCount;      // how many images?
    LPICONDIRENTRY    idEntries; // the entries for each image
} ICONDIR, *LPICONDIR;

// The following two structs are for the use of this program in
// manipulating icons. They are more closely tied to the operation
// of this program than the structures listed above. One of the
// main differences is that they provide a pointer to the DIB
// information of the masks.
typedef struct
{
    UINT            Width, Height, Colors; // Width, Height and bpp
    LPBYTE            lpBits;                // ptr to DIB bits
    DWORD            dwNumBytes;            // how many bytes?
    LPBITMAPINFO    lpbi;                  // ptr to header
    LPBYTE            lpXOR;                 // ptr to XOR image bits
    LPBYTE            lpAND;                 // ptr to AND image bits
} ICONIMAGE, *LPICONIMAGE;



typedef struct
{
    BYTE    bWidth;               // Width of the image
    BYTE    bHeight;              // Height of the image (times 2)
    BYTE    bColorCount;          // Number of colors in image (0 if >=8bpp)
    BYTE    bReserved;            // Reserved
    WORD    wPlanes;              // Color Planes
    WORD    wBitCount;            // Bits per pixel
    DWORD    dwBytesInRes;         // how many bytes in this resource?
    WORD    nID;                  // the ID
} MEMICONDIRENTRY, *LPMEMICONDIRENTRY;
typedef struct
{
    WORD            idReserved;   // Reserved
    WORD            idType;       // resource type (1 for icons)
    WORD            idCount;      // how many images?
    LPMEMICONDIRENTRY    idEntries; // the entries for each image
} MEMICONDIR, *LPMEMICONDIR;

using namespace std;

int filter(unsigned int code, struct _EXCEPTION_POINTERS *ep, WCHAR *fileName)
{
    
    if (code == EXCEPTION_ACCESS_VIOLATION)
    {
        MessageBoxW( hWndMain, L"Eroare LoadLibrary: verifica daca exista fisierul", fileName, MB_OK );
        CloseHandle(hFile);
        FreeLibrary(currentUI);
        
        return EXCEPTION_EXECUTE_HANDLER;
    }
    
    else
    {
        
        puts("didn't catch AV, unexpected.");
        
        return EXCEPTION_CONTINUE_SEARCH;
        
    }
    
}

LPICONIMAGE* ExtractIcoFromFileW(LPWSTR filename,LPICONDIR pIconDir)
{
    BOOL res=true;
    HANDLE    hFile1 = NULL, hFile2=NULL, hFile3=NULL;
    //LPICONDIR pIconDir;
    DWORD    dwBytesRead;
    LPICONIMAGE pIconImage;
    LPICONIMAGE *arrayIconImage;
    DWORD cbInit=0,cbOffsetDir=0,cbOffset=0,cbInitOffset=0;
    BYTE *temp;
    int i;
    
    
    if( (hFile1 = CreateFileW( filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL )) == INVALID_HANDLE_VALUE )
    {
        MessageBoxW( hWndMain, L"Error Opening File for Reading", filename, MB_OK );
        return NULL;
    }
    
    
    ReadFile( hFile1, &(pIconDir->idReserved), sizeof( WORD ), &dwBytesRead, NULL );
    ReadFile( hFile1, &(pIconDir->idType), sizeof( WORD ), &dwBytesRead, NULL );
    ReadFile( hFile1, &(pIconDir->idCount), sizeof( WORD ), &dwBytesRead, NULL );
    
#ifdef WRICOFILE
    hFile2 = CreateFileW(L"replicaICO.ico", GENERIC_READ | GENERIC_WRITE,0,(LPSECURITY_ATTRIBUTES) NULL,
                         CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,(HANDLE) NULL);
    if (hFile2 == INVALID_HANDLE_VALUE)
    {
        MessageBoxW( hWndMain, L"Error Opening File for Reading", L"replicaICO.ico", MB_OK );
        return NULL;
    }
    WriteFile(hFile2, &(pIconDir->idReserved),dwBytesRead, &cbWritten, NULL);
    WriteFile(hFile2, &(pIconDir->idType),dwBytesRead, &cbWritten, NULL);
    WriteFile(hFile2, &(pIconDir->idCount),dwBytesRead, &cbWritten, NULL);
#endif
    
    
    pIconDir->idEntries = new ICONDIRENTRY[pIconDir->idCount];
    
    // Read the ICONDIRENTRY elements
    temp=new BYTE[sizeof(ICONDIRENTRY)];
    for(i=0;i<pIconDir->idCount;i++)
    {
        ReadFile( hFile1, &pIconDir->idEntries[i], sizeof(ICONDIRENTRY), &dwBytesRead, NULL );
#ifdef WRICOFILE
        WriteFile(hFile2, &pIconDir->idEntries[i],dwBytesRead, &cbWritten, NULL);
#endif
    }
    arrayIconImage=new LPICONIMAGE[pIconDir->idCount];
    // Loop through and read in each image
    for(i=0;i<pIconDir->idCount;i++)
    {
        pIconImage = (LPICONIMAGE)malloc( pIconDir->idEntries[i].dwBytesInRes );
        SetFilePointer( hFile1, pIconDir->idEntries[i].dwImageOffset,NULL, FILE_BEGIN );
        ReadFile( hFile1, pIconImage, pIconDir->idEntries[i].dwBytesInRes,&dwBytesRead, NULL );
        arrayIconImage[i]=(LPICONIMAGE)malloc( pIconDir->idEntries[i].dwBytesInRes );
        memcpy( arrayIconImage[i],pIconImage, pIconDir->idEntries[i].dwBytesInRes );
        free(pIconImage);
        
#ifdef WRICOFILE
        SetFilePointer( hFile2, pIconDir->idEntries[i].dwImageOffset,NULL, FILE_BEGIN );
        WriteFile(hFile2, pIconImage,dwBytesRead, &cbWritten, NULL);
#endif
    }
    CloseHandle(hFile1);
#ifdef WRICOFILE
    CloseHandle(hFile2);
#endif
    return arrayIconImage;
}

BOOL ReplaceIconResourceW(LPWSTR lpFileName, LPCWSTR lpName, UINT langId, LPICONDIR pIconDir,    LPICONIMAGE* pIconImage)
{
    BOOL res=true;
    HANDLE    hFile3=NULL;
    LPMEMICONDIR lpInitGrpIconDir=new MEMICONDIR;
    
    //LPICONIMAGE pIconImage;
    HINSTANCE hUi;
    BYTE *test,*test1,*temp,*temp1;
    DWORD cbInit=0,cbOffsetDir=0,cbOffset=0,cbInitOffset=0;
    WORD cbRes=0;
    int i;
    
    hUi = LoadLibraryExW(lpFileName,NULL,DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
    HRSRC hRsrc = FindResourceExW(hUi, RT_GROUP_ICON, lpName,langId);
    //nu stiu de ce returneaza 104 wtf???
    //cbRes=SizeofResource( hUi, hRsrc );
    
    
    HGLOBAL hGlobal = LoadResource( hUi, hRsrc );
    test1 =(BYTE*) LockResource( hGlobal );
    temp1=test1;
    //    temp1=new BYTE[118];
    //    CopyMemory(temp1,test1,118);
    lpInitGrpIconDir->idReserved=(WORD)*test1;
    test1=test1+sizeof(WORD);
    lpInitGrpIconDir->idType=(WORD)*test1;
    test1=test1+sizeof(WORD);
    lpInitGrpIconDir->idCount=(WORD)*test1;
    test1=test1+sizeof(WORD);
    
    lpInitGrpIconDir->idEntries=new MEMICONDIRENTRY[lpInitGrpIconDir->idCount];
    
    for(i=0;i<lpInitGrpIconDir->idCount;i++)
    {
        lpInitGrpIconDir->idEntries[i].bWidth=(BYTE)*test1;
        test1=test1+sizeof(BYTE);
        lpInitGrpIconDir->idEntries[i].bHeight=(BYTE)*test1;
        test1=test1+sizeof(BYTE);
        lpInitGrpIconDir->idEntries[i].bColorCount=(BYTE)*test1;
        test1=test1+sizeof(BYTE);
        lpInitGrpIconDir->idEntries[i].bReserved=(BYTE)*test1;
        test1=test1+sizeof(BYTE);
        lpInitGrpIconDir->idEntries[i].wPlanes=(WORD)*test1;
        test1=test1+sizeof(WORD);
        lpInitGrpIconDir->idEntries[i].wBitCount=(WORD)*test1;
        test1=test1+sizeof(WORD);
        //nu merge cu (DWORD)*test
        lpInitGrpIconDir->idEntries[i].dwBytesInRes=pIconDir->idEntries[i].dwBytesInRes;
        test1=test1+sizeof(DWORD);
        lpInitGrpIconDir->idEntries[i].nID=(WORD)*test1;
        test1=test1+sizeof(WORD);
    }
    //    memcpy( lpInitGrpIconDir->idEntries, test, cbRes-3*sizeof(WORD) );
    
    UnlockResource((HGLOBAL)test1);
    
    LPMEMICONDIR lpGrpIconDir=new MEMICONDIR;
    lpGrpIconDir->idReserved=pIconDir->idReserved;
    lpGrpIconDir->idType=pIconDir->idType;
    lpGrpIconDir->idCount=pIconDir->idCount;
    cbRes=3*sizeof(WORD)+lpGrpIconDir->idCount*sizeof(MEMICONDIRENTRY);
    test=new BYTE[cbRes];
    temp=test;
    CopyMemory(test,&lpGrpIconDir->idReserved,sizeof(WORD));
    test=test+sizeof(WORD);
    CopyMemory(test,&lpGrpIconDir->idType,sizeof(WORD));
    test=test+sizeof(WORD);
    CopyMemory(test,&lpGrpIconDir->idCount,sizeof(WORD));
    test=test+sizeof(WORD);
    
    lpGrpIconDir->idEntries=new MEMICONDIRENTRY[lpGrpIconDir->idCount];
    for(i=0;i<lpGrpIconDir->idCount;i++)
    {
        lpGrpIconDir->idEntries[i].bWidth=pIconDir->idEntries[i].bWidth;
        CopyMemory(test,&lpGrpIconDir->idEntries[i].bWidth,sizeof(BYTE));
        test=test+sizeof(BYTE);
        lpGrpIconDir->idEntries[i].bHeight=pIconDir->idEntries[i].bHeight;
        CopyMemory(test,&lpGrpIconDir->idEntries[i].bHeight,sizeof(BYTE));
        test=test+sizeof(BYTE);
        lpGrpIconDir->idEntries[i].bColorCount=pIconDir->idEntries[i].bColorCount;
        CopyMemory(test,&lpGrpIconDir->idEntries[i].bColorCount,sizeof(BYTE));
        test=test+sizeof(BYTE);
        lpGrpIconDir->idEntries[i].bReserved=pIconDir->idEntries[i].bReserved;
        CopyMemory(test,&lpGrpIconDir->idEntries[i].bReserved,sizeof(BYTE));
        test=test+sizeof(BYTE);
        lpGrpIconDir->idEntries[i].wPlanes=pIconDir->idEntries[i].wPlanes;
        CopyMemory(test,&lpGrpIconDir->idEntries[i].wPlanes,sizeof(WORD));
        test=test+sizeof(WORD);
        lpGrpIconDir->idEntries[i].wBitCount=pIconDir->idEntries[i].wBitCount;
        CopyMemory(test,&lpGrpIconDir->idEntries[i].wBitCount,sizeof(WORD));
        test=test+sizeof(WORD);
        lpGrpIconDir->idEntries[i].dwBytesInRes=pIconDir->idEntries[i].dwBytesInRes;
        CopyMemory(test,&lpGrpIconDir->idEntries[i].dwBytesInRes,sizeof(DWORD));
        test=test+sizeof(DWORD);
        if(i<lpInitGrpIconDir->idCount) //nu am depasit numarul initial de RT_ICON
            lpGrpIconDir->idEntries[i].nID=lpInitGrpIconDir->idEntries[i].nID;
        else
        {
            nMaxID++;
            lpGrpIconDir->idEntries[i].nID=nMaxID; //adaug noile ICO la sfarsitul RT_ICON-urilor
        }
        CopyMemory(test,&lpGrpIconDir->idEntries[i].nID,sizeof(WORD));
        test=test+sizeof(WORD);
    }
    
    
    
    //offsetul de unde incep structurile ICONIMAGE
    cbInitOffset=3*sizeof(WORD)+lpGrpIconDir->idCount*sizeof(ICONDIRENTRY);
    cbOffset=cbInitOffset; //cbOffset=118
    
    FreeLibrary(hUi);
    
    HANDLE hUpdate;
    
    
    
    _wchmod((wchar_t *)lpFileName,_S_IWRITE);
    hUpdate = BeginUpdateResourceW(lpFileName, FALSE); //false sa nu stearga resursele neupdated
    if(hUpdate==NULL)
    {
        
        MessageBoxW( hWndMain, L"eroare BeginUpdateResource", lpFileName, MB_OK );
        res=false;
    }
    //aici e cu lang NEUTRAL
    //res=UpdateResource(hUpdate,RT_GROUP_ICON,MAKEINTRESOURCE(6000),langId,lpGrpIconDir,cbRes);
    res=UpdateResourceW(hUpdate,RT_GROUP_ICON,lpName,langId,temp,cbRes);
    if(res==false)
        MessageBoxW( hWndMain, L"eroare UpdateResource RT_GROUP_ICON", lpFileName, MB_OK );
    
    for(i=0;i<lpGrpIconDir->idCount;i++)
    {
        res=UpdateResourceW(hUpdate,RT_ICON,MAKEINTRESOURCE(lpGrpIconDir->idEntries[i].nID),langId,pIconImage[i],lpGrpIconDir->idEntries[i].dwBytesInRes);
        if(res==false)
            MessageBoxW( hWndMain, L"eroare UpdateResource RT_ICON", lpFileName, MB_OK );
    }
    
    for(i=lpGrpIconDir->idCount;i<lpInitGrpIconDir->idCount;i++)
    {
        res=UpdateResourceW(hUpdate,RT_ICON,MAKEINTRESOURCE(lpInitGrpIconDir->idEntries[i].nID),langId,(void *)"",0);
        if(res==false)
            MessageBoxW( hWndMain, L"eroare stergere resurse vechi", lpFileName, MB_OK );
        
    }
    
    if(!EndUpdateResourceW(hUpdate,FALSE)) //false ->resource updates will take effect.
        MessageBoxW( hWndMain, L"eroare EndUpdateResource", lpFileName, MB_OK );
    
    
    //    FreeResource(hGlobal);
    delete[] lpGrpIconDir->idEntries;
    delete lpGrpIconDir;
    delete[] temp;
    
    return res;
}

BOOL EnumLangsFuncW(HANDLE hModule,LPCWSTR lpType, LPCWSTR lpName,WORD wLang,LONG lParam)
{
    currentLangId=(UINT)wLang;
    return true;
}

BOOL EnumNamesFuncW(HANDLE hModule, LPCWSTR lpType, LPWSTR lpName, LONG lParam)
{
    
    
    if(IS_INTRESOURCE(lpName))
    {
        
        if(lpType==RT_ICON)
        {
            if((USHORT)lpName>nMaxID)
                nMaxID=(USHORT)lpName;
        }
        
        if(lpType==RT_GROUP_ICON)
        {
            EnumResourceLanguagesW((HMODULE)hModule,lpType,lpName,(ENUMRESLANGPROCW)EnumLangsFuncW,0);
            pBundles[cBundles].nBundles=(USHORT)lpName;
            pBundles[cBundles].nLangId=(USHORT)currentLangId;
            cBundles++;
        }
    }
    
    
    return true;
}

//////callback function pentru enumerate types//////
//(module handle, address of res type, extra pram)//
BOOL EnumTypesFuncW(HANDLE hModule, LPWSTR lpType, LONG lParam)
{
    LPWSTR szBuffer;
    szBuffer=(LPWSTR)new CHAR[300];    // print buffer for EnumResourceTypes
    
    
    ///// Find the names of all resources of type String. //////
    if(lpType==RT_ICON||lpType==RT_GROUP_ICON) //e string
    {
        if(EnumResourceNamesW((HINSTANCE)hModule,lpType,(ENUMRESNAMEPROCW)EnumNamesFuncW,0)==false)
        {
            MessageBoxW( hWndMain, L"eroare EnumResourceNames", L"RT_ICON||RT_GROUP_ICON", MB_OK );
            return false;
        }
    }
    delete[] szBuffer;
    return true;
}

BOOL CProcFile(LPWSTR lpSrcFileName)
{
    HINSTANCE hui;
    
    
    //hui=LoadLibraryA(fileName);
    
    hui = LoadLibraryExW(lpSrcFileName,NULL,DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
    if (hui == NULL)
    {
        MessageBoxW( hWndMain, L"Eroare LoadLibrary: verifica daca exista fisierul", lpSrcFileName, MB_OK );
        //DWORD dwLastError = GetLastError();
        //wprintf(L"failed with error %d: \n", dwLastError);
        return false;
    }
    else
    {
        //cout<<"\nOPENED library"<<lpSrcFileName;
        currentUI=hui;
    }
    
    if(EnumResourceTypesW(hui,(ENUMRESTYPEPROCW)EnumTypesFuncW,0)==false)
    {
        _tprintf(_T("eroare enum res types\n"));
        return 0;
    }
    //////////////////////////////////////
    
    FreeLibrary(hui);
    
    
    
    return true;
}

/*  The code below is added by Toshi Nagata  */
int
ReplaceWinAppIcon(wxString &appPath, wxString &iconPath)
{
    LPCWSTR appFileName = appPath.wc_str();
    LPCWSTR iconFileName = iconPath.wc_str();
    ICONDIR iconDir;
    LPICONIMAGE *iconImage;
    BOOL result;
    iconImage = ExtractIcoFromFileW((wchar_t *)iconFileName, &iconDir);
    /*  The icon name "myicon" is defined in wxLuaApp.rc  */
    result = ReplaceIconResourceW((wchar_t *)appFileName, L"MYICON", MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), &iconDir, iconImage);
    delete iconImage;
    return result;
}
