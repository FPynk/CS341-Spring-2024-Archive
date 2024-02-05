/**
 * vector
 * CS 341 - Spring 2024
 */
#include "sstring.h"
#include <assert.h>
#include <string.h>
void test_cstr_to_sstring_and_sstring_to_cstr() {
    const char *original = "Hello, World!";
    sstring *str = cstr_to_sstring(original);
    assert(str != NULL); // Ensure str is successfully created

    char *cstr = sstring_to_cstr(str);
    assert(strcmp(cstr, original) == 0); // Compare converted string to original

    free(cstr); // Remember to free the returned C-string
    sstring_destroy(str); // Clean up sstring
    printf("test_cstr_to_sstring_and_sstring_to_cstr passed\n");
}

void test_sstring_append() {
    sstring *str1 = cstr_to_sstring("abc");
    sstring *str2 = cstr_to_sstring("def");
    sstring *str3 = cstr_to_sstring("ghidsfjbasdhjfgaskjhdfgksafdsa");
    int len = sstring_append(str1, str2);
    assert(len == 6); // Check length after append
    len = sstring_append(str1, str3);
    assert(len == 36);

    char *result = sstring_to_cstr(str1);
    assert(strcmp(result, "abcdefghidsfjbasdhjfgaskjhdfgksafdsa") == 0); // Verify content after append

    free(result);
    sstring_destroy(str1);
    sstring_destroy(str2);
    sstring_destroy(str3);
    printf("test_sstring_append passed\n");
}

void test_sstring_split() {
    sstring *str = cstr_to_sstring("abcdeefg");
    vector *split_vec = sstring_split(str, 'e');
    assert(vector_size(split_vec) == 3); // Expected 3 segments

    // Validate each segment
    char *segment1 = vector_get(split_vec, 0);
    char *segment2 = vector_get(split_vec, 1);
    char *segment3 = vector_get(split_vec, 2);

    printf("segment1: %s\n", segment1);
    printf("segment2: %s\n", segment2);
    printf("segment3: %s\n", segment3);
    assert(strcmp(segment1, "abcd") == 0);
    assert(strcmp(segment2, "") == 0);
    assert(strcmp(segment3, "fg") == 0);

    vector_destroy(split_vec);
    sstring_destroy(str);
    printf("test_sstring_split passed\n");
}

void test_sstring_substitute() {
    sstring *replace_me = cstr_to_sstring("This is a {} day, {}!");
    int result1 = sstring_substitute(replace_me, 18, "{}", "friend");
    assert(result1 == 0); // Substitution should succeed

    char *after_first_sub = sstring_to_cstr(replace_me);
    assert(strcmp(after_first_sub, "This is a {} day, friend!") == 0);

    int result2 = sstring_substitute(replace_me, 0, "{}", "good");
    assert(result2 == 0); // Second substitution should succeed

    char *final_result = sstring_to_cstr(replace_me);
    assert(strcmp(final_result, "This is a good day, friend!") == 0);

    free(after_first_sub);
    free(final_result);
    sstring_destroy(replace_me);
    printf("test_sstring_substitute passed\n");
}

void test_sstring_slice() {
    sstring *slice_me = cstr_to_sstring("1234567890");
    char *slice = sstring_slice(slice_me, 2, 5);
    assert(strcmp(slice, "345") == 0); // Verify slice content

    free(slice);
    sstring_destroy(slice_me);
    printf("test_sstring_slice passed\n");
}

void test_sstring_split_hard() {
    sstring *str = cstr_to_sstring("aaaaaaaaaa");
    vector *split_vec = sstring_split(str, 'a');
    printf("vector size: %zu\n", vector_size(split_vec));
    assert(vector_size(split_vec) == 11); // Expected 3 segments

    // Validate each segment
    char *segment1 = vector_get(split_vec, 0);
    char *segment2 = vector_get(split_vec, 1);
    char *segment3 = vector_get(split_vec, 2);

    printf("segment1: %s\n", segment1);
    printf("segment2: %s\n", segment2);
    printf("segment3: %s\n", segment3);
    assert(strcmp(segment1, "") == 0);
    assert(strcmp(segment2, "") == 0);
    assert(strcmp(segment3, "") == 0);

    vector_destroy(split_vec);
    sstring_destroy(str);
    printf("test_sstring_split_hard passed\n");
}

int main(int argc, char *argv[]) {
    // TODO create some tests
    test_cstr_to_sstring_and_sstring_to_cstr();
    test_sstring_append();
    test_sstring_split();
    test_sstring_substitute();
    test_sstring_slice();
    test_sstring_split_hard();
    return 0;
}
