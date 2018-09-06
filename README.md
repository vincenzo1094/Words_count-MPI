# Words-count Simulation

***
## Programmazione Concorrente, Parallela e su Cloud

### Università  degli Studi di Salerno

#### *Anno Accademico 2017/2018*

**Professore:** *Vittorio Scarano,*
 **Dottor:** *Carmine Spagnuolo,*
 **Studente:** *Vincenzo Belmonte*


---

## Problem Statement  

Effettuare il conteggio delle parole su un numero elevato di file. 
Un processo dovrà leggere in un file principale che conterrà i nomi di tutti i file che devono essere contati. In seguito, ogni processo riceverà la propria parte del file dal processo principale. Una volta che un processo ha ricevuto il suo elenco di file da elaborare, dovrebbe quindi leggere in ciascuno dei file ed eseguire un conteggio delle parole, tenendo traccia della frequenza di ciascuna parola trovata nei file. Finita l'elaborazione ogni processo invierà il proprio risultato al processo principale, il quale stamperà a video l'output finale.

## Soluzione proposta
Words_count leggerà all'interno di un file index.txt i nomi dei file da processare, quest'ultimi saranno suddivisi tra i vari processi. Ogni processo conterà il numero di parole all'interno dei file che gli sono stati assegnati e comunicherà questo valore al master. Il master procederà a suddividere il carico di lavoro in base al numero totale di parole e quindi, ogni slave avrà un insieme di parole da analizzare. Infine gli slave comunicheranno il risultato delle loro analisi al master che stamperà in output il risultato finale.
La comunicazione è avvenuta usando  la comunicazione collettiva con la funzione **MPI_Scatter** , **MPI_Gather**, **MPI_Bcast**.
I test sono stati effettuati sulle istanze di AWS **m4.xlarge**.


## Implementazione
L'obiettivo del lavoro svolto è stato quello di parallelizzare il  conteggio delle parole su un numero elevato di file tenendo traccia della frequenza di ciascuna, partizionando equamente il lavoro tra i processi coinvolti all'interno del cluster.
L'implementazione parte dal presupposto che non viene utilizzata la rete per trasferire i file dal master agli slave ma bensì, ogni slavi avrà in memoria una copia del file in cui dovrà andare a leggere, inoltre, ogni nome dei file avrà una parte fissa (File) e una parte variabile costituita da un carattere numerico. 
### Descrizione variabili
**infoJob:** struttura utilizzata per assegnare il carico di lavoro ad ogni processore. Definisce l'insieme delle parole da analizzare.
Il campo **start** indica il file da dove iniziare a leggere, **finish** indica l'ultimo file in cui leggere, **offset** indica dove posizionare il cursore del file prima di iniziare a leggere la prima parola, **numWords** indica il numero di parole da leggere.
```c
  typedef struct Info{
	int start;
	int finish;
	int numWords;
	int offset;
} infoJob;
```
**infoWord**: struttura utilizzata per analizzare ogni singola parola e tener traccia dell'occorrenza.
**parola** indica la parola analizzata, **count** indica il numero di occorrenze.
```c
  typedef struct Word{
	char parola[255];
	int count;
} infoWord; 
```
### Descrizione funzioni

#### initArrayInfoWord
Inizializza la variabile listWords di tipo InfoWord, ponendo il campo **parola** uguale al carattere "*" e il campo **count** a zero
```c
 void initArrayInfoWord(infoWord* list, int size){
	int i = 0;
	while(i < size){
		sprintf(list[i].parola, "%s", "*");
		list[i].count = 0;
		i++;
	}	
}
  ```
  #### initArray
  Inizializza l'array nWordsFile ponendo ogni locazione uguale a -1
  ```c
void initArray(int* array, int size){
	int i = 0;
	while(i < size){
		array[i] =-1;
		i++;
	}	
}
  ```

#### countWordsFile
Legge il numero di parole all'interno di un file.
  ```c
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
  ```
  #### cleanArray
  Crea una array nel quale i-esima locazine contiene il totale delle parole del file i-esimo. In più calcola la somma totale delle parole dei file. 
   ``` c
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
 ```
 #### setJob
 Calcola il carico di lavoro per ogni slave.
 Il primo ciclo scorre tutti gli svale assegnando il numero di parole da analizzare. Nel secondo ciclo vengono iterati i file, assegnando ad ogni slave il range di file da cui leggere e l'offset . 
   ``` c
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
 ```
 #### runJobSlave
 Viene eseguita da ogni slave per contare le occorrenze delle parole presenti nel range di file assegnato.
 Dopo l'esecuzione ogni slave avrà una lista delle parole analizzati con le rispettive occorrenze.
   ``` c
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
 ```
 #### printWords
 Elimina, utilizzando la cancellazione logica, le possibili duplicazioni delle parole analizzate dagli slave, aggiornandone l'occorrenza e infine stampa l'output finale.
   ``` c
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
 ```
### Il main:
La prima operazione effettuata nel main è quella di inizializzare l'ambiente MPI.
```c
 /* start up MPI */
    MPI_Init(&argc, &argv);
    /* find out process rank */
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    /* find out number of processes */
    MPI_Comm_size(MPI_COMM_WORLD, &np);
    
```

In seguito vengono dichiatari i tipi di dati jobInfo e wordInfo e resi accessibili in maniera contigua.
```c
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
    
```
Fatto ciò, vengono calcolate le variabili remainderFile e partitionFile
 ```c
	int remainderFile = numFile % np;
    int partitionFile = numFile / np;
```
 ```c
	partitionFile = (my_rank<remainderFile) ? partitionFile+1 : partitionFile; //modifica il valore di partitionFile aggiungendo 1 nel caso in cui il remainderFile sia magiore di 0.
```
 e un offset per la distribuzione dei file per i vari  processi.
 ```c	
	//distribuzione del carico di lavoro, rappresentato dal numereo di file, utilizzando un offset calcolato tramite i valori di my_rank e remainderFile
	int offset = (my_rank==remainderFile)? my_rank*(partitionFile+1) : my_rank*partitionFile;
	if(my_rank>remainderFile) 
		offset = my_rank*partitionFile+remainderFile;
	/*fine*/
```
A questo punto, ogni processo conterà le parole all'interno dei file a lui assegnati e comunicherà il risultato al master.
 ```c	
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
	
	MPI_Gather(nWordsFile, size, MPI_INT, totWordsFile, size, MPI_INT, 0, MPI_COMM_WORLD);
```

Il master, avendo a disposizione la somma di tutte le parole presenti nei file, procede ad assegnare un sottoinsieme delle parole ad ogni slave.
 ```c
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
```
Gli slave cominicano la propria analisi al master.
```c
MPI_Gather(listWords, size, wordInfo, result, size, wordInfo, 0, MPI_COMM_WORLD);
```
### Testing
I test sono stati effettuati sulle istanze __m4.xlarge__ (4 core) di Amazon Web Services.  
Durante la fase di testing si è tenuto conto sia di strong scaling che di weak scaling

Risorse massime utilizzate:

* 8 Istanze EC2 m4.xlarge **StarCluster-Ubuntu-12.04-x86_64-hvm** - _ami-52a0c53b_
* 32 processori (4 core per Istanza).

I tempi presenti nelle immagini dello strong e del weak scaling sono riportati in **secondi**.
# Strong Scaling

Nella fase di testing che ha tenuto in considerazione lo strong scaling sono stati utilizzati 400 file per un totale di 61517 parole.
Nello strong scaling infatti il numero dell'input resta invariato, quello che cambia è il numero di processori.
Nella figura in basso è possibile osservare i risultati di questa fase di testing.
<div style="text-aling:center">
 <img src="https://github.com/CiccioTecchio/n-Body_MPI/blob/master/img/strong.png"/>
 </div>
 
 ## Weak Scaling

La fase di testing che ha tenuto in considerazione il weak scaling  sono stati utilizzati 20 file e 2000 parole  per processo.
Nel weak scaling infatti il numero di particelle cresce in maniera proporzionale al numero di processori.
Nella figura in basso è possibile osservare i risultati di questa fase di testing.
<div style="text-aling:center">
 <img src="https://github.com/CiccioTecchio/n-Body_MPI/blob/master/img/weak.png"/>
 </div>

## Come compilare il sorgente
Il sorgente va compilato con l'istruzione seguente
```bash
mpicc main.c -o main
```



## Come lanciare il main
Compilare il sorgente fileGen.c
```bash
gcc fileGen.c -o fileGen
```
Eseguire fileGen per creare i file da analizzare (utilizzare il file parole.txt come file dizionario) 
```bash
./fileGen <wordInputFile.txt> <prefix> <numFiles> <numWords> <seed>
```
Esempio
```bash
./fileGen parole.txt File 400 1000 100
```
Lo script produrrà 400 file con al più 1000 parole per file, scelte in maniera random dal file parole.txt.

---
Il file eseguibile del main dovrà trovarsi nella stessa directory dove sono stati generati i file da analizzare e dovrà essere lanciato con il seguente comando:
```bash
mpirun -np <num_processori> main 
```