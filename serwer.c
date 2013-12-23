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
#include <pthread.h>
#define MAX_TYPES_NUMBER 100
#define MAX_RESOURCES_NUMBER 9999

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
 * Tablica trzymająca informacje o dostępnych zasobach konkretnych typów.
 */
int resources[MAX_TYPES_NUMBER];

/*
 * Wiadomość.
 */
Msg msg;

/*
 * Mutex.
 */
pthread_mutex_t mutex;

/*
 * Atrybuty wątku.
 */
pthread_attr_t attr;

/*
 * Metoda wątkowa.
 */
void *do_thread()
{
}

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

/*
 * Metoda tworząca kolejki komunkatów.
 */
void create_queues()
{
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
}

int main(int argc, char* argv[])
{
	if (argc != 3)
		syserr("Improper number of arguments\n");

	K = atoi(argv[1]);
	N = atoi(argv[2]);

	for (int i = 1; i <= K; i++)
		resources[i] = N;

	if  ( signal(SIGINT, free_sysres) == SIG_ERR)
		syserr("Error in signal\n");

	if ((pthread_mutex_init(&mutex, 0)) != 0)
		syserr("mutex init\n");

	if ((pthread_attr_init(&attr)) != 0)
		syserr("attr init\n");

	if ((pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0)
		syserr("setdetach\n");

	create_queues();

	int size_rcv;
	while((size_rcv = msgrcv(req_qid, &msg, MAX_DATA_SIZE, 0, 0)))
	{
		printf("DOSTALEM REQUEST! :) %s\n", msg.data);
		if  ( signal(SIGINT, free_sysres) == SIG_ERR)
			syserr("Error in signal\n");

	}

	free_sysres();
	exit(0);
}
