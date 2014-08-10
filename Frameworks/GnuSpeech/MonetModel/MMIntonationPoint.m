//  This file is part of Gnuspeech, an extensible, text-to-speech package, based on real-time, articulatory, speech-synthesis-by-rules. 
//  Copyright 1991-2012 David R. Hill, Leonard Manzara, Craig Schock

#import "MMIntonationPoint.h"

#include <math.h>
#import "NSObject-Extensions.h"
#import "NSString-Extensions.h"

#import "EventList.h"
#import "MXMLParser.h"

#define MIDDLEC	261.6255653

@implementation MMIntonationPoint
{
    __weak EventList *_eventList;
    
    double _semitone;      // Value of the point in semitones
    double _offsetTime;    // Points are timed wrt a beat + this offset
    double _slope;         // Slope of point

    NSUInteger _ruleIndex; // Index of the rule for the phone which is the focus of this point
}

- (id)init;
{
    if ((self = [super init])) {
        _eventList = nil;

        _semitone = 0.0;
        _offsetTime = 0.0;
        _slope = 0.0;
        _ruleIndex = 0;
    }

    return self;
}

#pragma mark - Debugging

// TODO (2012-04-23): Separate this archiving description from a debugging description
- (NSString *)description;
{
    return [NSString stringWithFormat:@"<intonation-point offset-time=\"%g\" semitone=\"%g\" slope=\"%g\" rule-index=\"%lu\"/>\n",
            self.offsetTime, self.semitone, self.slope, self.ruleIndex];
}

#pragma mark -

// TODO (2012-04-21): Have event list use kvo to watch for changes.

- (double)semitone;
{
    return _semitone;
}

- (void)setSemitone:(double)newSemitone;
{
    if (newSemitone != _semitone) {
        _semitone = newSemitone;
        [_eventList intonationPointDidChange:self];
    }
}

- (double)offsetTime;
{
    return _offsetTime;
}

- (void)setOffsetTime:(double)newOffsetTime;
{
    if (newOffsetTime != _offsetTime) {
        _offsetTime = newOffsetTime;
        [_eventList intonationPointTimeDidChange:self];
    }
}

- (double)slope;
{
    return _slope;
}

- (void)setSlope:(double)newSlope;
{
    if (newSlope != _slope) {
        _slope = newSlope;
        [_eventList intonationPointDidChange:self];
    }
}

- (NSInteger)ruleIndex;
{
    return _ruleIndex;
}

- (void)setRuleIndex:(NSInteger)newRuleIndex;
{
    if (newRuleIndex != _ruleIndex) {
        _ruleIndex = newRuleIndex;
        [_eventList intonationPointTimeDidChange:self];
    }
}

- (double)absoluteTime;
{
    if (self.eventList == nil) {
        NSLog(@"Warning: no event list for intonation point in %s, returning 0.0", __PRETTY_FUNCTION__);
        return 0.0;
    }

    return [self.eventList getBeatAtIndex:self.ruleIndex] + self.offsetTime;
}

- (double)beatTime;
{
    if (self.eventList == nil) {
        NSLog(@"Warning: no event list for intonation point in %s, returning 0.0", __PRETTY_FUNCTION__);
        return 0.0;
    }

    return [self.eventList getBeatAtIndex:self.ruleIndex];
}

- (double)semitoneInHertz;
{
    double hertz = pow(2, self.semitone / 12.0) * MIDDLEC;

    return hertz;
}

// TODO (2012-04-23): Maybe just add a function to convert Hertz -> semitone.
- (void)setSemitoneInHertz:(double)newHertzValue;
{
    // i.e. 12.0 * log_2(newHertzValue / MIDDLEC)
    self.semitone = 12.0 * (log10(newHertzValue / MIDDLEC) / log10(2.0));
}

- (void)incrementSemitone;
{
    self.semitone = self.semitone + 1.0;
}

- (void)decrementSemitone;
{
    self.semitone = self.semitone - 1.0;
}

- (void)incrementRuleIndex;
{
    self.ruleIndex = self.ruleIndex + 1;
}

- (void)decrementRuleIndex;
{
    self.ruleIndex = self.ruleIndex - 1;
}

- (NSComparisonResult)compareByAscendingAbsoluteTime:(MMIntonationPoint *)otherIntonationPoint;
{
    double thisTime = [self absoluteTime];
    double otherTime = [otherIntonationPoint absoluteTime];

    if (thisTime < otherTime)      return NSOrderedAscending;
    else if (thisTime > otherTime) return NSOrderedDescending;

    return NSOrderedSame;
}

#pragma mark - XML - Archiving

- (void)appendXMLToString:(NSMutableString *)resultString level:(NSUInteger)level;
{
    [resultString indentToLevel:level];
    [resultString appendFormat:@"<intonation-point offset-time=\"%g\" semitone=\"%g\" slope=\"%g\" rule-index=\"%lu\"/>\n",
                  self.offsetTime, self.semitone, self.slope, self.ruleIndex];
}

#if 0
- (id)initWithXMLAttributes:(NSDictionary *)attributes context:(id)context;
{
    if ((self = [self init])) {
        NSString *value;
        
        value = [attributes objectForKey:@"offset-time"];
        if (value != nil) self.offsetTime = [value doubleValue];
        
        value = [attributes objectForKey:@"semitone"];
        if (value != nil) self.semitone = [value doubleValue];
        
        value = [attributes objectForKey:@"slope"];
        if (value != nil) self.slope = [value doubleValue];
        
        value = [attributes objectForKey:@"rule-index"];
        if (value != nil) self.ruleIndex = [value intValue];
    }

    return self;
}

- (void)parser:(NSXMLParser *)parser didStartElement:(NSString *)elementName namespaceURI:(NSString *)namespaceURI qualifiedName:(NSString *)qName attributes:(NSDictionary *)attributeDict;
{
    [(MXMLParser *)parser skipTree];
}

- (void)parser:(NSXMLParser *)parser didEndElement:(NSString *)elementName namespaceURI:(NSString *)namespaceURI qualifiedName:(NSString *)qName;
{
    [(MXMLParser *)parser popDelegate];
}
#endif

@end
