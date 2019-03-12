//
// Created by patryklin on 5/30/18.
//

#ifndef TELEFONY_PHONE_FORWARD_INTERFACE_H
#define TELEFONY_PHONE_FORWARD_INTERFACE_H

/**
 * Używany do identyfikacji operatorów
 */
typedef enum operator_enum {
    NEW_DB = 0,
    DEL_TEMP = 1,
    DEL_ID = 2,
    DEL_NUM = 3,
    FORWARD = 4,
    GET = 5,
    REVERSE = 6,
    IGNORE = 7,
    NON_TRIVIAL = 8
} Operator_enum;

/**
 * Przechowuje dane potrzebne do wykonania obecnej operacji.
 */
typedef struct command {
    Operator_enum type;     ///< Typ operacji do wykonania.
    char *arg1;             ///< Pierwszy argument.
    char *arg2;             ///< Drugi argument.

    size_t arg1_length;     ///< Długość pierwszego argumentu.
    size_t first_read_byte; ///< Numer pierwszego znaku operatora.
} Command;

/**
 * Przechowuje dane bazy przekierowań
 */
typedef struct database {
    struct PhoneForward *db;    ///< Wskaźnik na strukture PhoneForward.
    char *name;                 ///< Nazwa bazy przekierowań.
    size_t name_length;         ///< Długość nazwy bazy pzekierowań.

} Database;

/**
 * Funkcja parsująca dane wejściowe, oraz na bieżąco wykonująca zadane komendy.
 * Przerywa działanie po napotkaniu EOF.
 */
void parseInput();

void clearMemory();

#endif //TELEFONY_PHONE_FORWARD_INTERFACE_H
