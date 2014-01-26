#import <Foundation/Foundation.h>

@interface Bindable : NSObject

- (void)bindToModel:(id)m keyPath:(NSString *)kp change:(void (^)(void))blk;

@end

static void testFn(void)
{
    Bindable *obj;
    NSTimer *t = nil;
    [obj bindToModel:t keyPath:@"fireDate" change:^{}];
    [obj bindToModel:t keyPath:@"doesNotExst" change:^{}];
}
