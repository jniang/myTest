#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<string.h>
#include<errno.h>

#define BUFFER_SIZE 12 
void print(unsigned char *buffer, int size){
	for(int i = 0; i < size; i++){
		printf("%d", buffer[i]);
	}
	printf("\n");
}
//Don't need anymore!!!!
//static char recieve[BUFFER_LENGTH];

int main( int argc, char **argv ){
	
	int reinitialize, fileD;
	//testing earlier to see if it working
	//char data[BUFFER_LENGTH] = "HELLO kitty, hola, Bonjour, Yo!!";
	unsigned char bufferOne[BUFFER_SIZE];
	unsigned char bufferTwo[BUFFER_SIZE];
	unsigned char key[9] = "MadyNiang";
	
	printf("Device testing for module!!!!!\n");
	fileD = open("/dev/myRand", O_RDWR);
	if(fileD < 0){
		perror( "Failed device Test!!!!\n");
		return errno;
	}

	printf("Writing Message!!!!![%s].\n", key);
	//using rc4 to write to buffer
	reinitialize = write(fileD, key, strlen(key));
	if(reinitialize < 0){
		perror("Failed to Write Message!!!!");
		return errno;
	}
	
	/*Test I: Reintialize test with using random rc4 generator
	 * Result: have same data output results from the buffer
	 */
	
	//read to bufferOne
	reinitialize = read(fileD, bufferOne, BUFFER_SIZE);

	//read to bufferTwo
	reinitialize = read(fileD, bufferTwo, BUFFER_SIZE);

	//Print data content of: bufferOne
	print(bufferOne, BUFFER_SIZE);
	//Print data content of: bufferTwo
	print(bufferTwo, BUFFER_SIZE);
	
	/* Test II: using the rc4 generator create a random test
	 * Results: different data outcomes for this test
	 */
	//write to Buffer
	reinitialize = write(fileD, key, strlen(key));
	//read to bufferOne
	reinitialize = read(fileD, bufferOne, BUFFER_SIZE);
	//write to Buffer
	reinitialize = write(fileD, key, strlen(key));
	//read to bufferTwo
	reinitialize = read(fileD, bufferTwo, BUFFER_SIZE);

	//Print data conent of:bufferOne
	print(bufferOne, BUFFER_SIZE);
	//Print data content of: bufferTwo
	print(bufferTwo, BUFFER_SIZE);
	
}

