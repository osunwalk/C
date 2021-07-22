Jim Tang (Section 01)
Osun Kwon (Section 02)
Systems Programming Assignment 2 - File Compressor

main()
The first process done in the code is to check the flags from user input. Then we get the paths from user input and store them. We store up to two paths based on if the user is building(1 path to file/directory) or compressing/decompressing(2 paths one to file/directory and one to the codebook). After this, the program will go through different processes based on the flags.

------FLAG CASES------
Case 0: If -h flag
Our custom flag to check for './', './HuffmanCodebook', or '/HuffmanCodebook'. If the user inputs any of those three strings in their paths, we use getcwd to get the current working directory and then concatenate it with the rest of their input.

Case 1: If -b flag 
First, assign '%n', '%t' and ' ' to the first 3 indexes of our global WordArray structure, 'words'. These are used to represent escape characters in the codebook. Go to listdir(). Check if -r flag was in input. If yes, go to Case 2. If not, then the input must be a file so we send the file's path to tokenize(). Afterwards, we go back to main and sort 'words' based on frequencies. Then we create an array of frequencies based on number of occurrences. This is passed to HuffmanCodes() and the program ends afterwards.

Case 2: If -b && -R flags
Use readdir to traverse through directories and find files. If the current file is a directory, we call listdir() again with an updated path that includes the directory's name at the end. This recursively goes down each directory. Every time a file is found while coming out of the recursion, use the same process as case 1 with tokenize(). 

Case 3: If -c flag
listdir() is called. Check if file is a .txt file. If so, call compress().


Case 4: If -c && -R flags
listdir() is called. Instead of only iterating once, we once again enter readdir just like Case 2. However, instead of calling tokenize, we call compress() with each file found.

Case 5: If -d flag
listdir() is called. Here, we check to see if the end of the file ends in .hcz. If it does, we send the path to the file to decompress()

Case 6: If -d && -R flags
listdir() is called. If the -R flag is true, then we enter readdir and call decompress() with each file found. 
------FUNCTIONS------
tokenize()
First we store the file's text in a string called buffer. Then we iterate through to check for escape characters like \n \t and spaces. Then we use strtok to tokenize the string. Every token is added to our WordArray structure in which we keep track of each token's frequency. Each token's frequency is updated as we traverse through the file's text.

HuffmanCodes()
First, call buildHuffmanTree(). This function uses our createAndBuildMinHeap() to build a minHeap and ultimately build a Huffman Tree. Afterwards, call printCodes(). printCodes recurses through the Huffman Tree we built. Each time a leaf is reached, printArr() is called in which we write the current token's bit sequence into the HuffmanCodebook. After printArr() is done writing the bit sequence, we write the token itself a tab away from its sequence. This is repeated from recusing the tree until all bit sequences and tokens are correctly written into HuffmanCodebook.

compress()
First, open HuffmanCodebook and read the file content into 'buffer'. Then, tokenize 'buffer' into 'token'. Then we create have two arrays 'strings' and 'strings2'. We store bit sequences in 'strings' and their corresponding tokens in 'strings2'. Next, create the .hcz file (overwrite if exists already) and read in the file again into 'buffer2'. Now iterate through 'buffer2', char by char. We iterate until we hit an escape character in which we print the corresponding bit sequence for that escape character followed by the bit sequence of the token that comes after it. This is repeated until the entire file is compressed into the new .hcz file.

decompress()
Just like compress() we store bit sequences in array 'bitSeqs' and their corresponding toks in 'toks'. Here, we also store the corresponding bit sequence's lengths in 'bitLengths'. Then we find the length of the longest bit sequence and store that in var 'longest'. We store the .hcz file's bit sequence in 'buffer2'. After we create a new file that is the name of the original text file + 'New' at the end. Now to actually decompress the .hcz's bit sequence, we substring the first n bits of the sequence with n='longest'. We compare this sequence against the longest bit sequences stored in 'toks'. If there is a match, we write that token into the new text file. Do this check against each token. If no matches, we decrement 'longest', which makes the substring 1 character shorter and check again against all tokens. This continues until the entire sequence has been decompressed.

-----RUNTIME-----
-b with no -R flag: Whether its build or compress the runtime should be very similar. The program will find the file at the given path. It then builds a Huffman Tree which is takes O(n) time because we pass it a sorted list. Writing to a file is also O(n) because we have to traverse the tree. So overall we have O(n) runtime.

-b with -R flag: The program now has to do the above process, but it has to do it x amount of times depending on how many .txt files are in the given directory. However, since x is just a constant, the time complexity is still be O(n).

-c with no -R flag: Follows the same path as -b in taking in user input. Afterwards, it goes to the compress() function which will iterate through the file twice and write to the .hcz file the second iteration through. This time will vary based on how large the file is so it is O(n).

-c with -R flag: Since it will just be the same -c but x amount of times based on the number of files, the time complexity should remain O(n).

-d with no -R flag: The most time intensive part of decompress is the for loop in which we compare substrings of the .hcz bit sequence to the bit sequences of our tokens. This comparison is pretty inefficient since it compares each token's bit sequence against the string every time so it is O(n^2) runtime.

-d with -R flag: This is the same as above except it will run x amount of times depending on number of .hcz files. Since x is a constant, the time complexity should stay the same at O(n^2).