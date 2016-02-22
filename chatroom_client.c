#include<curses.h> /* add stdio.h automatically */
#include<sys/socket.h>
#include<netinet/in.h>
#include<stdlib.h>
#include<sys/types.h>
#include<string.h>
#include<strings.h>
#include<math.h>
#include<pthread.h>
#include<signal.h>
#define CLIENTNUM 21//one for root
void storehistory(void);
//function for curses
void initial(void);
void special(int attrs,WINDOW *swin,int y,int x,char *text);
int terminal(WINDOW *twin,char *str,int n);
void membermod(void);
void selectmod(void);
char* omitname(char *str);
char omitstr[15];
///funcrion for socket
void recemsg(void);
void escape(int a);
int climission(void);
void redraw(int mod);
void memberctrl(char *mod,char *name);
//dimentation for socket

struct clientinfo {
	int hold;
	char name[10];
};

struct clientinfo client[CLIENTNUM+1]={
					   	[1 ... CLIENTNUM]={
							.hold=0 },
						[0]={
							.hold=1,
					   		.name="All"	}
};
int memberlist[CLIENTNUM+1]={[0]=0};
int online=0;

char buffer0[310];//to receive msg

char myname[11];
char receivername[11]={"All"};

int serverfd;
int clientfd;
int PORT;
char IP[17];
int ch;
char non;//for getchar nothing
//dimentation for curses
WINDOW *win[7],*curwin,*helpwin;
int onlinetop=0;//reference point for online box to scrl
char history[300][300]={"\0"};
int topline=0;//for communication box 
int curline=0;//for history
int localine=0;//for communication bodx
int x,y;
int new=1;
//start main program
int main(){
	char output[17];
	char stroutput[300];
	signal(SIGINT,escape);
	int i;
	strcpy(myname,"nobody");
	// input name
	printf("Input your name(less than 10 letters):");
	fgets(myname,sizeof(myname),stdin);
	for(i=0;i<sizeof(myname);i++){
		if(myname[i]=='\n'){
			myname[i]='\0';
		}
	}

	setbuf(stdin,NULL);

	if(climission()==-1){
		printf("port not be used\n");
		escape(1);
	}

	// use thread to listen msg //
	pthread_t id;
	int ret;
	ret = pthread_create(&id,NULL,(void *)recemsg,NULL);
	if(ret!=0){
		printf("create pthread error!\n");
		exit(1);
	}

	sprintf(output,"name %s",myname);
	send(serverfd,output,sizeof(output),0);
//
	// start curses terminal

	int hch;//helpwin ch for getch

	initial();

	win[2]=newwin(CLIENTNUM+2,12,0,COLS-13);// online box

	win[3]=newwin(3,COLS-13,0,0);//command box
	win[0]=newwin(300,COLS-13,3,0); // set communication box

	win[6]=newwin(5,2,LINES-5,0);// > to whom box (in insert box
	win[1]=newwin(5,COLS-23,LINES-5,2); // set insert box
	win[4]=newwin(5,20,LINES-5,COLS-21); //opinion line

	win[5]=newwin(1,15,LINES-5+win[1]->_maxy,win[1]->_maxx-12);

	keypad(win[0],TRUE);
   	keypad(win[1],TRUE);
	keypad(win[2],TRUE);
	keypad(win[3],TRUE);

	wsetscrreg(win[0],0,299);
	scrollok(win[0],TRUE);// let box can be scrolled
	scrollok(win[2],TRUE);
	idlok(win[0],TRUE);
	leaveok(win[0],TRUE);

	helpwin=newwin(3,30,2,COLS/2-15);
	box(helpwin,'|','-');
	mvwaddstr(helpwin,0,10,"help");
	mvwaddstr(helpwin,1,4," press any key to continue...");
	
	curwin=win[1];
	redraw(0);//need curwin
//	sleep(5);

//	refresh();
	wmove(win[1],1,0);
//	mvwaddstr(win[1],1,1,">");//
	mvwaddstr(win[6],1,1,">");
	wrefresh(win[6]);	
	wrefresh(win[1]);
	mvwprintw(win[2],0,4,"All");
	wrefresh(win[2]);
	getyx(win[1],y,x);
	
	char string[300];
	while(1){
		if (wmove(win[1],1,0) == ERR){
			printf("wmove ERR\n");
			escape(0);
		}
		wrefresh(win[1]);
		if(terminal(win[1],string,280)==ERR){
			printf("terminal ERR\n");
			escape(0);
		}
		if(strcmp(receivername,"All")==0){
			sprintf(stroutput,"wall %s",string);
		}
		else{
			sprintf(stroutput,"write %s %s",receivername,string);
		}
//		mvwprintw(win[3],0,0,"wirte            %d fd=%d",send(serverfd,stroutput,sizeof(stroutput),0));
		send(serverfd,stroutput,sizeof(stroutput),0);
//		wrefresh(win[3]);
		wclear(win[1]);
//		mvwprintw(win[1],1,1,">");
		mvwaddstr(win[6],1,1,">");
		wrefresh(win[6]);
		redraw(1);
	}
	return 0;
}

void escape(int a){
	endwin();
	close(serverfd);
	if(a==0){
		puts("Exit chatroom");
	}
	else if(a==1){
		puts("connection break");
	}
	else if(a==2){
		puts("recvmsg break;");
	}
	exit(0);
	return;
}

void recemsg(void){
	char output[400];
	char string[300]={'\0'};
	int len;
	int i;
//	int inserty,insertx;
	char mod[10];
	char sendername[10];

	while(1){
	// Receive message from the server and print to screen
		len = recv(serverfd, buffer0, sizeof(buffer0),0);

		if(len==0){
			escape(2);
			break;
		}
		else if(len>=1){
			sscanf(buffer0,"%s",mod);
//				mvwprintw(win[3],0,15,"%s","recv");
//				wrefresh(win[3]);
			if(strcmp(mod,"public")==0){
				sscanf(buffer0,"%*s %s %[^\n]",sendername,string);
				sprintf(output,"%s:%s",sendername,string);
			}
			else if(strcmp(mod,"private")==0){
				sscanf(buffer0,"%*s %[^\n]",string);
				sprintf(output,"%s",string);
			}
			else if(strcmp(mod,"sys")==0){
				sscanf(buffer0,"%*s %[^\n]",string);
				sprintf(output,"system:%s",string);
				endwin();
				close(serverfd);
				sleep(2);
				puts(output);
				exit(1);
			}
			else if(strcmp(mod,"add")==0){
	//			mvwprintw(win[0],7,0,"%s","recv add msg");
	//			wrefresh(win[0]);	
				sscanf(buffer0,"%*s %s",sendername);
				memberctrl("add",sendername);
				continue;
			}
			else if(strcmp(mod,"remove")==0){
	//			mvwprintw(win[0],7,0,"%s","recv remove msg");
	//			wrefresh(win[0]);
				sscanf(buffer0,"%*s %s",sendername);
				memberctrl("remove",sendername);
				continue;
			}
			if(win[0]->_curx==0){
				mvwaddstr(win[0],localine,0,output);
		//		mvwprintw(win[3],1,15,"%s","write");
		//		wrefresh(win[3]);
			}
			else{
				mvwaddstr(win[0],localine,0,output);
		//		mvwprintw(win[3],1,15,"%s","write");
		//		wrefresh(win[3]);
			}	
			strcpy(history[curline],output);
			for(i=0;i<(strlen(output)/(win[0]->_maxx+1));i++){
				localine++;
				curline++;
				sprintf(history[curline],"^ scroll up to get complete msg");
			}
			
			if((strlen(output)%(win[0]->_maxx+1))!=0){
				localine++;
				curline++;
				sprintf(history[curline],"^ scroll up to get complete msg");
			}

			while(localine>LINES-(win[1]->_maxy+1)-(win[3]->_maxy+1)){
				wscrl(win[0],1);
				localine--;
				topline++;
			}
			redraw(1);
		}
	}
	return;
}
void memberctrl(char *mod,char *name){
	int i,j;
	int namelen=strlen(name);
	while(curwin==win[2]){
		sleep(1);
	}
	if(strcmp(mod,"add")==0){
		for(i=1;i<CLIENTNUM+1;i++){
			if(client[i].hold==0){
				client[i].hold=1;
				strcpy(client[i].name,name);
				online++;
				break;
			}
		}
	}
	if(strcmp(mod,"remove")==0){
		for(i=1;i<CLIENTNUM+1;i++){
			if(strcmp(client[i].name,name)==0){
//				mvwprintw(win[0],8,0,"%s %s","find man",name);
//				mvwprintw(win[3],0,15,"%s leave",name);
//				wrefresh(win[3]);
				wrefresh(win[0]);
				client[i].name[0]='\0';
				client[i].hold=0;
				online--;
				break;
			}
		}
	}
	j=1;//
	for(i=1;i<CLIENTNUM+1;i++){//i!=0 because 0 is All
		if(client[i].hold==1){
			memberlist[j]=i;
			j++;
		}
	}
	if((j-1)!=online){
		mvwprintw(win[3],1,0,"ERR ONLINE NUM j-1=%d online=%d",j-1,online);
		wrefresh(win[3]);
	}
	for(i=online+1;i<CLIENTNUM+1;i++){//clear names on online box
		mvwprintw(win[2],i,4,"         ");
	}
//	for(i=0;i<=online;i++){
//		mvwprintw(win[2],i,4,"%.9s",client[memberlist[i]].name);
//	}
	for(i=onlinetop;i<=online;i++){
		mvwprintw(win[2],i-onlinetop,4,"%s",omitname(client[memberlist[i]].name));
//		if(i==0){
//		endwin();
//		exit(1);
//		}
	}
//	sleep(3);
//	mvwprintw(win[3],0,15,"%*s",namelen,"      ");
//	wrefresh(win[3]);
//	endwin();
//	exit(1);
	redraw(1);
	return;
}
int climission(void){

	struct sockaddr_in dest0;
	/* input IP */
//	printf("Input IP(local:127.0.0.1):\n");
//	fgets(IP,sizeof(IP),stdin);
//	if(IP[0]=='\n');
	sprintf(IP,"127.0.0.1");

	/* input port */
	printf("Input port(ex:8889):");
	scanf("%d",&PORT);

	/* create socket */
	serverfd = socket(PF_INET, SOCK_STREAM, 0);

	/* initialize value in dest0 */
	bzero(&dest0, sizeof(dest0));
	dest0.sin_family = PF_INET;
	dest0.sin_port = htons(PORT);
	inet_aton(IP, &dest0.sin_addr);

	/* Connecting to server */
	if(connect(serverfd, (struct sockaddr*)&dest0, sizeof(dest0))==-1){
		puts("connect()==-1");
		return -1;
	}
	else{
		puts("please wait");
	}
	bzero(buffer0, 128);
	
	return 0;
}

void initial(void){

	initscr();
	cbreak();
	nonl();
	noecho();
	intrflush(stdscr,FALSE);
	intrflush(win[2],FALSE);
	keypad(stdscr,TRUE);
	refresh();
	return;
}
void special(int attrs,WINDOW *swin,int y,int x,char *text){
	wattron(swin,attrs);
	mvwprintw(swin,y,x,text);
	wattroff(swin,attrs);
}
void redraw(int mod){
	int w1cy,w1cx;
	int i;

	w1cy=curwin->_cury;
	w1cx=curwin->_curx;

	for(i=0;i<=win[1]->_maxx;i++) // insert box border
		mvwaddch(win[1],0,i,'-');

	curwin->_cury=w1cy;
	curwin->_curx=w1cx;

	for(i=0;i<=win[6]->_maxx;i++)
		mvwaddch(win[6],0,i,'-');
	for(i=0;i<=win[4]->_maxx;i++) //opinion box
		mvwaddch(win[4],0,i,'-');
	for(i=1;i<=win[4]->_maxy;i++)
		mvwaddch(win[4],i,0,'|');

	mvwprintw(win[4],1,5,"Write message");
	mvwprintw(win[4],2,5,"Choose member");
	mvwprintw(win[4],3,5,"Store history");
	mvwprintw(win[4],4,5,"Exit chatroom");

	for(i=0;i<=win[3]->_maxx;i++)//command line
		mvwaddch(win[3],2,i,'-');

	if(mod==0){
		mvwprintw(win[3],1,15,"Administrator:root");
		mvwprintw(win[3],1,0,"===CHATROOM===");
		wrefresh(win[3]);
	}

	for(i=0;i<=win[2]->_maxy;i++)//online box
		mvwaddch(win[2],i,0,'|');

	if(onlinetop<(online-(LINES-6))){
		mvwprintw(win[4],0,12,"v");
	}
	else{
		mvwprintw(win[4],0,12,"-");
	}

	for(i=0;i<win[2]->_maxx;i++)//it's not <= maxx because =maxx it will be scrl
		mvwaddch(win[2],win[2]->_maxy,i,'-');

	if(mod==0){
		mvwprintw(win[5],0,0,"               ");
		special(A_BOLD,win[5],0,10-strlen(receivername)," To ");
		special(A_BOLD,win[5],0,14-strlen(receivername),receivername);
	}
	//	mvwprintw(win[5],0,0," To s10410abcd ");
	mvwprintw(win[3],0,4,"Hi!");
	special(A_BOLD,win[3],0,7,myname);

	wrefresh(win[0]);
	wrefresh(win[2]);
	wrefresh(win[3]);
	touchwin(win[4]);
	wrefresh(win[4]);
	touchwin(win[1]);
	wrefresh(win[1]);
	touchwin(win[5]);
	wrefresh(win[5]);
	touchwin(win[6]);
	wrefresh(win[6]);

	if(mod==1){
		wmove(curwin,curwin->_cury,curwin->_curx);
		wrefresh(curwin);
	}
	return;
}
int terminal(WINDOW *twin,char *str,int n)
{
	char *ostr, ec, kc;
	int c, oldx, remain;
	int prex,prey;
	int i;
//	mvwprintw(win[0],6,0,"curx %d cury %d",twin->_curx,twin->_cury);
//	wrefresh(win[0]);
//	wmove(twin,1,4);
//	wrefresh(twin);
//	mvwprintw(win[0],7,0,"curx %d cury %d",twin->_curx,twin->_cury);
//	wrefresh(win[0]);
//	wmove(twin,1,3);

	ostr = str;
	ec = erasechar();
	kc = killchar();

	oldx = twin->_curx;
//	_DIAGASSERT(n == -1 || n > 1);
	remain = n - 1;
int test=0;
	while ((c = wgetch(twin)) != ERR && c != '\n' && c != '\r') {
test++;
//mvwprintw(win[0],test+9,0,"%d",c);

		*str = c;
		touchline(twin, twin->_cury, 1);
		if (c == ec || c == KEY_BACKSPACE || c==263 || c==127 || c==8) {
			*str = '\0';
			if (str != ostr) {
				/* getch() displays the key sequence */
				if(mvwaddch(twin, twin->_cury, twin->_curx - 1,' ')==ERR){
					if(mvwaddch(twin,twin->_cury-1,twin->_maxx,' ')==ERR){
					//DEBUG
					}
				}
				if(wmove(twin, twin->_cury, twin->_curx - 1)==ERR){
					if(wmove(twin,twin->_cury-1,twin->_maxx)==ERR){
					//DEBUG
					}
				}
				str--;
				*str='\0';
				if (n != -1) {
					/* We're counting chars */
					remain++;
				}
			} 
//			else {        /* str == ostr */
//				if (c == KEY_BACKSPACE || c == KEY_LEFT || c==263)
//					/* getch() displays the other keys */
//					mvwaddch(twin, twin->_cury, twin->_curx - 1,' ');
//				wmove(twin, twin->_cury, oldx);
//			}
		} else if (c == kc) {
			*str = '\0';
			if (str != ostr) {
				/* getch() displays the kill character */
//				if(mvwaddch(twin, twin->_cury, twin->_curx - 1, ' ')==ERR)
//					mvwaddch(twin,twin->_cury-1,twin->_maxx,' ');
				/* Clear the characters from screen and str */
				while (str != ostr) {
					if(mvwaddch(twin, twin->_cury, twin->_curx - 1,' ')==ERR){
						if(mvwaddch(twin,twin->_cury-1,twin->_maxx,' ')==ERR){
						}
					}
					if(wmove(twin, twin->_cury, twin->_curx - 1)==ERR){
						if(wmove(twin,twin->_cury-1,twin->_maxx)==ERR){
						}
					}
					str--;
					if (n != -1)
						/* We're counting chars */
						remain++;
				}
//				mvwaddch(twin, twin->_cury, twin->_curx - 1, ' ');
//				wmove(twin, twin->_cury, twin->_curx - 1);
			} else
			/* getch() displays the kill character */
				mvwaddch(twin, twin->_cury, oldx, ' ');
				wmove(twin, twin->_cury, oldx);
			
		} 
		else if (c == '\t' || c==27){
			prey=twin->_cury;//record text position
			prex=twin->_curx;
//			sleep(3);
			curs_set(0);
//			sleep(3);
			mvwprintw(win[6],1,1," ");
//			sleep(3);
			wrefresh(win[6]);
//			sleep(3);
			selectmod();
			curwin=win[1];
//			mvwprintw(win[1],1,1,">");
			mvwprintw(win[6],1,1,">");
			wrefresh(win[6]);
			curs_set(1);
			twin->_cury=prey;
			twin->_curx=prex;
			wmove(twin,twin->_cury,twin->_curx);
			wrefresh(win[1]);

		}
		else if(c==KEY_UP || c==KEY_DOWN || c== KEY_LEFT || c== KEY_UP){
			if(c==KEY_UP){
				if(topline!=0){
					if(strcmp(history[topline],"^ scroll up to get complete msg")==0){
						mvwprintw(win[0],0,0,"                               ");
					}
					wscrl(win[0],-1);
					mvwprintw(win[0],0,0,"%s",history[topline-1]);
					for(i=0;i<=6;i++){
						if((i!=2)&&(i!=3)){
							touchwin(win[i]);
							wrefresh(win[i]);
						}
					}
					topline--;
					localine++;
				}
			}
			else if(c==KEY_DOWN){
				if(topline<(curline-(LINES-(win[1]->_maxy+1)-(win[3]->_maxy+1)))){
					wscrl(win[0],1);
					for(i=0;i<=6;i++){
						if((i!=2)&&(i!=3)){
							touchwin(win[i]);
							wrefresh(win[i]);
						}
					}
					topline++;
					localine--;
				}
			}
		}
		else if(c>=KEY_MIN&&c<= KEY_MAX){}//disable other function key
		else if(c>=32 && c<=126){
			//normal keys
	//		mvwprintw(win[0],7,0,"                ");
	//		mvwprintw(win[0],7,0,"%d    %d",c,test);
	//		wrefresh(win[0]);
			mvwaddch(twin, twin->_cury, twin->_curx,c);//good job	
			wrefresh(win[1]);
			if (remain) {
				str++;
				remain--;
			} else {
				mvwaddch(twin, twin->_cury, twin->_curx - 1, ' ');//
				wmove(twin, twin->_cury, twin->_curx - 1);
			}
		}		
		wrefresh(win[1]);
		touchwin(win[5]);
		wrefresh(win[5]);
		if(wmove(twin,twin->_cury,twin->_curx)==ERR)
			wmove(twin,twin->_cury-1,twin->_maxx);
	}

	if (c == ERR) {
		*str = '\0';
		return (ERR);
	}
	*str = '\0';
//mvwprintw(win[0],1,0,"                                                   ");
//mvwprintw(win[0],1,0,"|%s|",ostr);
wrefresh(win[0]);
	return (OK);
}
void membermod(void){
	int input;
	static int cursor=0;
	curwin=win[2];
	mvwprintw(win[2],cursor-onlinetop,2,">");
	special(A_REVERSE,win[2],cursor-onlinetop,4,omitname(client[memberlist[cursor]].name));
	wrefresh(win[2]);
	while(1){
		input=getch();
		mvwprintw(win[2],cursor-onlinetop,2," ");
		if(input=='\r' || input=='\n'){
			strcpy(receivername,client[memberlist[cursor]].name);
			mvwprintw(win[5],0,0,"          ");
			special(A_BOLD,win[5],0,10-strlen(receivername)," To ");
			special(A_BOLD,win[5],0,14-strlen(receivername),receivername);
			wrefresh(win[5]);
			mvwprintw(win[2],cursor-onlinetop,4,"%s",omitname(client[memberlist[cursor]].name));
			break;
		}
		else{
			mvwprintw(win[2],cursor-onlinetop,4,"%s",omitname(client[memberlist[cursor]].name));
			if(input==KEY_UP){	
				if(cursor>0){
					cursor--;
				}
			}
			else if(input==KEY_DOWN){
				if(cursor<online){
					cursor++;
				}
			}
			if(onlinetop>cursor){
				onlinetop--;
				wscrl(win[2],-1);
				mvwprintw(win[2],cursor-onlinetop,0,"|");
	//	mvwprintw(win[3],0,10,"scrl -1");
	//	wrefresh(win[3]);
				//first line will be space but it
				//will be drawn by below codes
			}
			if(onlinetop<(cursor-(LINES-6))){
				onlinetop++;
				wscrl(win[2],1);
//		mvwprintw(win[3],0,10,"scrl 1");
//		wrefresh(win[3]);
			}
//			wrefresh(win[2]);
			special(A_REVERSE,win[2],cursor-onlinetop,4,omitname(client[memberlist[cursor]].name));
			mvwprintw(win[2],cursor-onlinetop,2,">");
		}
//		if(curosr<online){
//			mvwprintw(
		if(onlinetop<(online-(LINES-6))){
			mvwprintw(win[4],0,12,"v");
		}
		else{
			mvwprintw(win[4],0,12,"-");
		}	
		wrefresh(win[2]);
		touchwin(win[4]);
		wrefresh(win[4]);//wait to check
	}
	wrefresh(win[2]);
	touchwin(win[4]);
	wrefresh(win[4]);//wait to check
	return;
}
void selectmod(void){
	int input;
	static int selecter=1;
	curwin=win[4];
	mvwprintw(win[4],selecter,2,">");
	wrefresh(win[4]);
	while(1){
		input=getch();
		mvwprintw(win[4],selecter,2," ");
		wrefresh(win[4]);
		if(input=='\r' || input=='\n'){
			if(selecter==1){
				break;
			}
			else if(selecter==2){
				membermod();
				curwin=win[4];
			}
			else if(selecter==3){
				storehistory();
			}
			else if(selecter==4){
				escape(0);
			}
		}
		else{
			if(input==KEY_UP){
				if(selecter>1){
					selecter--;
				}
			}
			else if(input==KEY_DOWN){
				if(selecter<4){
					selecter++;
				}
			}
		}
		mvwprintw(win[4],selecter,2,">");
		wrefresh(win[4]);	
	}
	return;
}
void storehistory(void){
	return;
}
char* omitname(char *str){
	if(strlen(str)>8){
		sprintf(omitstr,"%.5s...",str);
	}
	else{
		sprintf(omitstr,"%.8s",str);
	}
//	exit(1);
	return omitstr;
}