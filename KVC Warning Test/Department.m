//
//  Department.m
//  KVC Warning Test
//
//  Created by Jonathon Mah on 2014-05-15.
//  Copyright (c) 2014 Jonathon Mah. All rights reserved.
//

#import "Department.h"
#import "Employee.h"


@implementation Department {
	NSMutableSet *_backingEmployees;
}

- (id)init
{
	if (!(self = [super init]))
		return nil;
	_backingEmployees = [NSMutableSet new];
	return self;
}


- (NSString *)description
{
	NSSet *employeesProxy = self.employees;
	employeesProxy = [self valueForKeyPath:@"employees"]; // ok, despite no -employees method

	NSNumber *active = [self valueForKey:@"active"]; // ok, uses -isActive

	id foo = self.foo;
	foo = [self valueForKey:@"foo"]; // oops
	foo = [(id)self valueForKey:@"foo"]; // cast to id suppresses warning
	foo = [self valueForKey:@"fooNotValueForKeyCompliant"];

	return [NSString stringWithFormat:@"<%@ %p employees=%@ active=%@>",
			[self class], self, employeesProxy, active];
}


#pragma mark Employees Collection

- (NSMutableSet *)employeesKVCProxy
{ return [self valueForKey:@"employees"]; }

// "Unordered Accessor Pattern" <https://developer.apple.com/library/ios/documentation/Cocoa/Conceptual/KeyValueCoding/Articles/AccessorConventions.html#//apple_ref/doc/uid/20002174-SW5>
- (NSUInteger)countOfEmployees
{ return _backingEmployees.count; }

- (NSEnumerator *)enumeratorOfEmployees
{ return [_backingEmployees objectEnumerator]; }

- (Employee *)memberOfEmployees:(Employee *)employee
{ return [_backingEmployees member:employee]; }

- (void)addEmployeesObject:(Employee *)employee
{ [_backingEmployees addObject:employee]; }

- (void)removeEmployeesObject:(Employee *)employee
{ [_backingEmployees removeObject:employee]; }

@end
