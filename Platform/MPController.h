
#import <Cocoa/Cocoa.h>

/*------------------------------------------------------------------------------
	M P V i e w
------------------------------------------------------------------------------*/

@interface MPView : NSView
- (BOOL) acceptsFirstResponder;
- (void) mouseDown: (NSEvent *)inEvent;
- (void) mouseDragged: (NSEvent *)inEvent;
- (void) mouseUp: (NSEvent *)inEvent;
@end

/*------------------------------------------------------------------------------
	M P C o n t r o l l e r
------------------------------------------------------------------------------*/

@interface MPController : NSObject
@property(weak) IBOutlet NSView * newtonView;
@end
