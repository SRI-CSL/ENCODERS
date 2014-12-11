//
//  iphoneAppDelegate.h
//  iphone
//
//  Created by Christian Rohner on 2008-06-30.
//  Copyright __MyCompanyName__ 2008. All rights reserved.
//

#import <UIKit/UIKit.h>
#include "HaggleMain.h"

@class RootViewController;

@interface iphoneAppDelegate : NSObject <UIApplicationDelegate> {
	IBOutlet UIWindow *window;
    IBOutlet UITabBarController *tabBarController;
	IBOutlet RootViewController *rootViewController;
	Thread *myThread;
}

@property (nonatomic, retain) UIWindow *window;
@property (nonatomic, retain) UITabBarController *tabBarController;
@property (nonatomic, retain) RootViewController *rootViewController;

@end

