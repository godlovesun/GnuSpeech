////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 1991-2009 David R. Hill, Leonard Manzara, Craig Schock
//  
//  Contributors: Steve Nygard
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////
//
//  Event.h
//  GnuSpeech
//
//  Created by Steve Nygard in 2004.
//
//  Version: 0.9
//
////////////////////////////////////////////////////////////////////////////////

#import <Foundation/NSObject.h>

/*===========================================================================

	Author: Craig-Richard Taube-Schock
		Copyright (c) 1994, Trillium Sound Research Incorporated.
		All Rights Reserved.

=============================================================================
*/

#define NaN (1.0/0.0)
#define MAX_EVENTS 36

@interface Event : NSObject
{
    int time;
    BOOL flag;
    double events[MAX_EVENTS];
}

- (id)init;
- (id)initWithTime:(int)aTime;

- (int)time;

- (BOOL)flag;
- (void)setFlag:(BOOL)newFlag;

- (double)getValueAtIndex:(int)index;
- (void)setValue:(double)newValue ofIndex:(int)index;

- (NSString *)description;

@end
