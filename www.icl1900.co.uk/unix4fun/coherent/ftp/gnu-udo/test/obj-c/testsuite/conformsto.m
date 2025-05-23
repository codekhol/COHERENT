/*
   Test of [Object conformsTo:] contributed by Paul Burchard
*/

#include <objc/Object.h>
#include <stdio.h>

@protocol Foo
- foo;
@end

@interface Bar : Object <Foo>
- cow;
@end

@implementation Bar
- cow
{
  printf("cow\n");
}
- foo
{
  printf("foo\n");
}
@end

@interface Bar2 : Bar
//- foo;
@end

@implementation Bar2
/*
- foo
{
  return [self notImplemented:_cmd];
}
*/
@end

main()
{
  printf("Bar conforms to @protocol Foo %d\n", 
	 (int)[Bar conformsTo:@protocol(Foo)]);

  printf("Bar2 conforms to @protocol Foo %d\n", 
	 (int)[Bar2 conformsTo:@protocol(Foo)]);

  if ([Bar conformsTo: @protocol (Foo)]
      && [Bar2 conformsTo: @protocol (Foo)])
    exit (0);
  else
    exit (1);
}
