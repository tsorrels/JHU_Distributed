/* ncp.c
 * runs as a client to send a reliable transfer of a file to a host running
 * rcv.c
 *
 * Usage: ./rcv.c <loss_rate_percent> <source_file_name> <dest_file_name> @
 *                   <compe_name>
 *
 * 
 */


#include "net_include.h"


int main (int argc, char** argv)
{
  int lossRate = atoi(argv[1]);



  return 0;
}
