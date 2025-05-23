/* add_sys.c: works just like add_port.c, see comments there. */

#include "uuinstall.h"

add_sys()
{
	char b;
	FILE *sysfd;

	sys_template();
	wrefresh(portwin);

	b = get_sys_data();
	wclear(portwin);
	wrefresh(portwin);
	if (b == 'y'){
		if( (sysfd = fopen(SYSFILE,"a")) == NULL){
			mvwaddstr(portwin,12,28,"Error opening sys file!");
			wrefresh(portwin);
			sleep(1);
			wclear(portwin);
			wrefresh(portwin);
			return;
		}
			
		wclear(portwin);
		mvwaddstr(portwin,12,32,"Adding entry...");
		wrefresh(portwin);
		sleep(1);

		fprintf(sysfd,"\n%s\n",sysname);				
		fprintf(sysfd,"%s\n",systime);
		fprintf(sysfd,"%s\n",sysspeed);
		if (strlen(sysport) > 0)
			fprintf(sysfd,"%s\n",sysport);
		if (strlen(sysphone) > 0)
			fprintf(sysfd,"%s\n",sysphone);
		fprintf(sysfd,"%s\n",syschat);
		if (strlen(sysmyname) > 0)
			fprintf(sysfd,"%s\n",sysmyname);

		fprintf(sysfd,"%s\n",sysprot);
		fprintf(sysfd,"%s\n",syscmds);
		fprintf(sysfd,"%s\n",sysread);
		fprintf(sysfd,"%s\n",syswrite);

		fclose(sysfd);
	}
	wclear(portwin);
	wrefresh(portwin);

}

/* sys_template(): draw a template for a sys file entry */

sys_template()
{
	mvwaddstr(portwin,3,1,"system: ");
	mvwaddstr(portwin,4,1,"time: ");
	mvwaddstr(portwin,5,1,"speed: ");
	mvwaddstr(portwin,6,1,"port: ");
	mvwaddstr(portwin,7,1,"phone: ");
	mvwaddstr(portwin,8,1,"chat: ");
	mvwaddstr(portwin,9,1,"myname: ");
	mvwaddstr(portwin,10,1,"protocol: ");
	mvwaddstr(portwin,11,1,"commands: ");
	mvwaddstr(portwin,12,1,"read path: ");
	mvwaddstr(portwin,13,1,"write path: ");

	wstandout(portwin);
	mvwaddstr(portwin,0,29,"Sys File Entry Screen");
	mvwaddstr(portwin,3,12,"        ");
	mvwaddstr(portwin,4,12,"                                             ");
	mvwaddstr(portwin,5,12,"     ");
	mvwaddstr(portwin,6,12,"              ");
	mvwaddstr(portwin,7,12,"                                             ");
	mvwaddstr(portwin,8,12,"                                                       ");
	mvwaddstr(portwin,9,12,"        ");
	mvwaddstr(portwin,10,12,"    ");
	mvwaddstr(portwin,11,12,"                                                       ");
	mvwaddstr(portwin,12,12,"                                                       ");
	mvwaddstr(portwin,13,12,"                                                       ");
	wstandend(portwin);
	wrefresh(portwin);
}


/* get_sys_data(): functions just like get_port_data(). See add_port.c */

get_sys_data()
{
	char *workstring;
	char b;
	/* initialize fields we will use */

	strcpy(sysname,"system "); 
	strcpy(systime,"time ");
	strcpy(sysspeed,"baud ");
	strcpy(sysport,"port ");
	strcpy(sysphone,"phone ");
	strcpy(syschat,"chat ");
	strcpy(sysmyname,"myname ");
	strcpy(sysprot,"protocol ");
	strcpy(syscmds,"commands ");
	strcpy(sysread,"read ");
	strcpy(syswrite,"write ");

	/* get a remote system name */
	mvwaddstr(portwin,20,0,"Enter the name of a remote uucp system. You should");
	mvwaddstr(portwin,21,0,"limit the name to 8 characters.");
	mvwaddstr(portwin,23,0,"Leaving this field blank aborts entry.");
	wrefresh(portwin);

	workstring = get_data(portwin,3,12,8,0,0);
	if (strlen(workstring) == 0)
		return('n');

	strcat(sysname,workstring);
	strcpy(workstring,"");

	/* get a time to call */
	wmove(portwin,20,0);
	wclrtobot(portwin);

	mvwaddstr(portwin,20,0,"Enter a range of times to call this sytem. Each range must be separated");
	mvwaddstr(portwin,21,0,"a comma or semi-colon. Please refer to your manual for more information.");
	mvwaddstr(portwin,23,0,"Examples:");
	wstandout(portwin);
	mvwaddstr(portwin,23,11,"Su0100-0200,Mo0400-1600");
	mvwaddstr(portwin,23,35,"MoFr0300-0500");
	mvwaddstr(portwin,23,49,"Any");
	mvwaddstr(portwin,23,53,"Never");
	wstandend(portwin);
	wrefresh(portwin);

	workstring = get_data(portwin,4,12,45,0,0);
	if (strlen(workstring) == 0)
		strcpy(workstring,"Never");
	strcat(systime,workstring);
	strcpy(workstring,"");

	/* get the speed of the connection */

	do{
		strcpy(workstring,"");
		wmove(portwin,20,0);
		wclrtobot(portwin);
		mvwaddstr(portwin,20,0,"Enter the baud rate for communicating");
		mvwaddstr(portwin,21,0,"this remote system.");
		wrefresh(portwin);
		workstring = get_data(portwin,5,12,5,1,0);
	}
	while(0 == atoi(workstring));
	strcat(sysspeed,workstring);
	strcpy(workstring,"");

	/* get the port that the call will be made on */

	wmove(portwin,20,0);
	wclrtobot(portwin);
	mvwaddstr(portwin,20,0,"Enter the name of the port (from the port file)");
	mvwaddstr(portwin,21,0,"that this system will be called with.");
	wrefresh(portwin);
	workstring = get_data(portwin,6,12,14,0,1);
	if (strlen(workstring) == 0)
		strcpy(sysport,"");
	else
		strcat(sysport,workstring);


	/* get the phone number, if any is required */
	wmove(portwin,20,0);
	wclrtobot(portwin);
	mvwaddstr(portwin,20,0,"Enter the phone number to call this remote system with. If this is a direct");
	mvwaddstr(portwin,21,0,"connection, then press return to leave this field blank.");
	mvwaddstr(portwin,23,0,"Examples:");
	wstandout(portwin);
	mvwaddstr(portwin,23,11,"17085590412");
	mvwaddstr(portwin,23,24,"9,5590412");
	wrefresh(portwin);
	workstring = get_data(portwin,7,12,45,0,0);
	if (strlen(workstring) == 0)
		strcpy(sysphone,"");
	else
		strcat(sysphone,workstring);

	strcpy(workstring,"");


	/* now the killer, get the chat script. The killer isn't actually
	 * getting the script, it's the help info that's a bear.
	 */

	wmove(portwin,20,0);
	wclrtobot(portwin);
	mvwaddstr(portwin,15,0,"Enter the chat script used to log into the remote system. A chat script consists");
	mvwaddstr(portwin,16,0,"of pairs of expect_msgs and send_msgs. These messages are separated by a space.");
	mvwaddstr(portwin,17,0,"To represent a space within an expect_msg or send_msg, enter '\\s'.");
	mvwaddstr(portwin,18,0,"Example:");
	wstandout(portwin);
	mvwaddstr(portwin,18,10,"\"\" in:--BREAK--in: mylogin word: mypassword");
	wstandend(portwin);
	mvwaddstr(portwin,19,0,"Example:");
	wstandout(portwin);
	mvwaddstr(portwin,19,10,"\"\" in:--in: nuucp word: public word: 123456789");
	wstandend(portwin);
	mvwaddstr(portwin,21,0,"Escape characters that may be used in a chat script:");
	mvwaddstr(portwin,22,0,"\\b (backspace)  \\N (null)            \\s (space)  \\d (delay)  BREAK (break)");
	mvwaddstr(portwin,23,0,"\\n (newline)    \\r(carriage return)  \\t (tab)    \\K (break)  EOT (control-d)");
	wrefresh(portwin);

	workstring = get_data(portwin,8,12,55,1,0);
	strcat(syschat,workstring);
	strcpy(workstring,"");

	/* get a myname alias */
	wmove(portwin,15,0);
	wclrtobot(portwin);
	mvwaddstr(portwin,20,0,"If you need to identify your system as something other than the contents of");
	mvwaddstr(portwin,21,0,"/etc/uucpname when calling this remote system, enter that name here.");
	mvwaddstr(portwin,23,0,"Enter nothing to ignore this field.");
	wrefresh(portwin);
	workstring = get_data(portwin,9,12,8,0,0);
	if (strlen(workstring) == 0)
		strcpy(sysmyname,"");
	else
		strcat(sysmyname,workstring);

	strcpy(workstring,"");

	/* get protocols */

	wmove(portwin,20,0);
	wclrtobot(portwin);
	mvwaddstr(portwin,15,0,"Taylor uucp supports various protocols which may or may not be supported by");
	mvwaddstr(portwin,16,0,"the remote system being described. In part, these protocols are 'g', 'f' and");
	mvwaddstr(portwin,17,0,"'i'. Please refer to your manual for more information on available protocols.");
	mvwaddstr(portwin,18,0,"If uncertain about which protocol(s) to use, remember that the 'g' protocol is.");
	mvwaddstr(portwin,19,0,"common protocol supported amoung the various flavors of uucp.");
	mvwaddstr(portwin,20,0,"If nothing is entered in this field, 'g' protocol will be used by default.");
	wrefresh(portwin);

	workstring = get_data(portwin,10,12,5,0,0);
	if (strlen(workstring) == 0){
		strcpy(workstring,"g");
		wstandout(portwin);
		mvwaddstr(portwin,10,12,"g");
		wstandend(portwin);
		wrefresh(portwin);
	}
	strcat(sysprot,workstring);
	strcpy(workstring,"");

	/* get a list of commands */

	wmove(portwin,15,0);
	wclrtobot(portwin);
	mvwaddstr(portwin,15,0,"Specify a list of commands the remote system may execute on this system. If");
	mvwaddstr(portwin,16,0,"you enter nothing, a default list will be used, consisting of rnews, rmail, uux");
	mvwaddstr(portwin,17,0,"and uucp.");
	wrefresh(portwin);

	workstring = get_data(portwin,11,12,55,0,0);
	if (strlen(workstring) == 0){
		strcpy(workstring,"rmail rnews uucp uux");
		strcat(syscmds,"rmail rnews uucp uux");
		wstandout(portwin);
		mvwaddstr(portwin,11,12,workstring);
		wstandend(portwin);
		wrefresh(portwin);
	}else{
		strcat(syscmds,workstring);
	}
	strcpy(workstring,"");

	/* get a read pathlist */
	wmove(portwin,15,0);
	wclrtobot(portwin);
	mvwaddstr(portwin,15,0,"Specify a list of directories that the remote system may download from. To");
	mvwaddstr(portwin,16,0,"exclude a directory, precede the directory path with a '!'.");
	mvwaddstr(portwin,17,0,"Example:");
	wstandout(portwin);
	mvwaddstr(portwin,17,11,"/usr/spool/uucppublic /tmp !/usr/spool/uucppublic/my_files");
	wstandend(portwin);
	mvwaddstr(portwin,19,0,"If nothing is entered, a default of /usr/spool/uucppublic will be used.");
	wrefresh(portwin);

	workstring = get_data(portwin,12,12,55,0,0);
	if (strlen(workstring) == 0){
		strcpy(workstring,"/usr/spool/uucppublic");
		strcat(sysread,"/usr/spool/uucppublic");
		wstandout(portwin);
		mvwaddstr(portwin,12,12,workstring);
		wstandend(portwin);
		wrefresh(portwin);
	}else{
		strcat(sysread,workstring);
	}
	strcpy(workstring,"");

	/* get a write path */
	mvwaddstr(portwin,15,0,"Specify a list of directories that the remote system may send files to. To   ");
	wrefresh(portwin);
	workstring = get_data(portwin,13,12,55,0,0);
	if (strlen(workstring) == 0){
		strcpy(workstring,"/usr/spool/uucppublic");
		strcat(syswrite,"/usr/spool/uucppublic");
		wstandout(portwin);
		mvwaddstr(portwin,13,12,workstring);
		wstandend(portwin);
		wrefresh(portwin);
	}else{
		strcat(syswrite,workstring);
	}

	strcpy(workstring,"");

	wmove(portwin,15,0);
	wclrtobot(portwin);
	mvwaddstr(portwin,17,23,"Do you wish to save this entry? (y/n)");
	wrefresh(portwin);

	do{
		b = wgetch(portwin);
	}
	while ((b != 'n') && (b != 'y'));

	return(b);

}
