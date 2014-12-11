//
//  iphoneAppDelegate.m
//  iphone
//
//  Created by Christian Rohner on 2008-06-30.
//  Copyright __MyCompanyName__ 2008. All rights reserved.
//

#import "iphoneAppDelegate.h"
#import "RootViewController.h"

@implementation iphoneAppDelegate

@synthesize window;
@synthesize rootViewController;


- (void)applicationDidFinishLaunching:(UIApplication *)application {
	
	[window addSubview:[rootViewController view]];
	[window makeKeyAndVisible];
}


- (void)dealloc {
	[rootViewController release];
	[window release];
	[super dealloc];
}

@end
