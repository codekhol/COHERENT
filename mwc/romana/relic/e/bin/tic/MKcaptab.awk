#*********************************************************************
#                         COPYRIGHT NOTICE                           *
#*********************************************************************
#        This software is copyright (C) 1982 by Pavel Curtis         *
#                                                                    *
#        Permission is granted to reproduce and distribute           *
#        this file by any means so long as no fee is charged         *
#        above a nominal handling fee and so long as this            *
#        notice is always included in the copies.                    *
#                                                                    *
#        Other rights are reserved except as explicitly granted      *
#        by written permission of the author.                        *
#                Pavel Curtis                                        *
#                Computer Science Dept.                              *
#                405 Upson Hall                                      *
#                Cornell University                                  *
#                Ithaca, NY 14853                                    *
#                                                                    *
#                Ph- (607) 256-4934                                  *
#                                                                    *
#                Pavel.Cornell@Udel-Relay   (ARPAnet)                *
#                decvax!cornell!pavel       (UUCPnet)                *
#********************************************************************/

#
#  $Header: /src386/usr/bin/tic/RCS/MKcaptab.awk,v 1.1 92/03/13 10:45:33 bin Exp $
#


BEGIN	{
	    print  "/*"
	    print  " *	comp_captab.c -- The names of the capabilities in a form ready for"
	    print  " *		         the making of a hash table for the compiler."
	    print  " *"
	    print  " */"
	    print  ""
	    print  ""
	    print  "#include \"compiler.h\""
	    print  "#include \"term.h\""
	    print  ""
	    print  ""
	    print  "struct name_table_entry	cap_table[] ="
	    print  "{"
	}


$3 == "bool"	{
		    printf "\t0,%15s,\tBOOLEAN,\t%3d,\n", $2, BoolCount++
		}


$3 == "number"	{
		    printf "\t0,%15s,\tNUMBER,\t\t%3d,\n", $2, NumCount++
		}


$3 == "str"	{
		    printf "\t0,%15s,\tSTRING,\t\t%3d,\n", $2, StrCount++
		}


END	{
	    print  "};"
	    print  ""
	    printf "struct name_table_entry *cap_hash_table[%d];\n",(BoolCount + NumCount + StrCount) * 2
	    print  ""
	    printf "int	Hashtabsize = %d;\n",(BoolCount + NumCount + StrCount) * 2
	    printf "int	Captabsize = %d;\n", BoolCount + NumCount + StrCount
	    print  ""
	    print  ""
	    printf "#if (BOOLCOUNT!=%d)||(NUMCOUNT!=%d)||(STRCOUNT!=%d)\n",BoolCount, NumCount, StrCount
	    print  "	--> term.h and comp_captab.c disagree about the <--"
	    print  "	--> numbers of booleans, numbers and/or strings <--"
	    print  "#endif"
	}
