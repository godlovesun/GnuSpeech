#import <Foundation/NSObject.h>

#ifdef PORTING
#import <Foundation/NSArray.h>
#import <AppKit/NSBrowser.h>
#import <AppKit/NSBrowserCell.h>
#import <AppKit/NSForm.h>
#import "RuleList.h"
#import "CategoryList.h"
#import "BooleanParser.h"
#endif

@class BooleanParser, MonetList, RuleList;

/*===========================================================================

	Author: Craig-Richard Taube-Schock
		Copyright (c) 1994, Trillium Sound Research Incorporated.
		All Rights Reserved.

=============================================================================
*/

@interface RuleManager : NSObject
{
    int cacheValue;

    id controller;

    id ruleMatrix;
    id ruleScrollView;

    id matchBrowser1;
    id matchBrowser2;
    id matchBrowser3;
    id matchBrowser4;

    id expressionFields;
    id errorTextField;
    id possibleCombinations;

    BooleanParser *boolParser;

    MonetList *matchLists;
    MonetList *expressions;

    RuleList *ruleList;

    id phone1;
    id phone2;
    id phone3;
    id phone4;
    id ruleOutput;
    id consumedTokens;
    id durationOutput;

    id delegateResponder;
}

- init;
- (void)applicationDidFinishLaunching:(NSNotification *)notification;

- (void)browserHit:sender;
- (void)browserDoubleHit:sender;
- (int)browser:(NSBrowser *)sender numberOfRowsInColumn:(int)column;
- (void)browser:(NSBrowser *)sender willDisplayCell:(id)cell atRow:(int)row column:(int)column;

- expressionString:(char *) string forRule:(int) index;

- (void)setExpression1:sender;
- (void)setExpression2:sender;
- (void)setExpression3:sender;
- (void)setExpression4:sender;

- (void)realignExpressions;
- (void)evaluateMatchLists;
- (void)updateCombinations;
- (void)updateRuleDisplay;

- (void)add:sender;
- (void)rename:sender;
- (void)remove:sender;

- (void)parseRule:sender;

- ruleList;

- (void)addParameter;
- (void)addMetaParameter;
- (void)removeParameter:(int)index;
- (void)removeMetaParameter:(int)index;

/* Finding Stuff */

- (BOOL) isCategoryUsed: aCategory;
- (BOOL) isEquationUsed: anEquation;
- (BOOL) isTransitionUsed: aTransition;

- findEquation: anEquation andPutIn: aList;
- findTemplate: aTemplate andPutIn: aList;

- (void)cut:(id)sender;
- (void)copy:(id)sender;
- (void)paste:(id)sender;

/* Window Delegate Methods */
- (void)windowDidBecomeMain:(NSNotification *)notification;
- (BOOL)windowShouldClose:(id)sender;
- (void)windowDidResignMain:(NSNotification *)notification;

- (void)readDegasFileFormat:(FILE *)fp;

- (id)initWithCoder:(NSCoder *)aDecoder;
- (void)encodeWithCoder:(NSCoder *)aCoder;

- (void)writeRulesTo:(NSArchiver *)stream;
- (void)readRulesFrom:(NSArchiver *)stream;

@end
