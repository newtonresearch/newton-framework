
#import <Cocoa/Cocoa.h>

/*------------------------------------------------------------------------------
	M P W i n d o w C o n t r o l l e r
------------------------------------------------------------------------------*/

@interface MPWindowController : NSWindowController
- (void)setupDrawing;
@end


/*------------------------------------------------------------------------------
	M P V i e w
------------------------------------------------------------------------------*/

@interface MPView : NSView
@property CGContextRef qContext;
@property CGLayerRef qLayer;
- (BOOL) acceptsFirstResponder;
- (void) mouseDown: (NSEvent *)inEvent;
- (void) mouseDragged: (NSEvent *)inEvent;
- (void) mouseUp: (NSEvent *)inEvent;
@end

/*------------------------------------------------------------------------------
	M P C o n t r o l l e r
	The app delegate.
------------------------------------------------------------------------------*/

@interface MPController : NSObject
@property(weak) IBOutlet NSView * newtonView;
@end
