// RUN: %key_path_validator_cc1 -verify %s

#import <Foundation/Foundation.h>

__attribute__((annotate("objc_kvc_container")))
@interface MyDictLike : NSObject
@end


@protocol BarProtocol;

@interface Variation : NSObject
@property (getter = isFoo) BOOL foo;
@property (assign) id<BarProtocol> barLike;
@end

@interface SubVariation : Variation
@property (copy) NSString *sub;
@property (readonly, getter = collectionKVCProxy) NSArray *collection;
@property (readonly, getter = funkyGetter) NSObject *funky;
@end

@protocol BarProtocol <NSObject>
@property (copy) NSString *bar;
@end

@protocol BazProtocol <NSObject>
@property (copy) NSString *baz;
@end


static void testFn(void)
{
    NSTimer *t = nil;
    [t valueForKey:@"fireDate"];  // no-warning
    [t valueForKeyPath:@"fireDate.timeIntervalSinceNow"]; // no-warning
    [t valueForKeyPath:@"fireDate.foo.bar"]; // expected-warning {{key 'foo' not found on type NSDate}}
    [t valueForKey:@"fireDate.timeIntervalSinceNow"]; // expected-warning {{key 'fireDate.timeIntervalSinceNow' not found on type NSTimer}}
    [t valueForKey:@"doesNotExist"]; // expected-warning {{key 'doesNotExist' not found on type NSTimer}}

    id idObj = t;
    [idObj valueForKeyPath:@"fireDate.foo.bar"]; // no-warning

    NSString *stringVar;
    [t valueForKey:stringVar];

    NSDictionary *d;
    [d valueForKey:@"anythingIsOk"]; // no-warning
    [(NSMutableDictionary *)d valueForKey:@"anythingIsOk"]; // no-warning
    [(MyDictLike *)d valueForKey:@"anythingIsOk"]; // no-warning

    NSArray *a;
    [a valueForKey:@"anythingIsOk"]; // no-warning
    [a valueForKey:@"@count"]; // no-warning

    Variation *v;
    [v valueForKey:@"foo"]; // no-warning
    [v valueForKeyPath:@"barLike.bar"]; // no-warning
    [v valueForKeyPath:@"barLike.doesNotExist"]; // expected-warning {{key 'doesNotExist' not found on type id<BarProtocol>}}

    SubVariation *sv;
    [sv valueForKey:@"foo"]; // no-warning
    [sv valueForKeyPath:@"barLike.bar"]; // no-warning
    [sv valueForKeyPath:@"sub"]; // no-warning
    [sv valueForKeyPath:@"collection"]; // no-warning
    [sv valueForKeyPath:@"collectionKVCProxy"]; // no-warning
    [sv valueForKeyPath:@"funky"]; // expected-warning {{key 'funky' not found on type SubVariation}}
    [sv valueForKeyPath:@"funkyGetter"]; // no-warning

    NSObject <BarProtocol> *nsBar;
    [nsBar valueForKey:@"bar"]; // no-warning
    [nsBar valueForKey:@"doeNotExist"]; // expected-warning {{key 'doeNotExist' not found on type NSObject<BarProtocol>}}

    NSObject <BarProtocol, BazProtocol> *barBaz;
    [barBaz valueForKey:@"bar"]; // no-warning
    [barBaz valueForKey:@"baz"]; // no-warning
}


@implementation Variation

+ (NSSet *)keyPathsForValuesAffectingBaz
{
    return [NSSet setWithObjects:
        @"foo", // no-warning
        @"barLike.bar", // no-warning
        @"barLike.doesNotExist", // expected-warning {{key 'doesNotExist' not found on type id<BarProtocol>}}
        @"doesNotExist", // expected-warning {{key 'doesNotExist' not found on type Variation}}
        nil];
}
- (id)baz
{ return @"dummy"; }

+ (NSSet *)keyPathsForValuesAffectingBob
{
    return [NSSet setWithObject:@"self"]; // no-warning
}
- (id)bob
{ return @"dummy"; }

+ (NSSet *)keyPathsForValuesAffectingAlice
{
    return [NSSet setWithObject:@"secret"]; // no-warning
}
- (id)alice
{ return @"dummy"; }

- (id)secret
{ return nil; }

+ (NSSet *)keyPathsForValuesAffectingEve
{
    return [NSSet setWithObject:@"doesNotExist"]; // expected-warning {{key 'doesNotExist' not found on type Variation}}
}
- (id)eve
{ return @"dummy"; }

@end

