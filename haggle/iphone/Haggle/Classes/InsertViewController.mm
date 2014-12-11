//
//  InsertViewController.m
//  iphone
//
//  Created by Christian Rohner on 2008-06-30.
//  Copyright __MyCompanyName__ 2008. All rights reserved.
//

#import "InsertViewController.h"
//#include "InterestApp.h"

#define OS_MACOSX_IPHONE

#include "utils.h"
#include <libhaggle/haggle.h>
#include <libhaggle/ipc.h>

@implementation InsertViewController

- (id)init
{
	self.title = @"Insert";
	self.tabBarItem.image = [UIImage imageNamed:@"symbol_gray.png"];
	
	return self;
}


- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
	if (self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil]) {
		// Custom initialization
	}
	return self;
}


// If you need to do additional setup after loading the view, override viewDidLoad.
- (void)viewDidLoad {
}


- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
	// Return YES for supported orientations
	return (interfaceOrientation == UIInterfaceOrientationPortrait);
}


- (void)didReceiveMemoryWarning {
	[super didReceiveMemoryWarning]; // Releases the view if it doesn't have a superview
	// Release anything that's not essential, such as cached data
}


- (void)dealloc {
    [super dealloc];
}

@end


