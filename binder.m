#import <Foundation/Foundation.h>

@interface Bindable : NSObject

- (void)bindToModel:(id)m keyPath:(NSString *)kp change:(void (^)(void))blk;

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
}
