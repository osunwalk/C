#include "define.h"
int exist = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
struct arg_struct
{
	int version;
	int socket;
	char* name;
};
void substr(char* str, char* sub, int start, int len)
{
        memcpy(sub, &str[start], len);
        sub[len] = '\0';
}

void *currentVersion(void*ptr)
{
    pthread_mutex_lock(&lock);
    struct arg_struct *args = (struct arg_struct*) ptr;
    char * string = malloc(strlen(args->name)+1);
    string = args->name;
    int socket = args->socket;

    //Find current version
    char path[500];
    sprintf(path, "repo/%s", string);
    //printf("Path: %s\n", path);

	DIR *dir = opendir(path);
	if(dir == NULL) {
		printf("Project does not exist on this server.\n");
		return;
	}

	struct dirent *explorer;
	int highest = 0;
	int ver;

	while((explorer = readdir(dir)) != NULL) {
		if('.' == explorer->d_name[0]){
			continue;	
		}

		ver = atoi(explorer->d_name);
		if(highest < ver) { //If version number is higher than current highest, set equal
			highest = ver;
		}
	}
	closedir(dir);
	sprintf(path, "%s/%d", path, highest);
	
	//Scan manifest file
	strcat(path, "/.Manifest");
	FILE *fp = fopen (path , "rb" );
	if(fp == NULL) {
		printf("Error. No manifest found.\n");
		return;
	}
	printf("Path: %s\n", path);
	char line[500];
	fgets(line, 500, fp); //Skip mani version line
	int count=0;
	char *version[500];
	char *filePath[500];
	while(fgets(line, 500, fp) != NULL)
	{
		if (line[0] == ' ' || line[0] == '\t' || line[0] == '\n') {
			continue;
		}
		version[count] = malloc(200);
		filePath[count] = malloc(200);
		sscanf(line, "%s\t%s\t%*s", version[count], filePath[count]);
		count++;
	}

	//Write how many writes there will be
	char countStr[500];
	sprintf(countStr, "%d", count);
	write(socket,countStr,500);

	int i;
	char msg[500];
	for(i=0;i<count;i++) {
		sprintf(msg, "%s\t%s\n", version[i], filePath[i]);
		write(socket,msg,500);
	}

    pthread_mutex_unlock(&lock);
    return NULL;
}

void *commit(void*ptr)
{
    pthread_mutex_lock(&lock);
	//First check if project exists in repo
	char path[500];
	char path2[500];
	struct arg_struct *args = (struct arg_struct*) ptr;
	int socket = args->socket;
	char *string = malloc(strlen(args->name)+1);
	strcpy(string, args->name);
	sprintf(path, "repo/%s", string);
	strcpy(path2, path);
	printf("path %s\n", path);

	struct stat sb;
	if (stat(path, &sb) == -1) {
		printf("Fail. Project doesn't exist on this server\n");
		return;
	}

	//Find current version
	DIR *dir = opendir(path);
	if(dir == NULL) {
		printf("Project does not exist on this server.\n");
		return;
	}

	struct dirent *explorer;
	int highest = 0;
	int ver;

	while((explorer = readdir(dir)) != NULL) {
		if('.' == explorer->d_name[0]){
			continue;	
		}

		ver = atoi(explorer->d_name);
		if(highest < ver) { //If version number is higher than current highest, set equal
			highest = ver;
		}
	}
	sprintf(path, "%s/%d/", path, highest);

	//Confirmed it exists. Now send client manifest
	char maniPath[500];
	sprintf(maniPath, "%s.Manifest", path);
	int mani = open(maniPath, O_RDONLY);
	char fileSize[1000];
	struct stat file_stat;
	fstat(mani, &file_stat);
	int sent_bytes, remain_bytes;
	sprintf(fileSize, "%d", file_stat.st_size);
	int len = write(socket, fileSize, 100); //Send size of file
	if (len<0) {
		printf("Error sending file to client\n");
		return;
	}	

	remain_bytes = file_stat.st_size;
	while(((sent_bytes = sendfile(socket, mani, NULL, 500)) > 0) && (remain_bytes > 0))
	{
        remain_bytes -= sent_bytes;
	}
	printf("%s sent to client\n", maniPath);
	close(mani);

	//Receive .Commit from client
	char commPath[500];
	sprintf(commPath, "%s.Commit", path);

	char buffer[1000];
	int comSize = 0;
	read(socket, buffer, sizeof(buffer)); //Get size of file
	comSize = atoi(buffer);
	int fd = open(commPath, O_CREAT | O_RDWR | O_TRUNC, S_IWUSR | S_IRUSR);
	remain_bytes = comSize;
	while ((remain_bytes > 0) && ((len = read(socket, buffer, comSize)) > 0)) 
	{
		write(fd, buffer, len);
		remain_bytes -= len;
	}
	close(fd);

    pthread_mutex_unlock(&lock);
    return NULL;
}

void* rollback(void*ptr)
{
    pthread_mutex_lock(&lock);
    struct arg_struct *args = (struct arg_struct*) ptr;
    char * string = malloc(strlen(args->name)+1);
    int socket = args->socket;
	int version = args->version;
    char msg[256];
    char cwd[256];
    char path[256];
	char num[256];
    char *paths;
    char *token;
    int j = -3;
    getcwd(cwd, sizeof(cwd));
    strcat(cwd, "/repo");
    strcpy(string, args->name);

    DIR *dir;
    struct dirent *ent;
    if((dir= opendir(cwd))!=NULL)
    {
            while((ent= readdir(dir))!=NULL)
            {
                    if(strcmp(string, ent->d_name)==0)
                    {
                            exist =1;
                            break;
                    }
            }
            closedir(dir);
    }
    strcat(cwd, "/");
    strcat(cwd, string);
    if((dir = opendir(cwd))!=NULL)
    {
            while((ent = readdir(dir))!=NULL)
                    j++;
	if( j > version)
	{
		strcpy(path, cwd);
		strcat(path, "/");
		snprintf(num, sizeof num, "%d", j);
		strcat(path, num);
		paths = path;
		remove_directory(paths);
	}
    }
    pthread_mutex_unlock(&lock);
    return NULL;
}

void *update(void *ptr)
{
	pthread_mutex_lock(&lock);
	//First check if project exists in repo
	char path[500];
	struct arg_struct *args = (struct arg_struct*) ptr;
	int socket = args->socket;
	char *string = malloc(strlen(args->name)+1);
	strcpy(string, args->name);
	sprintf(path, "repo/%s", string);
	printf("path %s\n", path);

	struct stat sb;
	if (stat(path, &sb) == -1) {
		printf("Fail. Project doesn't exist on this server\n");
		return;
	}

	//Find current version
	DIR *dir = opendir(path);
	if(dir == NULL) {
		printf("Project does not exist on this server.\n");
		return;
	}

	struct dirent *explorer;
	int highest = 0;
	int ver;

	while((explorer = readdir(dir)) != NULL) {
		if('.' == explorer->d_name[0]){
			continue;	
		}

		ver = atoi(explorer->d_name);
		if(highest < ver) { //If version number is higher than current highest, set equal
			highest = ver;
		}
	}
	sprintf(path, "%s/%d/", path, highest);

	//Confirmed it exists. Now send client manifest
	char maniPath[500];
	sprintf(maniPath, "%s/.Manifest", path);
	int mani = open(maniPath, O_RDONLY);
	char fileSize[1000];
	struct stat file_stat;
	fstat(mani, &file_stat);
	int sent_bytes, remain_bytes;
	sprintf(fileSize, "%d", file_stat.st_size);
	int len = write(socket, fileSize, sizeof(fileSize)); //Send size of file
	if (len<0) {
		printf("Error sending file to client\n");
		return;
	}	

	remain_bytes = file_stat.st_size;
	while(((sent_bytes = sendfile(socket, mani, NULL, 500)) > 0) && (remain_bytes > 0))
	{
        remain_bytes -= sent_bytes;
	}
	printf("%s sent to client\n", maniPath);
	close(mani);
	pthread_mutex_unlock(&lock);
	return;
}

void *upgrade(void*ptr) {
	pthread_mutex_lock(&lock);
	struct arg_struct *args = (struct arg_struct*) ptr;
	int socket = args->socket;
	char *projName = malloc(strlen(args->name)+1);
	strcpy(projName, args->name);

	//Find most recent version
	char projPath[500];
	sprintf(projPath, "repo/%s",projName);
	
	DIR *dir = opendir(projPath);
	if(dir == NULL) {
		printf("Project does not exist on this server.\n");
		return;
	}
	struct dirent *explorer;
	int highest = 0;
	int ver;

	while((explorer = readdir(dir)) != NULL) {
		if('.' == explorer->d_name[0]){
			continue;	
		}

		ver = atoi(explorer->d_name);
		if(highest < ver) { //If version number is higher than current highest, set equal
			highest = ver;
		}
	}
	char highestStr[100];
	sprintf(highestStr, "%d", highest);
	write(socket, highestStr, 100);

	//Listen to how many writes will come
	char buffer[1000];
	read(socket,buffer,100);
	int count = atoi(buffer); //Get the count
	//printf("COUNT: %d\n", count);
	int i;
	char filePath[500];

	for(i = 0; i<count; i++) {
		read(socket, filePath, sizeof(filePath)); //Gets the file path 
		//printf("filePath: %s\n",filePath);

		//Send the current file
		int currFile = open(filePath, O_RDONLY);
		char fileSize[1000];
		struct stat file_stat;
		fstat(currFile, &file_stat);
		int sent_bytes, remain_bytes;
		sprintf(fileSize, "%d", file_stat.st_size);
		int len = write(socket, fileSize, sizeof(fileSize)); //Send size of file
		if (len<0) {
			printf("Error sending file to client\n");
			return;
		}	

		remain_bytes = file_stat.st_size;
		while(((sent_bytes = sendfile(socket, currFile, NULL, 500)) > 0) && (remain_bytes > 0))
		{
            remain_bytes -= sent_bytes;
		}
		//printf("%s sent to client\n", filePath);
		close(currFile);
	}
	pthread_mutex_unlock(&lock);
}

void *checkout(void *ptr)
{
	pthread_mutex_lock(&lock);
	struct arg_struct *args = (struct arg_struct*) ptr;
	int socket = args->socket;
	char * string = malloc(strlen(args->name)+1);
	strcpy(string, args->name);
	char path[1000];
	sprintf(path, "repo/%s", string);

	DIR *dir = opendir(path);
	if(dir == NULL) {
		printf("Project does not exist on this server.\n");
		return;
	}

	struct dirent *explorer;
	int highest = 0;
	int ver;

	while((explorer = readdir(dir)) != NULL) {
		if('.' == explorer->d_name[0]){
			continue;	
		}

		ver = atoi(explorer->d_name);
		if(highest < ver) { //If version number is higher than current highest, set equal
			highest = ver;
		}
	}

	sprintf(path, "%s/%d/", path, highest);
	//printf("%s\n", path);

	//Now that we have the folder to get files from, send all files over
	char maniPath[1000];
	sprintf(maniPath, "%s.Manifest",path); //Get manifest file path

	FILE *fd = fopen(maniPath, "r");
	if (fd == NULL) {
		printf("Error opening .Manifest\n");
		return;
	}

	//First check how many files there are to send
	int count = 0;
	char countStr[100];
	char file[500];
	char line[1000];
	fgets(line,500,fd); //Skip first line
	while(fgets(line,500,fd)) {
		count++;
	}
	
	sprintf(countStr, "%d", count);
	write(socket, countStr, sizeof(countStr)); //Send number of files to be sent
	//printf("COUNT: %d\n", count);

	fseek(fd, 0, SEEK_SET);
	fgets(line,500,fd);
	while(fgets(line,500,fd)) {
		sscanf(line, "%*d\t%s", file);
		char *filePath = malloc(strlen(file)+5);
		sprintf(filePath, "repo/%s", file);
		
		printf("Sending %s\n", filePath);
		write(socket, filePath, 200); //Send filePath so client can create file with name
		
		//Send the current file
		int currFile = open(filePath, O_RDONLY);
		char fileSize[1000];
		struct stat file_stat;
		fstat(currFile, &file_stat);
		int sent_bytes, remain_bytes;
		sprintf(fileSize, "%d", file_stat.st_size);
		int len = write(socket, fileSize, sizeof(fileSize)); //Send size of file
		if (len<0) {
			printf("Error sending file to client\n");
			return;
		}	

		remain_bytes = file_stat.st_size;
		while(((sent_bytes = sendfile(socket, currFile, NULL, 500)) > 0) && (remain_bytes > 0))
		{
            remain_bytes -= sent_bytes;
		}
		//printf("%s sent to client\n", filePath);
		close(currFile);
	}
	fclose(fd);

	//Now send manifest
	int mani = open(maniPath, O_RDONLY);
	char fileSize[1000];
	struct stat file_stat;
	fstat(mani, &file_stat);
	int sent_bytes, remain_bytes;
	sprintf(fileSize, "%d", file_stat.st_size);
	int len = write(socket, fileSize, sizeof(fileSize)); //Send size of file
	if (len<0) {
		printf("Error sending file to client\n");
		return;
	}	

	remain_bytes = file_stat.st_size;
	while(((sent_bytes = sendfile(socket, mani, NULL, 500)) > 0) && (remain_bytes > 0))
	{
        remain_bytes -= sent_bytes;
	}
	printf("%s sent to client\n", maniPath);
	close(mani);
	
	pthread_mutex_unlock(&lock);
}

int remove_directory(const char *path)
{
   DIR *d = opendir(path);
   size_t path_len = strlen(path);
   int r = -1;

   if (d)
   {
      struct dirent *p;

      r = 0;

      while (!r && (p=readdir(d)))
      {
          int r2 = -1;
          char *buf;
          size_t len;

          /* Skip the names "." and ".." as we don't want to recurse on them. */
          if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
          {
             continue;
          }

          len = path_len + strlen(p->d_name) + 2; 
          buf = malloc(len);

          if (buf)
          {
             struct stat statbuf;

             snprintf(buf, len, "%s/%s", path, p->d_name);

             if (!stat(buf, &statbuf))
             {
                if (S_ISDIR(statbuf.st_mode))
                {
                   r2 = remove_directory(buf);
                }
                else
                {
                   r2 = unlink(buf);
                }
             }

             free(buf);
          }

          r = r2;
      }

      closedir(d);
   }

   if (!r)
   {
      r = rmdir(path);
   }

   return r;
}

void *destroy(void*ptr)
{
        pthread_mutex_lock(&lock);
        struct arg_struct *args = (struct arg_struct*) ptr;
        char * string = malloc(strlen(args->name)+1);
        int socket = args->socket;
        char msg[256];
        char cwd[256];
	char *path;
        getcwd(cwd, sizeof(cwd));
        strcat(cwd, "/repo");
        strcpy(string, args->name);
	int number;

        DIR *dir;
        struct dirent *ent;
        if((dir= opendir(cwd))!=NULL)
        {
                while((ent= readdir(dir))!=NULL)
                {
                        if(strcmp(string, ent->d_name)==0)
                        {
                                exist =1;
                                break;
                        }
                }
                closedir(dir);
        }

	if(exist==1)
	{
		strcat(cwd, "/");
		strcat(cwd, string);
		path = cwd;
		remove_directory(path);
		strcpy(msg, "0");
		write(socket,msg,sizeof(msg));
	}
	else
	{
		printf("project doesn't exist\n");
		strcpy(msg, "1");
		write(socket, msg, sizeof(msg));
	}
	exist = 0;
        pthread_mutex_unlock(&lock);
        return NULL;
}

void *create(void *ptr)
{
	pthread_mutex_lock(&lock);
	struct arg_struct *args = (struct arg_struct*) ptr;
	char * string = malloc(strlen(args->name)+1);
	int socket = args->socket;
	char msg[256];
	char cwd[256];
	getcwd(cwd, sizeof(cwd));
	strcat(cwd, "/repo");
	strcpy(string, args->name);

	DIR *dir;
	struct dirent *ent;
	if((dir= opendir(cwd))!=NULL)
	{
		while((ent= readdir(dir))!=NULL)
		{
			if(strcmp(string, ent->d_name)==0)
			{
				exist =1;
				break;
			}
		}
		if(exist==0)
		{
			printf("Creating New Project + Manifest...\n");
			strcat(cwd, "/");
			strcat(cwd, string);
			int result = mkdir(cwd, 0777);
			strcat(cwd, "/0");
			result = mkdir(cwd, 0777);
            strcat(cwd, "/");
            strcat(cwd, ".Manifest");
			int fd = open(cwd, O_WRONLY | O_APPEND|O_CREAT,0644);
			write(fd, "0\n", 2);
			close(fd);
			strcat(msg, "0"); //0 means successfully created project
			write(socket,msg,sizeof(msg));

			//Send .Manifest file
			fd = open(cwd, O_RDONLY);
			char fileSize[1000];
			struct stat file_stat;
			fstat(fd, &file_stat);
			int sent_bytes, remain_bytes;
			sprintf(fileSize, "%d", file_stat.st_size);

			int len = write(socket, fileSize, sizeof(fileSize));
			if (len<0) {
				printf("Error sending .manifest file to client\n");
				return;
			}	

			int fileS = atoi(fileSize);
			remain_bytes = file_stat.st_size;
			while(((sent_bytes = sendfile(socket, fd, NULL, 500)) > 0) && (remain_bytes > 0))
			{
				remain_bytes -= sent_bytes;
			}
			printf(".Manifest sent to client\n");
			close(fd);
		}
		else
		{
			printf("Project already exists\n");
			strcat(msg, "1");
			write(socket,msg,sizeof(msg));
			exist = 0;
			pthread_mutex_unlock(&lock);
			return NULL;
		}
		closedir(dir);
	}
	pthread_mutex_unlock(&lock);
	return NULL;
}

int main(int argc, char* argv[])
{
	if(argc < 2)
	{
		fprintf(stderr,"ERROR, no port provided\n");
		exit(1);
	}
	int portno = atoi(argv[1]);
	if(portno <=1023) {
		printf("Incompatible port\n");
		return -1;
	}

	struct sockaddr_in addr;
	int stuff = socket(AF_INET,SOCK_STREAM,0);
	if(stuff <= 0)
	{
		error("ERROR opening socket");
	}
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(portno);

	if(bind(stuff, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		error("ERROR on binding");
		return 0;
	}

	if(listen(stuff, 50)<0){
		printf("ERROR on listen");
		return 0;
	}
	else {
		printf("Listening...\n");
	}

	//Check if repo exists. If no, create a repo folder
	struct stat sb;
	if (stat("repo", &sb) == -1) {
		mkdir("repo", S_IRWXU);
	}

	while(1)
	{	
		//-----------Accept connection from client--------------
		struct sockaddr_in clientAddr;
		memset(&clientAddr, 0, sizeof(clientAddr));
		socklen_t clientAddrSize = sizeof(clientAddr);
		int newsockfd = accept(stuff, (struct sockaddr*)&clientAddr, &clientAddrSize);
		if (newsockfd < 0) {
			printf("ERROR on accept\n");
		}
		else
		{
			printf("Accepted connection from client\n");

			//----------Read command from client---------------
			char cmd[20];
			char projName[50];
			recv(newsockfd, cmd, 20, 0);
			recv(newsockfd, projName, 50, 0);
			printf("Command received: %s\nProject Name received: %s\n", cmd, projName);
			if(strcmp(cmd, "create")==0)
			{
				struct arg_struct *args;
				args= malloc(sizeof(*args));
				args->name = malloc(strlen(projName)+1);
				strcpy(args->name, projName);
				args->socket = newsockfd;
				pthread_t temp;
				pthread_create(&temp, NULL, create, (void*)args);
			}
			else if(strcmp(cmd, "destroy")==0)
			{
				struct arg_struct *args;
				args = malloc(sizeof(*args));
				args->name = malloc(strlen(projName)+1);
				strcpy(args->name, projName);
				args->socket = newsockfd;
				pthread_t temp;
				pthread_create(&temp, NULL, destroy, (void*)args);
			}
			else if(strcmp(cmd, "checkout") == 0) 
			{
				struct arg_struct *args;
				args= malloc(sizeof(*args));
				args->name = malloc(strlen(projName)+1);
				strcpy(args->name, projName);
				args->socket = newsockfd;
				pthread_t temp;
				pthread_create(&temp, NULL, checkout, (void*)args);
			}
			else if(strcmp(cmd, "currentversion")==0)
			{
				struct arg_struct *args;
				args = malloc(sizeof(*args));
                args->name = malloc(strlen(projName)+1);
                strcpy(args->name, projName);
                args->socket = newsockfd;
                pthread_t temp;
                pthread_create(&temp, NULL, currentVersion, (void*)args);
			} 
			else if(strcmp(cmd, "rollback")==0)
			{
                char versions[20];
				recv(newsockfd, versions, 20, 0);
                struct arg_struct *args;
                args = malloc(sizeof(*args));
                args->name = malloc(strlen(projName)+1);
                strcpy(args->name, projName);
                args->socket = newsockfd;
				args->version = atoi(versions);
                pthread_t temp;
                pthread_create(&temp, NULL, rollback, (void*)args);
			}
			else if(strcmp(cmd, "update")==0)
			{
                struct arg_struct *args;
                args = malloc(sizeof(*args));
                args->name = malloc(strlen(projName)+1);
                strcpy(args->name, projName);
                args->socket = newsockfd;
                pthread_t temp;
                pthread_create(&temp, NULL, update, (void*)args);
			}
			else if(strcmp(cmd, "upgrade")==0)
			{
                struct arg_struct *args;
                args = malloc(sizeof(*args));
                args->name = malloc(strlen(projName)+1);
                strcpy(args->name, projName);
                args->socket = newsockfd;
                pthread_t temp;
                pthread_create(&temp, NULL, upgrade, (void*)args);
			}
			else if(strcmp(cmd, "commit") == 0){
				struct arg_struct *args;
                args = malloc(sizeof(*args));
                args->name = malloc(strlen(projName)+1);
                strcpy(args->name, projName);
                args->socket = newsockfd;
				pthread_t id;
				pthread_create(&id, NULL, commit, (void *) args);
			}
		}

	}
	return 0;
}
