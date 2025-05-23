#import "String.h"
#include <stdio.h>
#include <string.h>

@implementation String

// Factory method


+ str: (STR) aString
{
  id newInstance = [super new];
  [newInstance initialize : strlen(aString)];
  [newInstance assignSTR  : aString];
  return newInstance;
}


-initialize: (unsigned) nBytes
{
  length = 0;
  size   = nBytes + 1;
  string = (STR) calloc(nBytes + 1, sizeof(char));
  if(string == NULL)
  {
    fprintf(stderr,"Error 13a\n");
    exit(1);
  }
  string[0] = '\0';
  return self;
}

- assignSTR : (STR) aString;
{
  strcpy(string,aString);
}

- (STR) str;
{
  return string;
}


@end;
