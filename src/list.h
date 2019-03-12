/** @file
 * Interfejs klasy listy jednokierunkowej
 *
 * @author Patryk Banach
 * @copyright Uniwersytet Warszawski
 * @date 09.04.2018
 */


#ifndef TELEFONY_LIST_H
#define TELEFONY_LIST_H

/**
 * Lista jednokierunknowa przechowująca wskaźnik na char* oraz
 *
 */
typedef struct list {
    int num_length;     ///< Ilosc znakow przechowywanego ciagu znakow
    char *num;          ///< Wskaznik na ciag znakow
    struct list *next;  ///< Wskaznik na następny element listy
} list;

/**
 * Usuwa listę jednokierunkową.
 * @param l Wskaźnik na usuwaną liste.
 */
void removeList(list **l);

/**
 * Wstawia numer do posortowanej leksykograficznie listy jednokierunkowej.
 * @param l Wskaźnik na listę
 * @param num Wstawiany numer
 * @param num_length Długośc wstawianego napisu
 * @param head Wskaźnik na poczatek listy
 * @return True jeżeli wstawiono element, False w przeciwnym wypadku.
 */
bool insertIntoSortedList(list *l, const char *num, int num_length, list **head);

/**
 * Wstawia numer za podany element listy.
 * @param l Wskaźnik na element listy
 * @param num Wstawiany numer
 * @param num_length Długość wstawianego numeru
 */
void insertBehind(list *l, const char *num, int num_length);

#endif //TELEFONY_LIST_H
