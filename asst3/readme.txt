Jim Tang Section 01
Osun Kwon Section 07
Asst3: WTF

Overview: The primary tools we used for this project were sockets, multithreads, file-manipulation, and 
encryption/decryption with SHA256. Once the user makes the project they should first run configure which
is a simple function that creates and writes a text file with fopen with hostname and port.
Once the server is running, a repo folder is created to store projects if it doesn't exist already.
For every command, we create a new thread and lock it with mutex until the function is done running.

./WTF checkout:
We first check with stat() whether the project exists in the repo directory. If it does we use readdir 
and sendfile() with sockets to recursively send each file to the local directory.

./WTF update: 
We first use sendfile() to get the .Manifest from the server. Now we break down both the local and server's
.Manifest into string arrays of versions[][], paths[][], and hashcodes[][]. We use these arrays to find
the differences and place the differing entries into an update[][] array along with a UMAD[] array. 
Using these arrays, we write into a new file called .Update.

./WTF upgrade:
If .Update exists, read the file's commands and corresponding files into arrays. We then modify the .Manifests
by writing a tempManifest. Whether an existing entry is written to this tempManifest is determined by
the commands. Then we rename tempManifest to .Manifest to overwrite the previous. Now we use our usual
file transfer system based on the commands to receive files from server if 'M' or 'A'.

./WTF create:
Simple file/directory creation. Version will always be 0 on server side and the client just has a folder with
a .Manifest with version number 0 in it.

./WTF destroy:
Check if a project exists on server. If it does we delete it with rmdir.

./WTF add/remove: We recurse through the client side repository to see if the file exists. If it does, then
we open it on server side and send it over with sendfile(). Remove will simply delete the file. Finally,
we add an entry to the .Manifest on the client side by opening it with the path projName/.Manifest.

./WTF currentversion:

./WTF rollback:
