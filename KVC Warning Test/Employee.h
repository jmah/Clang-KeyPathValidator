//
//  Employee.h
//  KVC Warning Test
//
//  Created by Jonathon Mah on 2014-05-15.
//  Copyright (c) 2014 Jonathon Mah. All rights reserved.
//

#import <Foundation/Foundation.h>
@class Department;

@interface Employee : NSObject

@property (nonatomic, copy) NSString *firstName, *lastName;
@property (nonatomic, readonly) NSString *fullName;

@property (nonatomic) Department *department;

@end
