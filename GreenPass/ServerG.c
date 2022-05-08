#include "GreenPass.h"

int main(int argc, char *argv[]) {
    int listenSockFd, connSockFd, serverVSockFd;
    unsigned short int serverGPort, serverVPort;
    int operationCode;
    struct sockaddr_in serverGAddress, clientAddress;
    pid_t pid;
    char buffer[HEALTH_INSURE_CARD_NUMBER_LENGTH];
    ssize_t nleftWrite, nleftRead;
    packet packetFromServerV, packetToServerVViaClientS;
    packetGPUpdate packetFromClientT, packetToServerVViaClientT;

    // Controllo argomenti da linea di comando
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <Porta ServerG> <Porta ServerV>.\n", argv[0]);
        exit(INVALID_ARGUMENTS);
    }

    // Conversione numero di porta del ServerG da stringa ad unsigned short int
    serverGPort = stringToPort(argv[1], NULL, 10);

    // Conversione numero di porta del ServerV da stringa ad unsigned short int
    serverVPort = stringToPort(argv[2], NULL, 10);

    // Apertura del socket
    listenSockFd = Socket(AF_INET, SOCK_STREAM, 0);
    // Pulizia degli indirizzi relativi al ServerG e al client
    memset((void *)&serverGAddress, 0, sizeof(serverGAddress));
    memset((void *)&clientAddress, 0, sizeof(clientAddress));
    // Impostazione dei valori della struttura sockaddr_in relativa al ServerG
    serverGAddress.sin_family = AF_INET;
    serverGAddress.sin_port = htons(serverGPort);
    serverGAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    // Bind dell'indirizzo del ServerG
    Bind(listenSockFd, (struct sockaddr *)&serverGAddress, sizeof(serverGAddress));
    // Attesa dei client
    Listen(listenSockFd, CLIENT_QUEUE_SIZE);

    fprintf(stdout, "ServerG attivo\n");

    for ( ; ; ) {
        socklen_t len = (socklen_t) sizeof(clientAddress);
        // Accetta una nuova connessione
        connSockFd = Accept(listenSockFd, (struct sockaddr *)&clientAddress, &len);

        // Ricezione dell'operation code, che identifica se la richiesta proviene
        // dal ClientT o dal ClientS
        if ((nleftRead = FullRead(connSockFd, (void *)&operationCode, sizeof(operationCode))) != 0) {
            fprintf(stderr, "Full read error.\n");
            exit(nleftRead);
        }
        if ((nleftWrite = FullWrite(connSockFd, (void *)&operationCode, sizeof(operationCode))) != 0) {
            fprintf(stderr, "Full write error.\n");
            exit(nleftWrite);
        }

        // Creazione processo figlio
        if ((pid = fork()) == -1) {
            perror("fork");
            exit(FORK_ERROR);
        }
        // Processo figlio
        else if (pid == 0) {
            Close(listenSockFd);
            // Connessione con il ServerV
            serverVSockFd = connectWithServerV(serverVPort);

            // A seconda dell'operationCode ricevuto esegue determinate operazioni
            switch (operationCode) {
                // Controllo validità del Green Pass
                case FROM_CLIENT_S:
                    // Lettura codice tessera sanitaria
                    if ((nleftRead = FullRead(connSockFd, (void *)buffer, HEALTH_INSURE_CARD_NUMBER_LENGTH * sizeof(char))) != 0) {
                        fprintf(stderr, "Full read error.\n");
                        exit(nleftRead);
                    }
                    //fprintf(stdout, "\nLetto codice tessera: %s[size: %ld]\n", buffer, strlen(buffer));
                    // Pulizia dei campi del pacchetto
                    memset((void *)&packetToServerVViaClientS, 0, sizeof(packetToServerVViaClientS));
                    // Copia della tessera sanitaria nel primo campo del pacchetto da inviare al ServerV
                    strcpy(packetToServerVViaClientS.healthInsureCardNumber, buffer);
                    //fprintf(stdout, "Operation code: %d\n", operationCode);
                    // Invio codice operazione al ServerV
                    if ((nleftWrite = FullWrite(serverVSockFd, (void *)&operationCode, sizeof(operationCode))) != 0) {
                        fprintf(stderr, "Full write error.\n");
                        exit(nleftWrite);
                    }
                    // Ricezione codice operazione da ServerV
                    if ((nleftRead = FullRead(serverVSockFd, (void *)&operationCode, sizeof(operationCode))) != 0) {
                        fprintf(stderr, "Full read error.\n");
                        exit(nleftRead);
                    }
                    // Invio del pacchetto al ServerV
                    if ((nleftWrite = FullWrite(serverVSockFd, (void *)&packetToServerVViaClientS, sizeof(packetToServerVViaClientS))) != 0) {
                        fprintf(stderr, "Full write error.\n");
                        exit(nleftWrite);
                    }
                    // Ricezione del pacchetto da ServerV
                    if ((nleftRead = FullRead(serverVSockFd, (void *)&packetFromServerV, sizeof(packetFromServerV))) != 0) {
                        fprintf(stderr, "Full read error.\n");
                        exit(nleftRead);
                    }
                    // Inoltro del pacchetto al Client
                    if ((nleftWrite = FullWrite(connSockFd, (void *)&packetFromServerV, sizeof(packetFromServerV))) != 0) {
                        fprintf(stderr, "Full write error.\n");
                        exit(nleftWrite);
                    }
                break;

                // Attivazione / Disattivazione del Green Pass
                case FROM_CLIENT_T:
                    //fprintf(stdout, "Ricevuto pacchetto da ClientT\n");
                    // Lettura pacchetto contenente codice tessera sanitaria e valore di update da ClientT
                    if ((nleftRead = FullRead(connSockFd, (void *)&packetFromClientT, sizeof(packetFromClientT))) != 0) {
                        fprintf(stderr, "Full read error.\n");
                        exit(nleftRead);
                    }
                    //fprintf(stdout, "Handshake\n");
                    // Invio codice operazione al ServerV
                    if ((nleftWrite = FullWrite(serverVSockFd, (void *)&operationCode, sizeof(operationCode))) != 0) {
                        fprintf(stderr, "Full write error.\n");
                        exit(nleftWrite);
                    }
                    // Ricezione codice operazione da ServerV
                    if ((nleftRead = FullRead(serverVSockFd, (void *)&operationCode, sizeof(operationCode))) != 0) {
                        fprintf(stderr, "Full read error.\n");
                        exit(nleftRead);
                    }
                    // Inoltro del pacchetto al ServerV
                    strcpy(packetToServerVViaClientT.healthInsureCardNumber, packetFromClientT.healthInsureCardNumber);
                    packetToServerVViaClientT.updateValue = packetFromClientT.updateValue;
                    //fprintf(stdout, "Inoltro del pacchetto al ServerV\n");
                    if ((nleftWrite = FullWrite(serverVSockFd, (void *)&packetToServerVViaClientT, sizeof(packetToServerVViaClientT))) != 0) {
                        fprintf(stderr, "Full write error.\n");
                        exit(nleftWrite);
                    }
                    // Ricezione del pacchetto da ServerV
                    //fprintf(stdout, "Ricezione del pacchetto da ServerV\n");
                    if ((nleftRead = FullRead(serverVSockFd, (void *)&packetFromServerV, sizeof(packetFromServerV))) != 0) {
                        fprintf(stderr, "Full read error.\n");
                        exit(nleftRead);
                    }
                    //fprintf(stdout, "Inoltro del pacchetto al Client\n");
                    // Inoltro del pacchetto al Client
                    if ((nleftWrite = FullWrite(connSockFd, (void *)&packetFromServerV, sizeof(packetFromServerV))) != 0) {
                        fprintf(stderr, "Full write error.\n");
                        exit(nleftWrite);
                    }
                break;
                
                default:
                    fprintf(stderr, "L'operation code ricevuto non e' valido.\n");
                break;
            }
            exit(0);
        }
        // Processo padre
        else {
            Close(connSockFd);
        }
    }
    Close(listenSockFd);
    exit(0);
}