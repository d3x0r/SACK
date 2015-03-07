/**
 *	@file    SslDelegateProtocol.h
 *	@version e6a3d9f (HEAD, tag: MATRIXSSL-3-7-1-OPEN, origin/master, origin/HEAD, master)
 *
 *	Summary.
 */
#import <UIKit/UIKit.h>


@protocol SslDelegateProtocol

// The delegate can receive text notifications about status and error messages.
- (void) logDebugMessage:(NSString*)message;

// The delegate can receive data from the SSL connection.
- (void) handleData:(NSString*)data;

@end
