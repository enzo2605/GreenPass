#include "GreenPass.h"

int main(int argc, char *argv[]) {
    int serverGSockFd;
    int opcode;
    unsigned short int serverGPort;
    struct sockaddr_in serverGAddress;
    char healthInsureCardNumber[HEALTH_INSURE_CARD_NUMBER_LENGTH];
    packet packetFromServerG;
    ssize_t nleftRead, nleftWrite;
    const char ip_loopback[] = "127.0.0.1";

    // Controllo argomenti da linea di comando
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <Porta ServerG>.\n", argv[0]);
        exit(INVALID_ARGUMENTS);
    }
    // Conversione numero di porta del ServerG da stringa ad unsigned short int
    serverGPort = stringToPort(argv[1], NULL, 10);

    // Inserimento codice tessera sanitaria
    fprintf(stdout, "Verifica la validità del tuo Green Pass.\nInserisci il codice della tessera sanitaria (20 caratteri): ");
    scanf("%s", healthInsureCardNumber);
    // Controllo correttezza del numero della tessera sanitaria passato come argomento
    checkHealthInsureCardNumber(healthInsureCardNumber);

    // Apertura del socket
    serverGSockFd = Socket(AF_INET, SOCK_STREAM, 0);
    // Pulizia indirizzo ServerG
    memset((void *)&serverGAddress, 0, sizeof(serverGAddress));
    // Inizializzazione campi della struttura sockaddr_in
    serverGAddress.sin_family = AF_INET;
    serverGAddress.sin_port = htons(serverGPort);
    IPConversion(AF_INET, ip_loopback, &serverGAddress.sin_addr);
    // Connessione al ServerG
    Connect(serverGSockFd, (struct sockaddr *)&serverGAddress, sizeof(serverGAddress));
    fprintf(stdout, "Numero tessera sanitaria: %s.\n", healthInsureCardNumber);

    opcode = FROM_CLIENT_S;
    // Invio codice operazione
    if ((nleftWrite = FullWrite(serverGSockFd, (void *)&opcode, sizeof(opcode))) != 0) {
        fprintf(stderr, "Full write error.\n");
        exit(nleftWrite);
    }
    if ((nleftRead = FullRead(serverGSockFd, (void *)&opcode, sizeof(opcode))) != 0) {
        fprintf(stderr, "Full read error.\n");
        exit(nleftRead);
    }

    // Invio codice tessera sanitaria al ServerG
    if ((nleftWrite = FullWrite(serverGSockFd, (void *)healthInsureCardNumber, HEALTH_INSURE_CARD_NUMBER_LENGTH * sizeof(char))) != 0) {
        fprintf(stderr, "Full write error.\n");
        exit(nleftWrite);
    }
    // Ricezione pacchetto contenente l'esito della richiesta
    if ((nleftRead = FullRead(serverGSockFd, (void *)&packetFromServerG, sizeof(packetFromServerG))) != 0) {
        fprintf(stderr, "Full read error.\n");
        exit(nleftRead);
    }

    // Analizza l'esito derivante dal pacchetto
    if (packetFromServerG.returnedValue == GREEN_PASS_OK) {
        fprintf(stdout, "Il Green Pass associato alla tessera sanitaria numero %s risulta ancora valido.\n", packetFromServerG.healthInsureCardNumber);
    }
    else if (packetFromServerG.returnedValue == GREEN_PASS_NOT_FOUND) {
        fprintf(stdout, "Non è associato alcun Green Pass alla tessera sanitaria numero %s.\n", packetFromServerG.healthInsureCardNumber);
        fprintf(stdout, "Un Green Pass può essere ottenuto solo dopo la vaccinazione.\n");
    }
    else if (packetFromServerG.returnedValue == GREEN_PASS_EXPIRED) {
        fprintf(stdout, "Il Green Pass associato alla tessera sanitaria numero %s risulta scaduto.\n", packetFromServerG.healthInsureCardNumber);
    }
    else if (packetFromServerG.returnedValue == GREEN_PASS_DISABLED) {
        fprintf(stdout, "Il Green Pass associato alla tessera sanitaria numero %s risulta disabilitato.\n", packetFromServerG.healthInsureCardNumber);
    }

    Close(serverGSockFd);
    exit(0);
}