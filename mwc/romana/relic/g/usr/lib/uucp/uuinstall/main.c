
#include "uuinstall.h"

main()
{
	char choice;
	int position;			/* position of entry within file,
					 * returned by get_entry.
					 */

	umask(0177);
	init_curses();			/* initialize our curses stuff */


	do{
		dialflag = 0;		/* set our flags to a default condition */
		sysflag = 0;
		portflag = 0;

		prevrow = 0;		/* set up our coordinates to highlite */
		prevcol = 1;		/* an entry */
		newrow = 0;
		newcol = 1;

		open_menu();		/* print our opening (main) menu */

		choice = get_main_opt();   /* get user's choice of file to
					    * work with or possibly exit.
					    */

/* Here's the scoop: Now that we have a file to work with, we need to find
 * out what to do with it... modify, add, delete or view and entry. We call
 * get_action() to determine this. get_action() will return a -1 if we're not
 * going to do anything. If we're going to view, delete or edit a file, we
 * call open_config() file to open the proper file and set up our selection
 * screen, then we  call get_entry to get the entry number in the file we
 * will work with. If we don't select an entry with get_entry(), a -1 is
 * returned, else we display the record.

 * display_rec() does more than just display a record. From there, we delete
 * an entry or modify and entry.
 */
		switch(choice){
			case 'd':	dialflag = 1;	/* work with dial file */
					action = get_action();
					if ((action != 'a') && (action != -1)){
						open_config_file();
						position = get_entry();
						if (position != -1)
							display_rec(position,action);
					}
					if(action == 'a')
						add_dial();
					break;	

			case 'p':	portflag = 1;	/* work with port file */
					action = get_action();
					if ((action != 'a') && (action != -1)){
						open_config_file();
						position = get_entry();
						if (position != -1);
							display_rec(position,action);
					}
					if(action == 'a')
						add_port();
					break;

			case 's':	sysflag = 1;	/* work with sys file */
					action = get_action();
					if ((action != 'a') && (action != -1)){
						open_config_file();
						position = get_entry();
						if(position != -1)
							display_rec(position,action);
					}
					if (action == 'a')
						add_sys();
					break;

			case 'q':	exit_program(0);
					/* NOT REACHED */
		}
		choice = '\0';
		action = '\0';
		wclear(selwin);
		wclear(promptwin);
		wrefresh(selwin);
		wrefresh(promptwin);
	}
	while (choice != 'q');
}


/* initialize curses stuff: allocate new windows, set up stdscr */

init_curses()
{

	/* This is the window that will be used to display and highlight
	 * file entry selections.
	 */
	if ((selwin = newwin(20, 80, 0 ,0)) == NULL)
		fatal("Failed to allocate selection window.");

	/* this window will be used to display prompts and some help info */
	if((promptwin = newwin(4, 40, 20, 0)) == NULL)
		fatal("Failed to allocate prompt window.");

	/* this window will do the data entry and display work as well as
	 * some prompting and help functions.
	 */
	if((portwin = newwin(24, 80, 0, 0)) == NULL)
		fatal("Failed to allocate prompt window.");


	initscr();
	raw();
	noecho();

	if(!has_colors()){
		use_colors = 0;

	}else{
		use_colors = 1;
		start_color();
		init_pair(1,COLOR_BLUE, COLOR_WHITE);
		wattron(stdscr,COLOR_PAIR(1));
	}

	clear();
	refresh();
}


/* exit_program: resets curses stuff and exits program. */

exit_program(stat)
int stat;		/* exit status passed by calling function */
{
	clear();
	refresh();
	echo();
	noraw();
	exit(stat);
}

/* open_menu() prints our opening screen where we display the name of
 * the system we happen to be and prompt to examine the port, sys or
 * dial files.
 */

open_menu()
{

	move(3,25);

	if(use_colors){
		init_pair(1,COLOR_WHITE, COLOR_RED);
	}else{
		standout(); 
	}

	printw("UUCP Configuration: Main menu");

	if(use_colors) 
		init_pair(1,COLOR_BLUE, COLOR_WHITE); 
	else
		standend();

	move(SYSLINE,5);
	printw("<s>ys file		Configuration information for specific systems");
	move(PORTLINE,5);
	printw("<p>ort file	Information for individual ports");
	move(DIALLINE,5);
	printw("<d>ial file	Configuration information for dialing modems");
	

	move(20,5);
	printw("Press the letter corresponding to the file you wish to examine");
	move(21,27);
	printw("or <q> to quit.");
	refresh();
}


/* get_main_opt():	Gets the user's choice of which file to process,
 *			which is one of the uucp config files or to exit.
 */

get_main_opt()
{

	char ch;

	ch = ' ';

	while( (ch != 'q') && (ch != 'p') && (ch != 's') && (ch != 'd')){
		ch = getch();
	}

	return (ch);
}
		

/* get_action(): The user will have selected a configuration file to work
 *		with by this time. This will prompt the user to perform
 *		one of the following functions: ADD an entry, REMOVE an
 *		existing entry or MODIFY an existing entry.
 */

get_action()
{
	char ch;
	char file[5];
	clear();

	move(1,34);
	standout();
	printw("Action menu");
	standend();

	move(5,17);

	/* determine which file we are working with from the flag(s) set and
	 * print a small message to reflect this as a cue to the user.
	 */

	if(dialflag)
		strcpy(file,"dial");
	if(sysflag)
		strcpy(file,"sys");
	if(portflag)
		strcpy(file, "port");

	printw("You have selected to work with the %s file.",file);
	move(7,17);
	printw("Do you wish to:");
	move(8,32);
	printw("<a>dd an entry");
	move(9,32);
	printw("<d>elete an existing entry");
	move(10,32);
	printw("<m>odify an existing entry");
	move(11,32);
	printw("<v>iew an existing entry");

	move(15,12);
	printw("Press the letter corresponding to the action you wish to");
	move(16,18);
	printw("perform or press <RETURN> for the main menu.");

	refresh();

	do{
		ch = getch();
	}
	while( (ch!='\n') && (ch!='a') && (ch!='d') && (ch!='m') && (ch!='v'));
	clear();
	refresh();

	if (ch == '\n')
		ch = -1;

	return (ch);
}
