//Maciej Andrearczyk, 333856
#include "stdio.h"
#include "stdlib.h"
#include "err.h"
#include "mesg.h"
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>

/*
 * Rodzaj zasobu.
 */
int k;

/*
 * Żądana liczba sztuk zasobu.
 */
int n;

/*
 * Czas na jaki zatrzyma się klient przetrzymując zasób.
 */
int s;

/*
 * Id kolejki to wysylania komunikatow.
 */
int snd_qid;

/*
 * Id kolejki to odbierania komunikatow.
 */
int rcv_qid;

/*
 * Wiadomość z żądaniem.
 */ 
Msg req;

int main(int argc, char* argv[]) 
{
	k = atoi(argv[1]);
	n = atoi(argv[2]);
	s = atoi(argv[3]);

	req.msg_type = 1L;

	printf("MOJ PID %d\n", getpid());
	sprintf(req.data, "%d %d %d", n, k, getpid());

	if ( (snd_qid = msgget(CLI_TO_SER, 0)) == -1)
		syserr("Error in msgget\n");
	
	if ( (rcv_qid = msgget(SER_TO_CLI, 0)) == -1)
		syserr("Error in msgget\n");

	if ( msgsnd(snd_qid, (char *) &req, strlen(req.data), 0) != 0 )
		syserr("Error in msgsnd");

	printf("KONIEC\n");
	exit(0);
}
