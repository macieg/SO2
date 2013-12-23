//Maciej Andrearczyk, 333856
//Proces klienta.
//
//Wysyła żądanie.
//Czeka na odpowiedź.
//Wiesza się.
//Wysyła informację o zakończeniu.
//Kończy się.
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
 * Id kolejki do wysylania requestow.
 */
int req_qid;

/*
 * Id kolejki do odbierania potwierdzeń. 
 */
int conf_qid;

/*
 * Id kolejki do wysylania informacji o zakonczeniu przetwarzania.
 */ 
int fin_qid;

/*
 * Wiadomości.
 */ 
Msg snd, rcv;

int main(int argc, char* argv[]) 
{
	if (argc != 4)
		syserr("Improper number of arguments\n");

	k = atoi(argv[1]);
	n = atoi(argv[2]);
	s = atoi(argv[3]);

	long mypid = getpid();
	snd.msg_type = mypid;
	sprintf(snd.data, "%d %d %li", k, n, mypid);

	if ( (req_qid = msgget(REQ_KEY, 0)) == -1)
		syserr("Error in msgget | reqkey\n");

	if ( (conf_qid = msgget(CONF_KEY, 0)) == -1)
		syserr("Error in msgget | confkey\n");

	if ( (fin_qid = msgget(FIN_KEY, 0)) == -1)
		syserr("Error in msgget | finkey\n");

	if ( msgsnd(req_qid, (char *) &snd, strlen(snd.data), 0) != 0 )
		syserr("Error in msgsnd | request\n");

	if ( msgrcv(conf_qid, &rcv, MAX_DATA_SIZE, mypid, 0) == -1)
		syserr("Error in msgrcv\n");
	
	printf("%d %d %li %s\n", k, n, mypid, rcv.data);
	sleep(s);

	if ( msgsnd(fin_qid, (char *) &snd, strlen(snd.data), 0) != 0)
		syserr("Error in msgsnd | finish\n");

	printf("KONIEC %li\n", mypid);
	exit(0);
}
