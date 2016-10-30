#include "sp.h"


#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int debug = 0;





/* from class_user.c example code */
static	char	User[80];
static  char    Spread_name[80];

static  char    Private_group[MAX_GROUP_NAME];
static  mailbox Mbox;
static	int	    Num_sent;
static	unsigned int	Previous_len;

static  int     To_exit = 0;



static	void	Usage( int argc, char *argv[] );
static  void    Print_help();
static  void	Bye();





/* Custom print method for debugging */
void printDebug(char* message){
    if (debug){
	printf("%s\n", message);
    }	
}



int main(int argc, char ** argv){
    int	ret;
    int mver; 
    int miver; 
    int pver;
    sp_time test_timeout;
    
    test_timeout.sec = 5;
    test_timeout.usec = 0;






    /* initialize */
    if (argc == 5){
	if (atoi(argv[4]) == 1){
	    debug = 1;
	    printDebug("dubug set");
	}
    }

    /* I think this code is covered in the Usage() funciton call below
    if (argc < 4){
	printf("Usage: mcast <num_of_messages> <process_index> "); 
	printf("<num_of_processes> [debug]\n");
	exit(0);
	}*/
    

    Usage( argc, argv );


    /* connect to spread */
    ret = SP_connect_timeout( Spread_name, User, 0, 1, &Mbox, Private_group, 
			      test_timeout );
    if( ret != ACCEPT_SESSION ) 
    {
	SP_error( ret );
	Bye();
    }
    printf("User: connected to %s with private group %s\n", Spread_name, 
	   Private_group );


    return 0;
}





static	void	Usage(int argc, char *argv[])
{
    sprintf( User, "user" );
    sprintf( Spread_name, "4803");
    while( --argc > 0 )
    {
	argv++;

	if( !strncmp( *argv, "-u", 2 ) )
	{
	    if (argc < 2) Print_help();
	    strcpy( User, argv[1] );
	    argc--; argv++;
	}else if( !strncmp( *argv, "-r", 2 ) )
	{
	    strcpy( User, "" );
	}else if( !strncmp( *argv, "-s", 2 ) ){
	    if (argc < 2) Print_help();
	    strcpy( Spread_name, argv[1] ); 
	    argc--; argv++;
	}else{
	    Print_help();
	}
    }
}



static  void    Print_help()
{
    printf( "Usage: spuser\n%s\n%s\n%s\n",
            "\t[-u <user name>]  : unique (in this machine) user name",
            "\t[-s <address>]    : either port or port@machine",
            "\t[-r ]    : use random user name");
    exit( 0 );
}
static  void	Bye()
{
    To_exit = 1;

    printf("\nBye.\n");

    SP_disconnect( Mbox );

    exit( 0 );
}
