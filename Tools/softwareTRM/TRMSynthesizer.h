//
// $Id$
//

//  This file is part of __APPNAME__, __SHORT_DESCRIPTION__.
//  Copyright (C) 2004 __OWNER__.  All rights reserved.

#import <Foundation/NSObject.h>
#import <AudioUnit/AudioUnit.h>
#import "structs.h"
#import "output.h"

@class MMSynthesisParameters;

extern int verbose;

@interface TRMSynthesizer : NSObject
{
    TRMData *inputData;
    NSMutableData *soundData;

    AudioUnit outputUnit;

    int bufferLength;
    int bufferIndex;

    BOOL shouldSaveToSoundFile;
    NSString *filename;

    OutputCallbackContext context;
}

- (id)init;
- (void)dealloc;

- (void)setupSynthesisParameters:(MMSynthesisParameters *)synthesisParameters;
- (void)removeAllParameters;
- (void)addParameters:(float *)values;

- (BOOL)shouldSaveToSoundFile;
- (void)setShouldSaveToSoundFile:(BOOL)newFlag;

- (NSString *)filename;
- (void)setFilename:(NSString *)newFilename;

- (int)fileType;
- (void)setFileType:(int)newFileType;

- (void)synthesize;
- (void)startPlaying;
- (void)stopPlaying;

- (void)setupSoundDevice;
- (void)fillBuffer:(AudioBuffer *)ioData;

@end
