#import <Foundation/Foundation.h>

static void testFn(void)
{
    NSTimer *t = nil;
    [t valueForKey:@"fireDate"];
    [t valueForKeyPath:@"fireDate.timeIntervalSinceNow"];
    [t valueForKeyPath:@"fireDate.foo.bar"]; // warn
    [t valueForKey:@"fireDate.timeIntervalSinceNow"]; // warn
    [t valueForKey:@"doesNotExist"]; // warn

    NSString *stringVar;
    [t valueForKey:stringVar];

    NSDictionary *d;
    [d valueForKey:@"anythingIsOk"];
}
