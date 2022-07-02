#include "GreenPass.h"

void checkHealthInsureCardNumber(char *healthInsureCardNumber) {
    size_t length = strlen(healthInsureCardNumber);
    if (length + 1 != HEALTH_INSURE_CARD_NUMBER_LENGTH) {
        fprintf(stderr, "La tessera sanitaria deve contenere esattamente 20 caratteri.\n");
        exit(HEALTH_INSURE_CARD_NUMBER_ERROR);
    }
}

unsigned short int stringToPort(const char *nptr, char **endptr, int base) {
    unsigned short int port;
    // Conversione numero di porta del centro vaccinale da stringa ad unsigned short int
    port = (unsigned short int) strtoul(nptr, endptr, base);
    /**
     * Dopo la conversione andiamo a verificare se si sono verificati degli errori. Per fare questo interroghiamo
     * la variabile globale errno:
     * - EINVAL non è stato possibie effettuare la conversione, la stringa presenta caratteri non supportati
     *   dalla base
     * - ERANGE la conversione ha causato un overflow 
     */
    if (port == 0 && (errno == EINVAL || errno == ERANGE)) {
        perror("strtoul");
        exit(STRTOUL_ERROR);
    }
    return port;
}

int connectWithServerV(unsigned short int serverVPort) {
    int serverVsock;
    struct sockaddr_in serverVAddress;
    const char ip_loopback[] = "127.0.0.1";

    // Apertura del socket
    serverVsock = Socket(AF_INET, SOCK_STREAM, 0);

    // Inizializzazione campi struttura sockaddr_in relativa all'indirizzo di ServerV
    serverVAddress.sin_family = AF_INET;
    serverVAddress.sin_port = htons(serverVPort);
    IPConversion(AF_INET, ip_loopback, &serverVAddress.sin_addr);

    // Connessione al ServerV
    Connect(serverVsock, (struct sockaddr *)&serverVAddress, sizeof(serverVAddress));
    
    return serverVsock;
}

char *getGreenPassValidityDate(void) {
    char *strExpirationVaxDate;
    // Ricaviamo il tempo di sistema in secondi (il numero di secondi dalla mezzanotte del primo gennaio 1970 UTC)
    time_t systemTimeInSec = time(NULL);
    // Ricaviamo la data odierna
    struct tm *expirationVaxDate = localtime((const time_t *)&systemTimeInSec);
    // Per semplicità la data si riferisce al primo giorno del mese
    expirationVaxDate->tm_mday = 1;
    // Calcoliamo la data della prossima vaccinazione
    // Caso in cui la data di vaccinazione è l'anno successivo
    if (expirationVaxDate->tm_mon + 1 + MONTHS_FOR_NEXT_VACCINATION > MONTHS_IN_A_YEAR) {
        expirationVaxDate->tm_mon = (expirationVaxDate->tm_mon + 1 + MONTHS_FOR_NEXT_VACCINATION) % (MONTHS_IN_A_YEAR);
        expirationVaxDate->tm_year += 1;
    }
    else {
        expirationVaxDate->tm_mon = expirationVaxDate->tm_mon + MONTHS_FOR_NEXT_VACCINATION + 1;
    }
    // Salvataggio del valore calcolato in una stringa
    strExpirationVaxDate = (char *)calloc(DATE_LENGTH, sizeof(char));
    if (strExpirationVaxDate == NULL) {
        perror("calloc");
        exit(CALLOC_ERROR);
    }
    sprintf(strExpirationVaxDate, "%02d-%02d-%d", expirationVaxDate->tm_mday, expirationVaxDate->tm_mon, 1900 + expirationVaxDate->tm_year);
    return strExpirationVaxDate;
}

struct tm *dateToBDTstruct(char *date) {
    struct tm *res;
    char *token;
    const char *delim = "-";
    int i = 0;
    int dateInt[3];

    // Allocazione struct di ritorno
    res = (struct tm *)calloc(1, sizeof(struct tm));
    if (res == NULL) {
        perror("calloc error");
        exit(1);
    }

    // Prendi il primo token
    token = strtok(date, delim);
    // Analizza tutti gli altri token
    while (token != NULL) {
        // Converti il token in intero è salvalo nell'arra dateInt
        dateInt[i++] = atoi(token);
        token = strtok(NULL, delim);
    }

    // Valorizza i campi della struct di ritorno
    res->tm_mday = dateInt[0];
    res->tm_mon = dateInt[1];
    res->tm_year = dateInt[2];
    return res;
}

int checkGreenPassValidity(const char *healthInsureCardNumber) {
    FILE *filePointer;
    packet buffRead;
    int validity;
    int healthCardNumberFound = 0;
    struct tm *greenPassValidityBDTFormat, *todaysDateBDTFormat;
    time_t systemTimeInSec;

    // Spostiamoci sulla cartella database, se non ci troviamo già in tale cartella
    Chdir(dirName);
    
    // Apertura file in lettura
    if ((filePointer = fopen(fileName, "r")) == NULL) {
        perror("fopen error");
        exit(FOPEN_ERROR);
    }
    
    // Scorriamo il file riga per riga
    while ((fscanf(filePointer, "%s %s %d", buffRead.healthInsureCardNumber, buffRead.greenPassValidityDate, &validity)) != EOF) {
        // Lettura di una riga
        // Se il codice della tessera è già memorizzato nel file
        if (strcmp(buffRead.healthInsureCardNumber, healthInsureCardNumber) == 0) {
            healthCardNumberFound = 1;
            // Ricaviamo la data odierna in formato break down time
            // Ricaviamo il tempo di sistema in secondi (il numero di secondi dalla mezzanotte del primo gennaio 1970 UTC)
            systemTimeInSec = time(NULL);
            // Ricaviamo la data odierna
            todaysDateBDTFormat = localtime((const time_t *)&systemTimeInSec);
            todaysDateBDTFormat->tm_mon += 1;
            todaysDateBDTFormat->tm_year += 1900;

            // Ricaviamo la data di validità in formato break down time
            greenPassValidityBDTFormat = dateToBDTstruct(buffRead.greenPassValidityDate);

            // Controllo a cascata sulla data
            // Controllo sull'anno
            if ((greenPassValidityBDTFormat->tm_year - todaysDateBDTFormat->tm_year) > 0) {
                return (validity == 0) ? GREEN_PASS_DISABLED : GREEN_PASS_OK;
            }
            // A parità di anno
            else if ((greenPassValidityBDTFormat->tm_year - todaysDateBDTFormat->tm_year) == 0) {
                // Controllo sul mese
                if ((greenPassValidityBDTFormat->tm_mon - todaysDateBDTFormat->tm_mon) > 0) {
                    return (validity == 0) ? GREEN_PASS_DISABLED : GREEN_PASS_OK;
                }
                // A parità di mese
                else if ((greenPassValidityBDTFormat->tm_mon - todaysDateBDTFormat->tm_mon) == 0) {
                    // Controllo sul giorno
                    if ((greenPassValidityBDTFormat->tm_mday - todaysDateBDTFormat->tm_mday) > 0) {
                        return (validity == 0) ? GREEN_PASS_DISABLED : GREEN_PASS_OK;
                    }
                }
            }
        }
        // Se il numero della tessera sanitaria è stato trovato, non ha senso continuare a scorrere il file
        if (healthCardNumberFound) {
            break;
        }
    }
    fclose(filePointer);
    // Se non c'è nessun green pass associato all'utente
    if (!healthCardNumberFound) {
        return GREEN_PASS_NOT_FOUND;
    }
    // In tutti gli altri casi
    return GREEN_PASS_EXPIRED;
}

void updateGreenPassValidity(const char *healthInsureCardNumber, int newValue) {
    FILE *filePointer, *tmpFilePointer;
    packet buffRead;
    int validity;
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
     * temporeno TempFile.txt. Copiamo in esso tutte le tuple del file GreenPassDB.txt e aggiorniamo il valore (0 o 1) 
     * di validità associata al green pass che deve essere aggiornato. Successivamente, cancelliamo il vecchio 
     * file GreenPassDB.txt e rinominiamo il file TempFile.txt in GreenPassDB.txt.
     */
    while (!feof(filePointer)) {
        fscanf(filePointer, "%s %s %d", buffRead.healthInsureCardNumber, buffRead.greenPassValidityDate, &validity);
        if (strcmp(buffRead.healthInsureCardNumber, healthInsureCardNumber) == 0){
            // Aggiornamento valore
            validity = newValue;
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
}