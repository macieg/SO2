#define MAX_DATA_SIZE 1000

typedef struct {
	long msg_type;
	char data[MAX_DATA_SIZE];
} Msg;

#define CLI_TO_SER 100
#define SER_TO_CLI 101
