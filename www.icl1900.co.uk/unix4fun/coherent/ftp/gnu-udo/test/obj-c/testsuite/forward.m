#include <stdio.h>
#include <objc/Object.h>

id delegate;

id old_self = 0;
char* self_name = 0;
int self_num = 0;

@interface Foo : Object
{
    int num;
}
- initNum:(int)n;
- setNum:(int)n;
- (int)num;
- print;
@end
@implementation Foo
- initNum:(int)n { [super init]; num = n; return self; }
- setNum:(int)n { num = n; return self; }
- (int)num { return num; }
- print
{
  printf("0x%x, %s: num=%d\n", (unsigned)self, [self name], num);
  if (old_self)
    {
      if ((old_self != self)
	  || (strcmp([self name], self_name))
	  || (num != self_num))
	exit (1);
      else
	exit (0);
    }
  else
    {
      old_self = self;
      self_name = (char*)[self name];
      self_num = num;
    }

  return self;
}
@end

@interface Bar : Object
- forward:(SEL)aSel :(arglist_t)args;
@end
@implementation Bar
- forward:(SEL)aSel :(arglist_t)args
{
    return [delegate performv:aSel :args];
}
@end

main()
{
    id b = [[Bar alloc] init];

    delegate = [[Foo alloc] initNum:33];
    [delegate print];
    printf("should be the same as:\n");
    [b print];
    exit (0);
}
