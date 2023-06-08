#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "pipe_shared_struct.h"

#define MAX_SUB 1000


char *_read();
void _write();
void _log(char*);
void _readTestStruct();

int main(int argc, char *argv[])
{
	while(1){
		char *mensaje = _read();

		if(!strcmp(mensaje, "hola")) _write("qué tal");
		else if(!strcmp(mensaje, "adios")) _write("hasta mañana");
		else if(!strcmp(mensaje, "stru")) _readTestStruct();
		else _write("mensaje no reconocido");
		free(mensaje);
	}
	exit(0);

}



char *_read()
{
	int bytes;
	char *message;
	if((bytes = read(0, &bytes, sizeof(int))) > 0)
	{
		message = (char *) malloc(sizeof(char) * bytes);
		read(0, message, bytes);
		_log("Child: mensaje recibido -->");
		_log(message);
		return message;
	}

	return "";

}


void _write(char *message)
{
	int size = strlen(message);
	char error[100];
	sprintf(error, "\n child: se entra en write, intentar escribir %d bytes \n", size);
	write(2, error, strlen(error));
	write(1, &size, sizeof(int));
	write(1, message, size);

}


void _log(char *msg)
{
	char log[300];
	sprintf(log, "\n %s \n", msg );
	write(2, log, strlen(log) );

}

void _readTestStruct()
{
	_log("Entra en readTestStruct");
	if(testStructIndex <= TEST_STRUCT_SIZE)
	{
		testStruct t;

		if(read(0,&t, sizeof(testStruct)) > 0)
		{
			char info[200];
			sprintf(info, "x: %d y: %d, name: %s", t.x, t.y, t.name );
			_log("test struct added");
			_log(info);
		};


	}


}
