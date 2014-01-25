#import <Foundation/Foundation.h>

static void testFn(void)
{
    NSTimer *t = nil;
    [t valueForKey:@"fireDate"];
    [t self];
    [t valueForKey:@"doesNotExist"];
    NSString *stringVar;
    [t valueForKey:stringVar];
}
