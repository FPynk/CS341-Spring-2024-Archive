/**
 * extreme_edge_cases
 * CS 341 - Spring 2024
 */
#include <stdio.h>

#include "camelCaser_ref_utils.h"
int main() {
    // int TC1 = 0;
    // int TC2 = 1;
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

    // if (TC1) {
    //     char *s1 = " example ! example , example";
    //     char **s1r = camel_caser(s1);
    //     printf("check_output test 1: %d\n", check_output(s1, s1r));

    //     char *s2 = "A sentence without any punctuation";
    //     char **s2r = camel_caser(s2);
    //     // print_camelCaser(s2);
    //     // printCharArray(s2r);
    //     // printPointerAddresses(s2r);
    //     // printf("s2r points to %p\n", s2r);
    //     printf("check_output test 2: %d\n", check_output(s2, s2r));
        
    //     char *s3 = "Wait... What??? Really!!";
    //     char **s3r = camel_caser(s3);
    //     printf("check_output test 3: %d\n", check_output(s3, s3r));

    //     char *s4 = "     ";
    //     char **s4r = camel_caser(s4);
    //     printf("check_output test 4: %d\n", check_output(s4, s4r));

    //     char *s5 = "...Start in the middle? Yes! End.";
    //     char **s5r = camel_caser(s5);
    //     printf("check_output test 5: %d\n", check_output(s5, s5r));

    //     char *s6 = "Hello world! How are you? I'm fine; thanks for asking.";
    //     char **s6r = camel_caser(s6);
    //     printf("check_output test 6: %d\n", check_output(s6, s6r));

    //     char *s7 = "Hello";
    //     char **s7r = camel_caser(s7);
    //     printf("check_output test 7: %d\n", check_output(s7, s7r));

    //     char *s8 = "First sentence... Second sentence: here it is! And here's the fourth?";
    //     char **s8r = camel_caser(s8);
    //     printf("check_output test 8: %d\n", check_output(s8, s8r));

    //     char *s9 = "This is a long sentence, with several commas, but it's still one sentence NOT.";
    //     char **s9r = camel_caser(s9);
    //     printf("check_output test 9: %d\n", check_output(s9, s9r));

    //     char *s10 = "\"Is this real life?\" he asked. \"Or is this just fantasy?\" she replied.";
    //     char **s10r = camel_caser(s10);
    //     printf("check_output test 10: %d\n", check_output(s10, s10r));
        
    //     destroy(s1r);
    //     destroy(s2r);
    //     destroy(s3r);
    //     destroy(s4r);
    //     destroy(s5r);
    //     destroy(s6r);
    //     destroy(s7r);
    //     destroy(s8r);
    //     destroy(s9r);
    //     destroy(s10r);
    // }
    
    // if (TC2) {
    //     // TC 1: Sentence with various punctuation marks
    //     char *test1 = "Hello! Is this: a test? Yes, it is.";
    //     char *sol1[] = {"hello", "isThis", "aTest", "yes", "itIs", NULL};
    //     printf("check_output test 1: %d\n", check_output(test1, sol1));


    //     // TC 2: Sentence with only one word and no punctuation
    //     char *test2 = "word";
    //     char *sol2[] = {NULL};
    //     printf("check_output test 1: %d\n", check_output(test2, sol2));


    //     // TC 3: Sentence with numbers and special characters
    //     char *test3 = "Test1234 with special@characters.";
    //     char *sol3[] = {"test1234WithSpecial", "characters", NULL};
    //     printf("check_output test 1: %d\n", check_output(test3, sol3));


    //     // TC4: String with multiple spaces and tabs
    //     char *test4 = "This   is\t a \ttest.";
    //     char *sol4[] = {"thisIsATest", NULL};
    //     printf("check_output test 1: %d\n", check_output(test4, sol4));


    //     // TC 5: Empty string
    //     char *test5 = "";
    //     char *sol5[] = {NULL};
    //     printf("check_output test 1: %d\n", check_output(test5, sol5));


    //     // TC 6: String with newlines
    //     char *test6 = "Line1\nLine2\nLine3.";
    //     char *sol6[] = {"line1Line2Line3", NULL};
    //     printf("check_output test 1: %d\n", check_output(test6, sol6));


    //     // TC 7: String with repeated punctuation
    //     char *test7 = "Repeated... punctuation???";
    //     char *sol7[] = {"repeated", "", "", "punctuation", "", "", NULL};
    //     printf("check_output test 1: %d\n", check_output(test7, sol7));


    //     // TC 8: String with only punctuation
    //     char *test8 = "!?.;";
    //     char *sol8[] = {"", "", "", "", NULL};
    //     printf("check_output test 1: %d\n", check_output(test8, sol8));


    //     // TC 9: Long string with mixed content
    //     char *test9 = "LongStringWith123 Numbers, words, and&Symbols.";
    //     char *sol9[] = {"longstringwith123Numbers", "words", "and", "symbols", NULL};
    //     printf("check_output test 1: %d\n", check_output(test9, sol9));


    //     // TC 10: String with uppercase letters
    //     char *test10 = "This IS A Test.";
    //     char *sol10[] = {"thisIsATest", NULL};
    //     printf("check_output test 1: %d\n", check_output(test10, sol10));


    //     // TC 11: String with Mixed Case and Numbers
    //     char *test11 = "This1 Is2 A Test3.";
    //     char *sol11[] = {"this1Is2ATest3", NULL};
    //     printf("check_output test 1: %d\n", check_output(test11, sol11));


    //     // TC 12: String with Mixed Case and Numbers
    //     char *test12 = "   \t\n  \t ";
    //     char *sol12[] = {NULL};
    //     printf("check_output test 1: %d\n", check_output(test12, sol12));


    //     // TC 13: String with Mixed Case and Numbers
    //     char *test13 = "This has\\nnewlines and\\ttabs.";
    //     char *sol13[] = {"thisHas", "nnewlinesAnd", "ttabs", NULL};
    //     printf("check_output test 1: %d\n", check_output(test13, sol13));


    //     // TC 14: String with Mixed Case and Numbers
    //     char *test14 = "Sentence ends here.    ";
    //     char *sol14[] = {"sentenceEndsHere", NULL};
    //     printf("check_output test 1: %d\n", check_output(test14, sol14));


    //     // TC 15: String with Mixed Case and Numbers
    //     char *test15 = "Well-known compound-words are here.";
    //     char *sol15[] = {"well", "knownCompound", "wordsAreHere", NULL};
    //     printf("check_output test 1: %d\n", check_output(test15, sol15));


    //     // TC 16: String with Mixed Case and Numbers
    //     char *test16 = "Special #$% characters 123 with numbers 4567.";
    //     char *sol16[] = {"special", "", "", "characters123WithNumbers4567", NULL};
    //     printf("check_output test 1: %d\n", check_output(test16, sol16));

    // }
    // char *s11 = "This is\ta test\nwith new lines.";
    // print_camelCaser(s11);

    // char *s12 = "There are 2 numbers in this sentence.";
    // print_camelCaser(s12);
    return 0;
}
