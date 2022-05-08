#include "GreenPass.h"

int main(int argc, char *argv[]) {
    int listenSockFd, connSockFd, serverVSockFd;
    unsigned short int centroVaccinalePort, serverVPort;
    int operationCode;
    struct sockaddr_in centroVaccinaleAddress, clientAddress;
    pid_t pid;
    char buffer[HEALTH_INSURE_CARD_NUMBER_LENGTH];
    ssize_t nleftWrite, nleftRead;
    packet packetToServerV, packetFromServerV;

    // Controllo argomenti da linea di comando
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <Porta Centro Vaccinale> <Porta ServerV>.\n", argv[0]);
        exit(INVALID_ARGUMENTS);
    }

    // Conversione numero di porta del centro vaccinale da stringa ad unsigned short int
    centroVaccinalePort = stringToPort(argv[1], NULL, 10);

    // Conversione numero di porta del ServerV da stringa ad unsigned short int
    serverVPort = stringToPort(argv[2], NULL, 10);

    // Apertura del socket
    listenSockFd = Socket(AF_INET, SOCK_STREAM, 0);
    // Pulizia degli indirizzi relativi al centro vaccinale e al client
    memset((void *)&centroVaccinaleAddress, 0, sizeof(centroVaccinaleAddress));
    memset((void *)&clientAddress, 0, sizeof(clientAddress));
    // Impostazione dei valori della struttura sockaddr_in relativa al centro vaccinale
    centroVaccinaleAddress.sin_family = AF_INET;
    centroVaccinaleAddress.sin_port = htons(centroVaccinalePort);
    centroVaccinaleAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    // Bind dell'indirizzo del centro vaccinale
    Bind(listenSockFd, (struct sockaddr *)&centroVaccinaleAddress, sizeof(centroVaccinaleAddress));
    // Attesa dei client
    Listen(listenSockFd, CLIENT_QUEUE_SIZE);

    fprintf(stdout, "Centro vaccinale attivo\n");

    for ( ; ; ) {
        socklen_t len = (socklen_t) sizeof(clientAddress);

        // Accetta una nuova connessione
        connSockFd = Accept(listenSockFd, (struct sockaddr *)&clientAddress, &len);

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

            // Lettura codice tessera sanitaria
            if ((nleftRead = FullRead(connSockFd, (void *)buffer, HEALTH_INSURE_CARD_NUMBER_LENGTH * sizeof(char))) != 0) {
                fprintf(stderr, "Full read error.\n");
                exit(nleftRead);
            }
            
            // Pulizia dei campi del pacchetto
            memset((void *)&packetToServerV, 0, sizeof(packetToServerV));
            // Copia della tessera sanitaria nel primo campo del pacchetto da inviare al ServerV
            strcpy(packetToServerV.healthInsureCardNumber, buffer);
            // Calcolo periodo di validità del Green Pass
            char *greenPassExpDate = getGreenPassValidityDate();
            // Copia del valore nel secondo campo del pacchetto da inviare al ServerV
            strcpy(packetToServerV.greenPassValidityDate, greenPassExpDate);
            // Impostiamo il codice dell'operazione
            // In modo che il ServerV possa discriminare il mittente del pacchetto
            operationCode = FROM_CENTRO_VACCINALE;
            
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
            if ((nleftWrite = FullWrite(serverVSockFd, (void *)&packetToServerV, sizeof(packetToServerV))) != 0) {
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