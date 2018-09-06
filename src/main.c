  #include <stdio.h>
  #include <stdlib.h>
  #include <mpi.h>
  #include <string.h>
  
  typedef struct Info{
	int start;
	int finish;
	int numWords;
	int offset;
} infoJob;

typedef struct Word{
	char parola[255];
	int count;
} infoWord;

void initArrayInfoWord(infoWord* list, int size){
	int i = 0;
	while(i < size){
		sprintf(list[i].parola, "%s", "*");
		list[i].count = 0;
		i++;
	}	
}
void initArray(int* array, int size){
	int i = 0;
	while(i < size){
		array[i] =-1;
		i++;
	}	
}

int countWordsFile(char* nomeFile);
void cleanArray(int* wordFile, int* totWordsFile, int size);
void setJob(int* wordsFile, infoJob* listJob, int remainder, int partition);
void runJobSlave(infoJob myJob, infoWord* listWords);
void printWords(infoWord* result, int size);

FILE *fp;
int numFile=0, i=0; 
int my_rank; /* rank of process */
int	np; /* number of processes */
int totWords = 0; //somma del totale delle parole di ogni file

int main(int argc, char* argv[]){

	/* start up MPI */
    MPI_Init(&argc, &argv);
    /* find out process rank */
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    /* find out number of processes */
    MPI_Comm_size(MPI_COMM_WORLD, &np);
	
	const int nitems=2;
  
	MPI_Datatype jobInfo; //dichiariamo il tipo di dato jobInfo 
	MPI_Datatype wordInfo; //dichiariamo il tipo di dato wordInfo
	
	/*creazione del tipo di dato wordInfo*/
	MPI_Datatype types[2] = {MPI_CHAR, MPI_INT};
	int blocklengths[2] = {255,1};
	MPI_Aint disp[2];

	disp[0] = offsetof(infoWord, parola);
	disp[1] = offsetof(infoWord, count);

	MPI_Type_create_struct(nitems, blocklengths, disp, types, &wordInfo);
	/*fine*/
	infoJob *listJob = (infoJob*)malloc(sizeof(infoJob)*np); //alloca la memoria per la lista dei carichi di lavoro che dovranno eseguire i processo
	MPI_Type_contiguous(4, MPI_INT, &jobInfo); //creiamo un tipo contiguoi in modo da porterlo utilizzare nelle comunicazioni MPI
	
	/* rende effettiva la allocazione di memoria */
	MPI_Type_commit(&jobInfo); 
	MPI_Type_commit(&wordInfo); 
	/*fine*/
	
	infoJob myJob; //rappresenta il carico di lavoro di un determinato processo
	
	/* lettura del file index.txt eseguita dal master */
	if (my_rank == 0)
		numFile = countWordsFile("index.txt");
	
	MPI_Bcast(&numFile, 1, MPI_INT, 0, MPI_COMM_WORLD); //cominicazione broadcast eseguita dal master per comunicare il numero di file ai processori
	
	double tstart = MPI_Wtime(); //tempo iniziale
	
	int word = 0;
	int remainderFile = numFile % np;
    int partitionFile = numFile / np;
	int size = partitionFile+1; //calcolo della taglia 	di nWordsFile
	partitionFile = (my_rank<remainderFile) ? partitionFile+1 : partitionFile; //modifica il valore di partitionFile aggiungendo 1 nel caso in cui il remainderFile sia magiore di 0.
	
	//distribuzione del carico di lavoro, rappresentato dal numereo di file, utilizzando un offset calcolato tramite i valori di my_rank e remainderFile
	int offset = (my_rank==remainderFile)? my_rank*(partitionFile+1) : my_rank*partitionFile;
	if(my_rank>remainderFile) 
		offset = my_rank*partitionFile+remainderFile;
	/*fine*/
	
	int *nWordsFile = malloc(sizeof(int)*size); //array dove verrà inserito il numero totale di parole di ogni file 
	initArray(nWordsFile, size); //inizializza i campi di nWordsFile a -1
	int *totWordsFile = malloc(sizeof(int)*size*np);//array utillizzato in seguito nella MPI_Gather per contenere gli np nWordsFile inviati dagli slave
	char *nomeFile = malloc(sizeof(char)*255);//buffer per la costruzione del nome del file al quale accedere
	
	/* ogni processo leggera all'interno di un numero di file uguale a partitionFile */
	for(i=0; i<partitionFile; i++){
		word=0;
		
		/* costruzione nome file tramite l'offset calcolato in precedenza e l'indice del ciclo*/
		sprintf(nomeFile, "%s%d", "File-", offset+i);
		strcat(nomeFile, ".txt");
		/*fine*/
		
		word = countWordsFile(nomeFile);
		
		if(word==-1)//se il file è vuoto word = 0
			word =0;
		
		nWordsFile[i] = word;
	}
	/*output: numero totale di parole di ogni file*/
	
	MPI_Gather(nWordsFile, size, MPI_INT, totWordsFile, size, MPI_INT, 0, MPI_COMM_WORLD);//comunicazione degli svale del proprio nWordsFile al master
	
	free(nWordsFile);
	free(nomeFile);
	
	//master	
	if(my_rank==0){
		
		int *wordsFile = malloc(sizeof(int)*numFile);
		
		cleanArray(wordsFile, totWordsFile, size);
		
		int remainder = totWords % np;
		int partition = totWords / np;
		
		setJob(wordsFile, listJob, remainder, partition);
	
		free(wordsFile);
	}
	
	free(totWordsFile);
	
	MPI_Bcast(&totWords, 1, MPI_INT, 0, MPI_COMM_WORLD);//cominicazione broadcast eseguita dal master per comunicare il numero totale di parole ai processori
	MPI_Scatter(listJob,1,jobInfo,&myJob,1,jobInfo,0,MPI_COMM_WORLD);//il master comunica  agli slave il carico di lavoro da eseguire
	
	free(listJob);
	
	size = (totWords / np) +1;////calcolo della taglia 	di listWords
	infoWord *listWords = (infoWord*)malloc(sizeof(infoWord)*size);//conterrà la lista delle parole processate da ogni slave
	
	initArrayInfoWord(listWords, size);
	
	runJobSlave(myJob, listWords);

	infoWord *result = (infoWord*)malloc(sizeof(infoWord)*size*np);//allocazione memoria per il risultato finale
	MPI_Gather(listWords, size, wordInfo, result, size, wordInfo, 0, MPI_COMM_WORLD);//comunicazione degli slave al master del risultato finale
	
	free(listWords);
		
	double tend = MPI_Wtime();//tempo finale
	
	if(my_rank==0){
		printWords(result, size);
		printf("Tot time -->:%f\n", tend-tstart);
	}
	
	free(result);
	
	MPI_Finalize();
    return 0;	
}

/* lettura del numero di parole di un file */
int countWordsFile(char* nomeFile){
	char *next;
	next = malloc(sizeof(char)*255);
	int count =0;
	
	if((fp = fopen(nomeFile,"r"))==NULL){
			printf("Error!");   
			exit(1);
		}
	
	while(!feof(fp))
		count += fscanf(fp, "%s\n", next);
		
	fclose(fp);
	free(next);
	
	return count;
}

/*ripulisce l'array totWordsFile eliminando i -1, calcola la somma del totale delle parole di ogni file e ordina
all'interno di wordFile il numero totale di parole di ogni file in modo da avere wordFile[i] = al totale delle parole del file i*/		
void cleanArray(int* wordsFile, int* totWordsFile, int size){
	
	int k =0, j=0;
	
	for(i=0; i<size*np; i++){
		if(totWordsFile[i]!=-1){
			wordsFile[k] = totWordsFile[i];
			k++;
			totWords += totWordsFile[i];
		}
		else{
			j=i;
			while(totWordsFile[j]==-1)
				j++;
			if(j<size*np){
				wordsFile[k] = totWordsFile[j];
				k++;
				totWords += totWordsFile[j];
			}
			i=j;
		}
	}	
}

/*calcolo del carico di lavoro per ogni slave*/
void setJob(int* wordsFile, infoJob* listJob, int remainder, int partition){
	
	int dimPart = 0;
	int j = 0;
	int flag = 0;
	int temp = numFile;
	int offset=0;
		
	for(i=0; i<np; i++){
		dimPart = partition;
		numFile = temp;
		if(i<remainder)
			dimPart+=1;
		flag = 0;
		listJob[i].numWords = dimPart;//numero di parole da leggere(slave)
		while(j<numFile){
			if(wordsFile[j]==0)
				j++;
			else if(dimPart<wordsFile[j]){
				listJob[i].finish = j;//ultimo file in cui leggere (slave)
				listJob[i].offset =offset;//offset da dove iniziare a la lettura (slave)
				wordsFile[j] -=dimPart;
				if(flag==0){
					offset+=dimPart;
					listJob[i].start = j; //primo file in cui leggere (slave)
				}
				else
					offset = dimPart;

			numFile=0;
			}	
			else{
				dimPart -=wordsFile[j];
				if(flag==0)
					listJob[i].start = j;
				listJob[i].finish = j;
				listJob[i].offset =offset;
				flag=1;
				j++;
			}		
		}
	}
}

/*ogni slave calcola le occorrenze delle parole all'interno dell'insieme delle parole a lui assegnate*/
void runJobSlave(infoJob myJob, infoWord* listWords ){
	
	char *file = malloc(sizeof(char)*255);//buffer per la costruzione del nome del file al quale accedere
	int j = 0;
	int k = 0;
	int partition = myJob.numWords;	
	
	for(i=myJob.start; i<=myJob.finish; i++){
		
		sprintf(file, "%s%d", "File-", i);
		strcat(file, ".txt");
		
		if((fp = fopen(file,"r"))==NULL){
				printf("Error!");   
				exit(1);
			}
		if(fscanf(fp, "%*s\n")!=-1){
			rewind(fp);
		
		if(myJob.offset!=0 && i==myJob.start)//spostamentoo cursore file
			for(j=0; j<myJob.offset; j++)
				fscanf(fp, "%*s\n");
	
			while(k<partition && !feof(fp)){
				fscanf(fp, "%s\n", listWords[k].parola);
				for(j=k; j<partition; j++){
					if(strcmp(listWords[k].parola, listWords[j].parola)==0)//confronto parole
						listWords[j].count++;//incremento dell'occorrenza
				}
				k++;	
			}
		}
		
		fclose(fp);
	}
	
	free(file);	
}

/*eliminazione logica dei duplicati, aggiornamento dell'occorrenza di ogni parola e stampa al video del risultato finale*/
void printWords(infoWord* result, int size){
	
	int j=0;
	for(i=0; i<size*np; i++){
		if(strcmp(result[i].parola, "*")!=0){
			for(j=i+1; j<size*np; j++){
				if(strcmp(result[i].parola, result[j].parola)==0){
					result[i].count+=result[j].count;
					sprintf(result[j].parola, "%s", "*"); 
				}
			}
			printf("word:%s -->:%d\n", result[i].parola,result[i].count);
		}
	}
}