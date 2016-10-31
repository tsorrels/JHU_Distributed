#include "sp.h"


#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define MAX_MESSLEN     102400
#define MAX_VSSETS      10
#define MAX_MEMBERS     100



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
static	void	Read_message();



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

    E_init();

    E_attach_fd( Mbox, READ_FD, Read_message, 0, NULL, HIGH_PRIORITY );

    E_handle_events();

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


static	void	Read_message()
{

    static	char		 mess[MAX_MESSLEN];
    char		 sender[MAX_GROUP_NAME];
    char		 target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
    membership_info  memb_info;
    vs_set_info      vssets[MAX_VSSETS];
    unsigned int     my_vsset_index;
    int              num_vs_sets;
    char             members[MAX_MEMBERS][MAX_GROUP_NAME];
    int		 num_groups;
    int		 service_type;
    int16		 mess_type;
    int		 endian_mismatch;
    int		 i,j;
    int		 ret;

    service_type = 0;

    ret = SP_receive( Mbox, &service_type, sender, 100, &num_groups, 
		      target_groups, &mess_type, &endian_mismatch, 
		      sizeof(mess), mess );
    printf("\n============================\n");
    if( ret < 0 ) 
    {
	if ( (ret == GROUPS_TOO_SHORT) || (ret == BUFFER_TOO_SHORT) ) {
	    service_type = DROP_RECV;
	    printf("\n========Buffers or Groups too Short=======\n");
	    ret = SP_receive( Mbox, &service_type, sender, MAX_MEMBERS, 
			      &num_groups, target_groups, &mess_type, 
			      &endian_mismatch, sizeof(mess), mess );
	}
    }
    if (ret < 0 )
    {
	if( ! To_exit )
	{
	    SP_error( ret );
	    printf("\n============================\n");
	    printf("\nBye.\n");
	}
	exit( 0 );
    }

    if( Is_regular_mess( service_type ) )
    {
	mess[ret] = 0;
	if ( Is_unreliable_mess( service_type ) )printf("received UNRELIABLE ");
	else if( Is_reliable_mess(service_type ) ) printf("received RELIABLE ");
	else if( Is_fifo_mess( service_type ) ) printf("received FIFO ");
	else if( Is_causal_mess( service_type ) ) printf("received CAUSAL ");
	else if( Is_agreed_mess( service_type ) ) printf("received AGREED ");
	else if( Is_safe_mess(   service_type ) ) printf("received SAFE ");
	printf("message from %s, of type %d, (endian %d) to %d groups \n(%d bytes): %s\n",  sender, mess_type, endian_mismatch, num_groups, ret, mess );
    }else if( Is_membership_mess( service_type ) )
    {
	ret = SP_get_memb_info( mess, service_type, &memb_info );
	if (ret < 0) {
	    printf("BUG: membership message does not have valid body\n");
	    SP_error( ret );
	    exit( 1 );
	}
	if     ( Is_reg_memb_mess( service_type ) )
	{
	    printf("Received REGULAR membership for group %s with %d members, where I am member %d:\n", sender, num_groups, mess_type );
	    for( i=0; i < num_groups; i++ )
		printf("\t%s\n", &target_groups[i][0] );
	    printf("grp id is %d %d %d\n",memb_info.gid.id[0], memb_info.gid.id[1], memb_info.gid.id[2] );

	    if( Is_caused_join_mess( service_type ) )
	    {
		printf("Due to the JOIN of %s\n", memb_info.changed_member );
	    }else if( Is_caused_leave_mess( service_type ) ){
		printf("Due to the LEAVE of %s\n", memb_info.changed_member );
	    }else if( Is_caused_disconnect_mess( service_type ) ){
		printf("Due to the DISCONNECT of %s\n", memb_info.changed_member );
	    }else if( Is_caused_network_mess( service_type ) ){
		printf("Due to NETWORK change with %u VS sets\n", memb_info.num_vs_sets);
		num_vs_sets = SP_get_vs_sets_info( mess, &vssets[0], MAX_VSSETS, &my_vsset_index );
		if (num_vs_sets < 0) {
		    printf("BUG: membership message has more then %d vs sets. Recompile with larger MAX_VSSETS\n", MAX_VSSETS);
		    SP_error( num_vs_sets );
		    exit( 1 );
		}
		for( i = 0; i < num_vs_sets; i++ )
		{
		    printf("%s VS set %d has %u members:\n",
			   (i  == my_vsset_index) ?
			   ("LOCAL") : ("OTHER"), i, vssets[i].num_members );
		    ret = SP_get_vs_set_members(mess, &vssets[i], members, MAX_MEMBERS);
		    if (ret < 0) {
			printf("VS Set has more then %d members. Recompile with larger MAX_MEMBERS\n", MAX_MEMBERS);
			SP_error( ret );
			exit( 1 );
		    }
		    for( j = 0; j < vssets[i].num_members; j++ )
			printf("\t%s\n", members[j] );
		}
	    }
	}else if( Is_transition_mess(   service_type ) ) {
	    printf("received TRANSITIONAL membership for group %s\n", sender );
	}else if( Is_caused_leave_mess( service_type ) ){
	    printf("received membership message that left group %s\n", sender );
	}else printf("received incorrecty membership message of type 0x%x\n", service_type );
    } else if ( Is_reject_mess( service_type ) )
    {
	printf("REJECTED message from %s, of servicetype 0x%x messtype %d, (endian %d) to %d groups \n(%d bytes): %s\n",
	       sender, service_type, mess_type, endian_mismatch, num_groups, ret, mess );
    }else printf("received message of unknown message type 0x%x with ret %d\n", service_type, ret);


    printf("\n");
    printf("User> ");
    fflush(stdout);

}
