#include "phone_forward.h"
#include "list.h"

#include <string.h>
#include <ctype.h>
#include <stdio.h>

/**
 * Definicja początkowego rozmiaru tablicy numerów dla struktury PhoneNumbers
 */
#define initial_numbers_array_size (2)

/**
 * Domyslnie ustawione jako 0
 */

#define DEBUG_TEST 0

/**
 * Wywoluje fprintf dla zadanych argumentow gdy DEBUG_TEST = 1, nie robi nic w p.p.
 */
#define DEBUG_PRINT(...) \
            do { if (DEBUG_TEST) fprintf(stderr, __VA_ARGS__); } while (0)




/** @file
 * Implementacja klasy przechowującej przekierowania numerów telefonicznych
 * @author Patryk Banach
 * @copyright Uniwersytet Warszawski
 * @date 06.05.2018
 */

/**
 * @struct PhoneForward phone_forward.h
 * @details Struktura przechowująca przekierowania numerów telefonów, zaimplementowana
 * jako drzewo Patricia (Skompresowane drzewo trie).
 */
struct PhoneForward {
    char *key;                     ///< ciąg znaków zapisanych w danym węźle
    char *phfwd;                   ///< przekierowanie dla prefixu złożonego z kluczy przechowywanych w ciągu od korzenia do obecnego węzła

    size_t key_length;             ///< dlugosc klucza

    struct PhoneForward *child;    ///< wskaźnik na pierwsze dziecko obecnego węzła
    struct PhoneForward *next;     ///< wskaźnik na rodzeństwo obecnego węzła
};
typedef struct PhoneForward PhoneForward; ///< domyślny typedef

/**
 * @struct PhoneNumbers phone_forward.h
 * @details Struktura przechowująca numery telefonów indeksowane wedlug kolejnosci dodania, od zera.
 */
struct PhoneNumbers {
    char **numbers;             ///< Tablica wskaźników na numery telefonów

    size_t numbers_count;          ///< Ilość numerów telefonów przechowywanych w tablicy
    size_t numbers_array_size;     ///< Aktualny rozmiar tablicy
};
typedef struct PhoneNumbers PhoneNumbers; ///< domyślny typedef


bool isDigitWrapper(char c) {
    return (isdigit(c) || c == ';' || c == ':');
}

/**
 * Sprawdza czy w ciągu znaków num znajdują się tylko znaki uznawane za cyfry.
 * @param num Sprawdzany ciąg znaków
 * @return False gdy w ciągu znajduje się znak niebędący cyfrą od 0 do 9. True w pozostałych przypadkach.
 */
static inline bool isNumber(const char *num) {
    //Według definicji, pusty ciąg znaków nie jest numerem
    if (num == NULL) {
        return false;
    }

    size_t len = strlen(num);
    unsigned int i = 0;
    bool result = true;

    while (i < len && result) {
        result = (isDigitWrapper(num[i]) != 0);
        i++;
    }

    return result && len != 0;
}

/**
 * Inicjalizuje egzemplarz struktury PhoneForward.
 * @param[in] pf        Inicjalizowana struktura
 * @param[in] key       Przypisywany klucz
 * @param[in] phfwd     Przypisywane przekierowanie
 * @param[in] child     Wskaźnik na dziecko
 * @param[in] next      Wskaźnik na rodzeństwo
 */
static inline void
phfwdIni(PhoneForward *pf, const char *key, const char *phfwd, PhoneForward *child,
         PhoneForward *next) {
    if (key != NULL) {
        free((void *) pf->key);
        size_t key_len = strlen(key);
        pf->key = malloc((key_len + 1) * sizeof(char));
        pf->key[key_len] = '\0';
        strcpy(pf->key, key);
        pf->key_length = key_len;
    } else {
        pf->key = malloc(sizeof(char));
        pf->key[0] = '\0';
        pf->key_length = 0;
    }

    if (phfwd != NULL) {
        free((void *) pf->phfwd);
        size_t phfwd_len = strlen(phfwd);
        pf->phfwd = malloc((phfwd_len + 1) * sizeof(char));
        pf->phfwd[phfwd_len] = '\0';
        strcpy(pf->phfwd, phfwd);
    } else {
        pf->phfwd = malloc(sizeof(char));
        pf->phfwd[0] = '\0';
    }

    pf->child = child;
    pf->next = next;
}

/**
 * Inicjalizuje egzemplarz struktury PhoneNumbers
 * @param[in] (*t) Inicjalizowana struktura
 */
static inline void phNumIni(PhoneNumbers *(*t)) {
    if ((*t) != NULL) {
        (*t)->numbers_count = 0;
        (*t)->numbers_array_size = initial_numbers_array_size;
        (*t)->numbers = malloc(initial_numbers_array_size * sizeof(char *));
        for (int i = 0; i < initial_numbers_array_size; i++) {
            (*t)->numbers[i] = NULL;
        }
    }
}

PhoneForward *phfwdNew(void) {
    PhoneForward *ret = (PhoneForward *) malloc(sizeof(PhoneForward));
    // Inicjalizuję ret z domyślnymi wartościami
    phfwdIni(ret, NULL, NULL, NULL, NULL);
    return ret;
}

/**
 * Dzieli węzęł drzewa PhoneForward na dwa.
 * @details Funkcja pomocnicza dzielaca węzęł drzewa PhoneForward na dwa. W węźle-rodzicu pozostaje
 * numer o długości k, w węźle-dziecku jako klucz przypisywane zostaje pozostałe key_length - k znaków.
 * @param[in]  parent                    Wskaźnik na dzielony węzeł
 * @param[in]  remaining_length          Długość ciągu znaków, który pozostanie w kluczu węzła-rodzica
 */
void splitNode(PhoneForward **parent, size_t remaining_length) {

    PhoneForward *child = phfwdNew();
    child->child = (*parent)->child;

    /*-------nadpisuje przekierowanie ----*/
    free((void *) (child)->phfwd);

    char *a;
    size_t phfwd_len = strlen((*parent)->phfwd);
    a = (char *) malloc((phfwd_len + 1) * sizeof(char));
    if (a != NULL) {
        strcpy(a, (*parent)->phfwd);
        a[phfwd_len] = '\0';
    }
    child->phfwd = a;

    /*-------nadpisuje klucz ----*/

    size_t child_key_length = (*parent)->key_length - remaining_length;
    free((void *) (child)->key);
    child->key = (char *) malloc((child_key_length + 1) * sizeof(char));

    if (child->key != NULL) {
        strncpy(child->key, (*parent)->key + remaining_length, child_key_length);
        child->key[child_key_length] = '\0';
    }
    child->key_length = child_key_length;

    /*-------nadpisuje klucz rodzica pozostaloscia ----*/

    a = (char *) malloc((remaining_length + 1) * sizeof(char));
    if (a != NULL) {
        strncpy(a, (*parent)->key, remaining_length);
        a[remaining_length] = '\0';
    }

    free((void *) (*parent)->key);
    (*parent)->key = a;
    (*parent)->key_length = remaining_length;

    if ((*parent)->phfwd != NULL) {
        (*parent)->phfwd[0] = '\0';
    }

    (*parent)->child = child;

}

/**
 * Szuka długości najdłuższego wspólngo prefiksu ciągów num1 i num2.
 * @details Funkcja pomocnicza znajdująca długość najdłuższego wspólngo prefiksu ciągów num1 i num2.
 * @param[in] num1      Pierwszy z porównywanych napisów
 * @param[in] num1_len  Dlugość pierwszego ciągu znaków
 * @param[in] num2      Drugi z porównywanych napisów
 * @param[in] num2_len  Dlugość drugiego ciągu znaków
 * @return Dlugość najduższego porównywanego napisu.
 */
static inline size_t
lengthOfLongestCommonPrefix(const char *num1, size_t num1_len, const char *num2,
                            size_t num2_len) {
    for (unsigned int k = 0; k < num1_len; k++) {
        if (k == num2_len || num1[k] != num2[k]) {
            return k;
        }
    }

    return (int) num1_len;
}

/**
 * Funkcja pomocnicza znajdująca najogólniejszy prefix dla napisu num, w drzewie t.
 * @param[in]      t                     Przeszukiwane drzewo.
 * @param[in]      num                   Numer którego prefiksu poszukujemy.
 * @param[in]      num_length            Długość poszukiwanego numeru.
 * @param[in]  depth_in_characters   Ilość już znalezionych znaków poszukiwanego numeru.
 * @return  Wskaźnik na znaleziony węzęł.
 */
PhoneForward *
phfwdFindPrefixMatch(PhoneForward *t, const char *num, size_t num_length,
                     int *depth_in_characters) {
    // *number_length to dlugosc numeru od korzenia do obecnego miejsca
    if (!t) {
        return NULL;
    }

    if (num_length == 0) {
        num_length = strlen(num);
    }
    DEBUG_PRINT("phfwdFindPrefix: Wezel o kluczu %s, z przekierowaniem na %s\n",
                t->key, t->phfwd);
    DEBUG_PRINT("Pozostaje num:  %s\n", num);

    size_t common_prefix_len = lengthOfLongestCommonPrefix(num, num_length, t->key,
                                                           t->key_length);

    if (common_prefix_len == 0 && t->key_length != 0) {
        return phfwdFindPrefixMatch(t->next, (num), num_length, depth_in_characters);
    }

    // jezeli poszukiwany num zawiera sie w obecnym kluczu
    // to, poniewaz przeszukujemy drzewo od korzenia
    // nie znajdziemy ogolniejszego

    if (common_prefix_len > 0 || t->key_length == 0) {
        *depth_in_characters += common_prefix_len;
        PhoneForward *temp = NULL;
        //szukamy glebiej tylko jezeli caly obecny wezel jest zgodny
        if (common_prefix_len == t->key_length) {
            temp = phfwdFindPrefixMatch(t->child, (num + common_prefix_len),
                                        num_length - common_prefix_len,
                                        depth_in_characters);
            if (temp != NULL)
                return temp;
        }
        *depth_in_characters -= common_prefix_len;
        return t;
    }

    return NULL;
}

/**
 * Nadpisuje napis to, napisem from.
 * @param[out] to            Nadpisywany napis
 * @param[in] from      Docelowy napis
 * @param[in] num_len   Dlugosc docelowego napisu
 */
static inline void overwriteString(char **to, const char *from, size_t num_len) {
    free((void *) *to);
    char *temp = malloc((num_len + 1) * sizeof(char));
    strcpy(temp, from);
    temp[num_len] = '\0';
    *to = temp;
}


/**
 * Rekurencyjnie przeszukuje drzewo PhoneForward i dodająca przekierowanie num2.
 * @param[in] pf        Przeszukiwana struktura
 * @param[in] num1      Przekierowywany numer
 * @param[in] num2      Przekierowanie
 * @param[in] num1_len Dlugosc przekierowanego numeru
 * @return          Wskaźnik na dodany węzeł, lub NULL jeżeli z jakiegoś powodu to się nie udało.
 */
PhoneForward *
phfwdAddUtil(PhoneForward *pf, char const *num1, char const *num2, size_t num1_len) {
    //Najpierw szukam najgłębszego dopasowania w drzewie
    int depth_in_characters = 0;
    PhoneForward *temp = phfwdFindPrefixMatch(pf, num1, num1_len,
                                              &depth_in_characters);

    num1 = num1 + depth_in_characters;
    num1_len = num1_len - depth_in_characters;

    // Jeżeli temp == NULL, to tworzony jest nowy węzeł
    if (!temp) {
        DEBUG_PRINT("Brak wspolnych prefikow, tworze wezel %s ---> na %s\n",
                    num1,
                    num2);
        PhoneForward *ret = phfwdNew();
        phfwdIni(ret, num1, num2, NULL, NULL);
        ret->next = pf->next;
        pf->next = ret;
        return ret;
    }

    if (temp->key[0] == '\0') {
        DEBUG_PRINT("pf -> key jest pusty, nadpisuje go\n");
        phfwdIni(temp, num1, num2, temp->child, temp->next);
        return temp;
    }

    DEBUG_PRINT("Temp to wezel o kluczu %s, z przekierowaniem na %s\n",
                temp->key, temp->phfwd);
    DEBUG_PRINT(
            "Pomijajac go, do tej pory pokrylo sie %d znakow, pozostal numer  %s\n",
            depth_in_characters, num1);

    size_t common_prefix_len = lengthOfLongestCommonPrefix(num1, num1_len, temp->key,
                                                           temp->key_length);
    DEBUG_PRINT("Dlugosc common prefix miedzy %s a %s  to %ld \n", num1, temp->key,
                common_prefix_len);

    if (common_prefix_len == 0) {
        //koniec sparowania, czyli nalezy stworzyc nowy wezel jako next od temp
        DEBUG_PRINT(
                "Common prefix jest pusty, nalezy stworzyc nowy wezel jako dziecko temp\n");
        PhoneForward *ret = phfwdNew();
        if (temp->child != NULL) {
            phfwdIni(ret, num1, num2, NULL, temp->child->next);
        } else {
            phfwdIni(ret, num1, num2, NULL, NULL);
        }
        temp->child = ret;
    } else if (common_prefix_len < num1_len) {
        DEBUG_PRINT(
                "Istnieje wspolny prefix, ale nie jest calym dodawanym numerem\n");
        if (common_prefix_len < temp->key_length) {
            DEBUG_PRINT("Wspolny prefix jest krotszy od obecnego klucza\n");
            DEBUG_PRINT("Dziele obecny wezel\n");
            splitNode(&temp, common_prefix_len);
            DEBUG_PRINT("po podziale jestem w wezle o kluczu %s ---> %s\n",
                        temp->key, temp->phfwd);
            DEBUG_PRINT("dziecko to wezel o kluczu %s, z przekierowaniem na %s\n",
                        (temp->child)->key,
                        (temp->child)->phfwd);

            PhoneForward *ret = phfwdNew();
            phfwdIni(ret, num1 + common_prefix_len, num2, NULL, NULL);

            DEBUG_PRINT("Tworze wezel dla numeru %s ----> %s\n",
                        num1 + common_prefix_len, num2);
            ret->next = temp->child;
            temp->child = ret;

            return ret;
        } else if (common_prefix_len == temp->key_length) {
            //obecny klucz to najglebsze dopasowanie, i jest ono pelne
            //to znaczy ze ucinam num1 w dlugosci dopasowania, i tworze nowy wezel
            PhoneForward *ret = phfwdNew();
            phfwdIni(ret, num1 + common_prefix_len, num2, NULL, temp->child);
            temp->child = ret;
            return ret;
        }
    } else if (num1_len == common_prefix_len) {
        if (temp->key_length == num1_len) {
        } else {
            DEBUG_PRINT("Wspolny prefix jest krotszy od obecnego klucza\n");
            DEBUG_PRINT("Dziele obecny wezel\n");
            splitNode(&temp, common_prefix_len);

            DEBUG_PRINT("po podziale jestem w wezle o kluczu %s ---> %s\n",
                        temp->key, temp->phfwd);
            DEBUG_PRINT("dziecko to wezel o kluczu %s, z przekierowaniem na %s\n",
                        (temp->child)->key,
                        (temp->child)->phfwd);
        }
        size_t temp_length = strlen(num2);
        overwriteString(&temp->phfwd, num2, temp_length);
        DEBUG_PRINT("Nadpisuje przekierowanie dla temp\n");
        DEBUG_PRINT("po Nadpisuniu jestem w wezle o kluczu %s ---> %s\n",
                    temp->key, temp->phfwd);

    }
    return pf;
}


bool phfwdAdd(struct PhoneForward *pf, char const *num1, char const *num2) {
    if (!isNumber(num2) || !isNumber(num1) || !pf || strcmp(num1, num2) == 0) {
        return false;
    }
    DEBUG_PRINT("dodaje numer %s \n", num1);

    size_t num1_len = strlen(num1);
    PhoneForward *ret = phfwdAddUtil(pf, num1, num2, num1_len);

    DEBUG_PRINT("\n\n");
    return ret != NULL;
}

/**
 * Dokładnie jak phfwdAdd, z tą różnicą, że ta dopuszcza num1 == num2
 */
bool
phfwdAddforNonTrivial(struct PhoneForward *pf, char const *num1, char const *num2) {
    if (!isNumber(num2) || !isNumber(num1) || !pf) {
        return false;
    }
    DEBUG_PRINT("dodaje numer na potrzeby NonTrivial %s \n", num1);

    size_t num1_len = strlen(num1);
    PhoneForward *ret = phfwdAddUtil(pf, num1, num2, num1_len);

    DEBUG_PRINT("\n\n");
    return ret != NULL;
}

/**
 * Funkcja pomocnicza, znajdująca dokładne trafienie dla napisu num w drzewie t.
 * @param[in] t                 Przeszukiwane drzewo
 * @param[in] num               Wzór szukanego napisu
 * @param[in] len_from_root     Dlugośc napisu od korzenia do obecnego węzła
 * @param[in]  num_length       Dlugosc wzoru
 * @return wskaźnik na węzeł zawierający odpowiadający klucz, lub NULL, jeżeli taki nie istnieje
 */
PhoneForward *
phfwdFindExactMatch(PhoneForward *t, char const *num, int *len_from_root,
                    size_t num_length) {
    // *number_length to dlugosc numeru od korzenia do obecnego miejsca
    if (!t) {
        return NULL;
    }

    if (num_length == 0) {
        num_length = strlen(num);
    }
    DEBUG_PRINT("Szukam trafienia dla %s \n", num);
    DEBUG_PRINT("Jestem w wezle o kluczu %s \n", t->key);
    size_t common_prefix_len = lengthOfLongestCommonPrefix(num, num_length,
                                                           t->key, t->key_length);

    *len_from_root += common_prefix_len;
    DEBUG_PRINT("common prefix len to %ld, a len from root to %d \n",
                common_prefix_len, *len_from_root);

    // Brak wspolnego prefixu: szukam wsrod rodzenstwa
    if (common_prefix_len == 0) {
        return phfwdFindExactMatch(t->next, num, len_from_root, num_length);
    }
    // Jezeli caly klucz jest zgodnym prefiksem, to zapamiętuje
    // węzeł jako potencjalnie najlepsze trafienie, i przeszukuję drzewo w głąb
    if (common_prefix_len == t->key_length) {

        int temporary_num_len = *len_from_root;
        PhoneForward *temporary;
        temporary = phfwdFindExactMatch(t->child, num + common_prefix_len,
                                        len_from_root,
                                        num_length -
                                        common_prefix_len);
        if (temporary != NULL)
            return temporary;
        else {
            *len_from_root = temporary_num_len;
        }

    }
    if (common_prefix_len == t->key_length && (num_length >= t->key_length))
        return t;

    return NULL;
}


/**
 * Funkcja pomocnicza łącząca dwa napisy.
 * @param[in] s1
 * @param[in] s2
 * @return Wskaźnik na napis stworzony z połączonych napisów s1 i s2
 */
char *concatenate(const char *s1, const char *s2) {
    const size_t len1 = strlen(s1);
    size_t len2 = 0;
    len2 = strlen(s2);
    char *result = malloc(len1 + len2 + 1);
    memcpy(result, s1, len1);
    memcpy(result + len1, s2, len2 + 1);
    return result;
}

/**
 * Funkcja pomocnicza dodająca numer do struktury PhoneNumbers
 * @param[in] t    Struktura do której dodwany jest numer
 * @param[in] num   Dodawany numer
 */
void addNumber(PhoneNumbers *t, const char *num) {
    // Jeżeli liczba numerów w strukturze jest równa ich maksymalnej liczbie
    // to rozmiar tablicy wskaźnków zwiększany jest dwukrotnie
    if (t->numbers_count >= t->numbers_array_size) {
        t->numbers = (char **) realloc(t->numbers, (2 * (t->numbers_array_size)) *
                                                   sizeof(char *));
        size_t temp = t->numbers_array_size;
        t->numbers_array_size = 2 * (t->numbers_array_size);
        for (size_t i = temp; i < t->numbers_array_size; i++) {
            t->numbers[i] = NULL;
        }
    }

    size_t len = strlen(num);
    t->numbers[t->numbers_count] = malloc(len * sizeof(char) + 1);
    t->numbers[t->numbers_count][len] = '\0';
    strcpy(t->numbers[t->numbers_count], num);
    t->numbers_count++;
}

PhoneNumbers const *phfwdGet(struct PhoneForward *pf, char const *num) {


    PhoneNumbers *result;
    result = malloc(sizeof(PhoneNumbers));
    phNumIni(&result);

    if (result == NULL)
        return NULL;

    if (num == NULL || !isNumber(num))
        return result;

    /* szukam najdluzszego pasujacego prefixu przekierowania */
    PhoneForward *tmp;
    int number_length = 0;
    tmp = phfwdFindExactMatch(pf, num, &number_length, 0);

    if (tmp != NULL && tmp->phfwd[0] != '\0') {
        char *number;
        number = concatenate(tmp->phfwd, num + number_length);
        addNumber(result, number);
        free((void *) number);
    } else
        addNumber(result, num);


    return result;
}

/**
 * Wypisuje drzewo PhoneForward.
 * @param[in]  pf       Wskaznik na wypisywaną strukturę
 * @param[in]  indent   glebokosc wciecia dla obecnego wezla
 */
void phfwdPrint(PhoneForward *pf, int indent) {
    //rekurencyjnie wypisuje drzewo PhoneForward
    for (int i = 0; i < indent; i++)
        DEBUG_PRINT("---");

    if (pf != NULL) {
        DEBUG_PRINT("%s", (pf)->key);

        if (pf->phfwd[0] != '\0')
            DEBUG_PRINT("------>%s", (pf)->phfwd);

        DEBUG_PRINT("\n");

        phfwdPrint((pf)->child, indent + 1);
        phfwdPrint((pf)->next, indent);
    } else
        DEBUG_PRINT("NULL\n");
}

void phfwdDelete(PhoneForward *pf) {
    //rekurencyjnie usuwam drzewo PhoneForward
    if (pf != NULL) {
        phfwdDelete((pf)->next);
        phfwdDelete((pf)->child);

        free((void *) (pf)->key);
        free((void *) (pf)->phfwd);
        free((void *) (pf));
    }
}

/**
 * Funkcja pomocnicza służąca do usuwania struktury PhoneForward.
 * @details Funkcja pomocnicza służąca do usuwania struktury PhoneForward. Rekurencyjnie usuwa drzewo.
 * @param pf Wskaźnik na wskaźnik usuwanego drzewa.
 */
void phfwdDeleteUtil(PhoneForward **pf) {
    //rekurencyjnie usuwam drzewo PhoneForward
    if ((*pf) != NULL) {
        phfwdDeleteUtil(&(*pf)->next);
        (*pf)->next = NULL;
        phfwdDeleteUtil(&(*pf)->child);
        (*pf)->child = NULL;

        free((void *) (*pf)->key);
        (*pf)->key = NULL;
        free((void *) (*pf)->phfwd);
        (*pf)->phfwd = NULL;

        (*pf)->key_length = 0;
        free((void *) (*pf));
        (*pf) = NULL;
    }
}


void phfwdRemove(struct PhoneForward *pf, char const *num) {
    if (!isNumber(num))
        return;

    if (!pf)
        return;

    PhoneForward *result;
    size_t len_from_root = 0;
    int temp = 0;

    result = phfwdFindPrefixMatch(pf, num, len_from_root, &temp);

    if (result != NULL) {
        free((void *) result->phfwd);
        result->phfwd = malloc(sizeof(char));
        result->phfwd[0] = '\0';

        free((void *) result->key);
        result->key = malloc(sizeof(char));
        result->key[0] = '\0';

        result->key_length = 0;

        phfwdDeleteUtil(&result->child);
        result->child = NULL;
    }
}

/**
 * Funkcja pomocnicza dla phfwdReverse.
 * @param[in] t             Przeszukiwana struktura
 * @param[in] num           Szukany numer
 * @param[in] result        Lista do której dodawne sa odpowiadające przekierowania
 * @param[in] forwarded     Numer stworzony przez klucze od korzenia do obecnego węzła
 * @param[in] num_len       Dlugośc napisu num
 */
void phfwdReverseUtil(struct PhoneForward *t, char const *num, list **result,
                      char *forwarded, size_t num_len) {
    if (!t)
        return;

    if (num_len == 0)
        num_len = strlen(num);

    size_t phfwd_length = strlen(t->phfwd);
    size_t k = lengthOfLongestCommonPrefix(num, num_len, t->phfwd, phfwd_length);

    phfwdReverseUtil(t->next, num, result, forwarded, num_len);

    if (k == 0) {
        char *temp = concatenate(forwarded, t->key);
        phfwdReverseUtil(t->child, num, result, temp, num_len);
        free((void *) temp);
    } else {
        size_t phfwd_len = strlen(t->phfwd);
        if (k == phfwd_len) {
            char *temp = concatenate(forwarded, t->key);
            char *temp1 = concatenate(temp, num + phfwd_length);
            insertIntoSortedList((*result), temp1, strlen(temp1), result);
            free((void *) temp1);
            phfwdReverseUtil(t->child, num, result, temp, num_len);
            free((void *) temp);
        }
    }
}

/**
 * Funkca pomocnicza kopiująca numery z listy do struktury PhoneNumbers.
 * @param[in]  to
 * @param[in]  from
 */
void copyListToPhnumStruct(PhoneNumbers *to, list *from) {
    if ((from) == NULL) return;
    list *current = from;
    list *next;

    while (current != NULL) {
        next = current->next;
        addNumber(to, current->num);
        current = next;
    }
}

struct PhoneNumbers const *phfwdReverse(struct PhoneForward *pf, char const *num) {
    PhoneNumbers *result;
    result = malloc(sizeof(PhoneNumbers));
    phNumIni(&result);


    if (!isNumber(num))
        return result;

    list *temp;
    temp = NULL;
    size_t num_len = strlen(num);

    insertIntoSortedList(temp, num, strlen(num), &temp);
    char *forwarded;
    forwarded = malloc(sizeof(char));
    forwarded[0] = '\0';
    phfwdReverseUtil(pf, num, &temp, forwarded, num_len);

    free((void *) forwarded);

    copyListToPhnumStruct(result, temp);

    removeList(&temp);
    return result;
}

char const *phnumGet(struct PhoneNumbers const *pnum, size_t idx) {
    if (pnum == NULL)
        return NULL;

    if (idx < pnum->numbers_array_size)
        return pnum->numbers[idx];

    return NULL;
}

void phnumDelete(struct PhoneNumbers const *pnum) {
    if (pnum == NULL)
        return;


    for (size_t i = 0; i < pnum->numbers_count; i++)
        free((void *) pnum->numbers[i]);


    free((void *) pnum->numbers);
    free((void *) pnum);
}

/**
 * Funkcja sprawdzająca zbior set. Liczy dostępne znaki.
 * @param  set przeglądany zbiór.
 * @param    available_chars tablica boolowska zapamiętująca które z nich są dostępne.
 * @return Liczba dostępnych znaków.
 */
static inline unsigned int countAvailableChars(char const *set,
                                               bool available_chars[NUMBER_OF_DIGITS]) {
    unsigned int num_of_available_chars = 0;
    size_t i = 0;

    while (num_of_available_chars < NUMBER_OF_DIGITS && set[i] != '\0') {
        if (isDigitWrapper(set[i]) && !available_chars[set[i] - '0']) {
            // Na szczęście, w tablicy ascii : oraz ; występują w tej kolejności
            // zaraz po cyfrach 0 - 9
            available_chars[set[i] - '0'] = true;
            num_of_available_chars++;
        }

        i++;
    }
    return num_of_available_chars;
}

/**
 * Potęgowanie.
 * @param base podstawa.
 * @param exp wykladnik.
 * @return podstawa^wykładnik.
 */
size_t power(size_t base, size_t exp) {
    size_t result = 1;
    while (exp) {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        base *= base;
    }

    return result;
}


/**
 * Funkcja pomocnicza do phfwdNonTrivialCount.
 * Rekurencyjnie przeszukuje drzewo w poszukiwaniu kluczy zawierających tyko znaki
 * znajdujące sie w zbiorze set.
 * @param pf Przeszukiwana struktura.
 * @param available_chars tablica wskazująca które znaki są dostępne (znajdowały się
 * w tablicy set).
 * @param result struktura PhoneForward w której zapisane są odpowiednie przekierowania.
 * @param num_of_available_chars Liczba dostępnych znaków.
 * @param len maksymalna dlugosc napisu.
 */
void searchForNonTrivialNumbers(struct PhoneForward *pf,
                                struct PhoneForward *result,
                                bool available_chars[NUMBER_OF_DIGITS],
                                int num_of_available_chars,
                                size_t *len) {

    if (!pf)
        return;

    searchForNonTrivialNumbers(pf->next, result, available_chars,
                               num_of_available_chars,
                               len);
    searchForNonTrivialNumbers(pf->child, result, available_chars,
                               num_of_available_chars,
                               len);


    DEBUG_PRINT("Jestem w wezle o kluczu %s", pf->key);
    DEBUG_PRINT("-------->%s \n", pf->phfwd);

    if (pf->key_length == 0 || *len <= 0)
        return;

    size_t temp_len = (*len);
    size_t phfwd_length = 0;
    bool proceed = true;

    while (pf->phfwd[phfwd_length] != '\0' && proceed == true) {
        char char_to_int = pf->phfwd[phfwd_length] - '0';
        if (!available_chars[char_to_int]) {
            proceed = false;
            phfwd_length--;
        }
        phfwd_length++;
    }
    if (pf->phfwd[phfwd_length] != '\0')
        return;

    DEBUG_PRINT("Przekierowanie zawierało tylko znaki zawarte w set \n");
    if (pf->phfwd[0] != '\0') {
        long long remaining_len = (*len) - phfwd_length;
        if (remaining_len >= 0) {
            phfwdAddforNonTrivial(result, pf->phfwd, "1");
        }
    }

    *len = temp_len;

}

/**
 * Funkcja pomocnicza do phfwdNonTrivialCount.
 * Przeszukuje drzewo zapisanych przekierowań, i zlicza możliwe kombinacje
 * ustawienie dostępnych numerów na wolnych miejscach.
 * @param pf przeszukiwane drzewo
 * @param len maksymalna dlugosc napisu
 * @param num_of_available_chars liczba dozwolonych znaków.
 * @return suma możliwych kombinacji dla drzewa pf.
 */
size_t nonTrivialCountResults(struct PhoneForward *pf, size_t len,
                              unsigned int num_of_available_chars) {
    if (!pf)
        return 0;

    size_t result = 0;
    result += nonTrivialCountResults(pf->next, len, num_of_available_chars);

    DEBUG_PRINT("jestem w kluczu %s--->%s\n", pf->key, pf->phfwd);

    if (len < pf->key_length)
        return 0;

    len -= pf->key_length;
    if (pf->phfwd[0] == '\0')
        result += nonTrivialCountResults(pf->child, len, num_of_available_chars);


    else if (pf->key_length != 0) {
        result += (size_t) power((size_t) num_of_available_chars,
                                 (size_t) len);
        DEBUG_PRINT("dodaje %ld do potegi %ld ", num_of_available_chars,
                    (size_t) len);
        DEBUG_PRINT("czyli %ld \n", (size_t) power((size_t) num_of_available_chars,
                                                   (size_t) len));
        DEBUG_PRINT("SUMA TO %ld \n", result);

    }

    return result;
}

size_t phfwdNonTrivialCount(struct PhoneForward *pf, char const *set, size_t len) {

    if (!pf || !set || !len)
        return 0;

    struct PhoneForward *result = phfwdNew();
    bool available_chars[NUMBER_OF_DIGITS] = {false};

    unsigned int num_of_available_chars = countAvailableChars(set, available_chars);
    DEBUG_PRINT("Ilosc dostepnych znakow: %d\n", num_of_available_chars);

    if (!num_of_available_chars)
        return 0;


    searchForNonTrivialNumbers(pf, result, available_chars, num_of_available_chars,
                               &len);

    size_t temp = nonTrivialCountResults(result, len, num_of_available_chars);
    phfwdDelete(result);

    return temp;
}


