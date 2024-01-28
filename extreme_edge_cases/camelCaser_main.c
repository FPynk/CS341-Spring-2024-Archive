/**
 * extreme_edge_cases
 * CS 341 - Spring 2024
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "camelCaser.h"
#include "camelCaser_tests.h"

int main() {
    int TC1 = 0;
    int TC2 = 0;
    int TC3 = 0;
    int TC4 = 1;
    if (TC1) {
        // testing count sentences
        char *s1 = " example ! example , example";
        int s1r = countSentences(s1);
        assert(s1r == 2); // Expected: 2

        char *s2 = "A sentence without any punctuation";
        int s2r = countSentences(s2);
        assert(s2r == 0); // Expected: 0

        char *s3 = "Wait... What??? Really!!";
        int s3r = countSentences(s3);
        assert(s3r == 8); // Expected: 8

        char *s4 = "     ";
        int s4r = countSentences(s4);
        assert(s4r == 0); // Expected: 0

        char *s5 = "...Start in the middle? Yes! End.";
        int s5r = countSentences(s5);
        assert(s5r == 6); // Expected: 6

        char *s6 = "Hello world! How are you? I'm fine; thanks for asking.";
        int s6r = countSentences(s6);
        assert(s6r == 5); // Expected: 5

        char *s7 = "Hello";
        int s7r = countSentences(s7);
        assert(s7r == 0); // Expected: 0

        char *s8 = "First sentence... Second sentence: here it is! And here's the fourth?";
        int s8r = countSentences(s8);
        assert(s8r == 7); // Expected: 7

        char *s9 = "This is a long sentence, with several commas, but it's still one sentence NOT.";
        int s9r = countSentences(s9);
        assert(s9r == 4); // Expected: 4

        char *s10 = "\"Is this real life?\" he asked. \"Or is this just fantasy?\" she replied.";
        int s10r = countSentences(s10);
        assert(s10r == 8); // Expected: 8

        printf("All asserts passed\n");
        printf("--------\n");
        
        int *in1 = (int *)malloc(sizeof(int));
        int *in2 = (int *)malloc(sizeof(int));
        int *in3 = (int *)malloc(sizeof(int));
        int *in4 = (int *)malloc(sizeof(int));
        int *in5 = (int *)malloc(sizeof(int));
        int *in6 = (int *)malloc(sizeof(int));
        int *in7 = (int *)malloc(sizeof(int));
        int *in8 = (int *)malloc(sizeof(int));
        int *in9 = (int *)malloc(sizeof(int));
        int *in10 = (int *)malloc(sizeof(int));

        // Temporary pointers to store the results of splitSentences
        char **result1 = splitSentences(s1, in1);
        char **result2 = splitSentences(s2, in2);
        char **result3 = splitSentences(s3, in3);
        char **result4 = splitSentences(s4, in4);
        char **result5 = splitSentences(s5, in5);
        char **result6 = splitSentences(s6, in6);
        char **result7 = splitSentences(s7, in7);
        char **result8 = splitSentences(s8, in8);
        char **result9 = splitSentences(s9, in9);
        char **result10 = splitSentences(s10, in10);

        // Print the results
        printCharArray(result1);
        printCharArray(result2);
        printCharArray(result3);
        printCharArray(result4);
        printCharArray(result5);
        printCharArray(result6);
        printCharArray(result7);
        printCharArray(result8);
        printCharArray(result9);
        printCharArray(result10);

        // Destroy the results
        destroy(result1);
        destroy(result2);
        destroy(result3);
        destroy(result4);
        destroy(result5);
        destroy(result6);
        destroy(result7);
        destroy(result8);
        destroy(result9);
        destroy(result10);

        free(in1);
        free(in2);
        free(in3);
        free(in4);
        free(in5);
        free(in6);
        free(in7);
        free(in8);
        free(in9);
        free(in10);
        printf("TC1 Succeses\n");
    }
    
    if (TC2) {
        // Test Case 1: Basic sentence
        char *s1 = "Hello world";
        int s1r = countWords(s1);
        assert(s1r == 2); // Expected: 2

        // Test Case 2: Sentence with multiple spaces
        char *s2 = "This   is  a test";
        int s2r = countWords(s2);
        assert(s2r == 4); // Expected: 4

        // Test Case 3: Sentence with newlines and tabs
        char *s3 = "This is\ta test\nwith new lines";
        int s3r = countWords(s3);
        assert(s3r == 7); // Expected: 7

        // Test Case 4: Empty string
        char *s4 = "";
        int s4r = countWords(s4);
        assert(s4r == 0); // Expected: 0

        // Test Case 5: String with only spaces
        char *s5 = "     ";
        int s5r = countWords(s5);
        assert(s5r == 0); // Expected: 0

        // Test Case 6: Sentence with numbers and words
        char *s6 = "There are 2 numbers in this sentence";
        int s6r = countWords(s6);
        assert(s6r == 7); // Expected: 7

        // Test Case 7: Long sentence
        char *s7 = "This is a much longer sentence with several words in it";
        int s7r = countWords(s7);
        assert(s7r == 11); // Expected: 11

        // Test Case 8: String with a single word
        char *s8 = "Word";
        int s8r = countWords(s8);
        assert(s8r == 1); // Expected: 1

        // Test Case 9: String with mixed case
        char *s9 = "This String Has Mixed Case";
        int s9r = countWords(s9);
        assert(s9r == 5); // Expected: 5

        // Test Case 10: Sentence with a leading and trailing space
        char *s10 = " leading and trailing spaces ";
        int s10r = countWords(s10);
        assert(s10r == 4); // Expected: 4

        int *in1 = (int *)malloc(sizeof(int));
        int *in2 = (int *)malloc(sizeof(int));
        int *in3 = (int *)malloc(sizeof(int));
        int *in4 = (int *)malloc(sizeof(int));
        int *in5 = (int *)malloc(sizeof(int));
        int *in6 = (int *)malloc(sizeof(int));
        int *in7 = (int *)malloc(sizeof(int));
        int *in8 = (int *)malloc(sizeof(int));
        int *in9 = (int *)malloc(sizeof(int));
        int *in10 = (int *)malloc(sizeof(int));

        char **result1 = splitWords(s1, in1);
        char **result2 = splitWords(s2, in2);
        char **result3 = splitWords(s3, in3);
        char **result4 = splitWords(s4, in4);
        char **result5 = splitWords(s5, in5);
        char **result6 = splitWords(s6, in6);
        char **result7 = splitWords(s7, in7);
        char **result8 = splitWords(s8, in8);
        char **result9 = splitWords(s9, in9);
        char **result10 = splitWords(s10, in10);

        // Print the results
        printCharArray(result1);
        printCharArray(result2);
        printCharArray(result3);
        printCharArray(result4);
        printCharArray(result5);
        printCharArray(result6);
        printCharArray(result7);
        printCharArray(result8);
        printCharArray(result9);
        printCharArray(result10);

        // Destroy the results
        destroy(result1);
        destroy(result2);
        destroy(result3);
        destroy(result4);
        destroy(result5);
        destroy(result6);
        destroy(result7);
        destroy(result8);
        destroy(result9);
        destroy(result10);

        free(in1);
        free(in2);
        free(in3);
        free(in4);
        free(in5);
        free(in6);
        free(in7);
        free(in8);
        free(in9);
        free(in10);

        printf("TC2 Succeses\n");
    }

    if (TC3) {
        char *s1 = malloc(strlen("word") + 1);
        char *s2 = malloc(strlen("Word") + 1);
        strcpy(s1, "word");
        strcpy(s2, "Word");

        toCamelCase(s1, 1);
        printf("all lower: %s\n", s1);
        toCamelCase(s1, 0);
        printf("cap first: %s\n", s1);
        toCamelCase(s2, 0);
        printf("cap first: %s\n", s2);
        toCamelCase(s2, 1);
        printf("all lower: %s\n", s2);

        free(s1);
        free(s2);
    }
    
    if (TC4) {
        char *s1 = " example ! example , example";
        char **s1r = camel_caser(s1);

        char *s2 = "A sentence without any punctuation";
        char **s2r = camel_caser(s2);

        char *s3 = "Wait... What??? Really!!";
        char **s3r = camel_caser(s3);

        char *s4 = "     ";
        char **s4r = camel_caser(s4);

        char *s5 = "...Start in the middle? Yes! End.";
        char **s5r = camel_caser(s5);

        char *s6 = "Hello world! How are you? I'm fine; thanks for asking.";
        char **s6r = camel_caser(s6);

        char *s7 = "Hello";
        char **s7r = camel_caser(s7);

        char *s8 = "First sentence... Second sentence: here it is! And here's the fourth?";
        char **s8r = camel_caser(s8);

        char *s9 = "This is a long sentence, with several commas, but it's still one sentence NOT.";
        char **s9r = camel_caser(s9);

        char *s10 = "\"Is this real life?\" he asked. \"Or is this just fantasy?\" she replied.";
        char **s10r = camel_caser(s10);

        printf("--------\n");

        // Print the results
        printCharArray(s1r);
        printCharArray(s2r);
        printCharArray(s3r);
        printCharArray(s4r);
        printCharArray(s5r);
        printCharArray(s6r);
        printCharArray(s7r);
        printCharArray(s8r);
        printCharArray(s9r);
        printCharArray(s10r);

        // Destroy the results
        destroy(s1r);
        destroy(s2r);
        destroy(s3r);
        destroy(s4r);
        destroy(s5r);
        destroy(s6r);
        destroy(s7r);
        destroy(s8r);
        destroy(s9r);
        destroy(s10r);

        printf("TC4 Succeses\n");
    }
    // Feel free to add more test cases of your own!
    // if (test_camelCaser(&camel_caser, &destroy)) {
    //     printf("SUCCESS\n");
    // } else {
    //     printf("FAILED\n");
    // }
}
