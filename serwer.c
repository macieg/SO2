//Maciej Andrearczyk, 333856
//
//Proces serwera.
//Jeden muteks do ochrony zmiennych,
//zmienna warunkowa na każdy typ zasobów.
//Przy otrzymaniu SIG_INT czekam, az wszystkie wątki się zakończą
//po czym usuwam kolejki.
#include "err.h"
#include "mesg.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
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
 * Zmienna pomocnicza do wyłapywania sygnałów.
 */
struct sigaction sig_helper;

/*
 * Metoda wątku.
 */
void *do_thread(void *data)
{
	Msg th_msg;
	int th_k, n_1, n_2;
	int pid_1, pid_2;

	sscanf((char*) data, "%d %d %d %d %d", &th_k, &pid_1, &n_1, &pid_2, &n_2);

	//MUTEX LOCK
	if (pthread_mutex_lock(&mutex) != 0)
		syserr("Error in mutex lock | thread 1\n");
	thread_counter++;

	while (resources[th_k] < n_1 + n_2)
		if (pthread_cond_wait(type_cond + th_k, &mutex) != 0)
			syserr("Error in cond wait type_cond\n");

	resources[th_k] -= n_1;
	resources[th_k] -= n_2;

	printf("Wątek %ld przydziela %d+%d zasobów %d klientom %d %d, pozostało %d zasobów\n",
			(long)pthread_self(), n_1, n_2, th_k, pid_1, pid_2, resources[th_k]);

	if (pthread_mutex_unlock(&mutex) != 0)
	       syserr("Error in mutex unlock | thread 1\n");	
	///MUTEX UNLOCK
	
	th_msg.msg_type = pid_1;
	strcpy(th_msg.data, "");
	sprintf(th_msg.data, "%d", pid_2);
	if ( msgsnd(conf_qid, (char *) &th_msg, strlen(th_msg.data), 0) != 0)
		syserr("Error in msgsnd | conf pid1\n");

	th_msg.msg_type = pid_2;
	strcpy(th_msg.data, "");
	sprintf(th_msg.data, "%d", pid_1);
	if ( msgsnd(conf_qid, (char *) &th_msg, strlen(th_msg.data), 0) != 0) 
		syserr("Error in msgsnd | conf pid2\n");

	if ( msgrcv(fin_qid, &th_msg, MAX_DATA_SIZE, pid_1, 0) == -1)
		syserr("Error in msgrcv | fin_qid pid 1\n");

	if ( msgrcv(fin_qid, &th_msg, MAX_DATA_SIZE, pid_2, 0) == -1)
		syserr("Error in msgrcv | fin_qid pid 2\n");

	//MUTEX LOCK
	if (pthread_mutex_lock(&mutex) != 0)
		syserr("Error in lock | thread 2\n");

	resources[th_k] += n_1;
	resources[th_k] += n_2;
	if (pthread_cond_signal(type_cond + th_k) != 0)
		syserr("Error in cond signal type_cond\n");

	thread_counter--;
	if (pthread_mutex_unlock(&mutex) != 0)
		syserr("Error in unlock | thread 2\n");
	//MUTEX UNLOCK
	
	if (pthread_cond_signal(&fin_cond) != 0)
		syserr("Error in cond signal fin_cond\n");
	free(data);
	return 0;
}

/*
 * Ustawia flagę mówiącą o otrzymaniu SIGINT.
 */
void setFlag()
{
	//printf("Ustawiam isStopped\n");
	isStopped = 1;
}

/*
 * Metoda zwalniająca zasoby systemowe.
 */
void free_sysres()
{
	//printf("\nZWALNIAM ZASOBY!\n");

	if (msgctl(req_qid, IPC_RMID, 0) == -1)
		syserr("Error in msgctl\n");

	if (msgctl(conf_qid, IPC_RMID, 0) == -1)
		syserr("Error in msgctl\n");

	if (msgctl(fin_qid, IPC_RMID, 0) == -1)
		syserr("Error in msgctl\n");

	if (pthread_cond_destroy(&fin_cond) != 0)
		syserr("Error in destroy fin_cond\n");

	int i;
	for (i = 1; i <= K; i++)
		if (pthread_cond_destroy(type_cond + i) != 0)
			syserr("Error in destroy type_cond %d", i);

	if (pthread_mutex_destroy(&mutex) != 0)
		syserr("Error in destroy mutex \n");

	//printf("\nZWOLNILEM\n");	
}

/*
 * Tworzy obiekty do obsługi wątków.
 */
void create_thread_tools()
{
	atexit(free_sysres);
	if ((pthread_mutex_init(&mutex, 0)) != 0)
		syserr("mutex init\n");

	if ((pthread_attr_init(&attr)) != 0)
		syserr("attr init\n");

	if ((pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0)
		syserr("setdetach\n");

	if ((pthread_cond_init(&fin_cond, 0)) != 0)
		syserr("init fin_cond\n");

	int i;
	for (i = 1; i <= K; i++)
		if ((pthread_cond_init(type_cond + i, 0)) != 0)
			syserr("init type_cond %d\n", i);
}

/*
 * Metoda tworząca kolejki komunkatów.
 */
void create_queues()
{
	if ( (req_qid = msgget(REQ_KEY, 0666 | IPC_CREAT | IPC_EXCL)) == -1)
		syserr("Error in msgget | request\n");

	if ( (conf_qid = msgget(CONF_KEY, 0666 | IPC_CREAT | IPC_EXCL)) == -1)
		syserr("Error in msgget | confirmation\n");

	if ( (fin_qid = msgget(FIN_KEY, 0666 | IPC_CREAT | IPC_EXCL)) == -1)
		syserr("Error in msgget | finish\n");
}

int main(int argc, char* argv[])
{
	if (argc != 3)
		syserr("Improper number of arguments\n");

	K = atoi(argv[1]);
	N = atoi(argv[2]);
	isStopped = 0;

	int i;
	for (i = 1; i <= K; i++)
		resources[i] = N;

	create_thread_tools();

	sig_helper.sa_handler = setFlag;
	sigemptyset(&sig_helper.sa_mask);
	sig_helper.sa_flags = SA_RESTART;

	if (sigaction(SIGINT, &sig_helper, 0) == -1)
		syserr("Error in signal\n");	

	create_queues();

	int size_rcv;
	while((size_rcv = msgrcv(req_qid, &msg, MAX_DATA_SIZE, 0, 0)) && isStopped == 0)
	{

		int k_req, n_req;
		sscanf(msg.data, "%d %d", &k_req, &n_req);

		if (type_pid[k_req] == 0)
		{
			type_pid[k_req] = msg.msg_type;
			type_N[k_req] = n_req;
		}
		else
		{
			char * msg_buf = malloc(100 * sizeof(int));
			pthread_t th;
			sprintf(msg_buf, "%d %li %d %d %d", k_req, msg.msg_type, n_req,
					type_pid[k_req], type_N[k_req]);
			type_pid[k_req] = 0;
			type_N[k_req] = 0;
			if (pthread_create(&th, &attr, do_thread, msg_buf) != 0)
				syserr("Error in pthread_create\n");
		}
	}

	while (thread_counter > 0)
		if (pthread_cond_wait(&fin_cond, &mutex) != 0)
			syserr("Error in cond wait\n");

	exit(0);
}
