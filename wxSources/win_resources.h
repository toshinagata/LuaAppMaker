/*
 *  win_resources.h
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

#ifndef WIN_RESOURCES_H
#define WIN_RESOURCES_H

#include <wx/string.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int ReplaceWinAppIcon(wxString &appPath, wxString &iconPath);

#ifdef __cplusplus
}
#endif

#endif /* WIN_RESOURCES_H */
