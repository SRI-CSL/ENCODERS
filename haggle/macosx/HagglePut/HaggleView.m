//
//  HaggleSender.m
//  HagglePut
//
//  Created by Daniel Aldman on 2008-12-16.
//  Copyright 2008 Uppsala Universitet. All rights reserved.
//

#import "HaggleView.h"
#include "haggle.h"

@implementation HaggleView

- (BOOL) sendToHaggle:(NSString *)filePath 
		 withAttributeName:(const char *)attrName 
		 andValue:(const char *)attrValue
{
	BOOL retval = NO;
	haggle_handle_t	_haggle;
	int err;
	
	err = haggle_handle_get("HagglePut", &_haggle);
	if(err == HAGGLE_NO_ERROR)
	{
		struct dataobject *dObj;
		dObj = 
			haggle_dataobject_new_from_file(
				[filePath 
					cStringUsingEncoding:NSMacOSRomanStringEncoding]);
		if(dObj)
		{
			err = haggle_dataobject_add_attribute(dObj, attrName, attrValue);
			if(err > HAGGLE_NO_ERROR)
			{
				err = haggle_ipc_publish_dataobject(_haggle, dObj);
				if(err > 0)
					retval = YES;
			}
			haggle_dataobject_free(dObj);
		}
		haggle_handle_free(_haggle);
	}
	return retval;
}

- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender
{
    NSPasteboard *paste = [sender draggingPasteboard];
        //gets the dragging-specific pasteboard from the sender
    NSArray *types = [NSArray arrayWithObjects:NSTIFFPboardType, 
                    NSFilenamesPboardType, nil];
        //a list of types that we can accept
    NSString *desiredType = [paste availableTypeFromArray:types];
    NSData *carriedData = [paste dataForType:desiredType];

    if (nil == carriedData)
    {
        //the operation failed for some reason
        NSRunAlertPanel(@"Paste Error", @"Sorry, but the past operation failed", 
            nil, nil, nil);
        return NO;
    }
    else
    {
        //the pasteboard was able to give us some meaningful data
        if ([desiredType isEqualToString:NSFilenamesPboardType])
        {
			NSUInteger i;
			
            NSArray *fileArray = 
                [paste propertyListForType:@"NSFilenamesPboardType"];
            for(i = 0; i < [fileArray count]; i++)
			{
				BOOL isDir;
				NSString *path = [fileArray objectAtIndex:i];
				if( [[NSFileManager defaultManager] 
						fileExistsAtPath:path 
						isDirectory:&isDir] &&
					[[NSFileManager defaultManager] 
						isReadableFileAtPath:path])
				{
					if(!isDir)
					{
						// Send file into haggle:
						if([self 
							sendToHaggle:path 
							withAttributeName:"Please" 
							andValue:sendToWhere])
						{
							[[NSSound soundNamed:@"Glass"] play];
						}else{
							[[NSSound soundNamed:@"Sosumi"] play];
						}
					}else{
						// FIXME: Warn user that a folder cannot be sent:
					}
				}
			}
        }
        else
        {
            //this can't happen
            NSAssert(NO, @"This can't happen");
            return NO;
        }
    }
    [self setNeedsDisplay:YES];    //redraw us with the new image
    return YES;
}

- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender
{
    if ((NSDragOperationGeneric & [sender draggingSourceOperationMask]) 
                == NSDragOperationGeneric)
    {
        //this means that the sender is offering the type of operation we want
        //return that we want the NSDragOperationGeneric operation that they 
            //are offering
        return NSDragOperationGeneric;
    }
    else
    {
        //since they aren't offering the type of operation we want, we have 
            //to tell them we aren't interested
        return NSDragOperationNone;
    }
}

- (NSDragOperation)draggingUpdated:(id <NSDraggingInfo>)sender
{
    if ((NSDragOperationGeneric & [sender draggingSourceOperationMask]) 
                    == NSDragOperationGeneric)
    {
        //this means that the sender is offering the type of operation we want
        //return that we want the NSDragOperationGeneric operation that they 
            //are offering
        return NSDragOperationGeneric;
    }
    else
    {
        //since they aren't offering the type of operation we want, we have 
            //to tell them we aren't interested
        return NSDragOperationNone;
    }
}

- (void)draggingEnded:(id <NSDraggingInfo>)sender
{
    //we don't do anything in our implementation
    //this could be ommitted since NSDraggingDestination is an infomal
        //protocol and returns nothing
}

- (BOOL)prepareForDragOperation:(id <NSDraggingInfo>)sender
{
    return YES;
}

- (void)concludeDragOperation:(id <NSDraggingInfo>)sender
{
    //re-draw the view with our new data
    [self setNeedsDisplay:YES];
}

- (void)draggingExited:(id <NSDraggingInfo>)sender
{
    //we aren't particularily interested in this so we will do nothing
    //this is one of the methods that we do not have to implement
}

- (void)awakeFromNib
{
    [self 
		registerForDraggedTypes:
			[NSArray 
				arrayWithObjects:
					NSFilenamesPboardType, nil]];
	sendToWhere = NULL;
}

- (void)dealloc
{
    [self unregisterDraggedTypes];
    [super dealloc];
}

- (void)setToWhere:(const char *)where
{
	sendToWhere = where;
}
@end

@implementation HagglePrinterView
- (void)awakeFromNib
{
	[super awakeFromNib];
	[self setToWhere:"PrintThis"];
}
@end

@implementation HaggleProjectorView
- (void)awakeFromNib
{
	[super awakeFromNib];
	[self setToWhere:"ProjectThis"];
}
@end
