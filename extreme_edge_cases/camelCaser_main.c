/**
 * extreme_edge_cases
 * CS 341 - Spring 2024
 */
#include <stdio.h>
#include <stdlib.h>

#include "camelCaser.h"
#include "camelCaser_tests.h"

int main() {
    // testing count sentences
    char* s1 = " example ! example , example";
    int s1r = countSentences(s1);
    printf("%d, should be 3\n", s1r);

    char *s2 = "A sentence without any punctuation";
    int s2r = countSentences(s2);
    printf("%d, should be 1\n", s2r); // Expected output: 1

    char *s3 = "Wait... What??? Really!!";
    int s3r = countSentences(s3);
    printf("%d, should be 3\n", s3r); // Expected output: 3

    char *s4 = "     ";
    int s4r = countSentences(s4);
    printf("%d, should be 0\n", s4r); // Expected output: 0

    char *s5 = "...Start in the middle? Yes! End.";
    int s5r = countSentences(s5);
    printf("%d, should be 3\n", s5r); // Expected output: 3

    char *s6 = "Hello world! How are you? I'm fine; thanks for asking.";
    int s6r = countSentences(s6);
    printf("%d, should be 4\n", s6r); // Expected output: 4

        char *s7 = "Hello";
    int s7r = countSentences(s7);
    printf("%d, should be 1\n", s7r); // Expected output: 1

    char *s8 = "First sentence... Second sentence: here it is! And here's the fourth?";
    int s8r = countSentences(s8);
    printf("%d, should be 4\n", s8r); // Expected output: 4

    char *s9 = "This is a long sentence, with several commas, but it's still one sentence NOT.";
    int s9r = countSentences(s9);
    printf("%d, should be 4\n", s9r); // Expected output: 4

    char *s10 = "\"Is this real life?\" he asked. \"Or is this just fantasy?\" she replied.";
    int s10r = countSentences(s10);
    printf("%d, should be 4\n", s10r); // Expected output: 4


    // Feel free to add more test cases of your own!
    // if (test_camelCaser(&camel_caser, &destroy)) {
    //     printf("SUCCESS\n");
    // } else {
    //     printf("FAILED\n");
    // }
}
