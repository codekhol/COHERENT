#import <objc/Object.h>

@interface String:Object
{
  unsigned size;
  unsigned length;
  STR string;
}


+ str: (STR) aString;

- initialize: (unsigned) nBytes;
- assignSTR: (STR) aString;
- (STR) str;

@end
