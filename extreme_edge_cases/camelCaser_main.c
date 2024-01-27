/**
 * extreme_edge_cases
 * CS 341 - Spring 2024
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "camelCaser.h"
#include "camelCaser_tests.h"

int main() {
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

    printCharArray(splitSentences(s1, in1));
    printCharArray(splitSentences(s2, in2));
    printCharArray(splitSentences(s3, in3));
    printCharArray(splitSentences(s4, in4));
    printCharArray(splitSentences(s5, in5));
    printCharArray(splitSentences(s6, in6));
    printCharArray(splitSentences(s7, in7));
    printCharArray(splitSentences(s8, in8));
    printCharArray(splitSentences(s9, in9));
    printCharArray(splitSentences(s10, in10));

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
    // Feel free to add more test cases of your own!
    // if (test_camelCaser(&camel_caser, &destroy)) {
    //     printf("SUCCESS\n");
    // } else {
    //     printf("FAILED\n");
    // }
}
