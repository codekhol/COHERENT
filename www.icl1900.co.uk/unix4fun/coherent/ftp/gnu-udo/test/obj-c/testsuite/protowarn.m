
/*
    This is a sample to illustrate the meaning of `-Wprotcol' vs
    `-Wno-protocol'.  This warning is (stupidly) turned on by default.
*/

#include <objc/Object.h>


@protocol Printable
-print;
@end

@interface Dyt : Object
-print;
@end

@interface Bam : Dyt
-doNothing;
@end

@interface Slam : Bam
-doIt;
@end




@implementation Slam

-doIt
{
  printf("Who cares...\n");
}


@end


@interface Slam (Printing) <Printable>
- fooBar;
@end

@implementation Slam (Printing)
- fooBar {}
@end

main ()
{
  id <Printable> xx = [Slam new];

  [xx print];
  exit (0);
}
