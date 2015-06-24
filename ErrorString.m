
#define __MACMEMORY__
#import <Cocoa/Cocoa.h>

/* -----------------------------------------------------------------------------
	Return a string describing an error code.
	Args:		inErr		a NewtonErr code
	Return:	C string
----------------------------------------------------------------------------- */

const char *
GetFramesErrorString(int inErr)
{
	NSString * key = [NSString stringWithFormat: @"%d", inErr];
	NSString * errStr = NSLocalizedStringFromTable(key, @"Error", NULL);
	return errStr.UTF8String;
}


/* -----------------------------------------------------------------------------
	While we’re in the Cocoa environment:
	Return the URL to the Application Support directory.
	Args:		--
	Return:	a URL
----------------------------------------------------------------------------- */

NSURL *
ApplicationSupportFolder(void)
{
	NSFileManager * fmgr = [NSFileManager defaultManager];
	NSURL * baseURL = [fmgr URLForDirectory:NSApplicationSupportDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:YES error:nil];
	NSURL * appFolder = [baseURL URLByAppendingPathComponent:@"Newton"];
	// if folder doesn’t exist, create it
	[fmgr createDirectoryAtPath:appFolder.path withIntermediateDirectories:NO attributes:nil error:nil];
	return appFolder;
}


/* -----------------------------------------------------------------------------
	Return the path to the store backing file.
	Args:		inStoreName		*unique* name of the store
	Return:	C string			will be autoreleased by ARC
----------------------------------------------------------------------------- */

const char *
StoreBackingFile(const char * inStoreName)
{
	NSURL * url = [ApplicationSupportFolder() URLByAppendingPathComponent:[NSString stringWithUTF8String:inStoreName]];
	return url.fileSystemRepresentation;
}
