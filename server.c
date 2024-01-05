/**
 * @authors Karan Mahajan - 110119774, Niharika Khurana - 110124290 (Section 1 Monday)
 * @email [mahaja81@uwindsor.ca,khuranan@uwindsor.ca]
 * @create date 2023-12-02 00:50:57
 * @modify date 2023-12-13 23:00:43
 * @desc [Client-server project, a client can request a file or a set of files from the server]
 */
#define _XOPEN_SOURCE 500

#include <arpa/inet.h>
#include <dirent.h>
#include <ftw.h>
#include <libgen.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <utime.h>
#include <unistd.h>

// Took reference from stackoverflow to find the creation date
#ifdef HAVE_ST_BIRTHTIME
#define createTime(x) x.st_birthtime
#else
#define createTime(x) x.st_mtime
#endif

#define _GNU_SOURCE
#define SERVER_PORT 5000
#define MIRROR_PORT 6000
#define CURRENT_IP "10.60.8.51"
#define MAX_ALLOWED 255
#define MAX_CLIENT_MESSAGE 155
#define MAX_TAR_FILE 50

bool fileFound = false;
char messageToClient[1096] = "Connection established with the Server";
char folderPath[256];
char *fileExtension[4];
char *serverCode = "1";
char *mirrorCode = "0";
int clientCount = 0;
int getfzSize1;
int getfzSize2;
int fileCount = 0;
int childClientDescriptor;
long int errorCode = -99;
long int successCode = 100;
struct sockaddr_in clientId;
time_t providedDate;

/**
 * The below type defines a structure that holds information about a new IP address and port number.
 * @property {char} newIpAddress - The newIpAddress property is a character array that can store an IP
 * address. The length of the array is defined by the constant INET_ADDRSTRLEN, which typically has a
 * value of 16.
 * @property {int} newPortNumber - The newPortNumber property is an integer that represents the port
 * number for a network connection.
 */
typedef struct
{
    char newIpAddress[INET_ADDRSTRLEN];
    int newPortNumber;
} newAddressInformation;

/**
 * The function checks if the number of program arguments is not equal to 1 and prints the usage
 * message with the program name and a predefined port number before exiting the program.
 *
 * @param argc The parameter `argc` is an integer that represents the number of command-line arguments
 * passed to the program. It includes the name of the program itself as the first argument.
 * @param argv The `argv` parameter is an array of strings that represents the command-line arguments
 * passed to the program. Each element of the array is a string, and `argv[0]` is typically the name of
 * the program itself.
 */
void checkProgramArguments(int argc, char *argv[])
{
    if (argc != 1)
    {
        printf("Usage: %s\nPort Number used - %d\n", argv[0], SERVER_PORT);
        exit(2);
    }
}

/**
 * The function removes a folder and all its contents using the "rm -rf" command.
 *
 * @param folderName The `folderName` parameter is a pointer to a character array that represents the
 * name of the folder to be removed.
 *
 * @return an integer value of 0.
 */
int removeFolder(char *folderName)
{
    char *executionCommand = malloc(MAX_ALLOWED * sizeof(char));
    sprintf(executionCommand, "rm -rf %s", folderName);
    system(executionCommand);
    return 0;
}

/**
 * The function creates a directory with a unique name based on the client ID and copies a file to that
 * directory.
 *
 * @param client The client parameter is an integer that represents the client's ID or some unique
 * identifier for the client.
 * @param filePath The `filePath` parameter is a string that represents the path to the file that needs
 * to be copied to the temporary directory.
 */
void createTempDirectoryForTar(char *tempDirectoryName, const char *filePath)
{
    /* The below code is creating a new directory with the name stored in the variable
    `tempDirectoryName`. It then opens a file specified by the `filePath` variable for reading. If
    the file cannot be opened, an error message is printed and the function returns. */
    mkdir(tempDirectoryName, 0700); // Making the directory with the given name
    int readFd, writeFd;
    char buffer[MAX_ALLOWED];
    char newDestination[MAX_ALLOWED];
    sprintf(newDestination, "./%d/%s", childClientDescriptor, basename((char *)filePath));
    readFd = open(filePath, O_RDONLY);
    if (readFd == -1)
    {
        perror("Unable to read the current file");
        return;
    }
    writeFd = open(newDestination, O_CREAT | O_WRONLY, 0700); // Creating new file
    if (writeFd == -1)
    {
        perror("Unable to open the file with temp destination");
        return;
    }
    long int n1;
    while ((n1 = read(readFd, buffer, MAX_ALLOWED)) > 0)
    {
        int m1 = write(writeFd, buffer, n1);
        if (n1 != m1)
        {
            perror("Unable to write the current file");
            return;
        }
        if (n1 == -1)
        {
            perror("Unable to read the current file file writing");
            return;
        }
    }
    close(writeFd);
    close(readFd);
    fileCount++;
}

/**
 * The function creates a tar file by executing a system command.
 *
 * @return an integer value. If the tar file is successfully created, it returns 0. If there is an
 * error in creating the tar file, it returns -1.
 */
int createTarFile()
{
    char createTarCommand[MAX_ALLOWED];
    sprintf(createTarCommand, "tar -czvf temp%d.tar.gz %d", childClientDescriptor, childClientDescriptor); // Storing the command to create the tar file with the given destination folder
    // Using the system call to execute the command as a script
    system(createTarCommand);
    return 0;
}

/**
 * The function transfers the content of a file specified by tarFileLocation to a client socket.
 *
 * @param client The "client" parameter is the file descriptor of the client socket. It is used to send
 * data to the client.
 * @param tarFileLocation The `tarFileLocation` parameter is a string that represents the file location
 * of the file you want to transfer to the client. It should be the path to the file on the server's
 * file system.
 *
 * @return an integer value. If the file transfer is successful, it returns 1. If there is an error
 * during the file transfer, it returns -1.
 */
int transferFileToClientSocket(int client, char *tarFileLocation)
{
    char fileContent[999999];
    FILE *tarFile;
    int length = 0;
    long int tarFileSize;
    tarFile = fopen(tarFileLocation, "rb");
    if (tarFile == NULL)
    {
        return -1;
    }
    fseek(tarFile, 0, SEEK_END);
    tarFileSize = ftell(tarFile);                       // Getting the file size
    send(client, &tarFileSize, sizeof(tarFileSize), 0); // Sending the file size
    int fdTar = open(tarFileLocation, O_RDONLY);
    while ((length = read(fdTar, fileContent, 999999)) > 0)
    {
        // Sending the file
        if (send(client, fileContent, length, 0) == -1)
        {
            return -1;
        }
    }
    close(fdTar);
    fclose(tarFile);
    return 1;
}

/**
 * The function `printFileDetails` prints the details of a file, such as its name, size, creation date,
 * and permissions, to a client descriptor.
 *
 * @param filename A string representing the name of the file.
 * @param fileInformation The fileInformation parameter is a pointer to a struct stat object that
 * contains information about the file. This includes details such as the file size, creation date, and
 * permissions.
 * @param clientDescriptor The `clientDescriptor` parameter is an integer that represents the file
 * descriptor of the client socket. It is used to write the file details to the client.
 */
void printFileDetails(const char *filename, const struct stat *fileInformation, int clientDescriptor)
{
    /* The below code is dynamically allocating memory for a character buffer and then using sprintf to
    format and store information about a file in the buffer. The information includes the file name,
    size, date created, and permissions. The permissions are represented by a string of characters,
    where each character represents a specific permission (read, write, execute) for the owner,
    group, and others. */
    char *buffer = malloc(sizeof(char) * 1096);
    sprintf(buffer, "File: %s\nSize: %ld bytes\nDate created: %sPermissions: %s%s%s%s%s%s%s%s%s\n",
            filename,
            fileInformation->st_size,
            ctime(&fileInformation->st_ctime),
            (S_ISDIR(fileInformation->st_mode)) ? "d" : "-",
            (fileInformation->st_mode & S_IRUSR) ? "r" : "-",
            (fileInformation->st_mode & S_IWUSR) ? "w" : "-",
            (fileInformation->st_mode & S_IXUSR) ? "x" : "-",
            (fileInformation->st_mode & S_IRGRP) ? "r" : "-",
            (fileInformation->st_mode & S_IWGRP) ? "w" : "-",
            (fileInformation->st_mode & S_IXGRP) ? "x" : "-",
            (fileInformation->st_mode & S_IROTH) ? "r" : "-",
            (fileInformation->st_mode & S_IWOTH) ? "w" : "-",
            (fileInformation->st_mode & S_IXOTH) ? "x" : "-");

    // Sending the file details to the client
    long successCode = 100;
    send(clientDescriptor, &successCode, sizeof(successCode), 0);
    send(clientDescriptor, buffer, strlen(buffer), 0);
    fileFound = true;
}

/**
 * The getFileInformation function recursively searches for a specified file in a directory and its
 * subdirectories, and prints its details if found.
 *
 * @param dirPath The directory path where the function will start searching for the file.
 * @param filename The `filename` parameter is a string that represents the name of the file you are
 * searching for in the directory specified by `dirPath`.
 * @param clientDescriptor The parameter "clientDescriptor" is an integer that represents the
 * descriptor of the client. It is used to identify the client connection and send the file information
 * back to the client.
 *
 * @return The function does not have a return type specified, so it is returning nothing (void).
 */
void getFileInformation(const char *dirPath, const char *filename, int clientDescriptor)
{
    DIR *currentDirectory = opendir(dirPath);
    if (currentDirectory == NULL)
    {
        perror("Unable to open the server current directory");
        return;
    }
    struct dirent *currentFilePath; // To store the current file of the directory
    struct stat fileInformation;    // To Store the current file informt
    // Reading the directories one by one
    while ((currentFilePath = readdir(currentDirectory)) != NULL)
    {
        char path[1096];
        snprintf(path, sizeof(path), "%s/%s", dirPath, currentFilePath->d_name); // Concatinating the current director path
        if (lstat(path, &fileInformation) == -1)
        {
            continue;
        }
        if (S_ISDIR(fileInformation.st_mode)) // If the current path is the directory then call the function again
        {
            if (strcmp(currentFilePath->d_name, ".") != 0 && strcmp(currentFilePath->d_name, "..") != 0)
            {
                if (strcmp(currentFilePath->d_name, filename) == 0)
                {
                    if (!fileFound)
                    {
                        printFileDetails(filename, &fileInformation, clientDescriptor);
                    }
                    break; // Once the first file is found then breaking the loop
                }
                // Recursive call for subdirectories
                getFileInformation(path, filename, clientDescriptor);
            }
        }
        else if (S_ISREG(fileInformation.st_mode))
        {
            if (strcmp(currentFilePath->d_name, filename) == 0)
            {
                if (!fileFound)
                {
                    printFileDetails(filename, &fileInformation, clientDescriptor);
                }
                break; // Once the first file is found then breaking the loop
            }
        }
    }
    closedir(currentDirectory);
}

/**
 * The function checks if a given name starts with a dot or contains a dot after a forward slash.
 *
 * @param name A pointer to a character array representing the name of a file or directory.
 *
 * @return The function isHidden returns 1 if the name starts with a dot or contains a "/." substring,
 * indicating that the file or directory is hidden. Otherwise, it returns 0.
 */
int isHidden(const char *name)
{
    return name[0] == '.' || strstr(name, "/.") != NULL; // Check if the name starts with a dot
}

/**
 * The function finds files within a specified size range and copies them to a temporary directory.
 *
 * @param currentFilePath A string representing the current file path.
 * @param currentFileInformation The `currentFileInformation` parameter is a pointer to a `struct stat`
 * object that contains information about the current file being processed. This structure typically
 * includes details such as the file size, permissions, timestamps, and more.
 * @param fileType The parameter `fileType` represents the type of the current path. It is an integer
 * value that can have the following values:
 * @param ftwbuf The `ftwbuf` parameter is a pointer to a structure of type `struct FTW`. This
 * structure is used by the `nftw()` function to provide information about the current file being
 * processed during a file tree walk.
 *
 * @return an integer value of 0.
 */
int findFileWithinSize(const char *currentFilePath, const struct stat *currentFileInformation, int fileType, struct FTW *ftwbuf)
{
    if (fileType == FTW_F && currentFileInformation->st_size >= getfzSize1 && currentFileInformation->st_size <= getfzSize2 && !isHidden(currentFilePath)) // If the current path is a file
    {
        char tempDirectoryName[MAX_TAR_FILE];
        sprintf(tempDirectoryName, "./%d/", childClientDescriptor);
        createTempDirectoryForTar(tempDirectoryName, currentFilePath);
    }
    return 0;
}

/**
 * The function `findFileWithExtensions` searches for files with specific extensions in a directory and
 * copies them to a temporary directory.
 *
 * @param currentFilePath A string representing the current file path being processed.
 * @param currentFileInformation The currentFileInformation parameter is a pointer to a struct stat
 * object that contains information about the current file being processed. This information includes
 * file size, permissions, timestamps, and other attributes.
 * @param fileType The `fileType` parameter is used to determine the type of the current file being
 * processed. It is an integer value that is passed to the `findFileWithExtensions` function. The value
 * of `fileType` is determined by the `nftw` function, which is used
 * @param ftwbuf The `ftwbuf` parameter is a pointer to a structure of type `struct FTW`. This
 * structure is used by the `nftw` function to keep track of the state of the traversal of the file
 * tree. It contains the following members:
 *
 * @return an integer value of 0.
 */
int findFileWithExtensions(const char *currentFilePath, const struct stat *currentFileInformation, int fileType, struct FTW *ftwbuf)
{
    if (fileType == FTW_F && !isHidden(currentFilePath)) // If the current path is a file
    {
        char *currentFileExtension = strrchr(currentFilePath, '.');
        if (currentFileExtension != NULL)
        {
            int i = 0;
            for (i = 0; i < 4; i++)
            {
                if (fileExtension[i] != NULL)
                {
                    if (strcmp(currentFileExtension + 1, fileExtension[i]) == 0) // If the current file extension is one of the given extensions
                    {
                        char tempDirectoryName[MAX_TAR_FILE];
                        sprintf(tempDirectoryName, "./%d/", childClientDescriptor);
                        createTempDirectoryForTar(tempDirectoryName, currentFilePath);
                    }
                }
            }
        }
    }
    return 0;
}

/**
 * The function finds files with a modification date less than or equal to a provided date and copies
 * them to a temporary directory.
 *
 * @param currentFilePath The current file path being processed.
 * @param currentFileInformation The currentFileInformation parameter is a pointer to a struct stat
 * object that contains information about the current file being processed. This includes attributes
 * such as file size, permissions, and modification time.
 * @param fileType The parameter `fileType` represents the type of the current path being processed. It
 * is an integer value that can have the following possible values:
 * @param ftwbuf The `ftwbuf` parameter is a pointer to a structure of type `struct FTW`. This
 * structure is used by the `nftw()` function to provide information about the current file being
 * processed during a file tree walk. It contains the following members:
 *
 * @return an integer value of 0.
 */
int findFileWithLessDate(const char *currentFilePath, const struct stat *currentFileInformation, int fileType, struct FTW *ftwbuf)
{
    struct stat st;
    stat(currentFilePath, &st);
    if (fileType == FTW_F && createTime(st) <= providedDate && !isHidden(currentFilePath)) // If the current path is a file
    {
        char tempDirectoryName[MAX_TAR_FILE];
        sprintf(tempDirectoryName, "./%d/", childClientDescriptor);
        createTempDirectoryForTar(tempDirectoryName, currentFilePath);
    }
    return 0;
}

/**
 * The function finds files with a modification date greater than or equal to a provided date and
 * copies them to a temporary directory.
 *
 * @param currentFilePath The current file path being processed.
 * @param currentFileInformation A pointer to a struct stat object that contains information about the
 * current file being processed.
 * @param fileType The parameter `fileType` represents the type of the current path. It is an integer
 * value that can have the following possible values:
 * @param ftwbuf The `ftwbuf` parameter is a pointer to a structure of type `struct FTW`. This
 * structure is used by the `nftw()` function to provide information about the current file being
 * processed during the tree traversal.
 *
 * @return an integer value of 0.
 */
int findFileWithMoreDate(const char *currentFilePath, const struct stat *currentFileInformation, int fileType, struct FTW *ftwbuf)
{
    struct stat st;
    stat(currentFilePath, &st);
    if (fileType == FTW_F && createTime(st) >= providedDate && !isHidden(currentFilePath))
    {
        char tempDirectoryName[MAX_TAR_FILE];
        sprintf(tempDirectoryName, "./%d/", childClientDescriptor);
        createTempDirectoryForTar(tempDirectoryName, currentFilePath);
    }
    return 0;
}

/**
 * The function `pclientrequest` handles client requests, reads commands from the client, executes the
 * commands, and sends the results back to the client.
 *
 * @param clientDescriptor The `clientDescriptor` parameter is an integer that represents the file
 * descriptor of the client socket. It is used to communicate with the client and receive commands from
 * them.
 */
void pclientrequest(int clientDescriptor)
{
    childClientDescriptor = clientDescriptor;
    const char *serverHomeDirectory = getenv("HOME");
    if (serverHomeDirectory == NULL)
    {
        perror("Unable to get the home directory path");
        exit(20);
    }
    while (1)
    {
        fileCount = 0;
        char clientMessage[MAX_CLIENT_MESSAGE];
        long nfds = sysconf(_SC_OPEN_MAX) - 5; // Determine the maximum number of file descriptors
        memset(clientMessage, 0, sizeof(clientMessage));
        printf("Waiting for the client message\n");
        int val = read(clientDescriptor, clientMessage, MAX_CLIENT_MESSAGE);
        // If the connection is lost or cancelled by the client
        if (val <= 0)
        {
            printf("\nClient %d has been killed or error while reading the command from the client", clientCount + 1);
            printf("\nClosing the connection\n");
            close(clientDescriptor);
            printf("Now waiting for a new connection\n");
            exit(10); // Exiting from the child
        }
        if (strcmp(clientMessage, "quitc") == 0)
        {
            printf("Client %d has requested to break the connection\n", clientCount + 1);
            printf("Connection closed\n");
            close(clientDescriptor);
            exit(9);
        }
        printf("Now, starting the execution of the command - %s\n", clientMessage);
        /* The below code is parsing a client message and storing the command and its arguments in
        separate variables. */
        char *customCommand = strtok(clientMessage, " ");
        char *commandArgumentsArray[10];
        int length = 0;
        for (;;)
        {
            char *commandArguments = strtok(NULL, " ");
            if (commandArguments != NULL)
            {
                commandArgumentsArray[length] = commandArguments;
                length++;
            }
            else
            {
                break;
            }
        }
        if (strcmp(customCommand, "getfn") == 0)
        {
            getFileInformation(serverHomeDirectory, commandArgumentsArray[0], clientDescriptor); // Calling this function to get the information of the file
            if (!fileFound)
            {
                send(clientDescriptor, &errorCode, sizeof(&errorCode), 0);
            }
            fileFound = false;
        }
        else if (strcmp(customCommand, "getfz") == 0)
        {
            getfzSize1 = atoi(commandArgumentsArray[0]);                   // Changing the string to int
            getfzSize2 = atoi(commandArgumentsArray[1]);                   // Changing the string to int
            nftw(serverHomeDirectory, findFileWithinSize, nfds, FTW_PHYS); // Using nftw to traverse through the home directory and find the files
            if (fileCount == 0)
            {
                send(clientDescriptor, &errorCode, sizeof(errorCode), 0);
            }
            else
            {
                /* The below code is creating a tar file, sending it to a client, and then removing the
                temporary files and directories used in the process. */
                printf("Creating the tar file\n");
                int result = createTarFile();
                if (result == -1)
                {
                    send(clientDescriptor, &errorCode, sizeof(errorCode), 0);
                    continue;
                }
                printf("Sending the tar file to the client\n");
                char tempName[MAX_TAR_FILE];
                sprintf(tempName, "temp%d.tar.gz", childClientDescriptor);
                result = transferFileToClientSocket(clientDescriptor, tempName);
                if (result == -1)
                {
                    send(clientDescriptor, &errorCode, sizeof(errorCode), 0);
                    continue;
                }
                printf("File sent to client\n");
                char tempDirectoryName[MAX_TAR_FILE];
                sprintf(tempDirectoryName, "./%d/", childClientDescriptor);
                removeFolder(tempName);
                removeFolder(tempDirectoryName);
            }
        }
        else if (strcmp(customCommand, "getft") == 0)
        {
            int i = 0;
            for (i = 0; i < length; i++)
            {
                fileExtension[i] = commandArgumentsArray[i];
            }
            fileExtension[i] = NULL;
            nftw(serverHomeDirectory, findFileWithExtensions, nfds, FTW_PHYS); // Using nftw to traverse through the home directory and find the files
            if (fileCount == 0)
            {
                send(clientDescriptor, &errorCode, sizeof(errorCode), 0);
            }
            else
            {
                /* The below code is creating a tar file, sending it to a client, and then removing the
                temporary files and directories used in the process. */
                printf("Creating the tar file\n");
                int result = createTarFile();
                if (result == -1)
                {
                    send(clientDescriptor, &errorCode, sizeof(errorCode), 0);
                    continue;
                }
                printf("Sending the tar file to the client\n");
                char tempName[MAX_TAR_FILE];
                sprintf(tempName, "temp%d.tar.gz", childClientDescriptor);
                result = transferFileToClientSocket(clientDescriptor, tempName);
                if (result == -1)
                {
                    send(clientDescriptor, &errorCode, sizeof(errorCode), 0);
                    continue;
                }
                printf("File sent to client\n");
                char tempDirectoryName[MAX_TAR_FILE];
                sprintf(tempDirectoryName, "./%d/", childClientDescriptor);
                removeFolder(tempName);          // Deleting the tar file created in the server
                removeFolder(tempDirectoryName); // Deleting the temp folder in the server the current client
            }
        }
        else if (strcmp(customCommand, "getfdb") == 0)
        {
            struct tm providedTime = {0};
            if (strptime(commandArgumentsArray[0], "%Y-%m-%d", &providedTime) == NULL)
            {
                send(clientDescriptor, &errorCode, sizeof(errorCode), 0);
                continue;
            }
            providedTime.tm_isdst = -1;
            providedDate = mktime(&providedTime);
            nftw(serverHomeDirectory, findFileWithLessDate, nfds, FTW_PHYS); // Using nftw to traverse through the home directory and find the files
            if (fileCount == 0)
            {
                send(clientDescriptor, &errorCode, sizeof(errorCode), 0);
            }
            else
            {
                /* The below code is creating a tar file, sending it to a client, and then removing the
                temporary files and directories used in the process. */
                printf("Creating the tar file\n");
                int result = createTarFile();
                if (result == -1)
                {
                    send(clientDescriptor, &errorCode, sizeof(errorCode), 0);
                    continue;
                }
                printf("Sending the tar file to the client\n");
                char tempName[MAX_TAR_FILE];
                sprintf(tempName, "temp%d.tar.gz", childClientDescriptor);
                result = transferFileToClientSocket(clientDescriptor, tempName);
                if (result == -1)
                {
                    send(clientDescriptor, &errorCode, sizeof(errorCode), 0);
                    continue;
                }
                printf("File sent to client\n");
                char tempDirectoryName[MAX_TAR_FILE];
                sprintf(tempDirectoryName, "./%d/", childClientDescriptor);
                removeFolder(tempName);          // Deleting the tar file created in the server
                removeFolder(tempDirectoryName); // Deleting the temp folder in the server the current client
            }
        }
        else if (strcmp(customCommand, "getfda") == 0)
        {
            struct tm providedTime = {0};
            if (strptime(commandArgumentsArray[0], "%Y-%m-%d", &providedTime) == NULL)
            {
                send(clientDescriptor, &errorCode, sizeof(errorCode), 0);
                continue;
            }
            providedTime.tm_isdst = -1;
            providedDate = mktime(&providedTime);
            nftw(serverHomeDirectory, findFileWithMoreDate, nfds, FTW_PHYS); // Using nftw to traverse through the home directory and find the files
            if (fileCount == 0)
            {
                send(clientDescriptor, &errorCode, sizeof(errorCode), 0);
            }
            else
            {
                /* The below code is creating a tar file, sending it to a client, and then removing the
                temporary files and directories used in the process. */
                printf("Creating the tar file\n");
                int result = createTarFile();
                if (result == -1)
                {
                    send(clientDescriptor, &errorCode, sizeof(errorCode), 0);
                    continue;
                }
                printf("Sending the tar file to the client\n");
                char tempName[MAX_TAR_FILE];
                sprintf(tempName, "temp%d.tar.gz", childClientDescriptor);
                result = transferFileToClientSocket(clientDescriptor, tempName);
                if (result == -1)
                {
                    send(clientDescriptor, &errorCode, sizeof(errorCode), 0);
                    continue;
                }
                printf("File sent to client\n");
                char tempDirectoryName[MAX_TAR_FILE];
                sprintf(tempDirectoryName, "./%d/", childClientDescriptor);
                removeFolder(tempName);          // Deleting the tar file created in the server
                removeFolder(tempDirectoryName); // Deleting the temp folder in the server the current client
            }
        }
        printf("Execution completed\n");
        printf("-----------------------------------\n");
    }
}

/**
 * The function determines whether load balancing is needed based on the number of clients.
 *
 * @return The function loadBalancing() returns either 1 or 0, depending on the value of the variable
 * clientCount.
 */
int loadBalancing()
{
    if (clientCount <= 3)
    {
        return 1;
    }
    else if (clientCount > 3 && clientCount < 8)
    {
        return 0;
    }
    else if (clientCount >= 8 && (clientCount % 2) == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int main(int argc, char *argv[])
{
    /* The below code is retrieving the value of the "HOME" environment variable and storing it in the
    "homeDir" variable. It then uses the "snprintf" function to copy the value of "homeDir" into the
    "folderPath" variable. */
    const char *homeDir = getenv("HOME");
    snprintf(folderPath, sizeof(folderPath), "%s", homeDir);
    checkProgramArguments(argc, argv); // Function to check the program arguments
    int programServerDescriptor, client;
    struct sockaddr_in programServerAddress;

    if ((programServerDescriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Unable to create the socket for the program");
        exit(1);
    }

    /* The below code is setting up the server address for a program in the C programming language. It
    is using the `sin_family` field to specify the address family as `AF_INET`, which indicates
    IPv4. The `sin_addr.s_addr` field is set to `htonl(INADDR_ANY)`, which means the program will
    bind to any available network interface on the server. The `sin_port` field is set to
    `htons(SERVER_PORT)`, which specifies the port number that the program will listen on. */
    programServerAddress.sin_family = AF_INET;
    programServerAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    programServerAddress.sin_port = htons(SERVER_PORT);

    bind(programServerDescriptor, (struct sockaddr *)&programServerAddress, sizeof(programServerAddress));

    /* The below code is using the listen function to set the programServerDescriptor to listen for
    incoming connections. The second parameter, 5, specifies the maximum number of pending
    connections that can be queued for the programServerDescriptor. */
    listen(programServerDescriptor, 5);

    printf("\nWaiting for the 1st client to connect with the server and execute the commands\n");
    // Infinite loop to wait for the different clients
    for (;;)
    {
        int childStatus;
        int connectionType = loadBalancing();   // Checking the client Count
        socklen_t clientLen = sizeof(clientId); // Variable to get the client details
        if (connectionType == 1)
        {
            /* The below code is a snippet from a server program written in C. It is responsible for
            accepting client connections, forking a child process to handle each client connection,
            and then waiting for the child process to complete. */
            client = accept(programServerDescriptor, (struct sockaddr *)&clientId, &clientLen);
            write(client, serverCode, strlen(serverCode)); // Sending the message to client that they are connected
            int child = fork();
            if (child == 0)
            {
                // Created a child whenever there is client connection
                printf("Connected to the client using server %s:%d\n", inet_ntoa(clientId.sin_addr), ntohs(clientId.sin_port));
                pclientrequest(client);
                close(client);
                exit(0);
            }
            else if (child < 0)
            {
                perror("Error while connecting to client and making fork");
                continue;
            }
            else
            {
                waitpid(0, &childStatus, WNOHANG);
            }
        }
        else
        {
            client = accept(programServerDescriptor, (struct sockaddr *)&clientId, &clientLen);
            if (!fork())
            {
                newAddressInformation mirrorInfo;
                write(client, mirrorCode, strlen(mirrorCode)); // Sending the message to client that they are connected
                mirrorInfo.newPortNumber = MIRROR_PORT;
                strcpy(mirrorInfo.newIpAddress, CURRENT_IP);
                send(client, &mirrorInfo, sizeof(newAddressInformation), 0);
                close(programServerDescriptor);
                exit(0);
            }
            waitpid(0, &childStatus, WNOHANG);
        }
        clientCount++; // Incrementing the client Count
        sleep(1);
        printf("\nWaiting for the next client now to connect with the server and execute the commands\n");
    }
    return 0;
}