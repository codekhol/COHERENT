#include <stdio.h>
#include <objc/Object.h>

@interface Object(MyCategory)
- foo;
@end
@implementation Object(MyCategory)
- foo { printf("foo\n"); }
@end

main()
{
    id o = [[Object alloc] init];
    exit (0);
}
