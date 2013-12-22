//Maciej Andrearczyk, 333856
#include "stdio.h"
#include "stdlib.h"
#include "signal.h"
#include "err.h"
#include "mesg.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>

/*
 * Ilość rodzajów zasobów.
 */
int K;

/*
 * Ilość sztuk każdego z zasobów.
 */
int N;

/*
 *TODO
 */
int msg_qid;
Msg msg;


/*
 * Metoda zwalniająca zasoby systemowe.
 */
void free_sysres()
{
	printf("\nZWALNIAM ZASOBY!\n");
	if (msgctl(msg_qid, IPC_RMID, 0) == -1)
		syserr("Error in msgctl\n");
	exit(0);
}

int main(int argc, char* argv[])
{
	K = atoi(argv[1]);
	N = atoi(argv[2]);

	////
	if ( (msg_qid = msgget(1234L, IPC_CREAT | IPC_EXCL)) == -1)
		syserr("Error in msgget\n");
	////
	int size_rcv;
	while((size_rcv = msgrcv(msg_qid, &msg, MAX_DATA_SIZE, 1L, 0))) {
		printf("ALE %s\n", msg.data);
		if  ( signal(SIGINT, free_sysres) == SIG_ERR)
			syserr("Error in signal\n");
	}

	exit(0);
}
