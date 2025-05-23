/****************************************************************************
* color.c		  		  Date: 08-03-92   		    *
*        	 			Author: Bobby L. Billiot	    *
*        					488 Yew Dr.     	    *
*        					Westwego, La 70094	    *
*        				    Hm:	(504) 347-7946    	    *
*									    *
* This program uses ASCII escape codes to change the screen colors.	    *
* I decided to write this after I downloaded the screen program from        *
* the mwcbbs.  I basicly added a friendly interface.                        *
*									    *
* To compile:  cc color.c                                                   *
*									    *
* Once compiled you can move the file to /usr/bin so everyone can use it.   *
* If you type color and return.  The program will display all available     *
* colors and prompt you for you choice.  An example window will show you    *
* how you colors will appear.  Once you find a color pattern to your taste. *
* You can add the command color # (where # is you color pattern) to you     *
* .profile.  I hope you enjoy this program.				    *
****************************************************************************/

#include <stdio.h>

struct color_table{
	int foreground;
	int background;
};

main(argc, argv)
int argc; char *argv[];
{
struct color_table color[64];
int i,t,cnt, code, col, row;
char select[5];

for(i=30;i < 38; i++)
{
	for(t=40;t < 48; t++)
	{
	color[cnt].foreground=t;
	color[cnt].background=i;
	cnt++;
	}
}
if (argc == 1)
{
printf("\033[0m");                              /* Turn off all attributes.*/
printf("\033[2J");                              /* Clear the screen.*/

cnt=0;
col=40;row=1;
for(i=30;i < 38; i++)
{
	for(t=40;t < 48; t++)
	{
	printf("\033[%d;%df\033[%dm\033[%dm%3d\033[0m", row, col, i, t, cnt);
	cnt++;
	col=col + 5;
	}
	row++;col=40;
	printf("\n");
}

for(i=30;i < 38; i++)
{
	for(t=40;t < 48; t++)
	{
	printf("\033[1m",i);
	printf("\033[%d;%df\033[%dm\033[%dm%3d\033[0m", row, col, i, t, cnt);
	cnt++;
	col=col + 5;
	}
	row++;col=40;
	printf("\n");
}
cnt=60;code=60;
while(1)
{

	printf("\033[%dm\033[%dm", color[cnt].foreground,color[cnt].background);
	printf("\033[5;5f*********************");
	printf("\033[6;5f*                   *");
	printf("\033[7;5f*      Example      *");
	printf("\033[8;5f*       Text        *");
	printf("\033[9;5f*                   *");
	printf("\033[10;5f*********************");


	printf("\033[0m");              /* Turn off all attributes.*/
	printf("\033[20;63f             ");
	printf("\033[1m",i);
	printf("\033[%dm\033[%dm", color[60].foreground,color[60].background);
	printf("\033[20;20fEnter the color scheme # or Return to set:    ");
	printf("\033[20;63f");

	gets(select);

	printf("\033[0m");              /* Turn off all attributes.*/

	if (strlen(select) == 0)
	{
	if (code > 63)
	{
		printf("\033[1m",i);
	}
	printf("\033[%dm\033[%dm", color[cnt].foreground,color[cnt].background);
	printf("\033[2J");                      /* Clear the screen.*/
	exit(0);
	}

	code = atoi(select);
	if(code < 128)
	{
	if (code > 63)
	{
		printf("\033[1m",i);
		cnt = code - 64;
	}
	else
		cnt = code;

	printf("\033[%dm\033[%dm", color[cnt].foreground,color[cnt].background);
	}
	else
	{
	printf("Color code must be in the range of 0 - 127.\007\n");
	printf("Enter to continue.\n");
	printf("\033[20;63f");
	getchar();
	printf("\033[21;0f                                              ");
	printf("\033[22;0f                                              ");
	}
}
}
else
{
	if((cnt=atoi(argv[1])) > 127)
	{
	printf("Color code must be in the range of 0 - 127.\007\n");
	exit(0);
	}

	printf("\033[0m");              /* Turn off all attributes.*/
	
	if (cnt > 63)
	{
		printf("\033[1m",i);
		cnt = cnt - 64;
	}
	printf("\033[%dm\033[%dm", color[cnt].foreground,color[cnt].background);
	printf("\033[2J");                      /* Clear the screen.*/
}
}
