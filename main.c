#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include "pipe_shared_struct.h"

#define DEFAULT_LIST_SIZE 30

typedef struct t_process{
	int wpipe;
	int rpipe;
	int pid;

	void (*write) (struct t_process*, char*);
	char* (*read) (struct t_process*);
	void (*remove) (struct t_process*);

} t_process;
void writeProcess(struct t_process*, char*);
char *readProcess(struct t_process*);
void removeProcess(struct t_process*);

t_process *init_process();

typedef struct t_channel{
	t_process *process;
	char *name;

	void (*subscribe)(struct t_channel);
	void (*unsubscribe)(struct t_channel);
	void (*publish)(struct t_channel);
}t_channel;

t_channel *init_channel(char*);

typedef struct channelList{
	int size;
	int max;
	t_channel **list;
	

	void (*add)(struct channelList*, char *);
	void (*remove) (struct channelList*, char *);
	t_channel *(*getByName)(struct channelList*, char *);
	int (*exists)(struct channelList*, char *);
	int (*getIndex)(struct channelList*, char *);
	void (*resize)(struct channelList*);

}channelList;
/**How deletion will be handled
	When deleting a channel, we will search it on list,  we will replace it with the last index to then 
	decrease size variable and set last element to null.

	Example: size 10.
	Element to remove in index 3.
	free(index 3)
	index[2] = index[size -1];
	index[size -1] = null
 **/
channelList *init_channelList();

void addChannel(channelList *, char*);
void removeChannel(channelList *, char*);
t_channel *getChannel(channelList *, char *);
int existsChannel(channelList *, char *);
int getChannelIndex(channelList*, char *);
void resizeChannelList(channelList *);



void replaceStd(int, int);

//tests
void test(channelList *);
void sendTestStruct(int);

int main(int argc, char *argv[])
{
	channelList *list = init_channelList();
	t_channel *channel  = init_channel("firstChannel");
	t_process *p = channel->process;

//	p->write(p, "hola");

//	char *msg;
//	msg = p->read(p);
//	printf("el mensaje devuelto pro el proceso es: %s, on pointer %p \n", msg, &msg);

	p->write(p, "struct");
	testStruct t;
	t.x = 1;
	t.y = 2;
	memcpy(t.name, "Julen", 5);

	write(p->wpipe, &t, sizeof(testStruct));
}

t_channel *init_channel(char *name)
{
	t_channel *ch = (t_channel *) malloc(sizeof(t_channel));
	ch->name = name;
	ch->process = init_process();

	return ch;

}

channelList *init_channelList()
{
	channelList *chl = (channelList *) malloc(sizeof(channelList));
	chl->getIndex = getChannelIndex;
	chl->max = DEFAULT_LIST_SIZE;
	chl->exists = existsChannel;
	chl->remove = removeChannel;
	chl->add = addChannel;
	chl->getByName = getChannel;
	chl->list = (t_channel **) malloc(sizeof(t_channel *) * DEFAULT_LIST_SIZE);

}

void addChannel(channelList *list, char *topic)
{

	if(!(list->exists(list, topic)))
	{
		int size = list->size;
		if(list->size >= (list->size) * 0.7) list->resize(list);
		list->list[size] = init_channel(topic);
		list->size++;
	}
}
void removeChannel(channelList *list, char *name)
{
	int index;
	if((index = list->getIndex(list, name)) != -1)
	{
		t_process * p = list->list[index]->process;
		p->remove(p);

		free(list->list[index]);
		list->list[index] = list->list[list->size -1];
		list->size--;
	}
}

int  existsChannel(channelList *list, char *name)
{

	int exists = list->getIndex(list, name) >= 0 ? 1 : 0;
	return exists;

}

int getChannelIndex(channelList *list, char *name)
{
	int n = list->size;

	while(--n >= 0)
	{
		if(!strcmp(list->list[n]->name, name)) return n;

	}

	return -1; // not found

}

void resizeChannelList(channelList *list)
{

	int newSize = list->size *2;
	t_channel **ch = (t_channel **) malloc(sizeof(t_channel *) * newSize);

	memcpy(list->list, ch, sizeof(list->list));
	free(list->list);
	list->list = ch;
}

t_channel *getChannel(channelList *list, char *name)
{

	int i = list->getIndex(list, name);
	if(i >= 0) return list->list[i];
	else return (void *) 0;

}

t_process* init_process()
{
	t_process* p = (t_process *) malloc(sizeof(t_process));
	int pid;
	int readpipe[2];
	int writepipe[2];
	pipe(readpipe);
	pipe(writepipe);


	if((pid = fork()) > 0)
	{
		p->rpipe = readpipe[0];
		close(readpipe[1]);

		p->wpipe = writepipe[1];
		close(writepipe[0]);

		p->pid = pid;
		p->write = writeProcess;
		p->read = readProcess;
		p->remove =  removeProcess;

	}
	else if(pid == 0)
	{
		replaceStd(1, readpipe[1]);
		replaceStd(0, writepipe[0]);
		close(readpipe[0]);
		close(writepipe[1]);
		execlp("./subprocess", "subprocess", (void *) 0);
	}


	return p;
}

void writeProcess(t_process *self, char *word)
{
	int fd = self->wpipe;
	int size = strlen(word);

	write(fd, &size, sizeof(int));
	write(fd, word, size);
}


char*  readProcess(t_process *self)
{
	int fd = self->rpipe;
	int byteSize;
	char *str;

	if(read(fd, &byteSize, sizeof(int)) > 0)
	{
		printf("bytes to read: %d", byteSize);
		str = (char *) malloc(sizeof(char) * byteSize);
		int r;
		if((r = read(fd, str, byteSize)) > 0)
		{
			printf("%d bytes to read, word: %s, bytes read from word %d \n", byteSize, str, r);
			return str;
		}
	}

	return "read did not work\n";

}

void removeProcess(t_process *self)
{
	int state;

	kill(self->pid, SIGTERM);
	waitpid(self->pid, &state, 0);

	if(WIFSIGNALED(state))
	{
		printf("process %d has been closed \n", self->pid);
		close(self->wpipe);
		close(self->rpipe);

		free(self);
		self = (void *) 0;
	}

}

void replaceStd(int currentFd, int newFd)
{
	close(currentFd);
	dup(newFd);
	close(newFd);
}

void test(channelList *list)
{
	char *names[]  = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "ñ"};
	int size = list->size;
	for(int i= 0; names[i] != (void *) 0; i++)
	{
		list->add(list, names[i]);
		char *name = list->list[i]->name;
		printf("%s \t", name);
		if(list->size > size)
		{
			 printf("\n channel vector size has changed from %d to %d, on %d", size, list->max, list->size);
		}
	}


}

