// RUN: %key_path_validator_cc1 -verify %s

#import <Foundation/Foundation.h>

@interface Bindable : NSObject

- (void)bindToModel:(id)m keyPath:(NSString *)kp change:(void (^)(void))blk;
- (void)bindToModels:(NSArray *)ms keyPaths:(NSArray *)kps change:(void (^)(void))blk;

@end

static void testFn(void)
{
    Bindable *obj;
    NSTimer *t = nil;
    [obj bindToModel:t keyPath:@"self.fireDate" change:^{}]; // no-warning
    [obj bindToModel:t keyPath:@"fireDate.timeIntervalSinceNow" change:^{}]; // no-warning
    [obj bindToModel:[NSRunLoop mainRunLoop] keyPath:@"currentMode.length.foo" change:^{}]; // expected-warning {{key 'foo' not found on type NSNumber}}
    [obj bindToModel:[NSRunLoop mainRunLoop] keyPath:@"currentMode.length.integerValue" change:^{}]; // no-warning
    [obj bindToModel:t keyPath:@"doesNotExst" change:^{}]; // expected-warning {{key 'doesNotExst' not found on type NSTimer}}

    id idObj;
    [obj bindToModel:idObj keyPath:@"foo" change:^{}]; // no-warning

    [obj bindToModels:@[t, idObj] keyPaths:@[@[@"fireDate", @"doesNotExist"], @[@"doesNotExist"]] change:^{}]; // expected-warning {{key 'doesNotExist' not found on type NSTimer}}
}

