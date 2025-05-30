
/* this function will print the names of the 50 states and two other options
 * to a window which is sized the same as a filename listing from a Contents
 * file. The matching size is for continuity only AND there's too damn many
 * windows to track at his point anyways.
*/

#include <stdio.h>
#include <curses.h>
#include "contents.h"
#include "maillist.h"

void print_states(win1)
WINDOW *win1;

{
int row, col;


	wclear(win1);
	for(row = 0;row<11;row++)
		for(col=1;col<75;col+=15)
			{
			wmove(win1,row,col);
			waddstr(win1,state[(row*5)+(col/15)]);
			if (((row*5) + (col/15)) == 51)
				break;
			}
	wrefresh(win1);
	
}

/* print_mail_states.c
 *
 * This will take the 'statename' passed form the calling function,
 * open the maillist file and print the records that match the statename
 * to a window in formatted columns, If there are more records that lines
 * permitted on the window, the user will be prompted to press <return> to 
 * continue on with any following screens of info.
*/

void print_mail_states(win2)
WINDOW *win2;

{
FILE *infp;
int x=2;
int y=0;

	if ((infp=fopen(workfile,"r")) == NULL)
		{
		noraw();
		endwin();
		printf("Error opening %s for input!\n", workfile);
		exit(1);
		}


	/* print column titles */

	wclear(win2);
	wmove(win2,0,0);
	wstandout(win2);
	waddstr(win2,"Sitename");
	wmove(win2,0,15);
	waddstr(win2,"Login:");
	wmove(win2,0,24);
	waddstr(win2,"State/Country:");

	/* if we are looking for a US mailsite, then show a column for
	 * cities
	*/

	if(strcmp(selection,"NON-US") != 0)
		{
		wmove(win2,0,49);
		waddstr(win2,"City/Other:");
		}
	wstandend(win2);
	wmove(win2,2,0);

	wrefresh(win2);

	/* read each record, comparing statename for matches. When a 
	  match is found, print the record */

	while ( fread(&mail_rec,sizeof(struct mail),1,infp) == 1)
		{
		if( (strcmp(selection,mail_rec.state)== 0) || ((strcmp(selection,"NON-US")==0) && (strcmp(mail_rec.city,"COUNTRY")==0)))

			{
			y++;
			wmove(win2,x,0);
			waddstr(win2,mail_rec.site);
			wmove(win2,x,16);
			waddstr(win2,mail_rec.login);
			wmove(win2,x,26);
			waddstr(win2,mail_rec.state);
			if(strcmp(selection,"NON-US")!=0)
				{
				wmove(win2,x,50);
				waddstr(win2,mail_rec.city);
				}

	/* increment our line counter, if we've filled our screen,
	 * prompt the user to press <return> to continue on reading
	 * any followinf mail entries from the file.
	*/
			x++;
			if(x==18)
				{
				wmove(win2,x,0);
				waddstr(win2,"Press <RETURN> for more");
				wrefresh(win2);
				while (13 != wgetch(win2));
				x = 2;
				wclear(win2);

		/* reprint the column titles */

				wmove(win2,0,0);
				wstandout(win2);
				waddstr(win2,"Sitename");
				wmove(win2,0,15);
				waddstr(win2,"Login:");
				wmove(win2,0,24);
				waddstr(win2,"State/Country:");

				if(strcmp(selection,"NON-US") != 0)
					{
					wmove(win2,0,49);
					waddstr(win2,"City/Other:");
					}
				wstandend(win2);

				wrefresh(win2);
				}
			}
		}
		fclose(infp);

		wmove(win2,18,0);
		wstandout(win2);
	

		/* the variable 'y' was used as a counter in the read/print
		 * loop. If y = 0, the no matches were found and an approp.
		 * message to this affect will be printed.
		*/
	
		if(y==0)
			waddstr(win2,"A place not yet heard from!");
		else
			waddstr(win2,"That's all folks!");

		wstandend(win2);
		wmove(win2,19,0);
		waddstr(win2,"Press <RETURN> to continue..");		
		wrefresh(win2);

		while(13 != wgetch(win2));
		wclear(win2);

}










