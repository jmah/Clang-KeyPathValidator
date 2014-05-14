#import <Foundation/Foundation.h>

__attribute__((annotate("objc_kvc_container")))
@interface MyDictLike : NSObject
@end


@protocol BarProtocol;

@interface Variation : NSObject
@property (getter = isFoo) BOOL foo;
@property id<BarProtocol> barLike;
@end

@interface SubVariation : Variation
@property NSString *sub;
@property (readonly, getter = collectionKVCProxy) NSArray *collection;
@property (readonly, getter = funkyGetter) NSObject *funky;
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

    SubVariation *sv;
    [sv valueForKey:@"foo"];
    [sv valueForKeyPath:@"barLike.bar"];
    [sv valueForKeyPath:@"sub"];
    [sv valueForKeyPath:@"collection"];
    [sv valueForKeyPath:@"collectionKVCProxy"];
    [sv valueForKeyPath:@"funky"]; // warn
    [sv valueForKeyPath:@"funkyGetter"]; // warn

    NSObject <BarProtocol> *nsBar;
    [nsBar valueForKey:@"bar"];
    [nsBar valueForKey:@"doeNotExist"]; // warn

    NSObject <BarProtocol, BazProtocol> *barBaz;
    [barBaz valueForKey:@"bar"];
    [barBaz valueForKey:@"baz"];
}


@implementation Variation

+ (NSSet *)keyPathsForValuesAffectingBaz
{
    return [NSSet setWithObjects:
        @"foo",
        @"barLike.bar",
        @"barLike.doesNotExist", // warn
        @"doesNotExist", // warn
        nil];
}
- (id)baz
{ return @"dummy"; }

+ (NSSet *)keyPathsForValuesAffectingBob
{
    return [NSSet setWithObject:@"self"];
}
- (id)bob
{ return @"dummy"; }

+ (NSSet *)keyPathsForValuesAffectingAlice
{
    return [NSSet setWithObject:@"secret"];
}
- (id)alice
{ return @"dummy"; }

- (id)secret
{ return nil; }

+ (NSSet *)keyPathsForValuesAffectingEve
{
    return [NSSet setWithObject:@"doesNotExist"]; // warn
}
- (id)eve
{ return @"dummy"; }

@end
