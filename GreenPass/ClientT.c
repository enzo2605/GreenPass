#include "GreenPass.h"

int main(int argc, char *argv[]) {
    int serverGSockFd;
    unsigned short int serverGPort;
    struct sockaddr_in serverGAddress;
    char healthInsureCardNumber[HEALTH_INSURE_CARD_NUMBER_LENGTH];
    int updateValue;
    int opcode;
    packet packetFromServerG;
    packetGPUpdate packetToServerG;
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
    fprintf(stdout, "Aggiorna lo stato del tuo Green Pass.\nInserisci il codice della tessera sanitaria (20 caratteri): ");
    scanf("%s", healthInsureCardNumber);
    // Controllo correttezza del numero della tessera sanitaria passato come argomento
    checkHealthInsureCardNumber(healthInsureCardNumber);
    // Inserisci update value
    fprintf(stdout, "0 sospendi Green Pass\n1 Riattiva Green Pass\n");
    scanf("%d", &updateValue);
    // Controllo correttezza updateValue
    if (updateValue < 0 || updateValue > 1) {
        fprintf(stdout, "Il valore di update deve essere o 0 o 1. Non puoi inserire altri valori. Riprova.\n");
        exit(1);
    }

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
    //fprintf(stdout, "Numero tessera sanitaria: %s.\n", healthInsureCardNumber);

    opcode = FROM_CLIENT_T;
    // Invio codice operazione
    if ((nleftWrite = FullWrite(serverGSockFd, (void *)&opcode, sizeof(opcode))) != 0) {
        fprintf(stderr, "Full write error.\n");
        exit(nleftWrite);
    }
    if ((nleftRead = FullRead(serverGSockFd, (void *)&opcode, sizeof(opcode))) != 0) {
        fprintf(stderr, "Full read error.\n");
        exit(nleftRead);
    }

    strcpy(packetToServerG.healthInsureCardNumber, healthInsureCardNumber);
    packetToServerG.updateValue = updateValue;
    // Invio codice tessera sanitaria al ServerG ed esito a ServerG
    if ((nleftWrite = FullWrite(serverGSockFd, (void *)&packetToServerG, sizeof(packetToServerG))) != 0) {
        fprintf(stderr, "Full write error.\n");
        exit(nleftWrite);
    }
    // Ricezione pacchetto contenente l'esito della richiesta
    if ((nleftRead = FullRead(serverGSockFd, (void *)&packetFromServerG, sizeof(packetFromServerG))) != 0) {
        fprintf(stderr, "Full read error.\n");
        exit(nleftRead);
    }
    
    // Analizza l'esito derivante dal pacchetto
    if (packetFromServerG.returnedValue == GREEN_PASS_NOT_UPDATED) {
        fprintf(stdout, "Non è stato possibile abilitare o disabilitare il Green Pass associato alla tessera sanitaria numero %s.\n", packetFromServerG.healthInsureCardNumber);
        fprintf(stdout, "E' probabile che non sia associato alcun Green Pass alla tessera sanitaria o che il Green Pass sia scaduto.\n");
        fprintf(stdout, "Verifica lo stato del tuo Green Pass tramite l'apposito client.\n");
    }
    else if (packetFromServerG.returnedValue == GREEN_PASS_UPDATED) {
        fprintf(stdout, "Il Green Pass associato alla tessera sanitaria numero %s e' stato aggiornato con successo.\n", packetFromServerG.healthInsureCardNumber);
    }
    else if (packetFromServerG.returnedValue == GREEN_PASS_ALREADY_ACTIVE) {
        fprintf(stdout, "ATTENZIONE! il Green Pass associato alla tessera sanitaria numero %s risulta già attivo.\n", packetFromServerG.healthInsureCardNumber);
        fprintf(stdout, "Verifica lo stato del tuo Green Pass prima di effettuare qualsiasi operazione.\n");
    }
    else if (packetFromServerG.returnedValue == GREEN_PASS_ALREADY_DISABLED) {
        fprintf(stdout, "ATTENZIONE! il Green Pass associato alla tessera sanitaria numero %s risulta già disattivato.\n", packetFromServerG.healthInsureCardNumber);
        fprintf(stdout, "Verifica lo stato del tuo Green Pass prima di effettuare qualsiasi operazione.\n");
    }

    Close(serverGSockFd);
    exit(0);
}