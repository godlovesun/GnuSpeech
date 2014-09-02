//  This file is part of Gnuspeech, an extensible, text-to-speech package, based on real-time, articulatory, speech-synthesis-by-rules.
//  Copyright 1991-2012 David R. Hill, Leonard Manzara, Craig Schock

#import "letter_to_sound.h"
#import "letter_to_sound_private.h"
#import "tail.h"
#import <strings.h>
#import <stdio.h>


/*  LOCAL DEFINES  ***********************************************************/
#define MAX_WORD_LENGTH            1024
#define MAX_PRONUNCIATION_LENGTH   8192
#define WORDEND(word,string)       (!strcmp(MAX(word+strlen(word)-strlen(string),word),string))


/*  GLOBAL FUNCTIONS (LOCAL TO THIS FILE)  ***********************************/
static char *word_type(char *word);

static FILE *log_fp = NULL;

/// Want to log some intermediate state, for testing.
void lts_log_to_file(FILE *fp)
{
    log_fp = fp;
}

char *letter_to_sound(const char *word)
{
    char buffer[MAX_WORD_LENGTH+3];
    static char pronunciation[MAX_PRONUNCIATION_LENGTH + 1];
    long number_of_syllables = 0;

    memset(pronunciation, 0, MAX_PRONUNCIATION_LENGTH + 1); // Helps debugging.

    if (log_fp != NULL) fprintf(log_fp, "%s", word);

    /*  FORMAT WORD  */
    sprintf(buffer, "#%s#", word);

    /*  CONVERT WORD TO PRONUNCIATION  */
    if (word_to_patphone(buffer) == 0)
    {
        if (log_fp != NULL) fprintf(log_fp, "\t%s", buffer);
        isp_trans(buffer, pronunciation);
        /*  ATTEMPT TO MARK SYLL/STRESS  */
        number_of_syllables = syllabify(pronunciation);
        if (log_fp != NULL) fprintf(log_fp, "\t%s\n", pronunciation);
        if (apply_stress(pronunciation, (char *)word))
            return NULL;
        //fprintf(stderr, "word_to_patphone() case 1, pronunciation: '%s'\n", pronunciation);
    }
    else
    {
        if (log_fp != NULL) fprintf(log_fp, "\tn/a\t%s\n", buffer);
        strcpy(pronunciation, buffer);
        //fprintf(stderr, "word_to_patphone() case 2, pronunciation: '%s'\n", pronunciation);
    }

    /*  APPEND WORD_TYPE_DELIMITER  */
    pronunciation[strlen(pronunciation) - 1] = WORD_TYPE_DELIMITER;

    /*  GUESS TYPE OF WORD  */
    if (number_of_syllables != 1)
        strcat(pronunciation, word_type((char *)word));
    else
        strcat(pronunciation, WORD_TYPE_UNKNOWN);

    return pronunciation;
}



/// Returns the word type based on the word spelling.
static char *word_type(char *word)
{
    tail_entry *list_ptr;

    /*  IF WORD END MATCHES LIST, RETURN CORRESPONDING TYPE  */
    for (list_ptr = tail_list; list_ptr->tail; list_ptr++) {
        if (WORDEND(word, list_ptr->tail))
            return list_ptr->type;
    }

    /*  ELSE RETURN UNKNOWN WORD TYPE  */
    return WORD_TYPE_UNKNOWN;
}
