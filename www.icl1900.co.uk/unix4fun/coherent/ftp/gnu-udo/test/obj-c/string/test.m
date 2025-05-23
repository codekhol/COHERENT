#import "String.h"
#include <stdio.h>

void main()
{	
  String *theString = [String str: "A short String\n"];
  printf([theString str]);
}
