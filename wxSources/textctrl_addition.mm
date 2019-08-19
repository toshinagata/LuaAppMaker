/*
 *  textctrl_addition.mm
 *
 *  Created by Toshi Nagata on 14/02/04.
 *  Copyright 2014 Toshi Nagata. All rights reserved.
 *
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation version 2 of the License.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 */

#import <Cocoa/Cocoa.h>

bool
textCtrl_hasMarkedText(void)
{
	id firstResponder = [[NSApp keyWindow] firstResponder];
	if ([firstResponder isKindOfClass:[NSTextView class]] && [(NSTextView *)firstResponder hasMarkedText])
		return true;
	else return false;
}
