//
// Created by patryklin on 5/30/18.
//
#include "phone_forward.h"
#include "phone_forward_interface.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>


/**
 * Globalny licznik wczytanych bitów
 */
size_t number_of_bytes = 1;

/**
 * Domyslnie ustawione jako 0
 */

#define DEBUG_TEST 0

/**
 * Wywoluje fprintf dla zadanych argumentow gdy DEBUG_TEST = 1, nie robi nic w p.p.
 */
#define DEBUG_PRINT(...) \
            do { if (DEBUG_TEST) fprintf(stderr, __VA_ARGS__); } while (0)

/**
 * Straznik używany do sprawdzenia powodzenia alokacji pamięci.
 * Przy niepowodzeniu zwalnia zaalokowaną pamięc, oraz wypisuje komunikat o błędzie.
 */
#define NOT_NULL(f)                                     \
    do {                                                \
        if ((f) == 0){                                  \
            clearMemory();                              \
            printMemoryError();                         \
            }                                           \
        }                                               \
    while (0)

/**
 * @brief Rozmiar tablicy baz przekierowań.
 * Zgodnie z założeniami zadania, wystarczy możliwość obsługi 100 baz danych.
 */
#define INITIAL_DATABASE_ARRAY_SIZE 128

/**
 * Początkowy rozmiar tablicy char* używanej do zapisania numeru lub identyfikatora.
 */
#define INITIAL_NUMBER_SIZE 16


char const AT_SIGN = '@';
char const QMARK = '?';
char const R_ARROW = '>';
char const DOLLAR = '$';

char const *AT_SIGN_STR = "@";
char const *QMARK_STR = "?";
char const *R_ARROW_STR = ">";
char const *DOLLAR_STR = "$";

char const *DEL = "DEL";
char const *NEW = "NEW";

/**
 * Globalna tablica baz przekierowań.
 */
Database *Db_Array[INITIAL_DATABASE_ARRAY_SIZE];

/**
 * Obecnie uzywana struktura baz danych.
 */
Database *current_db;


/**
 * Przechowuje argumenty oraz typ aktualnie wykonywanej operacji.
 * Globalna, ze względu na ułatwione zwalnianie pamięci.
 */
Command command;

/**
 * Zwalnia pamięc zaalokowaną w trakcie działania programu.
 */
void clearMemory() {
    for (size_t i = 0; i < INITIAL_DATABASE_ARRAY_SIZE; i++) {
        if ((Db_Array[i]) != NULL) {
            phfwdDelete((Db_Array[i])->db);
            free((void *) (Db_Array[i])->name);
            free((void *) (Db_Array[i]));
            Db_Array[i] = NULL;
        }
    }
    if (command.arg1 != NULL)
        free((void *) (command.arg1));

    if (command.arg2 != NULL)
        free((void *) (command.arg2));
}

/**
 * Wypisuje informację o błędzie składniowym, oraz kończąca pracę programu.
 * @param n numer znaku powodującego błąd interpretaji.
 */
static inline void printSyntaxError(size_t n) {
    fprintf(stderr, "ERROR %ld\n", n);
    clearMemory();
    exit(1);
}

/**
 * Wypisuje informację o problemie z alokacją pamięci, oraz kończąca pracę programu.
 */
static inline void printMemoryError() {
    fprintf(stderr, "MEMORY ERROR\n");
    clearMemory();
    exit(1);
}

/**
 * Wypisuje informację o błędzie wykonania operacji, oraz kończąca pracę programu.
 * @param n Numer pierwszego znaku operatora
 * @param operator Nazwa operatora
 */
static inline void printOperatorError(size_t n, const char *operator) {
    fprintf(stderr, "ERROR %s %ld\n", operator, n);
    clearMemory();
    exit(1);
}

/**
 * Wypisuje informację o niespodziewanym końcu danych, oraz kończąca pracę programu.
 */
static inline void printEofError() {
    fprintf(stderr, "ERROR EOF\n");
    clearMemory();
    exit(1);
}

/**
 * Wypisuje typ komendy, oraz jej argumenty.
 * Używana przy debuggowaniu.
 */
void debugPrintCommand() {
    DEBUG_PRINT("Typ komendy: %d\n", command.type);
    DEBUG_PRINT("Argument 1: %s\n", command.arg1);
    DEBUG_PRINT("Argument 1 len: %ld\n", command.arg1_length);

    DEBUG_PRINT("Argument 2: %s\n", command.arg2);
}

/**
 * Inicjalizuje strukturę Command.
 * Zwalnia pamięc obu jej argumentów, oraz inicjalizuje pola danych.
 * @param command
 */
void resetCommand() {
    free((void *) ((command).arg1));
    free((void *) ((command).arg2));
    (command).arg1 = NULL;
    (command).arg2 = NULL;

    (command).type = IGNORE;
    (command).arg1_length = 0;
    (command).first_read_byte = 0;
}

/**
 * Sprawdza czy c == EOF, aby wypisac poprawny komunikat o błędzie.
 * @param c Sprawdzany znak.
 */
static inline void checkCorrectError(char c) {
    if (c == EOF) {
        printEofError();
    } else {
        printSyntaxError(number_of_bytes);
    }
}

/**
 * Wrapper dla funkcji ungetc, jednoczesnie obnizający liczbę wczytanych znaków.
 * @param c oddawany znak
 */
static inline void ungetChar(char c) {
    ungetc(c, stdin);
    number_of_bytes--;
}

/**
 * Wrapper dla funkcji getchar, jednoczesnie zwiększający liczbę wczytanych znaków.
 */
static inline char readByte() {
    number_of_bytes++;
    return getchar();
}

/**
 * Pomija znaki następujące aż do najbliższej pary "$$"
 */
void ignoreComment() {
    DEBUG_PRINT("Poczatek komentarza\n");

    // Zakładam że wczytany jest już pierwszy znak '$'
    // oznacza to ze musi nastapic po nim kolejny
    char c = readByte();
    char d = 0;
    if (c == EOF)
        printEofError();

    else if (c != DOLLAR)
        printSyntaxError(number_of_bytes - 2);

    c = readByte();
    bool proceed = true;
    while (proceed && c != EOF) {
        if (c == DOLLAR)
            d = c;
        else
            d = 0;

        c = readByte();
        if (c == DOLLAR && d == DOLLAR)
            proceed = false;
    }

    if (c == EOF)
        printEofError();
    else if (c != DOLLAR)
        printSyntaxError(number_of_bytes);

    DEBUG_PRINT("Koniec komentarza\n");
}

/**
 * Pomija wszystkie białe znaki, włącznie z komentarzami.
 */
void ignoreWhiteSpaces() {
    char c = 0;
    bool proceed = true;
    while (proceed) {
        c = readByte();

        if (c == DOLLAR)
            ignoreComment();
        else if (!isspace(c))
            proceed = false;
    }
    // po napotkaniu czegos co nie jest bialym znakiem, oddajemy do strumienia
    ungetChar(c);
}

/**
 * Zapisuje identyfikator ze standardowego wejścia do tablicy char.
 * @return Wskaźnik na tablicę char przechowującą wczytany numer, lub NULL jeżeli był on błędny.
 */
char *readNumber() {
    int c;
    char *str;
    size_t len = 0;
    size_t size = INITIAL_NUMBER_SIZE;
    NOT_NULL(str = realloc(NULL, sizeof(char) * size));

    while (EOF != (c = readByte()) && isDigitWrapper(c)) {
        str[len++] = c;
        if (len == size)
            NOT_NULL(str = realloc(str, sizeof(char) * (size *= 2)));
    }
    ungetChar(c);

    if (len == 0) {
        if (c == EOF)
            printEofError();

        printSyntaxError(number_of_bytes);
    }

    str[len++] = '\0';
    NOT_NULL(str = realloc(str, sizeof(char) * len));
    return str;
}

/**
 * Zapisuje identyfikator ze standardowego wejścia do tablicy char.
 * @return Wskaźnik na tablicę char przechowującą wczytany identyfikator, lub NULL jeżeli był on błędny.
 */
char *readIdentifier(size_t (*len)) {
    int c;
    char *str;
    (*len) = 0;
    size_t size = INITIAL_NUMBER_SIZE;
    NOT_NULL(str = realloc(NULL, sizeof(char) * size));

    c = readByte();

    if (!isalpha(c)) {
        if (c == EOF)
            printEofError();

        printSyntaxError(number_of_bytes - 1);
    }


    str[(*len)++] = c;
    while (EOF != (c = readByte()) && isalnum(c)) {
        str[(*len)++] = c;
        if ((*len) == size)
            NOT_NULL(str = realloc(str, sizeof(char) * (size *= 2)));
    }
    ungetChar(c);

    if ((*len) == 0) {
        if (c == EOF)
            printEofError();

        printSyntaxError(number_of_bytes);
    }
    str[(*len)++] = '\0';

    if (strcmp(str, DEL) == 0 || strcmp(str, NEW) == 0)
        printSyntaxError(number_of_bytes - 3);

    NOT_NULL(str = realloc(str, sizeof(char) * (*len)));

    return str;
}


/**
 * Zapisuje ciag znakow ze standardowego wejścia do tablicy char.
 * @return Wskaźnik na tablicę char przechowującą wczytany zbior, lub NULL jeżeli był on błędny.
 */
char *readSet(size_t (*len)) {
    int c;
    char *str;
    (*len) = 0;
    size_t size = INITIAL_NUMBER_SIZE;
    NOT_NULL(str = realloc(NULL, sizeof(char) * size));

    c = readByte();

    str[(*len)++] = c;
    while (EOF != (c = readByte()) && c != '\n') {
        str[(*len)++] = c;
        if ((*len) == size)
            NOT_NULL(str = realloc(str, sizeof(char) * (size *= 2)));
    }
    ungetChar(c);

    if ((*len) == 0) {
        if (c == EOF)
            printEofError();

        printSyntaxError(number_of_bytes);
    }

    str[(*len)++] = '\0';

    NOT_NULL(str = realloc(str, sizeof(char) * (*len)));

    return str;
}

/**
 * Parsuje komendę, jeżeli pierwszym argumentem była liczba.
 */
void handleADigit() {
    char c;

    command.arg1 = readNumber();
    DEBUG_PRINT("Odczytano numer 1: %s\n", command.arg1);

    ignoreWhiteSpaces();
    c = readByte();

    if (c == QMARK) {
        command.type = GET;
        command.first_read_byte = number_of_bytes - 1;
        DEBUG_PRINT("GET\n");

    } else if (c == R_ARROW) {
        command.type = FORWARD;
        command.first_read_byte = number_of_bytes - 1;

        ignoreWhiteSpaces();
        command.arg2 = readNumber();
        DEBUG_PRINT("Odczytano numer 2: %s\n", command.arg2);

    } else {
        if (c == EOF)
            printEofError();

        printSyntaxError(number_of_bytes - 1);
    }
}

/**
 * Parsuje komendę, jeżeli pierwszym argumentem był znak zapytania.
 */
void handleAQMark() {
    ignoreWhiteSpaces();

    command.arg1 = readNumber();
    command.type = REVERSE;
}

/**
 * Parsuje komendę, jeżeli pierwszym argumentem była litera.
 */
void delOrNew() {
    ignoreWhiteSpaces();

    char c;
    char *str;
    int i = 0;
    NOT_NULL(str = malloc(5 * sizeof(char)));


    while (EOF != (c = readByte()) && isalpha(c) && i < 4)
        str[i++] = c;
    str[i] = '\0';

    if (strcmp(str, NEW) == 0) {
        command.type = NEW_DB;
    } else if (strcmp(str, DEL) == 0) {
        command.type = DEL_TEMP;
    } else if (c == EOF) {
        printEofError();
    } else {
        printSyntaxError(command.first_read_byte);
    }

    free((void *) str);
    ungetChar(c);
}

/**
 * Parsuje komendę, jeżeli pierwszym argumentem był operator NEW.
 */
void handleNew() {
    command.arg1 = readIdentifier(&command.arg1_length);
}

/**
 * Parsuje komendę, jeżeli pierwszym argumentem był operator DEL.
 */
void handleDel() {
    char c = readByte();

    if (isDigitWrapper(c)) {
        ungetChar(c);
        command.arg1 = readNumber();
        command.type = DEL_NUM;
    } else if (isalpha(c)) {
        ungetChar(c);
        command.arg1 = readIdentifier(&command.arg1_length);
        command.type = DEL_ID;
    } else
        checkCorrectError(c);
}

/**
 * Parsuje komendę, jeżeli pierwszym argumentem był operator AT_SIGN.
 */
void handleAtSign() {
    ignoreWhiteSpaces();
    DEBUG_PRINT("HANDLE AT SIGN\n");
    command.arg1 = readSet(&command.arg1_length);
    command.type = NON_TRIVIAL;
}

/**
 * Szuka bazy przekierowań o nazwie name.
 * Przyjmuje dlugosc nazwy w celach optymalizacji.
 * @param name Nazwa poszukiwanej bazy
 * @param name_length Długość nazwy poszukiwanej bazy.
 * @return Wskaźnik na znalezioną bazę, lub NULL jeżeli taka nie istnieje.
 */
Database *searchForDatabase(char *name, size_t name_length) {
    for (size_t i = 0; i < INITIAL_DATABASE_ARRAY_SIZE; i++) {
        if ((Db_Array[i]) != NULL)
            if (name_length == (*Db_Array[i]).name_length)
                if (strcmp((*Db_Array[i]).name, name) == 0)
                    return Db_Array[i];

    }
    return NULL;
}

/**
 * Dodaje bazę przekierowań o zadanej nazwie, lub ustawia jako aktualnie używaną, w przypadku gdy ta już istnieje.
 * @param name Nazwa poszukiwanej bazy
 * @param name_length Długość nazwy poszukiwanej bazy.
 */
void addDatabase(char *name, size_t name_length) {
    for (size_t i = 0; i < INITIAL_DATABASE_ARRAY_SIZE; i++) {
        if ((Db_Array[i]) == NULL) {

            NOT_NULL((Db_Array[i]) = malloc(sizeof(Database)));

            char *new_name = malloc((name_length + 1) * sizeof(char));
            strcpy(new_name, name);
            new_name[name_length] = '\0';

            Db_Array[i]->name = new_name;
            Db_Array[i]->name_length = name_length;
            Db_Array[i]->db = phfwdNew();
            current_db = Db_Array[i];
            return;
        }
    }
    // Jezeli do tej pory nie dodalismy bazy danych
    // to znaczy ze zabraklo miejsca w tablicy
    printMemoryError();
}

/**
 * Wykonuje operację NEW.
 */
void operationNew() {
    Database *tmp = searchForDatabase(command.arg1, command.arg1_length);
    if (tmp != NULL)
        current_db = tmp;
    else {
        addDatabase(command.arg1, command.arg1_length);
    }
}

/**
 * Wykonuje operację DEL dla bazy przekierowań.
 */
void operationDelDb() {
    for (size_t i = 0; i < INITIAL_DATABASE_ARRAY_SIZE; i++)
        if ((Db_Array[i]) != NULL &&
            command.arg1_length == (*Db_Array[i]).name_length &&
            strcmp((*Db_Array[i]).name, command.arg1) == 0) {

            phfwdDelete((Db_Array[i])->db);
            free((void *) (Db_Array[i])->name);

            if (current_db == Db_Array[i])
                current_db = NULL;

            free((void *) (Db_Array[i]));
            Db_Array[i] = NULL;
            return;
        }
}

/**
 * Wykonuje operację DEL dla przekierowania w aktulnej bazie.
 */
void operationDelNum() {
    if (current_db != NULL)
        phfwdRemove(current_db->db, command.arg1);
    else {
        printOperatorError(command.first_read_byte, DEL);
    }
}


/**
 * Wykonuje operację ">" dla aktualnie zadanej bazy przekierowań.
 */
void operationForward() {
    bool r = false;
    if (current_db != NULL)
        r = phfwdAdd(current_db->db, command.arg1, command.arg2);

    if (!r) {
        printOperatorError(command.first_read_byte, R_ARROW_STR);
    }
}

/**
 * Wykonuje operację GET dla aktualnie zadanej bazy przekierowań.
 */
void operationGet() {
    bool ret = false;
    if (current_db != NULL) {
        struct PhoneNumbers const *pnum;
        NOT_NULL(pnum = phfwdGet(current_db->db, command.arg1));

        printf("%s\n", phnumGet(pnum, 0));

        phnumDelete(pnum);
        ret = true;
    }

    if (!ret)
        printOperatorError(command.first_read_byte, QMARK_STR);
}

/**
 * Wykonuje operację REVERSE dla aktualnie zadanej bazy przekierowań.
 */
void operationReverse() {
    bool ret = false;

    if (current_db != NULL) {

        size_t idx = 0;
        char const *num;
        struct PhoneNumbers const *pnum;
        NOT_NULL(pnum = phfwdReverse(current_db->db, command.arg1));

        while ((num = phnumGet(pnum, idx)) != NULL) {
            printf("%s\n", num);
            idx++;
        }

        phnumDelete(pnum);

        if (idx > 0)
            ret = true;

    }
    if (!ret) {
        printOperatorError(command.first_read_byte, QMARK_STR);
    }
}

/**
 * Wykonuje operację NON_TRIVIAL dla aktualnie zadanej bazy przekierowań.
 */
void operationNonTrivial() {
    bool ret = false;

    if (current_db != NULL) {
        size_t len = 0;
        if (command.arg1_length > NUMBER_OF_DIGITS) {
            // -1 za znak konca '\0'
            len = command.arg1_length - NUMBER_OF_DIGITS - 1;
            DEBUG_PRINT("LEN = %ld\n", len);
        }
        printf("%ld\n", phfwdNonTrivialCount(current_db->db, command.arg1, len));
        ret = true;
    }
    if (!ret) {
        printOperatorError(command.first_read_byte, AT_SIGN_STR);
    }
}


/**
 * Wywołuje funkcję wykonującą operację zadaną przez typ komendy.
 */
void runCommand() {
    switch (command.type) {
        case NEW_DB:
            operationNew();
            break;
        case DEL_ID:
            operationDelDb();
            break;
        case DEL_NUM:
            operationDelNum();
            break;
        case FORWARD:
            operationForward();
            break;
        case GET:
            operationGet();
            break;
        case REVERSE:
            operationReverse();
            break;
        case NON_TRIVIAL:
            operationNonTrivial();
            break;
        default:
            break;
    }
}

void parseInput() {
    char c;
    resetCommand();
    ignoreWhiteSpaces();

    c = readByte();
    if (isDigitWrapper(c)) {
        ungetChar(c);
        handleADigit();
        debugPrintCommand();

    } else if (c == QMARK) {
        command.first_read_byte = number_of_bytes - 1;
        handleAQMark();
        debugPrintCommand();
        DEBUG_PRINT("ustawiam first read byte jako %ld\n", command.first_read_byte);

    } else if (c == AT_SIGN) {
        command.first_read_byte = number_of_bytes - 1;
        handleAtSign();
        debugPrintCommand();
        DEBUG_PRINT("ustawiam first read byte jako %ld\n", command.first_read_byte);

    } else if (isalpha(c)) {
        command.first_read_byte = number_of_bytes - 1;

        ungetChar(c);
        delOrNew();
        ignoreWhiteSpaces();

        if (command.type == NEW_DB)
            handleNew();

        if (command.type == DEL_TEMP)
            handleDel();

        debugPrintCommand();
    } else if (c != EOF)
        printSyntaxError(number_of_bytes - 1);

    runCommand();

    if (c != EOF)
        parseInput();
}


