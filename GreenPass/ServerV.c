#include "GreenPass.h"

int main(int argc, char *argv[]) {
    // Variabili per la gestione della connessione
    int listenSockFd, connSockFd;
    unsigned short int serverVPort;
    int max_fd, active_fd, ith_fd, operationCode;
    struct sockaddr_in serverVAddress, clientAddress;
    socklen_t clientLen;

    // Variabili per la gestione del server iterativo
    fd_set fset;
    char buffer[MAXLINE];
    char fd_open[FD_SETSIZE];

    // Pacchetti con cui lavora il ServerV
    packet packetFromCentroVaccinale, packetToCentroVaccinale;
    packetGPUpdate packetFromClientT;
    packet packetToClientT;
    packet packetFromClientS, packetToClientS;

    // Variabili utilizzate dai client
    ssize_t nleftWrite, nleftRead;
    FILE *filePointer, *tmpFilePointer;
    packet buffRead;
    int validity;
    int returnValue;

    // Controllo argomenti da linea di comando
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <Porta ServerV>.\n", argv[0]);
        exit(INVALID_ARGUMENTS);
    }
    // Conversione numero di porta del centro vaccinale da stringa ad unsigned short int
    serverVPort = stringToPort(argv[1], NULL, 10);

    // Apertura del socket
    listenSockFd = Socket(AF_INET, SOCK_STREAM, 0);
    // Pulizia degli indirizzi relativi al centro vaccinale e al client
    memset((void *)&serverVAddress, 0, sizeof(serverVAddress));
    memset((void *)&clientAddress, 0, sizeof(clientAddress));
    // Impostazione dei valori della struttura sockaddr_in relativa al centro vaccinale
    serverVAddress.sin_family = AF_INET;
    serverVAddress.sin_port = htons(serverVPort);
    serverVAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    // Bind dell'indirizzo del centro vaccinale
    Bind(listenSockFd, (struct sockaddr *)&serverVAddress, sizeof(serverVAddress));
    // Attesa dei client
    Listen(listenSockFd, CLIENT_QUEUE_SIZE);
    
    memset(fd_open, 0, FD_SETSIZE); // pulizia array dei descrittori aperti
    max_fd = listenSockFd;          // il descrittore della socket è il nuovo massimo
    fd_open[max_fd] = 1;

    fprintf(stdout, "ServerV attivo\n");
    // Ciclo principale, attesa di connessione e dati mediante select
    for ( ; ; ) {
        // Inizializzazione dei descrittori che dovranno essere monitorati
        for (ith_fd = listenSockFd; ith_fd <= max_fd; ith_fd++) {
            if (fd_open[ith_fd] != 0) {
                FD_SET(ith_fd, &fset);
            }
        }
        //fprintf(stdout, "Attesa di connessione o invio dati\n");
        // Attesa di connessione o invio dati
        while (((active_fd = select(max_fd + 1, &fset, NULL, NULL, NULL)) < 0) && (errno == EINTR));
        if (active_fd < 0) {
            perror("select");
            exit(SELECT_ERROR);
        }
        //fprintf(stdout, "Se c'è una nuova connessione\n");
        // Se c'è una nuova connessione
        if (FD_ISSET(listenSockFd, &fset)) {
            active_fd--; // Decrementa il numero di descrittori attivi
            clientLen = sizeof(clientAddress);
            connSockFd = Accept(listenSockFd, (struct sockaddr *)&clientAddress, &clientLen);
            fd_open[connSockFd] = 1; // Segna nell'array fd_open che il descrittore connSockFd è aperto
            // Imposta il nuovo massimo, se necessario
            if (max_fd < connSockFd) {
                max_fd = connSockFd;
            }
        }
        // Ciclo per monitorare gli altri descrittori
        ith_fd = listenSockFd;
        while (active_fd != 0) {
            ith_fd++;
            // Se l'i-esimo descrittore è chiuso, portati sul successivo descrittore
            if (fd_open[ith_fd] == 0) {
                continue;
            }
            if (FD_ISSET(ith_fd, &fset)) {
                active_fd--;
                //fprintf(stdout, "Attesa invio codice\n");
                nleftRead = FullRead(ith_fd, (void *)&operationCode, sizeof(operationCode));
                // Se la connessione viene interrotta
                if (nleftRead != 0) {
                    fd_open[ith_fd] = 0; // Segna il descrittore come chiuso
                    // Se il descrittore chiuso era il massimo
                    if (max_fd == ith_fd) {
                        // Calcola il nuo massimo
                        while (fd_open[--ith_fd] == 0);
                        max_fd = ith_fd;
                        break;
                    }
                    continue;
                }
                //fprintf(stdout, "Operation code: %d\n", operationCode);
                // Inoltro del codice operazione al CentroVaccinale
                if ((nleftWrite = FullWrite(ith_fd, (void *)&operationCode, sizeof(operationCode))) != 0) {
                    fprintf(stderr, "Full write error.\n");
                    continue;
                }
                // In base al codice ricevuto effettua una determinata azione
                switch (operationCode) {
                    case FROM_CENTRO_VACCINALE:
                        // Ricezione del pacchetto da CentroVaccinale
                        if ((nleftRead = FullRead(ith_fd, (void *)&packetFromCentroVaccinale, sizeof(packetFromCentroVaccinale))) != 0) {
                            fprintf(stderr, "Full read error.\n");
                            continue;
                        }
                        // Copia della tessera sanitaria e della data di validità nel pacchetto di ritorno
                        packetToCentroVaccinale = packetFromCentroVaccinale;
                        // Spostiamoci sulla cartella database, se non ci troviamo già in tale cartella
                        Chdir(dirName);
                        // Verifica la validità del green pass
                        returnValue = checkGreenPassValidity(packetFromCentroVaccinale.healthInsureCardNumber);
    
                        // Green pass non presente nella tabella
                        if (returnValue == GREEN_PASS_NOT_FOUND) {
                            if ((filePointer = fopen(fileName, "a")) == NULL) {
                                perror("fopen error");
                                exit(1);
                            }
                            // Scrivi dati green pass nel file nella forma
                            // NUMERO TESSERA SANITARIA DATA SCADENZA GREEN PASS VALIDITA' GREEN PASS
                            fprintf(filePointer, "%s %s %d\n", packetFromCentroVaccinale.healthInsureCardNumber, packetFromCentroVaccinale.greenPassValidityDate, 1);
                            fclose(filePointer);
                            //fprintf(stdout, "Scrittura effettuata.\n");
                            packetToCentroVaccinale.returnedValue = VACCINE_OK;
                        }
                        // Green pass ancora valido
                        else if (returnValue == GREEN_PASS_OK || returnValue == GREEN_PASS_DISABLED) {
                            //fprintf(stdout, "Non è stato possibile effettuare la vaccinazione. Devono passare ancora 5 mesi.\n");
                            packetToCentroVaccinale.returnedValue = VACCINE_ALREADY_DONE;
                        }
                        // Green pass scaduto
                        else if (returnValue == GREEN_PASS_EXPIRED) {
                            //fprintf(stdout, "Aggiornamento Green Pass.\n");
                            // Apertura file database
                            if ((filePointer = fopen(fileName, "r")) == NULL) {
                                perror("fopen error");
                                exit(1);
                            }
                            // Apertura file temporaneo
                            if ((tmpFilePointer = fopen(tmpFileName, "w")) == NULL) {
                                perror("fopen error");
                                exit(1);
                            }
                            /**
                             * Per aggiornare la data di validità relativa ad una specifica tupla nella tabella sfruttiamo il file 
                             * temporeno TempFile.txt. Copiamo in esso tutte le tuple del file GreenPassDB.txt e aggiorniamo la data 
                             * di validità associata al green pass che deve essere aggiornato. Successivamente, cancelliamo il vecchio 
                             * file GreenPassDB.txt e rinominiamo il file TempFile.txt in GreenPassDB.txt.
                             */
                            while (!feof(filePointer)) {
                                fscanf(filePointer, "%s %s %d", buffRead.healthInsureCardNumber, buffRead.greenPassValidityDate, &validity);
                                if (strcmp(buffRead.healthInsureCardNumber, packetFromCentroVaccinale.healthInsureCardNumber) == 0){
                                    // Aggiornamento data
                                    strcpy(buffRead.greenPassValidityDate, packetFromCentroVaccinale.greenPassValidityDate);
                                    validity = 1;
                                }
                                // Copia dei dati nel file temporaneo
                                if (!feof(filePointer)) {
                                    fprintf(tmpFilePointer, "%s %s %d\n", buffRead.healthInsureCardNumber, buffRead.greenPassValidityDate, validity);
                                }
                            }
                            // Chiusura dei file pointer
                            fclose(filePointer);
                            fclose(tmpFilePointer);
                            // Rimozione del file GreenPassDB.txt
                            Remove(fileName);
                            // Rinominiamo il file TempFile.txt in GreenPassDB.txt
                            Rename(tmpFileName, fileName);
                            packetToCentroVaccinale.returnedValue = VACCINE_OK;
                        }
                        // Inoltro pacchetto al CentroVaccinale
                        if ((nleftWrite = FullWrite(ith_fd, (void *)&packetToCentroVaccinale, sizeof(packetToCentroVaccinale))) != 0) {
                            fprintf(stderr, "Full write error.\n");
                            continue;
                        }
                    break;
                    
                    case FROM_CLIENT_S:
                        // Ricezione del pacchetto da ServerG
                        if ((nleftRead = FullRead(ith_fd, (void *)&packetFromClientS, sizeof(packetFromClientS))) != 0) {
                            fprintf(stderr, "Full read error.\n");
                            continue;
                        }
                        packetToClientS = packetFromClientS;
                        // Verifica validità del Green Pass
                        packetToClientS.returnedValue = checkGreenPassValidity(packetFromClientS.healthInsureCardNumber);
                        // Inoltro pacchetto al ServerG
                        if ((nleftWrite = FullWrite(ith_fd, (void *)&packetToClientS, sizeof(packetToClientS))) != 0) {
                            fprintf(stderr, "Full write error.\n");
                            continue;
                        }
                    break;

                    case FROM_CLIENT_T:
                        // Ricezione del pacchetto da ServerG
                        if ((nleftRead = FullRead(ith_fd, (void *)&packetFromClientT, sizeof(packetFromClientT))) != 0) {
                            fprintf(stderr, "Full read error.\n");
                            continue;
                        }
                        strcpy(packetToClientT.healthInsureCardNumber, packetFromClientT.healthInsureCardNumber);
                        // Spostiamoci sulla cartella database, se non ci troviamo già in tale cartella
                        Chdir(dirName);

                        returnValue = checkGreenPassValidity(packetFromClientT.healthInsureCardNumber);

                        if (returnValue == GREEN_PASS_NOT_FOUND || returnValue == GREEN_PASS_EXPIRED) {
                            packetToClientT.returnedValue = GREEN_PASS_NOT_UPDATED;
                        }
                        else if (returnValue == GREEN_PASS_DISABLED) {
                            // Se il valore di update è 0 significa che si sta provando a disattivare un 
                            // green pass già disattivato
                            if (packetFromClientT.updateValue == 0) {
                                packetToClientT.returnedValue = GREEN_PASS_ALREADY_DISABLED;
                            }
                            // Riattiva il Green Pass
                            else {
                                //fprintf(stdout, "Annullamento validità Green Pass.\n");
                                updateGreenPassValidity(packetFromClientT.healthInsureCardNumber, packetFromClientT.updateValue);
                                packetToClientT.returnedValue = GREEN_PASS_UPDATED;
                            }
                        }
                        else if (returnValue == GREEN_PASS_OK) {
                            // Se il valore di update è 1 significa che si sta provando a riattivare un 
                            // green pass già attivo
                            if (packetFromClientT.updateValue == 1) {
                                packetToClientT.returnedValue = GREEN_PASS_ALREADY_ACTIVE;
                            }
                            // Annulla validità Green Pass
                            else {
                                //fprintf(stdout, "Aggiornamento validità Green Pass.\n");
                                updateGreenPassValidity(packetFromClientT.healthInsureCardNumber, packetFromClientT.updateValue);
                                packetToClientT.returnedValue = GREEN_PASS_UPDATED;
                            }
                        }

                        // Inoltro pacchetto al ServerG
                        if ((nleftWrite = FullWrite(ith_fd, (void *)&packetToClientT, sizeof(packetToClientT))) != 0) {
                            fprintf(stderr, "Full write error.\n");
                            continue;
                        }
                    break;
                    
                    default:
                        fprintf(stderr, "L'operation code ricevuto non e' valido.\n");
                    break;
                }
            }
        }
    }
    exit(0);
}