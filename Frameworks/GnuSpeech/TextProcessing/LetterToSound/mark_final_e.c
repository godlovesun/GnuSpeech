//  This file is part of Gnuspeech, an extensible, text-to-speech package, based on real-time, articulatory, speech-synthesis-by-rules.
//  Copyright 1991-2012 David R. Hill, Leonard Manzara, Craig Schock

#import "letter_to_sound_private.h"

/*  GLOBAL VARIABLES (LOCAL TO THIS FILE)  ***********************************/
static char *suffix_list_1 = "elba/ylba/de/ne/re/yre/tse/ye/gni/ssel/yl/tnem/ssen/ro/luf/";

static char *suffix_list_2 = "ci/laci/";

/// Handle preprocessing in section McIlroy section 4.3, Final E.
void mark_final_e(char *in, char **eow)
{
    fprintf(stderr, "mark_final_e(%s)\n", in);
    char *end = *eow;
    char *prior_char;

    /*  McIlroy 4.3 - a)  */
    /*  IF ONLY ONE VOWEL IN WORD && IT IS AN 'e' AT THE END  */
    if ((*(end - 1) == 'e') && !vowel_before(in, end - 1))
    {
        *(end - 1) = 'E';
        return;
    }

    /*  McIlroy 4.3 - g)  */
    /*  LOOK FOR #^[aeiouy]* [aeiouy] ^[aeiouywx] [al | le | re | us | y]  */
    /*  IF FOUND CHANGE       ------   TO UPPER CASE */
    if ((prior_char = (char *)ends_with(in, end, "#la/#el/#er/#su/#y/")))
    {
        if (!member(*prior_char, "aeiouywx"))
        {
            if (member(*--prior_char, "aeiouy"))
            {
                if (!vowel_before(in, prior_char))
                {
                    *prior_char &= 0xdf; // Uppercase letter.
                }
            }
        }
    }

    /* McIlroy 4.3 - a)  */
    char *temp = prior_char = end - 1;
    while ((prior_char = (char *)suffix_with_vowel_before(in, prior_char, suffix_list_1)))
    {
        insert_mark(&end, prior_char);
        temp = prior_char;
    }

    prior_char = temp;
    if ((prior_char = (char *)suffix_with_vowel_before(in, prior_char, suffix_list_2)))
    {
        insert_mark(&end, prior_char);
        *eow = end;
        return;
    }

    prior_char = temp;
    if ((prior_char = (char *)suffix_with_vowel_before(in, prior_char, "e/")))
    {
        if (prior_char[2] != 'e')
        {
            if (prior_char[2] != '|')
                insert_mark(&end, prior_char);
        }
        else
        {
            *eow = end;
            return;
        }
    }
    else
    {
        prior_char = temp;
    }

    /*  McIlroy 4.3 -b)  */
    if (((*(prior_char + 1) == '|') && (member(*(prior_char + 2), "aeio")))
        || (member(*(prior_char + 1), "aeio")))
    {
        if (!member(*prior_char, "aeiouywx"))
        {
            if (member(*(prior_char - 1), "aeiouy"))
            {
                if (!member(*(prior_char - 2), "aeo"))
                {
                    *(prior_char - 1) &= 0xdf; // Uppercase letter.
                }
            }
        }

        /*  McIlroy 4.3 -c)  */
        if ((*prior_char == 'h') && (*(prior_char - 1) == 't'))
        {
            if (member(*(prior_char - 2), "aeiouy"))
            {
                if (!member(*(prior_char - 3), "aeo"))
                    *(prior_char - 2) &= 0xdf; // Uppercase letter.
                *(prior_char - 1) = 'T';
                *prior_char = 'H';
            }
        }
    }

    /*  McIlroy 4.3 - d)  */
    if ((member(*prior_char, "iuy")) && !vowel_before(in, prior_char))
    {
        *prior_char &= 0xdf; // Uppercase letter.
        *eow = end;
        return;
    }

    /*  McIlroy 4.3 - e)  */
    if ((*(prior_char + 1) == 'e') && (member(*prior_char, "cg")))
    {
        temp = (char *)vowel_before(in, prior_char);
        if (vowel_before(in, temp))
        {
            *temp |= 0x20; // Lowercase letter.
            *eow = end;
            return;
        }
    }

    /*  McIlroy 4.3 - f)  */
    if ((*prior_char == 'l') && (*(prior_char - 1) == 'E'))
        *(prior_char - 1) |= 0x20; // Lowercase letter.
    *eow = end;

    return;
}
