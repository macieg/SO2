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
#include <unistd.h>
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
 * Zmienna warunkowa do kończenia wątków.
 */
pthread_cond_t fin_cond;

/*
 * Zmienne warunkowe dla każdego typu.
 */
pthread_cond_t type_cond[MAX_TYPES_NUMBER];

/*
 * Dla każdego typu trzyma informację o czekającym żądaniu.
 */
int type_pid[MAX_TYPES_NUMBER];
int type_N[MAX_TYPES_NUMBER];

/*
 * Licznik dodatkowych wątków.
 */
int thread_counter;

/*
 * Flaga mówiąca, czy nastąpiło przerwanie przez SIGINT.
 */
int isStopped;

/*
 * Metoda wątku.
 */
void *do_thread(void *data)
{
	printf("WĄTECZEK!\n");
	int th_k, n_1, n_2;
	long pid_1, pid_2;

	sscanf((char*) data, "%d %li %d %li %d", &th_k, &pid_1, &n_1, &pid_2, &n_2);
	printf("AA %d %li %d %li %d\n", th_k, pid_1, n_1, pid_2, n_2);

	pthread_mutex_lock(&mutex);
	thread_counter++;

	while (resources[th_k] < n_1 + n_2)
		pthread_cond_wait(type_cond + k, &mutex);
	
	thread_counter--;
	pthread_mutex_unlock(&mutex);
	pthread_cond_signal(&fin_cond);
	return 0;
}

/*
 * Ustawia flagę mówiącą o otrzymaniu SIGINT.
 */
void setFlag()
{
	printf("Ustawiam isStopped\n");
	isStopped = 1;
}

/*
 * Metoda zwalniająca zasoby systemowe.
 */
void free_sysres()
{
	printf("\nZWALNIAM ZASOBY!\n");
	if (pthread_mutex_lock(&mutex) != 0)
		syserr("Error in mutex lock sysres\n");

	if (msgctl(req_qid, IPC_RMID, 0) == -1)
		syserr("Error in msgctl\n");

	if (msgctl(conf_qid, IPC_RMID, 0) == -1)
		syserr("Error in msgctl\n");

	if (msgctl(fin_qid, IPC_RMID, 0) == -1)
		syserr("Error in msgctl\n");

	if (pthread_cond_destroy(&fin_cond) != 0)
		syserr("Error in destroy fin_cond\n");

	for (int i = 1; i <= K; i++)
		if (pthread_cond_destroy(type_cond + i) != 0)
			syserr("Error in destroy type_cond %d", i);

	//if (pthread_mutex_destroy(&mutex) != 0)
	//	syserr("Error in mutex destroy\n");

	exit(0);
}

/*
 * Tworzy obiekty do obsługi wątków.
 */
void create_thread_tools()
{
	if ((pthread_mutex_init(&mutex, 0)) != 0)
		syserr("mutex init\n");

	if ((pthread_attr_init(&attr)) != 0)
		syserr("attr init\n");

	if ((pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0)
		syserr("setdetach\n");

	if ((pthread_cond_init(&fin_cond, 0)) != 0)
		syserr("init fin_cond\n");

	for (int i = 1; i <= K; i++)
		if ((pthread_cond_init(type_cond + i, 0)) != 0)
			syserr("init type_cond %d\n", i);
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
	isStopped = 0;

	for (int i = 1; i <= K; i++)
		resources[i] = N;

	create_thread_tools();

	if  (signal(SIGINT, free_sysres) == SIG_ERR)
		syserr("Error in signal\n");	

	create_queues();

	int size_rcv;
	char msg_buf[100];
	while((size_rcv = msgrcv(req_qid, &msg, MAX_DATA_SIZE, 0, 0)) && isStopped == 0)
	{
		printf("DOSTALEM REQUEST! :) %s\n", msg.data);
		if (signal(SIGINT, setFlag) == SIG_ERR)
			syserr("Error in signal\n");

		int th_id, k_req, n_req;
		sscanf(msg.data, "%d %d", &k_req, &n_req);

		//pthread_mutex_lock(&mutex);
		if (type_pid[k_req] == 0)
		{
			type_pid[k_req] = msg.msg_type;
			type_N[k_req] = n_req;
		}
		else
		{
			pthread_t th;
			sprintf(msg_buf, "%d %li %d %d %d", k_req, msg.msg_type, n_req,
					type_pid[k_req], type_N[k_req]);
			type_pid[k_req] = 0;
			type_N[k_req] = 0;
			th_id = pthread_create(&th, &attr, do_thread, msg_buf);
		}
		//pthread_mutex_unlock(&mutex);
	}

	while (thread_counter > 0)
		pthread_cond_wait(&fin_cond, &mutex);

	free_sysres();
	exit(0);
}
