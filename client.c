/**
 * @authors Karan Mahajan - 110119774, Niharika Khurana - 110124290 (Section 1 Monday)
 * @email [mahaja81@uwindsor.ca,khuranan@uwindsor.ca]
 * @create date 2023-12-02 00:50:57
 * @modify date 2023-12-13 23:00:47
 * @desc [Client-server project, a client can request a file or a set of files from the server]
 */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/fcntl.h>

#define SERVER_PORT 5000
#define MAX_ALLOWED 255
#define MAX_BYTES 255
#define MAX_MESSAGE_ALLOWED 10000

char folderPath[MAX_ALLOWED];

typedef struct
{
    char newIpAddress[INET_ADDRSTRLEN];
    int newPortNumber;
} newAddressInformation;

/**
 * The function checks if the number of program arguments is not equal to 2 and prints a usage message
 * if it is not.
 *
 * @param argc The "argc" parameter is an integer that represents the number of command-line arguments
 * passed to the program. It includes the name of the program itself as the first argument.
 * @param argv The parameter `argv` is an array of strings that represents the command-line arguments
 * passed to the program. Each element of the array `argv` is a null-terminated string, where `argv[0]`
 * is the name of the program itself, and `argv[1]` is the
 */
void checkProgramArguments(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s [IP Address]\nPlease check the mirror and server PORT in the server.c and mirror.c file and replace the SERVER_PORT and MIRROR_PORT\n", argv[0]);
        exit(2);
    }
}

/**
 * The function checks if a given string argument is a number.
 *
 * @param arg The parameter `arg` is a pointer to a character array, which represents a string.
 *
 * @return The function isArgumentNumber is returning a boolean value.
 */
bool isArgumentNumber(char *arg)
{
    int len = 0;
    while (arg[len] != '\0')
    {
        if (!isdigit(arg[len]))
        {
            return false;
        }
        len++;
    }
    return true;
}

/**
 * The function "validateDate" checks if a given date is valid or not.
 *
 * @param date The "date" parameter is a string representing a date in the format "YYYY-MM-DD".
 *
 * @return an integer value. If the date is valid, it returns 0. If the date is invalid (either the
 * format is incorrect or the values are out of range), it returns -1.
 */
int validateDate(char *date)
{
    int daysInMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int year, month, day;
    if (sscanf(date, "%d-%d-%d", &year, &month, &day) != 3)
    {
        return -1;
    }
    if (year < 1000 || year > 9999 || month < 1 || month > 12 || day < 1)
    {
        return -1; // Invalid ranges
    }
    // Checking for the leap year
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
    {
        daysInMonth[2] = 29;
    }
    if (day > daysInMonth[month])
    {
        return -1;
    }
    return 0;
}

/**
 * The function `validateCommandArguments` checks if the given command and its arguments are valid
 * based on specific conditions for each command.
 *
 * @param command A string representing the command entered by the user.
 * @param commandArgumentArray An array of strings containing the command arguments.
 * @param length The length parameter represents the number of elements in the commandArgumentArray.
 *
 * @return a boolean value, either true or false.
 */
bool validateCommandArguments(char *command, char *commandArgumentArray[], int length)
{
    bool isValidCommand = true;
    if (strcmp(command, "getfn") == 0)
    {
        if (length != 1)
        {
            printf("Usage : %s filename\n", command);
            return false;
        }
    }
    else if (strcmp(command, "getfz") == 0)
    {
        if (length != 2)
        {
            printf("Usage : %s size1 size2\n", command);
            return false;
        }
        char *stringValue;
        long numValue = strtol(commandArgumentArray[0], &stringValue, 10);
        if (*stringValue == '\0')
        {
            if (numValue < 0)
            {
                printf("Usage : %s size1 size2\nSize1 and Size2 should be Integer and positive value\n", command);
                return false;
            }
        }
        else
        {
            printf("Usage : %s size1 size2\nSize1 and Size2 should be Integer and positive value\n", command);
            return false;
        }
        numValue = strtol(commandArgumentArray[1], &stringValue, 10);
        if (*stringValue == '\0')
        {
            if (numValue < 0)
            {
                printf("Usage : %s size1 size2\nSize1 and Size2 should be Integer and positive value\n", command);
                return false;
            }
        }
        else
        {
            printf("Usage : %s size1 size2\nSize1 and Size2 should be Integer and positive value\n", command);
            return false;
        }
        if (atoi(commandArgumentArray[0]) > atoi(commandArgumentArray[1]))
        {
            printf("Usage : %s size1 size2\nSize2 should be greater than size1\n", command);
            return false;
        }
    }
    else if (strcmp(command, "getft") == 0)
    {
        if (length < 1 || length > 3)
        {
            printf("Usage : %s extension\nUpto 3 extensions are allowed\n", command);
            return false;
        }
    }
    else if (strcmp(command, "getfdb") == 0)
    {
        if (length != 1)
        {
            printf("Usage : %s date\n", command);
            return false;
        }
        if (validateDate(commandArgumentArray[0]) == -1)
        {
            printf("Usage: %s date\nPlease enter the correct date\n", command);
            return false;
        }
    }
    else if (strcmp(command, "getfda") == 0)
    {
        if (length != 1)
        {
            printf("Usage : %s date\n", command);
            return false;
        }
        if (validateDate(commandArgumentArray[0]) == -1)
        {
            printf("Usage: %s date\nPlease enter the correct date\n", command);
            return false;
        }
    }
    else
    {
        printf("Please enter the valid command for the server\n");
        return false;
    }
    return isValidCommand;
}

/**
 * The function "validateUserCommand" takes a user command as input, separates the command and its
 * arguments, and then calls another function to validate the command and its arguments.
 *
 * @param userCommand A string containing the user's command and its arguments. For example, "add 5 10"
 * or "delete file.txt".
 *
 * @return an integer value. If the command and its arguments are valid, it returns 1. Otherwise, it
 * returns 0.
 */
int validateUserCommand(char *userCommand)
{
    char *customCommand = strtok(userCommand, " "); // Storing the command
    char *commandArgumentsArray[10];                // Array to store the command arguments
    int length = 0;
    //  Calculating the arguments for each command
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
    bool isValid = validateCommandArguments(customCommand, commandArgumentsArray, length);
    if (!isValid)
    {
        return 0;
    }
    return 1;
}

int main(int argc, char *argv[])
{
    checkProgramArguments(argc, argv); // Function to check the program arguments
    int programClientDescriptor;
    struct sockaddr_in programClientAddress;

    /* The below code is creating a socket for a client program to connect to a server. */
    if ((programClientDescriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Unable to create the socket for the program:");
        exit(10);
    }
    programClientAddress.sin_family = AF_INET;                    // Internet
    programClientAddress.sin_port = htons((uint16_t)SERVER_PORT); // Port number

    if (inet_pton(AF_INET, argv[1], &programClientAddress.sin_addr) < 0)
    {
        perror("Connection to inet_pton failed for this client");
        exit(10);
    }

    /* The below code is attempting to establish a connection with a server using the `connect`
    function. If the connection fails, it will display an error message and then attempt to connect
    again after a delay of 3 seconds. If the second connection attempt also fails, it will display
    another error message and exit the program with an exit code of 10. */
    if (connect(programClientDescriptor, (struct sockaddr *)&programClientAddress, sizeof(programClientAddress)) < 0)
    {
        perror("Connection failed for this client");
        printf("Trying to connect again with the after changing the PORT NUMBER\n");
        sleep(3);
        if (connect(programClientDescriptor, (struct sockaddr *)&programClientAddress, sizeof(programClientAddress)) < 0)
        {
            perror("Connection failed for this client again, please restart the program");
            exit(10);
        }
        exit(10);
    }

    char *messageFromServer = malloc(sizeof(char) * MAX_MESSAGE_ALLOWED);
    int total = 0;
    if (total = read(programClientDescriptor, messageFromServer, 1) < 0)
    {
        perror("Unable to read the message from the server");
        exit(6);
    }
    if (strcmp(messageFromServer, "1") == 0)
    {
    }
    else if (strcmp(messageFromServer, "0") == 0)
    {
        /* The below code is establishing a connection with a server and receiving information about a
        new address (IP address and port number) from the server. It then closes the current
        connection with the server and creates a new socket for the program. It sets the address
        family and port number for the new socket based on the received information. It then
        attempts to connect to the new address using the new socket. If the connection fails, it
        tries again after a delay of 3 seconds. If the second connection attempt also fails, it
        prints an error message and exits the program. */
        newAddressInformation mirrorInfo;
        recv(programClientDescriptor, &mirrorInfo, sizeof(newAddressInformation), 0); // Reading the mirror info from the server
        close(programClientDescriptor);                                               // Closing the current connection with the server
        // Creating the socket for the program
        if ((programClientDescriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            perror("Unable to create the socket for the program:");
            exit(10);
        }
        programClientAddress.sin_family = AF_INET;                       // Internet
        programClientAddress.sin_port = htons(mirrorInfo.newPortNumber); // Port number

        if (inet_pton(AF_INET, mirrorInfo.newIpAddress, &programClientAddress.sin_addr) < 0)
        {
            perror("Connection to inet_pton failed for this client");
            exit(10);
        }

        if (connect(programClientDescriptor, (struct sockaddr *)&programClientAddress, sizeof(programClientAddress)) < 0)
        {
            perror("Connection failed for this client");
            printf("Trying to connect again with the after changing the PORT NUMBER\n");
            sleep(3);
            if (connect(programClientDescriptor, (struct sockaddr *)&programClientAddress, sizeof(programClientAddress)) < 0)
            {
                perror("Connection failed for this client again, please restart the program");
                exit(10);
            }
            exit(10);
        }
        memset(messageFromServer, 0, sizeof(messageFromServer));
    }

    /* The below code is creating a directory named "f23project" in the user's home directory. It first
    retrieves the home directory path using the `getenv` function and stores it in the `homeDir`
    variable. Then, it uses the `snprintf` function to format the folder path by appending
    "/f23project" to the `homeDir` variable. Finally, it creates the directory using the `mkdir`
    function with the folder path and permissions set to 0777. */
    const char *homeDir = getenv("HOME");
    snprintf(folderPath, sizeof(folderPath), "%s/f23project", homeDir);
    mkdir(folderPath, 0777);

    while (1)
    {
        char userCommand[MAX_ALLOWED];
        char existingFile[MAX_ALLOWED + 100];
        long int bytesReadFromServer = 0;

        memset(userCommand, 0, sizeof(userCommand));
        snprintf(existingFile, sizeof(existingFile), "%s/temp.tar.gz", folderPath);
        unlink(existingFile);
        printf("Client$ ");
        fgets(userCommand, sizeof(userCommand), stdin);
        userCommand[strcspn(userCommand, "\n")] = '\0';
        char *copyOfCommand = malloc(strlen(userCommand) + 1);
        strcpy(copyOfCommand, userCommand); // Creating the copy of the command
        printf("-----------------------------\n");
        if (strcmp(userCommand, "quitc") == 0)
        {
            write(programClientDescriptor, userCommand, strlen(userCommand) + 1);
            printf("Closing the connection between the server and client\n");
            break;
        }
        int isValid = validateUserCommand(copyOfCommand); // Validating the command
        if (isValid > 0)
        {
            int err = send(programClientDescriptor, userCommand, strlen(userCommand), 0);
            if (err < 0)
            {
                continue;
            }
            printf("Server is now processing the command\n");
            char *customCommand = strtok(copyOfCommand, " ");
            long serverResponse;
            // If the command is to get the file information
            if (strcmp(customCommand, "getfn") == 0)
            {
                int totalBytes = read(programClientDescriptor, &serverResponse, sizeof(serverResponse));
                if (totalBytes > 0)
                {
                    if (serverResponse > 0)
                    {
                        memset(messageFromServer, 0, sizeof(messageFromServer));
                        read(programClientDescriptor, messageFromServer, serverResponse);
                        printf("Response from the server is:\n%.*s\n", serverResponse, messageFromServer);
                    }
                    else if (serverResponse == -99)
                    {
                        printf("File not found\n");
                    }
                }
            }
            else // If the command is to get the tar file
            {
                bool isCreated = false;
                int totalBytes = read(programClientDescriptor, &serverResponse, sizeof(serverResponse));
                if (totalBytes > 0)
                {
                    memset(messageFromServer, 0, sizeof(messageFromServer));
                    if (serverResponse == -99)
                    {
                        printf("No File Found");
                    }
                    else
                    {
                        int writeFd;
                        if (!isCreated)
                        {
                            snprintf(folderPath, sizeof(folderPath), "%s/f23project/temp.tar.gz", homeDir);
                            writeFd = open(folderPath, O_CREAT | O_WRONLY, 0777); // Creating new file
                            if (writeFd < 0)
                            {
                                perror("Unable to create the temp file in the client location");
                                continue;
                            }
                            isCreated = true;
                        }
                        while (bytesReadFromServer != serverResponse)
                        {
                            int bytesNeed = MAX_BYTES;
                            if (bytesReadFromServer + MAX_BYTES > serverResponse)
                            {
                                bytesNeed = serverResponse - bytesNeed;
                            }
                            total = recv(programClientDescriptor, messageFromServer, bytesNeed, 0);
                            int m1 = write(writeFd, messageFromServer, total);
                            if (m1 < 0)
                            {
                                perror("Unable to write the tar file from the server");
                                break;
                            }
                            bytesReadFromServer = bytesReadFromServer + total;
                            if (bytesReadFromServer >= serverResponse)
                            {
                                break;
                            }
                        }
                        printf("File received successfully\n");
                    }
                }
            }
            printf("\n-----------------------------\n");
        }
    }
    close(programClientDescriptor);
    return 0;
}