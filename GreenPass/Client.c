#include "GreenPass.h"

int main(int argc, char *argv[]) {
    int centroVaccinalesockfd;
    unsigned short int centroVaccinalePort;
    struct sockaddr_in centroVaccinaleAddress;
    char healthInsureCardNumber[HEALTH_INSURE_CARD_NUMBER_LENGTH];
    packet packetFromCentroVaccinale;
    ssize_t nleftRead, nleftWrite;
    const char ip_loopback[] = "127.0.0.1";

    // Controllo argomenti da linea di comando
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <Porta Centro Vaccinale>.\n", argv[0]);
        exit(INVALID_ARGUMENTS);
    }
    // Conversione numero di porta del centro vaccinale da stringa ad unsigned short int
    centroVaccinalePort = stringToPort(argv[1], NULL, 10);

    // Inserimento codice tessera sanitaria
    fprintf(stdout, "Inserisci il codice della tessera sanitaria (20 caratteri): ");
    scanf("%s", healthInsureCardNumber);
    // Controllo correttezza del numero della tessera sanitaria passato come argomento
    checkHealthInsureCardNumber(healthInsureCardNumber);

    // Apertura del socket
    centroVaccinalesockfd = Socket(AF_INET, SOCK_STREAM, 0);
    // Pulizia indirizzo centro vaccinale
    memset((void *)&centroVaccinaleAddress, 0, sizeof(centroVaccinaleAddress));
    // Inizializzazione campi della struttura sockaddr_in
    centroVaccinaleAddress.sin_family = AF_INET;
    centroVaccinaleAddress.sin_port = htons(centroVaccinalePort);
    IPConversion(AF_INET, ip_loopback, &centroVaccinaleAddress.sin_addr);
    // Connessione al centro vaccinale
    Connect(centroVaccinalesockfd, (struct sockaddr *)&centroVaccinaleAddress, sizeof(centroVaccinaleAddress));
    fprintf(stdout, "Benvenuti al centro vaccinale. \nNumero tessera sanitaria: %s.\n", healthInsureCardNumber);

    // Invio codice tessera sanitaria al centro vaccinale
    if ((nleftWrite = FullWrite(centroVaccinalesockfd, (void *)healthInsureCardNumber, HEALTH_INSURE_CARD_NUMBER_LENGTH * sizeof(char))) != 0) {
        fprintf(stderr, "Full write error.\n");
        exit(nleftWrite);
    }
    // Ricezione pacchetto contenente l'esito della richiesta
    if ((nleftRead = FullRead(centroVaccinalesockfd, (void *)&packetFromCentroVaccinale, sizeof(packetFromCentroVaccinale))) != 0) {
        fprintf(stderr, "Full read error.\n");
        exit(nleftRead);
    }
    
    // Analizza l'esito derivante dal pacchetto
    if (packetFromCentroVaccinale.returnedValue == VACCINE_ALREADY_DONE) {
        fprintf(stdout, "\nNon e' possibile richiedere un'altra dose di vaccino.\n");
        fprintf(stdout, "Devono passare 5 mesi da quando e' stata effettuata la dose.\n");
        fprintf(stdout, "Data in cui potrai effettuare un'altra dose: %s\n", packetFromCentroVaccinale.greenPassValidityDate);
    }
    else if (packetFromCentroVaccinale.returnedValue == VACCINE_OK) {
        fprintf(stdout, "\nVaccinazione effettuata con successo.\n");
        fprintf(stdout, "Data in cui potrai effettuare un'altra dose: %s\n", packetFromCentroVaccinale.greenPassValidityDate);
    }

    Close(centroVaccinalesockfd);
    exit(0);
}