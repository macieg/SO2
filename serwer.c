//Maciej Andrearczyk, 333856
//
//Proces serwera.
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
 * Id kolejki o odbierania requestow.
 */
int req_qid;

/*
 * Id kolejki do wysylania potwierdzeń.
 */
int conf_qid;

/*
 * Id kolejki do odbierania informacji o zakoczeniu przetwarzania.
 */
int fin_qid;

/*
 * Wiadomość.
 */
Msg msg;

/*
 * Metoda zwalniająca zasoby systemowe.
 */
void free_sysres()
{
	printf("\nZWALNIAM ZASOBY!\n");
	if (msgctl(req_qid, IPC_RMID, 0) == -1)
		syserr("Error in msgctl\n");
	
	if (msgctl(conf_qid, IPC_RMID, 0) == -1)
		syserr("Error in msgctl\n");
	
	if (msgctl(fin_qid, IPC_RMID, 0) == -1)
		syserr("Error in msgctl\n");

	exit(0);
}

int main(int argc, char* argv[])
{
	if (argc != 3)
		syserr("Improper number of arguments\n");

	K = atoi(argv[1]);
	N = atoi(argv[2]);

	if  ( signal(SIGINT, free_sysres) == SIG_ERR)
			syserr("Error in signal\n");

	if ( (req_qid = msgget(REQ_KEY, 0666 | IPC_CREAT | IPC_EXCL)) == -1)
	{
		free_sysres();
		syserr("Error in msgget | request\n");
	}

	if ( (conf_qid = msgget(CONF_KEY, 0666 | IPC_CREAT | IPC_EXCL)) == -1)
	{
		free_sysres();
		syserr("Error in msgget | confirmation\n");
	}

	if ( (fin_qid = msgget(FIN_KEY, 0666 | IPC_CREAT | IPC_EXCL)) == -1)
	{
		free_sysres();
		syserr("Error in msgget | finish\n");
	}

	int size_rcv;
	while((size_rcv = msgrcv(req_qid, &msg, MAX_DATA_SIZE, 0, 0))) {
		printf("ALE %s\n", msg.data);
		if  ( signal(SIGINT, free_sysres) == SIG_ERR)
			syserr("Error in signal\n");
	}

	free_sysres();
	exit(0);
}
