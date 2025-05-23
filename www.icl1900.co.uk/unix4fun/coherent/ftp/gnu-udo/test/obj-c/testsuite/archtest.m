/*
  This test verifies the archival facilities
*/


#include <objc/Object.h>
#include <objc/objc-api.h>

@interface KeepTwo : Object
{
@public
  id first;
  id second;
}

-setFirst: anObject;
-setSecond: anObject;

-write: (TypedStream*)aStream;
-read: (TypedStream*)aStream;

@end

@implementation KeepTwo

-setFirst: anObject { first = anObject; }
-setSecond: anObject { second = anObject; }

-write: (TypedStream*)aStream
{
  [super write: aStream];
  objc_write_types(aStream, "@@", &first, &second);
}

-read: (TypedStream*)aStream
{
  [super read: aStream];
  objc_read_types(aStream, "@@", &first, &second);
}

@end

do_read() 
{
  __label__ bad;
  KeepTwo* kt1;
  KeepTwo* kt2;

  TypedStream *s
    = objc_open_typed_stream_for_file ("archtest.data", OBJC_READONLY);

  objc_read_type(s, "@", &kt1);
  
  if (!(kt2 = kt1->first))
    goto bad;
  if (kt2->second != kt1)
    goto bad;
  if (kt1->second != nil)
    goto bad;
  if (![kt2->first isMemberOf: [Object class]])
    goto bad;

  printf ("archive facility ok\n");
  exit (0);

 bad:
  printf ("archive facility failed\n");
  exit(1);

}

do_write () 
{
  TypedStream *s
    = objc_open_typed_stream_for_file ("archtest.data", OBJC_WRITEONLY);

  KeepTwo* kt1 = [KeepTwo new];
  KeepTwo* kt2 = [KeepTwo new];

  [kt1 setFirst: kt2];
  [kt1 setSecond: nil];

  [kt2 setFirst: [Object new]];
  [kt2 setSecond: kt1];

  objc_write_object(s, kt1);

  objc_close_typed_stream(s);
}


main()
{
  do_write ();
  do_read ();
}
