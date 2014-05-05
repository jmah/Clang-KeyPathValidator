#import <Foundation/Foundation.h>

@interface Bindable : NSObject

- (void)bindToModel:(id)m keyPath:(NSString *)kp change:(void (^)(void))blk;
- (void)bindToModels:(NSArray *)ms keyPaths:(NSArray *)kps change:(void (^)(void))blk;

@end

static void testFn(void)
{
    Bindable *obj;
    NSTimer *t = nil;
    [obj bindToModel:t keyPath:@"self.fireDate" change:^{}];
    [obj bindToModel:t keyPath:@"fireDate.timeIntervalSinceNow" change:^{}];
    [obj bindToModel:[NSRunLoop mainRunLoop] keyPath:@"currentMode.length.foo" change:^{}];
    [obj bindToModel:[NSRunLoop mainRunLoop] keyPath:@"currentMode.length.integerValue" change:^{}];
    [obj bindToModel:t keyPath:@"doesNotExst" change:^{}];

    id idObj;
    [obj bindToModel:idObj keyPath:@"foo" change:^{}];

    [obj bindToModels:@[t, idObj] keyPaths:@[@[@"fireDate", @"doesNotExist"], @[@"doesNotExist"]] change:^{}];
}
