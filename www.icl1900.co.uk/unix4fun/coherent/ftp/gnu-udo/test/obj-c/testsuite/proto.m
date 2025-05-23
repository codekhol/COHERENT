
/*
  This program simply tests if protocols works at all.  
*/

#include <objc/Object.h>


@protocol Printable
-print;
@end

@interface Slam : Object <Printable>
-print;
-doIt;
@end


@implementation Slam

-print
{
  printf("Hello world!\n");
}

-doIt
{
  printf("Who cares...\n");
}


@end


main ()
{
  id <Printable> xx = [Slam new];

  [xx print];

  exit (0);
}
