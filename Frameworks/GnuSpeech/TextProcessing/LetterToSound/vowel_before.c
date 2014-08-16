//  This file is part of Gnuspeech, an extensible, text-to-speech package, based on real-time, articulatory, speech-synthesis-by-rules.
//  Copyright 1991-2012 David R. Hill, Leonard Manzara, Craig Schock

#import "letter_to_sound_private.h"

/// Return the position of a vowel prior to 'position'.  If no vowel prior return 0.

char *vowel_before(char *start, char *position)
{
    position--;
    while (position >= start) {
        if (member(*position, "aeiouyAEIOUY"))
            return(position);
        position--;
    }
    return(0);
}