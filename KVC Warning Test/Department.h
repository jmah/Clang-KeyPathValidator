//
//  Department.h
//  KVC Warning Test
//
//  Created by Jonathon Mah on 2014-05-15.
//  Copyright (c) 2014 Jonathon Mah. All rights reserved.
//

#import <Foundation/Foundation.h>
@class Employee;

@interface Department : NSObject

@property (nonatomic, copy) NSString *name;
@property (nonatomic) Employee *leadEmployee;

@property (nonatomic, getter = isActive) BOOL active;
@property (nonatomic, getter = fooNotValueForKeyCompliant) id foo;

@property (nonatomic, readonly, getter = employeesKVCProxy) NSMutableSet *employees;

@end
