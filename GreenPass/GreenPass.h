/**
 * GreenPass.h
 * Created by Vincenzo Iannucci
 * 
 * La libreria GreenPass.h definisce la struttura dei pacchetti, i valori di ritorno associati ad 
 * ogni pacchetto e una serie di costanti e funzioni che sono di comune utilità per i file che 
 * implementano la libreria.
 * 
 * */

#ifndef GreenPass_h
#define GreenPass_h

#include "LabUtilities.h"

#define HEALTH_INSURE_CARD_NUMBER_LENGTH 21
#define MONTHS_FOR_NEXT_VACCINATION 5
#define MONTHS_IN_A_YEAR 12
#define DATE_LENGTH 11

static const char *fileName = "GreenPassDB.txt", *tmpFileName = "TempFile.txt", *dirName = "database";

// Pacchetti coinvolti nello scambio dati
typedef struct {
    char healthInsureCardNumber[HEALTH_INSURE_CARD_NUMBER_LENGTH];
    char greenPassValidityDate[DATE_LENGTH];
    int returnedValue;
} packet;

typedef struct {
    char healthInsureCardNumber[HEALTH_INSURE_CARD_NUMBER_LENGTH];
    unsigned int updateValue;
} packetGPUpdate;

// I vari enum rappresentano i valori di ritorno di ciascuna operazione
// che viene eseguita
enum returnValueVaccination {
    VACCINE_ALREADY_DONE,
    VACCINE_OK
};

enum returnValueGreenPassCheck {
    GREEN_PASS_EXPIRED,
    GREEN_PASS_OK,
    GREEN_PASS_NOT_FOUND,
    GREEN_PASS_DISABLED
};

enum returnValueGreenPassUpdate {
    GREEN_PASS_NOT_UPDATED,
    GREEN_PASS_UPDATED,
    GREEN_PASS_ALREADY_ACTIVE,
    GREEN_PASS_ALREADY_DISABLED
};

// Questo enum permette al ServerV di capire da
// quale client proviene la richiesta
enum opCode {
    FROM_CENTRO_VACCINALE,
    FROM_CLIENT_T,
    FROM_CLIENT_S
};

// Verifica la validità del numero della tessera sanitaria
void checkHealthInsureCardNumber(char *healthInsureCardNumber);

// Converte una stringa in numero di porta
unsigned short int stringToPort(const char *nptr, char **endptr, int base);

// Effettua la connessione con il ServerV
int connectWithServerV(unsigned short int serverVPort);

// Ritorna la data di validità del Green Pass in stringa e nel formato DD-MM-YYYY
char *getGreenPassValidityDate(void);

// Ritorna una data in formato break down time
struct tm *dateToBDTstruct(char *date);

// Verifica la validità di un Green Pass associato ad una tessera sanitaria
int checkGreenPassValidity(const char *healthInsureCardNumber);

// Attiva o disattiva un Green Pass associato ad una tessera sanitaria
void updateGreenPassValidity(const char *healthInsureCardNumber, int newValue);

#endif