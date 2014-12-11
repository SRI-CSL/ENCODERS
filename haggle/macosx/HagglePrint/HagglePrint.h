//
//  HagglePrint.h
//  HagglePrint
//
//  Created by Daniel Aldman on 2008-12-17.
//  Copyright 2008 Uppsala Universitet. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface HagglePrint : NSObject {
IBOutlet NSTableView *theTable; 
NSInteger rows;
IBOutlet NSButton *getPrintFiles; 
IBOutlet NSButton *getProjectFiles; 
@public IBOutlet NSButton *printFiles; 
@public IBOutlet NSButton *projectFiles; 
}

- (void)awakeFromNib;
- (void)reloadData;

- (IBAction) myDoubleClickAction:(id)sender;
- (IBAction) myClickCheckboxAction:(id)sender;

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView;
- (id)tableView:(NSTableView *)tableView 
	objectValueForTableColumn:(NSTableColumn *)tableColumn 
	row:(NSInteger)row;

@end
