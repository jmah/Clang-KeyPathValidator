//
//  Employee.m
//  KVC Warning Test
//
//  Created by Jonathon Mah on 2014-05-15.
//  Copyright (c) 2014 Jonathon Mah. All rights reserved.
//

#import "Employee.h"
#import "Department.h"

@implementation Employee

- (NSString *)description
{
	NSString *leadName;
	leadName = [self valueForKeyPath:@"department.laedEmployee.fulllName"]; // oops

	leadName = [self valueForKeyPath:@"department.leadEmployee.fulllName"]; // oops

	leadName = [self valueForKey:@"department.leadEmployee.fullName"]; // oops

	leadName = [self valueForKeyPath:@"department.leadEmployee.fullName"];

	return [NSString stringWithFormat:@"<%@ %p '%@' departmentLead=%@>",
			[self class], self, [self valueForKey:@"fullName"], leadName];
}


+ (NSSet *)keyPathsForValuesAffectingFullName
{
	return [NSSet setWithObjects:
			@"firstName",
			@"fristName", // oops
			@"lastName",
			nil];
}

- (NSString *)fullName
{
	return [@[self.firstName, self.lastName] componentsJoinedByString:@" "];
}


+ keyPathsForValuesAffectingTestDeps
{
	return [NSSet setWithObjects:
			@"self",
			@"privateDeclaredBeneath",
			@"fulllName", // oops
			nil];
}
- (id)testDeps
{ return nil; }

- (id)privateDeclaredBeneath
{ return nil; }

@end
