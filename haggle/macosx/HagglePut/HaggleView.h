//
//  HaggleSender.h
//  HagglePut
//
//  Created by Daniel Aldman on 2008-12-16.
//  Copyright 2008 Uppsala Universitet. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface HaggleView : NSImageView {
const char *sendToWhere;
}

- (BOOL) sendToHaggle:(NSString *)filePath 
		 withAttributeName:(const char *)attrName 
		 andValue:(const char *)attrValue;
- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender;
- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender;
- (NSDragOperation)draggingUpdated:(id <NSDraggingInfo>)sender;
- (void)draggingEnded:(id <NSDraggingInfo>)sender;
- (BOOL)prepareForDragOperation:(id <NSDraggingInfo>)sender;
- (void)concludeDragOperation:(id <NSDraggingInfo>)sender;
- (void)draggingExited:(id <NSDraggingInfo>)sender;
- (void)awakeFromNib;
- (void)dealloc;
- (void)setToWhere:(const char *)where;
@end

@interface HagglePrinterView : HaggleView {
}
- (void)awakeFromNib;
@end

@interface HaggleProjectorView : HaggleView {
}
- (void)awakeFromNib;
@end