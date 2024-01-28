/**
 * extreme_edge_cases
 * CS 341 - Spring 2024
 */
#include <stdio.h>

#include "camelCaser_ref_utils.h"
#include "camelCaser.h"
#include "camelCaser.c"

int main() {
    // // Enter the string you want to test with the reference here.
    // char *input = "hello. welcome to cs241";

    // // This function prints the reference implementation output on the terminal.
    // print_camelCaser(input);

    // // Put your expected output for the given input above.
    // char *correct[] = {"hello", NULL};
    // char *wrong[] = {"hello", "welcomeToCs241", NULL};

    // // Compares the expected output you supplied with the reference output.
    // printf("check_output test 1: %d\n", check_output(input, correct));
    // printf("check_output test 2: %d\n", check_output(input, wrong));


    // char* test = "The Heisenbug is an incredible creature. Facenovel servers get their power from its indeterminism. Code smell can be ignored with INCREDIBLE use of air freshener. God objects are the new religion. sdafasdf safd";
    // print_camelCaser(test);

    // char* test1 = "asdffd";
    // print_camelCaser(test1);
    // Feel free to add more test cases.

    
    char *s1 = " example ! example , example";
    char **s1r = camel_caser(s1);
    printf("check_output test 1: %d\n", check_output(s1, s1r));

    char *s2 = "A sentence without any punctuation";
    char **s2r = camel_caser(s2);
    // print_camelCaser(s2);
    // printCharArray(s2r);
    // printPointerAddresses(s2r);
    // printf("s2r points to %p\n", s2r);
    printf("check_output test 2: %d\n", check_output(s2, s2r));
    
    char *s3 = "Wait... What??? Really!!";
    char **s3r = camel_caser(s3);
    printf("check_output test 3: %d\n", check_output(s3, s3r));

    char *s4 = "     ";
    char **s4r = camel_caser(s4);
    printf("check_output test 4: %d\n", check_output(s4, s4r));

    char *s5 = "...Start in the middle? Yes! End.";
    char **s5r = camel_caser(s5);
    printf("check_output test 5: %d\n", check_output(s5, s5r));

    char *s6 = "Hello world! How are you? I'm fine; thanks for asking.";
    char **s6r = camel_caser(s6);
    printf("check_output test 6: %d\n", check_output(s6, s6r));

    char *s7 = "Hello";
    char **s7r = camel_caser(s7);
    printf("check_output test 7: %d\n", check_output(s7, s7r));

    char *s8 = "First sentence... Second sentence: here it is! And here's the fourth?";
    char **s8r = camel_caser(s8);
    printf("check_output test 8: %d\n", check_output(s8, s8r));

    char *s9 = "This is a long sentence, with several commas, but it's still one sentence NOT.";
    char **s9r = camel_caser(s9);
    printf("check_output test 9: %d\n", check_output(s9, s9r));

    char *s10 = "\"Is this real life?\" he asked. \"Or is this just fantasy?\" she replied.";
    char **s10r = camel_caser(s10);
    printf("check_output test 10: %d\n", check_output(s10, s10r));
    
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

    // char *s11 = "This is\ta test\nwith new lines.";
    // print_camelCaser(s11);

    // char *s12 = "There are 2 numbers in this sentence.";
    // print_camelCaser(s12);
    return 0;
}
