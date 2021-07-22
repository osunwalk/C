#include "define.h"

char ipHost[100];
int port;
int sock;
int localExists;
int commitBool = 0; //To check if addFile will need to write a commit file

char * hashFile(char* path)
{
	int i= 0;
	unsigned char temp[SHA_DIGEST_LENGTH];
    	char buf[SHA_DIGEST_LENGTH*2];
	memset(buf, 0x0, SHA_DIGEST_LENGTH*2);
    	memset(temp, 0x0, SHA_DIGEST_LENGTH);
	FILE *fp;
	long ls;
	char * buffer;
	char* temp2;
	fp = fopen(path, "rb");
	if(!fp)perror(path),exit(1);
	fseek( fp , 0L , SEEK_END);
	ls = ftell( fp );
	rewind( fp );

	/* allocate memory for entire content */
	buffer = calloc( 1, ls+1 );
	if( !buffer ) fclose(fp),fputs("memory alloc fails",stderr),exit(1);

	/* copy the file into the buffer */
	if( 1!=fread( buffer , ls, 1 , fp) )
		fclose(fp),free(buffer),fputs("entire read fails",stderr),exit(1);
	SHA1((unsigned char*)buffer,strlen(buffer), temp);
	for (i=0; i < SHA_DIGEST_LENGTH; i++)
	{
        	sprintf((char*)&(buf[i*2]), "%02x", temp[i]);
    	}
	/* do your work here, buffer is a string contains the whole text */

	fclose(fp);
	free(buffer);
	temp2 = buf;
	return temp2;
}

int file_exist (char *filename)
{
  struct stat buffer;   
  return (stat (filename, &buffer) == 0);
}

void configure(char* host, char* port){
	int fd = open(".configure", O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR); 
	write(fd, host, strlen(host));
	write (fd, "\n", 1);
	write(fd, port, strlen(port));
	return;
}

void currentversionClient(char* projName)
{
	char message[500];
	read(sock, message, sizeof(message)); //Get how many loops to do
	//printf("COUNT: %s\n", message);

	int count = atoi(message);
	int i;
	for(i=0;i<count;i++) {
		read(sock,message,500);
		printf("%s", message);
	}
}

void commitClient(char* projName)
{
	commitBool = 1; //To tell addFile to write a commit file
	char comPath[500];
	sprintf(comPath, "%s/.Commit", projName);
	FILE *comF = fopen(comPath, "w"); //Create a commit file to overwrite existing ones
	fclose(comF);

    //Receive .Manifest
    char path[500];
    sprintf(path, "%s/.ServerManifest", projName);
	//printf(path);
    int fd = open(path, O_CREAT | O_RDWR | O_TRUNC, S_IWUSR | S_IRUSR);
    char buffer[1000];
    int fileSize = 0, len = 0;
    printf("REACHED\n");
    read(sock, buffer, 100); //Get size of file
    printf("RECEIVED FILESIZE: %s\n",buffer);
    fileSize = atoi(buffer);
    int remain_bytes = fileSize;
    while ((remain_bytes > 0) && ((len = read(sock, buffer, fileSize)) > 0))
    {
        write(fd, buffer, len);
        remain_bytes -= len;
    }

    //Create/update own .Manifest
    char clientPath[300];
    sprintf(clientPath, "%s/.Manifest", projName);
    FILE *clientMani = fopen(clientPath, "r+");
    if (clientMani == NULL) {
            printf("No .Manifest file found in local project.\n");
            return;
    }

    DIR *dr = opendir(projName);
    struct dirent *de;
    if (dr == NULL) {
            printf("Could not open current directory\n");
            return;
    }
    char filePath[300];
    while((de = readdir(dr)) != NULL) {
            if (de->d_name[0] == '.') {
                    continue;
            }
            sprintf(filePath, "%s/%s", projName, de->d_name);
            //printf("FILE PATH: %s\n", filePath);
            addFile(projName, filePath);
    }
    //printf("We made it boys\n");
    close(fd);
    fclose(clientMani);
	
	//Now compare Manifests
	char line[500];
	int cliManVers, servManVers;
	clientMani = fopen(clientPath, "r");
	FILE *serverMani = fopen(path, "r");

	//Check version numbers. If different, return
	fgets(line, 500, clientMani);
	cliManVers = atoi(line);
	fgets(line, 500, serverMani);
	servManVers = atoi(line);
	if (cliManVers != servManVers) {
		printf("Manifest versions do not match. Please update local project.\n");
		remove(comPath);
		fclose(clientMani);
		fclose(serverMani);
		return;
	}

	int cliCount = 0;
	int servCount = 0;
	
	while(fgets(line, 500, clientMani)) {
		cliCount++;
	}
	while(fgets(line, 500, serverMani)) {
		servCount++;
	}

	//Reset back to start
	fseek(clientMani,0,SEEK_SET);
	fseek(serverMani,0,SEEK_SET);

	//Skip first lines
	fgets(line, 500, clientMani);
	fgets(line, 500, serverMani);

	char *clientVersions[cliCount+1];
	char *clientPaths[cliCount+1];
	char *clientHashes[cliCount+1];

	int i = 0;
	char *token;
	while (fgets(line, 500, clientMani)) { //Store client manifest info
		if (line[0] == ' ' || line[0] == '\t' || line[0] == '\n') {
			continue;
		}
		clientVersions[i] = malloc(200);
		clientPaths[i] = malloc(500);
		clientHashes[i] = malloc(500);
		sscanf(line, "%s\t%s\t%s", clientVersions[i], clientPaths[i], clientHashes[i]);
		//printf("%s\t%s\t%s", clientVersions[i], clientPaths[i], clientHashes[i]);
		i++;
	}

	char *serverVersions[servCount+1];
	char *serverPaths[servCount+1];
	char *serverHashes[servCount+1];
	
	i = 0;
	while (fgets(line, 500, serverMani)) { //Store server manifest info
		if (line[0] == ' ' || line[0] == '\t' || line[0] == '\n') {
			continue;
		}
		serverVersions[i] = malloc(200);
		serverPaths[i] = malloc(500);
		serverHashes[i] = malloc(500);
		sscanf(line, "%s\t%s\t%s", serverVersions[i], serverPaths[i], serverHashes[i]);
		//printf("%s\t%s\t%s", serverVersions[i], serverPaths[i], serverHashes[i]);
		i++;
	}

	//Fix the paths in the server paths
	for (i=0; i<servCount; i++) {
		token = strtok(serverPaths[i], "/");
		//printf("TOKEN: %s\n", token);
		char *first = malloc(sizeof(token)+1);
		strcpy(first, token);
		token = strtok(NULL, "/");
		token = strtok(NULL, "/");
		char *third = malloc(sizeof(token)+1);
		strcpy(third, token);
		sprintf(serverPaths[i], "%s/%s", first,third);
		printf("New: %s\n", serverPaths[i]);
	}

	//Now for each file listed in client's .Manifest, look for it in server's .Manifest
	int j;
	int cliVersion;
	int servVersion;
	char str[256];
	for(i=0; i<cliCount; i++) {
		for (j = 0; j<servCount; j++) {
			if (strcmp(clientPaths[i],serverPaths[j]) == 0) { //We have a match
				//printf("ClientPath: %s\nServerPath: %s\n", clientPaths[i],serverPaths[j]);
				//Check version numbers and hash codes
				if(strcmp(clientHashes[i],serverHashes[j]) != 0) {
					cliVersion = atoi(clientVersions[i]);
					servVersion = atoi(serverVersions[j]);
					//printf("cliVersion: %d\nservVersion: %d\n", cliVersion, servVersion);
					if(servVersion>cliVersion) {
						printf("Commit failed. Server file version greater than client's file version. Please sync with repository before committing\n");
						remove(path);
						remove(comPath); //Delete commit file
						fclose(clientMani);
						fclose(serverMani);
						return;
					}
				}
			}//End if matching
		}
	}

	//If we got here there are no problems. Send the .Commit to the sersver
	int comm = open(comPath, O_RDONLY);
	char commitSize[1000];
	struct stat file_stat;
	fstat(comm, &file_stat);
	int sent_bytes;
	sprintf(commitSize, "%d", file_stat.st_size);
	len = write(sock, commitSize, 100); //Send size of file
	if (len<0) {
		printf("Error sending file to client\n");
		return;
	}	

	remain_bytes = file_stat.st_size;
	while(((sent_bytes = sendfile(sock, comm, NULL, 500)) > 0) && (remain_bytes > 0))
	{
        remain_bytes -= sent_bytes;
	}
	printf("%s sent to server\n", comPath);
	close(comm);

	fclose(clientMani);
	fclose(serverMani);
	remove(path); //Remove server .Manifest
}

void updateClient(char* projName)
{
	//Receive .Manifest
	char path[300];
	sprintf(path, "%s/.ServerManifest", projName);
	int fd = open(path, O_CREAT | O_RDWR | O_TRUNC, S_IWUSR | S_IRUSR);
	char buffer[1000];
	int fileSize = 0, len = 0;
	read(sock, buffer, sizeof(buffer)); //Get size of file
	fileSize = atoi(buffer);
	int remain_bytes = fileSize;
	while ((remain_bytes > 0) && ((len = read(sock, buffer, fileSize)) > 0)) 
	{
		write(fd, buffer, len);
		remain_bytes -= len;
	}

	//Create/update own .Manifest
	char clientPath[300];
	sprintf(clientPath, "%s/.Manifest", projName);
	FILE *clientMani = fopen(clientPath, "r+");
	if (clientMani == NULL) {
		printf("No .Manifest file found in local project.\n");
		return;
	}

	DIR *dr = opendir(projName);
	struct dirent *de;
	if (dr == NULL) {
		printf("Could not open current directory\n");
		return;
	}
	char filePath[300];
	while((de = readdir(dr)) != NULL) {
		if (de->d_name[0] == '.') {
			continue;
		}
		sprintf(filePath, "%s/%s", projName, de->d_name);
		//printf("FILE PATH: %s\n", filePath);
		addFile(projName, filePath);
	}
	//printf("We made it boys\n");
	close(fd);
	fclose(clientMani);

	//Now compare Manifests
	char line[500];
	int cliCount = -1; //-1 because first line is version number
	int servCount = -1;

	clientMani = fopen(clientPath, "r");
	FILE *serverMani = fopen(path, "r");
	while(fgets(line, 500, clientMani)) {
		cliCount++;
	}
	while(fgets(line, 500, serverMani)) {
		servCount++;
	}

	//Reset back to start
	fseek(clientMani,0,SEEK_SET);
	fseek(serverMani,0,SEEK_SET);

	char *clientVersions[cliCount+1];
	char *clientPaths[cliCount+1];
	char *clientHashes[cliCount+1];
	int cliVersion;
	int servVersion;
	int sameVersion = 0; //0 for false
	
	fgets(line, 500, clientMani); //Skip first line
	cliVersion = atoi(line); //Store mani version 
	fgets(line, 500, serverMani); //Skip first line
	servVersion = atoi(line); //Store mani version 
	if (cliVersion == servVersion) {
		sameVersion = 1;
	}

	int i = 0;
	char *token;
	while (fgets(line, 500, clientMani)) { //Scan the file for matches in filePath
		if (line[0] == ' ' || line[0] == '\t' || line[0] == '\n') {
			continue;
		}
		token = strtok(line, "\t\n");
		clientVersions[i] = malloc(strlen(token));
		strcpy(clientVersions[i], token);
		//printf("%s\t", clientVersions[i]);

		token = strtok(NULL, "\t\n");
		clientPaths[i] = malloc(strlen(token));
		strcpy(clientPaths[i], token);
		//printf("i: %s\t", clientPaths[i]);

		token = strtok(NULL, "\t\n");
		clientHashes[i] = malloc(strlen(token));
		strcpy(clientHashes[i], token);
		//printf("%s\n", token);
		i++;
	}

	char *serverVersions[servCount+1];
	char *serverPaths[servCount+1];
	char *serverHashes[servCount+1];
	
	i = 0;
	while (fgets(line, 500, serverMani)) { //Scan the file for matches in filePath
		if (line[0] == ' ' || line[0] == '\t' || line[0] == '\n') {
			continue;
		}
		token = strtok(line, "\t\n");
		serverVersions[i] = malloc(strlen(token));
		strcpy(serverVersions[i], token);
		//printf("%s\t", token);

		token = strtok(NULL, "\t\n");
		serverPaths[i] = malloc(strlen(token));
		strcpy(serverPaths[i], token);
		//printf("%s\t", serverPaths[i]);

		token = strtok(NULL, "\t\n");
		serverHashes[i] = malloc(strlen(token));
		strcpy(serverHashes[i], token);
		//printf("%s\n", token);
		i++;
	}

	//Fix the paths in the server paths
	char* first;
	char* third;
	for (i=0; i<servCount; i++) {
		token = strtok(serverPaths[i], "/");
		strcpy(first, token);
		token = strtok(NULL, "/");
		token = strtok(NULL, "/");
		third = token;
		sprintf(serverPaths[i], "%s/%s", first,third);
		//printf("New: %s\n", serverPaths[i]);
	}

	char *UMAD[5];
	char *updateVersion[5];
	char *updatePath[200];
	char *updateHash[200];
	//Now for each file listed in client's .Manifest, look for it in server's .Manifest
	int j;
	int k = 0;
	int found = 0; //0 means not found
	for(i=0; i<cliCount; i++) {
		found = 0;
		for (j = 0; j<servCount; j++) {
			//printf("PATHS: \ni: %d\t%s\nj: %d\t%s\n", i, clientPaths[i], j, serverPaths[j]);
			if (strcmp(clientPaths[i],serverPaths[j]) == 0) { //We have a match
				//Now check for differences
				if (strcmp(clientVersions[i], serverVersions[j]) == 0) { //Check file versions
					if(strcmp(clientHashes[i], serverHashes[j]) == 0) { //Check hash codes
						found = 1;
						break; //Do nothing
					}
					else {
						found = 1;
						UMAD[k] = "U";
						updateVersion[k] = clientVersions[i];
						updatePath[k] = clientPaths[i];
						updateHash[k] = clientHashes[i];
						k++;
						break;
					}
				}//end if file versions
				else { //File versions are not equal
					if(sameVersion == 1) { //Something went wrong
						printf("Error. Manifest version cannot be equal when file versions aren't.\n");
						printf("Resolve conflicts in following file first\nClient:%s\nServer:%s\n", clientPaths[i], serverPaths[j]);
						return;
					}

					if(strcmp(clientHashes[i], serverHashes[j])==0) { //Check hash codes
						found = 1;
						UMAD[k] = "M";
						updateVersion[k] = serverVersions[i];
						updatePath[k] = serverPaths[i];
						updateHash[k] = serverHashes[i];
						k++;
						break; 
					}
					else {
						printf("Error. Manifest version cannot be equal when file versions aren't.\n");
						printf("Resolve conflicts in following file first\nClient:%s\nServer:%s\n", clientPaths[i], serverPaths[j]);
						return;
					}
				}//end else file versions
			}//End if matching
		}
		if (found == 0) {//Reaching here means no matches were found on the server, so upload or delete
			if (sameVersion == 1) {
				UMAD[k] = "U";
				updateVersion[k] = clientVersions[i];
				updatePath[k] = clientPaths[i];
				updateHash[k] = clientHashes[i];
				k++;
			}
			else {
				UMAD[k] = "D";
				updateVersion[k] = clientVersions[i];
				updatePath[k] = clientPaths[i];
				updateHash[k] = clientHashes[i];
				k++;
			}
		}
		else {
			continue;
		}

	}

	//Check for Add case
	if (sameVersion == 0) {
		for(i=0; i<servCount; i++) {
			found = 0;
			for (j = 0; j<cliCount; j++) {
				if (strcmp(clientPaths[j],serverPaths[i]) == 0) {
					found = 1;
					break; //Found, so no move on to next file
				}
			}
			//Reaching here means it was not found
			if (found == 0) {//Reaching here means no matches were found on the server, so upload or delete
				UMAD[k] = "A";
				updateVersion[k] = serverVersions[i];
				updatePath[k] = serverPaths[i];
				updateHash[k] = serverHashes[i];
				k++;
			}
			else {
				continue;
			}			
		}
	}

	char upPath[500];
	sprintf(upPath, "%s/.Update", projName);
	FILE *upF = fopen(upPath, "w");
	//Check if there is anything in 
	//Write results to file
	i=0;
	int empty = 1; //1 means update file is empty
	while(i<k) {
		printf("UMAD: %s\n", UMAD[i]);
		if (strcmp(UMAD[i],"U")==0) {
			i++;
			continue;
		}
		empty = 0;
		fprintf(upF, "%s\t%s\t%s\t%s\n", UMAD[i], updateVersion[i], updatePath[i], updateHash[i]);
		printf("%s\t%s\t%s\t%s\n", UMAD[i], updateVersion[i], updatePath[i], updateHash[i]);
		i++;
	}

	if(empty) {
		printf("Up to date\n");
	}
	
	fclose(upF);
	remove(path); //Remove server .Manifest
}

void upgradeClient(char* projName) {
	char path[500];
	sprintf(path, "%s/.Update",projName);
	//printf("projPath: %s\n",path);
	FILE *fd = fopen(path, "r");
	if (fd == NULL) {
		printf("Fail. .Update file does not exist.");
	}

	//From .Update, store the commands and file paths in arrays
	char *commands[100];
	char *filePaths[500];
	char line[500];
	char* token;
	int count=0;
	while (fgets(line, 500, fd)) { 
		if (line[0] == ' ' || line[0] == '\t' || line[0] == '\n') {
			if(count==0) {
				printf("Fail. .Update is empty.\n");
				return;
			}
			continue;
		}
		token = strtok(line, "\t\n");
		commands[count] = malloc(strlen(token)+1);
		strcpy(commands[count], token);
		//printf("%s\t", commands[count]);

		token = strtok(NULL, "\t\n");//Skip version number
		token = strtok(NULL, "\t\n");//This is the file path
		filePaths[count] = malloc(strlen(token)+1);
		strcpy(filePaths[count], token);
		//printf("%s\n", filePaths[count]);
		count++;
	}

	int highest;
	char highestStr[100];
	read(sock, highestStr, 100);
	highest = atoi(highestStr);
	//printf("highest: %d\n",highest);

	char projPath[500];
	sprintf(projPath, "repo/%s/%d/",projName, highest);
	//printf("projPath: %s\n", projPath);

	//Now go through the array of command/file and carry out commands
	int i=0;
	while(i<count) {
		if(strcmp(commands[i],"D")== 0|| strcmp(commands[i],"M")==0 || strcmp(commands[i],"A")==0) {
			//Rewrite Manifest
			char maniPath[500];
			sprintf(maniPath, "%s/.Manifest",projName);
			//printf("maniPath: %s\n", maniPath);
			FILE *fd = fopen(maniPath, "r");
			char tempPath[500];
			sprintf(tempPath, "%s/tempMani",projName);
			FILE *temp = fopen(tempPath, "w");

			char line[500];
			char comparePath[500];
			fgets(line,500,fd); //Get version number
			fprintf(temp, line, sizeof(line));
			//printf("D: %s\n", filePaths[i]);
			//Remove file
			remove(filePaths[i]);
			while(fgets(line,500,fd)) {
				sscanf(line,"%*d\t%s",comparePath);
				//printf("comparePath: %s\n", comparePath);
				if(strcmp(comparePath,filePaths[i])==0) {
					//printf("SKIPPED!\n");
					continue;
				}
				else {
					fprintf(temp, line, sizeof(line));
				}
			}
			
			if(strcmp(commands[i],"M")==0 || strcmp(commands[i],"A")==0) {
				//printf("MA: %s\n", filePaths[i]);
				char updPath[500];
				sprintf(updPath, "%s/.Update",projName);
				FILE *up = fopen(updPath, "r");

				fgets(line,500,fd); //Get version number
				fprintf(temp, line, sizeof(line));

				while(fgets(line,500,up)) {
					int versionNum;
					char comparePath[500];
					char hash[500];
					sscanf(line, "%*c\t%d\t%s\t%s", versionNum, comparePath, hash);
					if (strcmp(filePaths[i],comparePath) == 0) {
						char toPrint[500];
						sprintf(toPrint, "%d\t%s\t%s", versionNum, comparePath, hash);
						fprintf(temp, toPrint, sizeof(toPrint));
					}
				}
				fclose(up);
			}
			rename(tempPath, maniPath);
			
			fclose(temp);
			fclose(fd);
		}
		i++;
	}

	char countStr[100];
	sprintf(countStr, "%d", count);
	write(sock, countStr, 100);
	i=0;
	char tempToken[500];
	while(i<count) {
		if (commands[i][0] == 'M' || commands[i][0] == 'A') {
			strcpy(tempToken,filePaths[i]);
			token = strtok(tempToken, "/");
			token = strtok(NULL, "/");
			printf("token: %s\n", token);
			strcat(projPath, token);
			printf("projPath: %s\n", projPath);
			write(sock, projPath, sizeof(projPath)); //Send path

			//Receive file from server
			char buffer[1000];
			int fileSize = 0, len = 0;
			read(sock, buffer, sizeof(buffer)); //Get size of file
			fileSize = atoi(buffer);
			int fd = open(filePaths[i], O_RDONLY);
			int remain_bytes = fileSize;
			while ((remain_bytes > 0) && ((len = read(sock, buffer, fileSize)) > 0)) 
			{
				write(fd, buffer, len);
				remain_bytes -= len;
			}
			close(fd);
		}
		i++;
	}
	
	return;

}

void addFile(char* projName, char* filePath) {
	char maniPath[256];
	sprintf(maniPath, "%s/.Manifest", projName);
	
	//Check the current .Manifest file to see if already exists
	FILE *fd = fopen(maniPath, "r+");
	char line[200];
	char *token;
	int version;

	unsigned char hash[100];
	strcpy(hash, hashFile(filePath));
	//printf("hash: %s\n",hash);

	fgets(line,500,fd); 
	version = atoi(line); //Store the version number 
	int lineNum = 1;

	while (fgets(line, 500, fd)) { //Scan the file for matches in filePath
		//Get hashcode of file
		if (line[0] == ' ' || line[0] == '\t' || line[0] == '\n') {
			continue;
		}
		token = strtok(line, "\t\n");
		token = strtok(NULL, "\t\n");

		if (strcmp(filePath, token) == 0) {//Exists		
			token = strtok(NULL, "\t\n");
			if (strcmp(token, hash)==0) {
				printf("File not added. Already exists in repo\n");
				return;
			}
			else { //Update the hash code by creating a new file and adding it to the bottom
				printf("File already exists, but hash code has been updated\n");

				//-----------Case for if addFile is being called from Commit------------
				if (commitBool) {
					char commitPath[500];
					sprintf(commitPath, "%s/.Commit", projName);
					int newVersion = version+1;
					FILE *comF = fopen(commitPath, "a");
					fseek(comF, 0, SEEK_END);
					fprintf(comF, "%d\t%s\t%s\n", newVersion, filePath, hash);
					fclose(comF);
				}


				//printf("Token %s\nHash %s\n", token, hash);
				char *tempPath = malloc(strlen(maniPath)+4);
				sprintf(tempPath, "%sTemp", maniPath);
				FILE *tempFile = fopen(tempPath, "w");
				int x = 0;
				fseek(fd, 0, SEEK_SET);
				while (fgets(line, 200, fd) != NULL) {
					if (line[0] == ' ' || line[0] == '\t' || line[0] == '\n') {
						continue;
					}
					if (x == lineNum) {
						x=100000;
						
						continue;
					}
					else {
						fprintf(tempFile, "%s", line);
						x++;
					}
				}
				//fseek(tempFile, 0, SEEK_END);
				//printf("hash: %s\n",hash);
				fprintf(tempFile, "%d\t%s\t%s\n", version, filePath, hash);
				
				fclose(fd);
				fclose(tempFile);
				rename(tempPath,maniPath);
				return;
			}
		}//end if
		lineNum++;
	}

	//If not found in loop simply write to end of file
	fseek(fd, 0, SEEK_END);
	fprintf(fd, "%d\t%s\t%s\n", version, filePath, hash); //Write to end of .Manifest
	fclose(fd);
	return;
}

void removeFile(char* projName, char* filePath) {
	char maniPath[256];
	sprintf(maniPath, "%s/.Manifest", projName);

	FILE *fd = fopen(maniPath, "r");
	char line[200];
	char *token;
	fgets(line,200,fd); //Skip the first line

	int lineNum = 1;
	while (fgets(line, 200, fd)) {
		token = strtok(line, "\t\n");
		token = strtok(NULL, "\t\n");

		if (strcmp(filePath, token) == 0) {//Exists		
			//Create a temporary new file to write all previous info except for deleted line	
			char *tempPath = malloc(strlen(maniPath)+4);
			sprintf(tempPath, "%sTemp", maniPath);
			FILE *tempFile = fopen(tempPath, "w");
			int x = 0;
			fseek(fd, 0, SEEK_SET);
			while (fgets(line, 200, fd)) {
				if (x != lineNum) {
					fprintf(tempFile, "%s", line);
					//printf("LINE ADDED: %s", line);
					x++;
				}
				else {
					x=100000;
				}
			}
			fclose(fd);
			fclose(tempFile);
			rename(tempPath,maniPath);
			return;
		}//end if
		lineNum++;
	}

	//If while doesn't match, that means it doesn't exist in .Manifest
	printf("The file specified does not exist in .Manifest\n");
	fclose(fd);
	return;
}

void createClient(char* projName) {
// THIS FUNCTION READS EITHER A 0 OR 1. IF 0, IT IS SUCCESS.
//IF 0, .MANIFEST JUST CONTAINS A 0 AND A NEW LINE TO REPRESENT VERSION 0
//AND NO FILES YET
	char message[256];
	read(sock, message, sizeof(message));
	printf("Message read: %s\n", message);
	if (atoi(message) == 1) {
		printf("Project already exists\n");
		return;
	}

	//Create project folder on client-side
	char cwd[256];
	getcwd(cwd, sizeof(cwd));
	struct stat sb;

	//Receive .Manifest from server
	char buffer[1000];
	int fileSize = 0, len = 0;
	read(sock, buffer, sizeof(buffer)); //Get size of file
	fileSize = atoi(buffer);

	mkdir(projName, S_IRWXU);
	sprintf(cwd, "%s/.Manifest", projName);
	int fd = open(cwd, O_CREAT | O_RDWR | O_TRUNC, S_IWUSR | S_IRUSR);
	int remain_bytes = fileSize;
	while ((remain_bytes > 0) && ((len = read(sock, buffer, fileSize)) > 0)) 
	{
		write(fd, buffer, len);
		remain_bytes -= len;
	}
	close(fd);
	//printf("Project created on client-side\n");
	return;
}

void destroyClient(char* projName) {
//THIS FUNCTION READS EITHER A 0 OR 1. IF 0, IT IS SUCCESS AND VICE VERSA
	char message[256];
	read(sock, message, sizeof(message));
	printf("Message read: %s\n",message);
}

void checkoutClient(char* projName) {
	char countStr[100];
	int count;
	int i;
	mkdir(projName, S_IRWXU);
	char path[500];
	read(sock, countStr, sizeof(countStr));
	count = atoi(countStr);
	//printf("COUNT: %d\n", count);

	for (i=0; i<count; i++) {
		char fileName[200];
		read(sock, fileName, 200);
		//printf("received path: %s\n",fileName);
		//Tokenize to get actual file name
		char *token = strtok(fileName, "/");
		token = strtok(NULL, "/");
		token = strtok(NULL, "/");
		token = strtok(NULL, "/");
		char *newFilePath = malloc(strlen(projName)+strlen(token));
		sprintf(newFilePath, "%s/%s",projName,token);
		//printf("newFilePath: %s\n", newFilePath);

		//Receive file from server
		char buffer[1000];
		int fileSize = 0, len = 0;
		read(sock, buffer, sizeof(buffer)); //Get size of file
		fileSize = atoi(buffer);
		int fd = open(newFilePath, O_CREAT | O_RDWR | O_TRUNC, S_IWUSR | S_IRUSR);
		int remain_bytes = fileSize;
		while ((remain_bytes > 0) && ((len = read(sock, buffer, fileSize)) > 0)) 
		{
			write(fd, buffer, len);
			remain_bytes -= len;
		}
		close(fd);
	}

	//Receive .Manifest
	sprintf(path, "%s/.Manifest", projName);
	int fd = open(path, O_CREAT | O_RDWR | O_TRUNC, S_IWUSR | S_IRUSR);
	char buffer[1000];
	int fileSize = 0, len = 0;
	read(sock, buffer, sizeof(buffer)); //Get size of file
	fileSize = atoi(buffer);
	int remain_bytes = fileSize;
	while ((remain_bytes > 0) && ((len = read(sock, buffer, fileSize)) > 0)) 
	{
		write(fd, buffer, len);
		remain_bytes -= len;
	}
}

int main(int argc, char* argv[]) {
	//-------Check if configure----------
	if (strcmp(argv[1], "configure") == 0) {
		if (argc != 4) {
			printf("Wrong number of arguments.\n");
			return 0;
		}
		strncpy(ipHost, argv[2], sizeof(ipHost)-1);
		port = atoi(argv[3]);
		printf("Successfully configured.\nHost: %s\tPort: %d\n",ipHost, port);
		configure(argv[2], argv[3]);
		return 0;
	}
	//-------If not configure, check if configured already. If yes, get ip & port from file-------
	else {
		char line[100];
		FILE* fp = fopen(".configure", "r");
		if (fp == NULL) {
			printf("Configure file does not exist\n");
			return 0;
		}
		fgets(line, sizeof(line), fp);
		strcpy(ipHost,line);
		strtok(ipHost, "\n");
		fgets(line, sizeof(line), fp);
		port = atoi(line);
		printf("IPHOST: %s\nPORT: %d\n", ipHost, port);
		int valread;

		//--------------Connect to Server---------------
		struct sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET; 
	    	serv_addr.sin_port = htons(port); 

		if((sock = socket(AF_INET, SOCK_STREAM, 0)) <= 0) {
			printf("\nSocket creation error \n"); 
			return 0;
		}

	    /*if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) { 
			printf("\nInvalid address/ Address not supported \n"); 
	    } */

		if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { 
	        printf("\nConnection Failed \n");  
	        return -1;
	    } 
	    else {
	    	printf("Successfully connected to server.\n");
	    }
	}

	//--------Check for add, remove, or rollback which all take 3 parameters--------
	if (strcmp(argv[1], "add") == 0 || strcmp(argv[1], "remove") == 0 || strcmp(argv[1], "rollback") == 0) {
		//printf("ARGV[1]: %s\n",argv[1]);
		if (argc != 4) {
			printf("Error. Too many or too few arguments.\n");
			return 0;
		}
		else {
			send(sock, argv[1], 20, 0);
			send(sock, argv[2], 50, 0);
			printf("Command sent: %s %s sent\n", argv[1], argv[2]);
		}

		//Check if file exists
		if(!file_exist(argv[2])) {
			printf("Error. Files does not exist.\n");
			return 0;
		}

		//Check command to send to functions
		if (strcmp(argv[1], "add") == 0) {
			addFile(argv[2], argv[3]);
		}
		else if (strcmp(argv[1], "remove") == 0) {
			removeFile(argv[2], argv[3]);
		}
	}

	//--------Check for all other commands which take 2 parameters--------
	else {
		if(argc != 3) {
			printf("Error. Too many or too few arguments.\n");
			return 0;
		}
		else {
			//------Send user's command and project name to server-------
			if (strcmp(argv[1], "create") == 0) {
				send(sock, argv[1], 20, 0);
				send(sock, argv[2], 50, 0);
				printf("Command sent: %s %s sent\n", argv[1], argv[2]);
				createClient(argv[2]);
			}
			else if (strcmp(argv[1], "destroy") == 0) {
				send(sock, argv[1], 20, 0);
				send(sock, argv[2], 50, 0);
				printf("Command sent: %s %s sent\n", argv[1], argv[2]);
				destroyClient(argv[2]);
			}
			else if (strcmp(argv[1], "checkout") == 0) {
				struct stat sb;
			    if (stat(argv[2], &sb) == 0 && S_ISDIR(sb.st_mode))
			    {
			        printf("Local project already exists. Returning.\n");
			        send(sock, "FAIL", 20, 0);
					send(sock, "FAIL", 50, 0);
					return 0;
			    }
				send(sock, argv[1], 20, 0);
				send(sock, argv[2], 50, 0);
				printf("Command sent: %s %s sent\n", argv[1], argv[2]);
				checkoutClient(argv[2]);
			}
			else if (strcmp(argv[1], "currentversion") == 0) {
				send(sock, argv[1], 20, 0);
				send(sock, argv[2], 50, 0);
				currentversionClient(argv[2]);
			}
			else if (strcmp(argv[1], "rollback") == 0) {
				send(sock, argv[1], 20, 0);
				send(sock, argv[2], 50, 0);
			}
			else if (strcmp(argv[1], "update") == 0) {
				struct stat sb;
			    if (stat(argv[2], &sb) != 0 && S_ISDIR(sb.st_mode))
			    {
			        printf("Local project does not exist. Returning.\n");
			        send(sock, "FAIL", 20, 0);
					send(sock, "FAIL", 50, 0);
					return 0;
			    }
				send(sock, argv[1], 20, 0);
				send(sock, argv[2], 50, 0);
				updateClient(argv[2]);
			}
			else if (strcmp(argv[1], "upgrade") == 0) {
				char path[500];
				sprintf(path, "%s/.Update",argv[2]);
				if(!file_exist(path)) {
					printf("Fail. .Update file does not exist.\n");
					return 0;
				}
				send(sock, argv[1], 20, 0);
				send(sock, argv[2], 50, 0);
				upgradeClient(argv[2]);
			}
			else if (strcmp(argv[1], "commit") == 0) {
				struct stat sb;
			    if (stat(argv[2], &sb) != 0 && S_ISDIR(sb.st_mode))
			    {
			        printf("Local project does not exist. Returning.\n");
			        send(sock, "FAIL", 20, 0);
					send(sock, "FAIL", 50, 0);
					return 0;
			    }
				send(sock, argv[1], 20, 0);
				send(sock, argv[2], 50, 0);
				commitClient(argv[2]);
			}


		}
	}

}
