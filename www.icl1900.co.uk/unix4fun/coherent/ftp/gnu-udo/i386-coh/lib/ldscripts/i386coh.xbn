OUTPUT_FORMAT("coff-i386")
SEARCH_DIR(/lib); SEARCH_DIR(/usr/lib);
ENTRY(_start)
SECTIONS
{
  .text  SIZEOF_HEADERS : {
    *(.init)
    *(.text)
    *(.fini)
     __end_text  =  .;
  }
  .data  0x400000 + (. & 0xffc00fff) : {
    *(.data .data2)
     __end_data  =  .;
  }
  .bss  SIZEOF(.data) + ADDR(.data) :
  { 					
    *(.bss)
    *(COMMON)
     __end_bss = .;
     __end = .;
  }
  .stab  . (NOLOAD) : 
  {
    [ .stab ]
  }
  .stabstr  . (NOLOAD) :
  {
    [ .stabstr ]
  }
}
