
/*
  The following test case checks if 
*/

#include <objc/Object.h>

int numInitializes = 0;

@interface Foo : Object
+initialize;
@end

@interface Foo (Too)
+initialize;
@end

@implementation Foo
+initialize { printf ("[Foo initialize]\n"); numInitializes++; };
@end

@implementation Foo (Too)
+initialize { printf ("[Foo(Too) initialize]\n"); numInitializes++; };
@end

main() {
  id x = [Foo new];
  if (numInitializes == 2)
    exit (0);
  else
    exit (1);
}
