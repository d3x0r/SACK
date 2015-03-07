/**
 *	@file    iphoneClientAppDelegate.h
 *	@version e6a3d9f (HEAD, tag: MATRIXSSL-3-7-1-OPEN, origin/master, origin/HEAD, master)
 *
 *	Summary.
 */
#import <UIKit/UIKit.h>

@class IphoneClientViewController;

@interface iphoneClientAppDelegate : NSObject <UIApplicationDelegate>
{
    UIWindow* window;
    IphoneClientViewController* viewController;
}

@property (nonatomic, retain) IBOutlet UIWindow* window;
@property (nonatomic, retain) IBOutlet IphoneClientViewController* viewController;

@end

