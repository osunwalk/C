#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <errno.h>
#include <stdbool.h>
//FOR HEAP
bool first = true;
bool b, r, c, d, h;
bool n = false;
bool t = false;
bool s = false;
int inc1,inc2,inc3;
int counter = 0;
int global = 0;
//FOR COMPARE
typedef struct {
        char word[2000];
        int frequency;
} WordArray ;
WordArray words[2000];

struct mhnode
{
	unsigned freq;
	struct mhnode *left, *right;
};
struct mh
{
	unsigned size;
	unsigned capacity;
	struct mhnode** array;
};
struct mhnode* newNode(unsigned freq)
{
	struct mhnode* temp = (struct mhnode*)malloc (sizeof(struct mhnode));
	temp->left = temp->right = NULL;
	temp->freq = freq;
	return temp;
}
struct mh* createmh(unsigned capacity)
{
	struct mh* temp = (struct mh*)malloc(sizeof(struct mh));
	temp->size =0;
	temp->capacity = capacity;
	temp->array = (struct mhnode**)malloc(temp->capacity*sizeof(struct mhnode*));
	return temp;
}
void swap(struct mhnode** a, struct mhnode** b)
{
	struct mhnode* t = *a;
	*a = *b;
	*b = t;
}
void heapify(struct mh* temp, int idx)
{
	int smallest = idx;
	int left = 2*idx +1;
	int right = 2*idx +2;
	if (left < temp->size && temp->array[left]-> freq < temp->array[smallest]->freq)
		smallest = left;
	if (right < temp->size && temp->array[right]-> freq < temp->array[smallest]->freq)
		smallest = right;
	if (smallest != idx) {
		swap(&temp->array[smallest], &temp->array[idx]);
        heapify(temp, smallest);
    }
}


// A standard function to extract
// minimum value node from heap
struct mhnode* extractMin(struct mh* minHeap)
{
    struct mhnode* temp = minHeap->array[0];
    minHeap->array[0] = minHeap->array[minHeap->size - 1];
    --minHeap->size;
    heapify(minHeap, 0);

    return temp;
}

// A utility function to insert
// a new node to Min Heap
void insertMinHeap(struct mh* minHeap, struct mhnode* minHeapNode)
{
	++minHeap->size;
	int i = minHeap->size - 1;

	while (i && minHeapNode->freq < minHeap->array[(i - 1) / 2]->freq) {
		minHeap->array[i] = minHeap->array[(i - 1) / 2];
        i = (i - 1) / 2;
    }

    minHeap->array[i] = minHeapNode;
}

// A standard funvtion to build min heap
void buildMinHeap(struct mh* minHeap)
{
    int n = minHeap->size - 1;
    int i;
    for (i = (n - 1) / 2; i >= 0; --i)
        heapify(minHeap, i);
}

// Utility function to check if this node is leaf 
int isLeaf(struct mhnode* root)
{
    return !(root->left) && !(root->right);
}

// Creates a min heap of capacity 
// equal to size and inserts all character of 
// data[] in min heap. Initially size of 
// min heap is equal to capacity 
struct mh* createAndBuildMinHeap( int freq[], int size)
{ 
	int i;
    struct mh* minHeap = createmh(size); 
  
    for (i = 0; i < size; ++i) 
        minHeap->array[i] = newNode( freq[i]); 
  
    minHeap->size = size; 
    buildMinHeap(minHeap); 
  
    return minHeap; 
} 
  
// The main function that builds Huffman tree 
struct mhnode* buildHuffmanTree( int freq[], int size)  
{ 
    struct mhnode *left, *right, *top;
    struct mh* minHeap = createAndBuildMinHeap( freq, size); 

    while (minHeap->size !=1) {   
        left = extractMin(minHeap);
        right = extractMin(minHeap); 
        top = newNode( left->freq + right->freq); 
  
        top->left = left; 
        top->right = right; 
  
        insertMinHeap(minHeap, top); 
    } 

    return extractMin(minHeap); 
} 

// A utility function to print an array of size n
void printArr(int arr[], int n, char *path)
{
    int fd = open(path, O_RDWR|O_APPEND);
    int i;
    for (i = 0; i < n; ++i) {
        char* buffer = malloc(sizeof(char)*sizeof(int));
        sprintf(buffer, "%d", arr[i]);
        write(fd, buffer, 1);
        printf("%d", arr[i]);
    }
}
// Prints huffman codes from the root of Huffman Tree. 
// It uses arr[] to store codes 
void printCodes(struct mhnode* root, int arr[], int top, char* path)  
{  
    // Assign 0 to left edge and recur 
    if (root->left) {  
        arr[top] = 0; 
        printCodes(root->left, arr, top + 1, path); 
    } 
  
    // Assign 1 to right edge and recur 
    if (root->right) {   
        arr[top] = 1; 
        printCodes(root->right, arr, top + 1, path); 
    } 
  
    // If this is a leaf node, then 
    // it contains one of the input 
    // characters, print the character 
    // and its code from arr[] 
    if (isLeaf(root)) {
        printArr(arr,top,path);
        int fd = open(path, O_RDWR|O_APPEND);
        int size = strlen(words[global].word)+2;
        //printf("word: %s\n",words[global].word);
        //printf("size: %d\n",size);
        char *buffer = malloc(sizeof(char)*size);
        sprintf(buffer, "\t%s\n", words[global].word);
        //printf("buffer: %s",buffer);
        write(fd, buffer, size);
        printf("\t%s\n", words[global].word); 
        global++;
    } 
} 
  
// The main function that builds a 
// Huffman Tree and print codes by traversing 
// the built Huffman Tree 
void HuffmanCodes( int freq[], int size, char *path)  
{ 
    printf("huffPATH: %s\n",path);
    // Construct Huffman Tree 
    struct mhnode* root = buildHuffmanTree(freq, size); 
  
    // Print Huffman codes using 
    // the Huffman tree built above 
    int arr[counter], top = 0; 
  
    printCodes(root, arr, top, path); 
} 

int compareWords(const void *f1, const void *f2)
{
	WordArray *a = (WordArray *)f1;
	WordArray *b = (WordArray *)f2;
	return -(a->frequency - b->frequency);
}

void tokenize(char* path)
{
    int isUnique;
	int i;
    char delimit[]="  \t\n";
    int fd = open(path, O_RDONLY, 0);
    //Get size of file
    int size = lseek(fd, 0, SEEK_END);
    //printf("Size is %d\n", size);

    char *buffer = malloc(sizeof(char)*size+1);
    lseek(fd,0,SEEK_SET); //Go back to the start    
    read(fd, &buffer[0], size);
                for(i = 0; i < size; i++)
                {
                        if(buffer[i] == '\n'&&n)
                                words[inc1].frequency++;
                        if(buffer[i] == '\t'&&t)
                                words[inc2].frequency++;
                        if(buffer[i] == ' '&&s)
                                words[inc3].frequency++;
                }

    buffer[size] = '\0';
    //printf("buffer: %s\n",buffer);

    char *token;
    //First token
    token = strtok(buffer, delimit);

    while(token != NULL) {
        isUnique=-1;
        int k;
        for(k=0; k <counter;k++)
        {
            if(strcmp(words[k].word,token)==0)
                isUnique=k;
        }
        if(isUnique==-1)
        {
            strcpy(words[counter].word, token);
            words[counter].frequency = 1;
            counter++;
        }
        else
            words[isUnique].frequency++;
                token = strtok(NULL, delimit);
    }
}
void substr(char* str, char* sub, int start, int len)
{
	memcpy(sub, &str[start], len);
	sub[len] = '\0';
}
void decompress(char*path, const char* huff)
{
	int i=0;
	int k;
	int s1 =0;
	int t, n, s;
	const char *bitSeqs[100];
	const char *toks[100];
	int bitLengths[100];
	char delimit[] = "\t\n";
	int fd = open(huff,O_RDONLY, 0);
	int size = lseek(fd,0,SEEK_END);
	char *buffer = malloc(sizeof(char)*size+1);
	lseek(fd,0,SEEK_SET);
	read(fd, &buffer[0], size);
	buffer[size]= '\0';
	char *token;
	token = strtok(buffer, delimit);
	token = strtok(NULL,delimit);
	while(token!=NULL)
	{
	        if(i%2==0)
	        {
	                bitSeqs[s1]= token;
	                bitLengths[s1]=(int)strlen(token);
	        }
	        else
	        {
	                if(strcmp(token, "%t")==0)
	                        t = s1;
	                else if(strcmp(token, "%n")==0)
	                        n = s1;
	                else if(strcmp(token," ")==0)
	                        s = s1;
	                toks[s1]= token;
	                s1++;
	        }
	        i++;
	        token = strtok(NULL, delimit);
	}

	printf("SIZE OF bitSeqs: %d\n",s1);
	for(i = 0; i<s1; i++) {
		printf("bitSeq: %s\t",bitSeqs[i]);
		printf("length: %d\t",bitLengths[i]);
		printf("tok: %s\n",toks[i]);
	}

	//Find longest bitSeq and assign its size to length
	int longest = 0;
	for (i=0; i<s1; i++) {
		if (longest<bitLengths[i]) 
			longest=bitLengths[i];
	}
	printf("longest: %d\n",longest);

	fd = open(path, O_RDONLY);
	size = lseek(fd,0,SEEK_END);
	char *buffer2 = malloc(sizeof(char)*size+1);
	char *sub = malloc(sizeof(char)*size+1);
	lseek(fd,0,SEEK_SET);
	read(fd, &buffer2[0], size);
	
	
	char newFile[100];
	substr(path, newFile, 0, strlen(path)-8);
	strcat(newFile,"New.txt");
	printf("path: %s\n",newFile);
	open(newFile, O_CREAT|O_TRUNC, 00700);
    int fd2 = open(newFile, O_RDWR|O_APPEND);

	int curPos = 0; //Position in the string of buffer2
	int lenBuf2 = strlen(buffer2);
	int curLen = longest;

	//substr(buffer2, sub, curPos, curLen);
	//printf("Current substring: %s\n", sub);
	//printf("s1: %d\n",s1);
	//printf("length of bitSeqs,toks,bitLengths: %lu%lu%lu",)
	while(curPos<lenBuf2) {	
		for (i=0; i<s1; i++) {
			//printf("I: %d\n",i);
			substr(buffer2, sub, curPos, curLen);
			if(strcmp(sub,bitSeqs[i])==0) {
				//Write to file
				//printf("substring: %s\n",sub);
				printf("%s",toks[i]);
				if(strcmp(toks[i], "%t")==0)
					write(fd2,"\t",1);
                                else if(strcmp(toks[i], "%n")==0)
                                        write(fd2,"\n",1);
				else
					write(fd2,toks[i],strlen(toks[i]));
				if(curPos+bitLengths[i]>=lenBuf2){
					printf("\n");
					return;
				}
				curPos+=bitLengths[i];
				curLen=longest;
				break;
			}
			else if(i==s1-1) {
				--curLen;
			}
		}
	}
	
}

void compress(char* path,const char* huff)
{
	int i=0;
	int k;
	int s1 =0;
	int t, n, s;
	const char *strings[300];
	const char *strings2[100];
	char delimit[] = "\t\n";
	int fd = open(huff,O_RDONLY, 0);
	int size = lseek(fd,0,SEEK_END);
	char *buffer = malloc(sizeof(char)*size+1);
	lseek(fd,0,SEEK_SET);
	read(fd, &buffer[0], size);
	buffer[size]= '\0';
	char *token;
	token = strtok(buffer, delimit);
	token = strtok(NULL,delimit);
	while(token!=NULL)
	{
		if(i%2==0)
		{
			strings[s1]= token;
		}
		else
		{
			if(strcmp(token, "%t")==0)
				t = s1;
			else if(strcmp(token, "%n")==0)
				n = s1;
			else if(strcmp(token," ")==0)
				s = s1;
			strings2[s1]= token;
			s1++;
		}
		i++;
		token = strtok(NULL, delimit);
	}
	fd = open(path, O_RDONLY);
        size = lseek(fd,0,SEEK_END);
        char *buffer2 = malloc(sizeof(char)*size+1);
	char *sub = malloc(sizeof(char)*size+1);
        lseek(fd,0,SEEK_SET);
        read(fd, &buffer2[0], size);
        sprintf(path, "%s%s", path,".hcz");
        open(path, O_CREAT|O_TRUNC, 00700);
        int fd2 = open(path, O_RDWR|O_APPEND);
	int len = 0;
	int start = 0;
	for(i = 0; i < size; i++)
	{
		if(buffer2[i] == '\n')
		{
			substr(buffer2, sub, start, len);
			if(strcmp(sub,"")==0)
			{
				write(fd2,strings[n],strlen(strings[n]));
				start = i+1;
			}
			else
			{
            	for(k=0; k < s1; k++)
            	{
                   		 	if(strcmp(sub, strings2[k])==0)
                    		{
                    		        write(fd2, strings[k], strlen(strings[k]));
                    		        write(fd2, strings[n], strlen(strings[n]));
				start = i+1;
				len = 0;
                    		        break;
                    		}
            	}
			}
			if (k==s1)
				exit(0);
		}
		else if(buffer2[i] == '\t')
                {
			substr(buffer2, sub, start, len);
                        if(strcmp(sub,"")==0)
			{
                                write(fd2,strings[t],strlen(strings[t]));
				start = i+1;
			}
                        else
                        {
                        	for(k=0; k < s1; k++)
                        	{
                                	if(strcmp(sub, strings2[k])==0)
                                	{
                                        	write(fd2, strings[k], strlen(strings[k]));
                                        	write(fd2, strings[t], strlen(strings[t]));
						start = i+1;
						len = 0;
						break;
                                	}
                        	}
			}
                        if (k==s1)
                                exit(0);

                }
		else if(buffer2[i] == ' ')
                {
			substr(buffer2, sub, start, len);
                        if(strcmp(sub,"")==0)
			{
                                write(fd2,strings[s],strlen(strings[s]));
				start = i+1;
			}
                        else
                        {
                        	for(k=0; k < s1; k++)
                        	{
                               		 if(strcmp(sub, strings2[k])==0)
                                	{
                                       		write(fd2, strings[k], strlen(strings[k]));
                                       		write(fd2, strings[s], strlen(strings[s]));
						start = i+1;
						len = 0;
						break;
                                	}
                        	}
			}
                        if (k==s1)
                                exit(0);

                }
		else
		{
			len++;
			continue;
		}
	}
	if(len>0)
        {
                substr(buffer2, sub, start, len);
                for(k=0;k < s1;k++)
                {
                        if(strcmp(sub,strings2[k])==0)
                        {
                                write(fd2,strings[k],strlen(strings[k]));
                        }
                }
        }
	//write(fd2, "\0", 1);
}
	

void listdir(const char *name, const char *huff)
{
	int i;
	if(!r) { //If not recursive, it's a file
		char path[2000];
		strcpy(path,name);
		int len = strlen(path);
		const char *ext = &path[len-4];
		if(b&&(strcmp(ext,".txt")==0))
			tokenize(path);
		else if(c&&(strcmp(ext,".txt")==0))
			compress(path,huff);
		else if(d&&strcmp(ext,".hcz")==0)
			decompress(path,huff);
		else
			exit(0);
	}
	else if (r)
	{
		DIR *dir;
		struct dirent *entry;
		if (!(dir = opendir(name)))
			return;

		while ((entry = readdir(dir)) != NULL) {
			if (entry->d_type == DT_DIR)
			{
				if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
					continue;
				char path[500];
 				strcpy(path, name);
				snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
				listdir(path, huff);
			}
			else if(entry->d_type == DT_REG)
			{
				char path[500];
				int len = strlen(entry->d_name);
				const char *ext = &entry->d_name[len-4];
				strcpy(path, name);
				strcat(path,"/");
				strcat(path,entry->d_name);
 
               int fd = open(path,O_RDONLY, 0);
                int size = lseek(fd,0,SEEK_END);
                char *buffer = malloc(sizeof(char)*size+1);
                lseek(fd,0,SEEK_SET);
                read(fd, &buffer[0], size);


                                        for(i = 0; i < size; i++)
                                        {
                                                if(buffer[i] == '\n'&& n!=true)
                                                {
                                                        n = true;
                                                        strcpy(words[counter].word, "%n");
                                                        inc1= counter;
                                                        counter++;
                                                }
                                                if(buffer[i] == '\t' && t!=true)
                                                {
                                                        t = true;
                                                        strcpy(words[counter].word, "%t");
                                                        inc2 = counter;
                                                        counter++;
                                                }
                                                if(buffer[i] == ' ' && s!=true)
                                                {
                                                        s = true;
                                                        strcpy(words[counter].word, " ");
                                                        inc3 = counter;
                                                        counter++;
                                                }
                                        }

				if(b&&(strcmp(ext,".txt")==0))
					tokenize(path);
				else if(c&&(strcmp(ext,".txt")==0))
					compress(path,huff);
				else if(d&&strcmp(ext,".hcz")==0)
					decompress(path,huff);
				else
					continue;
			}
		}
		closedir(dir);
	}
}

int main(int argc, const char* argv[])
{
	char path[2000];
	char path2[2000];
	char huff[2000];
	int i;

	//-----CHECK FLAGS-----
	for(i =1; i < argc; i++)
	{
		if(strcmp(argv[i], "-b")==0)
		{
			b= true;
			if(c||d)
				return 1;
		}
		else if(strcmp(argv[i], "-c")==0)
		{
                        c= true;
			if(d||b)
				return 1;
		}
		else if(strcmp(argv[i], "-d")==0)
		{
                        d= true;
			if(b||c)
				return 1;
		}
		else if(strcmp(argv[i], "-R")==0)
                        r= true;
		else
			continue;
	}

	//Check if HuffmanCodebook is an input. If so, h=true because must be compress or decompress
	if(strcmp(argv[argc-1], "/HuffmanCodebook")==0 ||strcmp(argv[argc-1], "./HuffmanCodebook")==0)
		h = true;
	if(b&&h)
		return 1;
	if(!h && c)
		return 1;
	if(!h && d)
		return 1;
	if(h)
	{       
        if(argv[argc-2][0] == '.'){
        	strcpy(path2, &argv[argc-2][1]);
	        sprintf(path, "%s%s", getcwd(huff,sizeof(huff)),path2);
	    }
	    else {
	    	size_t length = strlen(argv[argc-2])+1;
	    	memcpy(path,argv[argc-2], length);
	    }
		strcpy(huff, argv[argc-1]);
        sprintf(huff, "%s%c%s", getcwd(huff,sizeof(huff)),'/',"HuffmanCodebook");

	}
	else
	{
		size_t length = strlen(argv[argc-1])+1;
		memcpy(path, argv[argc-1], length);
	}
	if(b)
	{
		int fd = open(path,O_RDONLY, 0);
		int size = lseek(fd,0,SEEK_END);
		char *buffer = malloc(sizeof(char)*size+1);
        	lseek(fd,0,SEEK_SET);
        	read(fd, &buffer[0], size);


                for(i = 0; i < size; i++)
                {
                        if(buffer[i] == '\n')
				n = true;
                        if(buffer[i] == '\t')
                                t = true;
                        if(buffer[i] == ' ')
                                s = true;
                }
		int inc = 0;
		if(n== true)
		{
			strcpy(words[inc].word, "%n");
			for(i=0;i < size;i++)
			{
				if(buffer[i]=='\n')
					words[inc].frequency++;
			}
			inc1 = inc;
			inc++;
		}
		if (t == true)
		{
			strcpy(words[inc].word, "%t");
			for(i=0;i < size;i++)
			{
				if(buffer[i]=='\t')
					words[inc].frequency;
			}
			inc2= inc;
			inc++;
		}
		if(s==true)
		{
			strcpy(words[inc].word," ");
                        for(i=0;i < size;i++)
                        {
                                if(buffer[i]==' ')
                                        words[inc].frequency;
                        }
			inc3 = inc;
			inc++;
		}
		counter = inc;

		sprintf(path, "%s%c%s", getcwd(path,sizeof(path)),'/',"HuffmanCodebook");
		printf("PATH: %s\n",path);
		open(path, O_CREAT|O_TRUNC, 00700);
		//char*buffer2 = malloc(4);
		fd = open(path, O_RDWR|O_APPEND);
		sprintf(buffer, "%%\n");
		write(fd, buffer, strlen(buffer));
		listdir(argv[argc-1], path);
		qsort(words, counter, sizeof(WordArray),compareWords);
		int freq[counter];
		for(i = 0; i < counter;i++)
        	{
                	freq[i] = words[i].frequency;
//              	printf("%s %d\n", words[i].word,words[i].frequency);
        	}
        	HuffmanCodes(freq,counter,path);

	}
	else
	{
		listdir(path,huff);
	}


    return 0;
}
