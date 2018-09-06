#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main (int argc, char**argv){
	
	FILE *input, *index, *writed;
	char word[30];
	char namePrefix[30];
	char actual[30];
	unsigned long limit;
	int numFiles;
	int numWords;
	int i,j;
	int seed = atoi(argv[5]);
	
	srand(seed);
	
	if(argc < 6)
		printf("usage: fileGen.o <wordInputFile.txt> <prefix> <numFiles> <numWords> <seed>\n");
		//nome eseguibile, dizionario, prefisso nome file, numero di file da generare, numero massimo di parole per ognuno, seed.
	else{
		
		/*
		if(argc == 5){numWords = atoi(argv[4]); numFiles = atoi(argv[3]);}
		else
			if(argc == 4) {numWords = rand()%20; numFiles = atoi(argv[3]);}
				else
				{numWords = rand()%2000; numFiles = rand()%20;}
			
		*/
		
		//numWords = rand() % atoi(argv[4]);
		numFiles = atoi(argv[3]);
		
		input = fopen(argv[1], "r");
		sprintf(namePrefix,"%s",argv[2]);
		index = fopen("index.txt", "w");
	
		fseek(input, 0, SEEK_END);
		limit = ftell(input);
		fseek(input, 0, SEEK_SET);
		//fprintf(index,"%d\n",numFiles);
		
		for(i=0; i<numFiles; i++){
			sprintf(actual,"%s-%d.txt",namePrefix,i);
			writed = fopen(actual, "w");
			fprintf(index,"%s\n",actual);
			numWords = rand() % atoi(argv[4]);
			for(j=0; j<numWords; j++){
				int off = rand() % limit;
				fseek(input, off, SEEK_SET);
				fscanf(input, "%*s %s ",word);
				fprintf(writed, "%s\n", word);
			}
			
			fflush(writed);
			fclose(writed);
		}
		
		fclose(input);
		fflush(index);
		fclose(index);
	
	}
		
	return 0;
}
