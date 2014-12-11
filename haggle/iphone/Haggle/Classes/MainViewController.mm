//
//  MainViewController.m
//  iphone
//
//  Created by Christian Rohner on 2008-06-30.
//  Copyright __MyCompanyName__ 2008. All rights reserved.
//

#import "MainViewController.h"

#include "utils.h"
#include <libhaggle/haggle.h>
#include <libhaggle/ipc.h>

haggle_handle_t haggleHandleM;
MainViewController *controler;


@implementation MainViewController

@synthesize myWebView;

// return the data used by the navigation controller and tab bar item
- (NSString *)name {
	return @"Symbol";
}

- (UIImage *)tabBarImage {
	return [UIImage imageNamed:@"symbol_gray.png"];
}

- (BOOL)showDisclosureIcon
{
	return YES;
}


- (id)init
{
	self.title = @"Search";
	self.tabBarItem.image = [UIImage imageNamed:@"symbol_gray.png"];
/*
	CGRect searchFrame = [[UIScreen mainScreen] applicationFrame];
	searchFrame.origin.y += 20.0 + 5.0;	// leave from the URL input field and its label
	searchFrame.size.height = 20.0;
	mySearchBar = [[UISearchBar alloc] initWithFrame:searchFrame];
*/
	
	
	return self;
}


- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
	if (self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil]) {
		// Custom initialization
	}
	controler = self;
	return self;
}


// If you need to do additional setup after loading the view, override viewDidLoad.
- (void)viewDidLoad {

	milli_sleep(3000);	// hack to make sure haggle is ready...
	
	haggleHandleM = haggle_handle_get("iPhoneSearch");
	haggle_event_loop_run_async(haggleHandleM);

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


