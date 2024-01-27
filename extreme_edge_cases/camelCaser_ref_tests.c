/**
 * extreme_edge_cases
 * CS 341 - Spring 2024
 */
#include <stdio.h>

#include "camelCaser_ref_utils.h"

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
    print_camelCaser(s1);
    
    char *s2 = "A sentence without any punctuation";
    print_camelCaser(s2);

    char *s3 = "Wait... What??? Really!!";
    print_camelCaser(s3);

    char *s4 = "     ";
    print_camelCaser(s4);

    char *s5 = "...Start in the middle? Yes! End.";
    print_camelCaser(s5);

    char *s6 = "Hello world! How are you? I'm fine; thanks for asking.";
    print_camelCaser(s6);

    char *s7 = "Hello";
    print_camelCaser(s7);

    char *s8 = "First sentence... Second sentence: here it is! And here's the fourth?";
    print_camelCaser(s8);

    char *s9 = "This is a long sentence, with several commas, but it's still one sentence NOT.";
    print_camelCaser(s9);

    char *s10 = "\"Is this real life?\" he asked. \"Or is this just fantasy?\" she replied.";
    print_camelCaser(s10);

    return 0;
}
