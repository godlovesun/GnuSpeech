#import "IntonationScrollView.h"

#import <AppKit/AppKit.h>
#import "MAIntonationView.h"
#import "MAIntonationScaleView.h"

@implementation IntonationScrollView

#define SCALE_WIDTH 50

// TODO (2004-03-15): This doesn't get called when loaded from a nib.
- (id)initWithFrame:(NSRect)frameRect;
{
    //NSRect scaleRect, clipRect;
    NSRect scaleFrame;
    MAIntonationView *aView;

    NSLog(@"<%@>[%p]  > %s", NSStringFromClass([self class]), self, _cmd);

    if ([super initWithFrame:frameRect] == nil)
        return nil;

    /* Set display attributes */
    [self setBorderType:NSLineBorder];
    [self setHasHorizontalScroller:YES];

    /* alloc and init a intonation view instance.  Make Doc View */
    //clipRect = NSZeroRect;
    // TODO (2004-03-31): See if we can remove this code:
    aView = [[MAIntonationView alloc] initWithFrame:frameRect];
    [self setDocumentView:aView];

    scaleFrame = NSMakeRect(0, 0, SCALE_WIDTH, frameRect.size.height);
    scaleView = [[MAIntonationScaleView alloc] initWithFrame:scaleFrame];
    [self addSubview:scaleView];
    [aView setScaleView:scaleView];

    [aView release];

    [self setBackgroundColor:[NSColor whiteColor]];

    NSLog(@"<%@>[%p] <  %s", NSStringFromClass([self class]), self, _cmd);

    return self;
}

- (void)dealloc;
{
    [scaleView release];

    [super dealloc];
}

- (void)awakeFromNib;
{
    NSSize contentSize;
    NSRect frameRect, scaleFrame;
    NSRect documentVisibleRect;

    contentSize = [self contentSize];
    frameRect = [self frame];

    scaleFrame = NSMakeRect(0, 0, SCALE_WIDTH, contentSize.height);
    scaleView = [[MAIntonationScaleView alloc] initWithFrame:scaleFrame];
    [self addSubview:scaleView];

    [[self documentView] setScaleView:scaleView];

    [self tile];

    documentVisibleRect = [self documentVisibleRect];

    [[self documentView] setFrame:documentVisibleRect];
    [[self documentView] setNeedsDisplay:YES];
}

/*===========================================================================

	Method: tile
	Purpose: Hack to avoid a bug(?) or feature(?).

===========================================================================*/
- (void)tile;
{
    NSRect scaleFrame, contentFrame;

    [super tile];

    contentFrame.origin = NSZeroPoint;
    contentFrame.size = [self contentSize];
    NSDivideRect(contentFrame, &scaleFrame, &contentFrame, SCALE_WIDTH, NSMinXEdge);
    [scaleView setFrame:scaleFrame];
    [scaleView setNeedsDisplay:YES];
    [[self contentView] setFrame:contentFrame];
    [[self contentView] setNeedsDisplay:YES];
}

/*===========================================================================

	Method: printPSCode
	Purpose: Set up and print post script code of the FFT.

===========================================================================*/
- (void)print:(id)sender;
{
    /* Turn off some things to make output look better */
    [self setBorderType:NSNoCellMask];
    [self setHasHorizontalScroller:NO];

    /* Send code */
    [super print:sender];

    /* Reinstate original settings */
    [self setBorderType:NSLineBorder];
    [self setHasHorizontalScroller:YES];
}

- (NSView *)scaleView;
{
    return scaleView;
}

@end
