//Maciej Andrearczyk, 333856
#define MAX_DATA_SIZE 1000

typedef struct {
	long msg_type;
	char data[MAX_DATA_SIZE];
} Msg;

#define REQ_KEY 100L
#define CONF_KEY 200L
#define FIN_KEY 300L
