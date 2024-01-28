/**
 * extreme_edge_cases
 * CS 341 - Spring 2024
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "camelCaser.h"
#include "camelCaser_tests.h"

int checker(char **arr1, char **arr2) {
    // Check if both arrays are NULL
    if (arr1 == NULL && arr2 == NULL) {
        return 1;
    }

    // If one is NULL and the other isn't, they aren't identical
    if (arr1 == NULL || arr2 == NULL) {
        return 0;
    }

    // Iterate through both arrays
    int i = 0;
    for (i = 0; arr1[i] != NULL && arr2[i] != NULL; i++) {
        // If corresponding strings are not equal, return 0
        if (strcmp(arr1[i], arr2[i]) != 0) {
            return 0;
        }
    }

    // Check if both arrays have reached their end
    return arr1[i] == NULL && arr2[i] == NULL;
}

int test_camelCaser(char **(*camelCaser)(const char *),
                    void (*destroy)(char **)) {
    // TODO: Implement me!
    // TC 1: Sentence with various punctuation marks
    char *test1 = "Hello! Is this: a test? Yes, it is.";
    char *sol1[] = {"hello", "isThis", "aTest", "yes", "itIs", NULL};
    char **result1 = camelCaser(test1);
    if (!checker(sol1, result1)) { destroy(result1); return 0; }
    destroy(result1);

    // TC 2: Sentence with only one word and no punctuation
    char *test2 = "word";
    char *sol2[] = {NULL};
    char **result2 = camelCaser(test2);
    if (!checker(sol2, result2)) { destroy(result2); return 0; }
    destroy(result2);

    // TC 3: Sentence with numbers and special characters
    char *test3 = "Test1234 with special@characters.";
    char *sol3[] = {"test1234WithSpecial", "characters", NULL};
    char **result3 = camelCaser(test3);
    if (!checker(sol3, result3)) { destroy(result3); return 0; }
    destroy(result3);

    // TC4: String with multiple spaces and tabs
    char *test4 = "This   is\t a \ttest.";
    char *sol4[] = {"thisIsATest", NULL};
    char **result4 = camelCaser(test4);
    if (!checker(sol4, result4)) { destroy(result4); return 0; }
    destroy(result4);

    // TC 5: Empty string
    char *test5 = "";
    char *sol5[] = {NULL};
    char **result5 = camelCaser(test5);
    if (!checker(sol5, result5)) { destroy(result5); return 0; }
    destroy(result5);

    // TC 6: String with newlines
    char *test6 = "Line1\nLine2\nLine3.";
    char *sol6[] = {"line1Line2Line3", NULL};
    char **result6 = camelCaser(test6);
    if (!checker(sol6, result6)) { destroy(result6); return 0; }
    destroy(result6);

    // TC 7: String with repeated punctuation
    char *test7 = "Repeated... punctuation???";
    char *sol7[] = {"repeated", "", "", "punctuation", "", "", NULL};
    char **result7 = camelCaser(test7);
    if (!checker(sol7, result7)) { destroy(result7); return 0; }
    destroy(result7);

    // TC 8: String with only punctuation
    char *test8 = "!?.;";
    char *sol8[] = {"", "", "", "", NULL};
    char **result8 = camelCaser(test8);
    if (!checker(sol8, result8)) { destroy(result8); return 0; }
    destroy(result8);

    // TC 9: Long string with mixed content
    char *test9 = "LongStringWith123 Numbers, words, and&Symbols.";
    char *sol9[] = {"longstringwith123Numbers", "words", "and", "symbols", NULL};
    char **result9 = camelCaser(test9);
    if (!checker(sol9, result9)) { destroy(result9); return 0; }
    destroy(result9);

    // TC 10: String with uppercase letters
    char *test10 = "This IS A Test.";
    char *sol10[] = {"thisIsATest", NULL};
    char **result10 = camelCaser(test10);
    if (!checker(sol10, result10)) { destroy(result10); return 0; }
    destroy(result10);

    // TC 11: String with Mixed Case and Numbers
    char *test11 = "This1 Is2 A Test3.";
    char *sol11[] = {"this1Is2ATest3", NULL};
    char **result11 = camelCaser(test11);
    if (!checker(sol11, result11)) { destroy(result11); return 0; }
    destroy(result11);

    // TC 12: String with Mixed Case and Numbers
    char *test12 = "   \t\n  \t ";
    char *sol12[] = {NULL};
    char **result12 = camelCaser(test12);
    if (!checker(sol12, result12)) { destroy(result12); return 0; }
    destroy(result12);

    // TC 13: String with Mixed Case and Numbers
    char *test13 = "This has\\nnewlines and\\ttabs.";
    char *sol13[] = {"thisHas", "nnewlinesAnd", "ttabs", NULL};
    char **result13 = camelCaser(test13);
    if (!checker(sol13, result13)) { destroy(result13); return 0; }
    destroy(result13);

    // TC 14: String with Mixed Case and Numbers
    char *test14 = "Sentence ends here.    ";
    char *sol14[] = {"sentenceEndsHere", NULL};
    char **result14 = camelCaser(test14);
    if (!checker(sol14, result14)) { destroy(result14); return 0; }
    destroy(result14);

    // TC 15: String with Mixed Case and Numbers
    char *test15 = "Well-known compound-words are here.";
    char *sol15[] = {"well", "knownCompound", "wordsAreHere", NULL};
    char **result15 = camelCaser(test15);
    if (!checker(sol15, result15)) { destroy(result15); return 0; }
    destroy(result15);

    // TC 16: String with Mixed Case and Numbers
    char *test16 = "Special #$% characters 123 with numbers 4567.";
    char *sol16[] = {"special", "", "", "characters123WithNumbers4567", NULL};
    char **result16 = camelCaser(test16);
    if (!checker(sol16, result16)) { destroy(result16); return 0; }
    destroy(result16);

    return 1;
}