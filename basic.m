#import <Foundation/Foundation.h>

__attribute__((annotate("objc_kvc_container")))
@interface MyDictLike : NSObject
@end


@protocol BarProtocol;

@interface Variation : NSObject
@property (getter = isFoo) BOOL foo;
@property id<BarProtocol> barLike;
@end

@protocol BarProtocol <NSObject>
@property NSString *bar;
@end

@protocol BazProtocol <NSObject>
@property NSString *baz;
@end


static void testFn(void)
{
    NSTimer *t = nil;
    [t valueForKey:@"fireDate"];
    [t valueForKeyPath:@"fireDate.timeIntervalSinceNow"];
    [t valueForKeyPath:@"fireDate.foo.bar"]; // warn
    [t valueForKey:@"fireDate.timeIntervalSinceNow"]; // warn
    [t valueForKey:@"doesNotExist"]; // warn

    id idObj = t;
    [idObj valueForKeyPath:@"fireDate.foo.bar"];

    NSString *stringVar;
    [t valueForKey:stringVar];

    NSDictionary *d;
    [d valueForKey:@"anythingIsOk"];
    [(NSMutableDictionary *)d valueForKey:@"anythingIsOk"];
    [(MyDictLike *)d valueForKey:@"anythingIsOk"];

    NSArray *a;
    [a valueForKey:@"anythingIsOk"];
    [a valueForKey:@"@count"];

    Variation *v;
    [v valueForKey:@"foo"];
    [v valueForKeyPath:@"barLike.bar"];
    [v valueForKeyPath:@"barLike.doesNotExist"]; // warn

    NSObject <BarProtocol> *nsBar;
    [nsBar valueForKey:@"bar"];
    [nsBar valueForKey:@"doeNotExist"]; // warn

    NSObject <BarProtocol, BazProtocol> *barBaz;
    [barBaz valueForKey:@"bar"];
    [barBaz valueForKey:@"baz"];
}
