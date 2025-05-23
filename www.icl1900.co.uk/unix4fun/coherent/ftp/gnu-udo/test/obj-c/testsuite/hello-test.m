#include <objc/objc.h>
#include <objc/Object.h>

static char     *p;

@interface Object (Private)

+ initialize;
- show;
@end

@implementation Object (Private)

+ initialize
{
    p = "Hello, the world\n";
    return self;
}

- show
{
    fprintf (stderr, "%s\n", p);
    return self;
}

@end

main (void)
{
    id  this_object;

    this_object = [Object new];
    [this_object show];

    exit (0);
}
