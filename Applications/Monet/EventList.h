#import <Foundation/NSObject.h>

@class Event, IntonationPoint, MModel, MMPosture, MMRule, PhoneList;

/*===========================================================================

	Author: Craig-Richard Taube-Schock
		Copyright (c) 1994, Trillium Sound Research Incorporated.
		All Rights Reserved.

=============================================================================
*/

#define MAXPHONES	1500
#define MAXFEET		110
#define MAXTONEGROUPS	50

#define MAXRULES	MAXPHONES-1

#define STATEMENT	0
#define EXCLAIMATION	1
#define QUESTION	2
#define CONTINUATION	3
#define SEMICOLON	4

NSString *NSStringFromToneGroupType(int toneGroupType);

struct _intonationParameters {
    float notionalPitch;
    float pretonicRange;
    float pretonicLift;
    float tonicRange;
    float tonicMovement; // TODO (2004-03-30): Apparently not used.
};

struct _phone {
    MMPosture *phone;
    int syllable;
    double onset;
    float ruleTempo;
};

struct _foot {
    double onset1;
    double onset2;
    double tempo;
    int start; // index into postures
    int end;   // index into postures
    int marked;
    int last; // Is this the last foot of (the tone group?)
};

struct _toneGroup {
    int startFoot;
    int endFoot;
    int type;
};

// This is used by EventListView, IntonationView
struct _rule {
    int number;
    int firstPhone;
    int lastPhone;
    double duration;
    double beat;
};


@interface EventList : NSObject
{
    int zeroRef;
    int zeroIndex;
    int duration;
    int timeQuantization; // in msecs.  By default it generates parameters every 4 msec

    BOOL shouldStoreParameters; // YES -> -generateOutput writes to /tmp/Monet.parameters
    BOOL shouldUseMacroIntonation;
    BOOL shouldUseMicroIntonation;
    BOOL shouldUseDrift;
    BOOL shouldUseSmoothIntonation;

    double radiusMultiply;
    double pitchMean;
    double globalTempo;
    double multiplier;
    struct _intonationParameters intonationParameters;

    /* NOTE phones and phoneTempo are separate for Optimization reasons */
    int currentPhone;
    struct _phone phones[MAXPHONES];
    double phoneTempo[MAXPHONES];

    int currentFoot;
    struct _foot feet[MAXFEET];

    int currentToneGroup;
    struct _toneGroup toneGroups[MAXTONEGROUPS];

    int currentRule;
    struct _rule rules[MAXRULES];

    int cache;
    double min[16]; // Min of each parameter value
    double max[16]; // Max of each parameter value

    NSMutableArray *events;
    NSMutableArray *intonationPoints;

    id delegate;
}

- (id)init;
- (void)dealloc;

- (id)delegate;
- (void)setDelegate:(id)newDelegate;

- (void)setUp;

- (int)zeroRef;
- (void)setZeroRef:(int)newValue;

- (int)duration;
- (void)setDuration:(int)newValue;

- (double)radiusMultiply;
- (void)setRadiusMultiply:(double)newValue;

- (void)setFullTimeScale;

- (int)timeQuantization;
- (void)setTimeQuantization:(int)newValue;

- (BOOL)shouldStoreParameters;
- (void)setShouldStoreParameters:(BOOL)newFlag;

- (double)pitchMean;
- (void)setPitchMean:(double)newMean;

- (double)globalTempo;
- (void)setGlobalTempo:(double)newTempo;

- (double)multiplier;
- (void)setMultiplier:(double)newValue;

- (BOOL)shouldUseMacroIntonation;
- (void)setShouldUseMacroIntonation:(BOOL)newFlag;

- (BOOL)shouldUseMicroIntonation;
- (void)setShouldUseMicroIntonation:(BOOL)newFlag;

- (BOOL)shouldUseDrift;
- (void)setShouldUseDrift:(BOOL)newFlag;

- (BOOL)shouldUseSmoothIntonation;
- (void)setShouldUseSmoothIntonation:(BOOL)newValue;

- (struct _intonationParameters)intonationParameters;
- (void)setIntonationParameters:(struct _intonationParameters)newIntonationParameters;

- (MMPosture *)getPhoneAtIndex:(int)phoneIndex;
- (struct _rule *)getRuleAtIndex:(int)ruleIndex;
- (NSString *)ruleDescriptionAtIndex:(int)ruleIndex;
- (double)getBeatAtIndex:(int)ruleIndex;
- (int)numberOfRules;

// Tone groups
- (void)newToneGroup;
- (void)setCurrentToneGroupType:(int)type;

- (void)newFoot;
- (void)setCurrentFootMarked;
- (void)setCurrentFootLast;
- (void)setCurrentFootTempo:(double)tempo;

- (void)newPhone;
- (void)newPhoneWithObject:(MMPosture *)anObject;
- (void)replaceCurrentPhoneWith:(MMPosture *)anObject;
- (void)setCurrentPhoneTempo:(double)tempo;
- (void)setCurrentPhoneRuleTempo:(float)tempo;
- (void)setCurrentPhoneSyllable;


- (NSArray *)events;

- (Event *)insertEvent:(int)number atTime:(double)time withValue:(double)value;
- (void)finalEvent:(int)number withValue:(double)value;

- (void)generateOutput;
- (void)generateEventListWithModel:(MModel *)aModel;

- (void)applyRule:(MMRule *)rule withPhones:(NSArray *)phoneList andTempos:(double *)tempos phoneIndex:(int)phoneIndex;
- (void)synthesizeToFile:(NSString *)filename;

- (void)applyIntonation;

- (NSString *)description;
- (void)printDataStructures:(NSString *)comment;

- (NSArray *)intonationPoints;
- (void)removeIntonationPoint:(IntonationPoint *)aPoint;

// Moved from IntonationView
- (void)clearIntonationPoints;
- (void)addIntonationPoint:(IntonationPoint *)iPoint;
- (void)addPoint:(double)semitone offsetTime:(double)offsetTime slope:(double)slope ruleIndex:(int)ruleIndex;

- (void)applyIntonation_fromIntonationView;
- (void)applySmoothIntonation;

@end
