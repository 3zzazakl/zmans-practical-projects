#!/bin/bash
COMPILER="./c_mini_compiler"
TEST_DIR="."

echo "========== COMPILING COMPILER =========="
gcc -std=c99 -Wall -o c_mini_compiler c_mini_compiler.c

if [ ! -f "$COMPILER" ]; then
    echo "Compilation failed!"
    exit 1
fi

echo -e "\n========== RUNNING TESTS ==========\n"

# Tests that should PASS
for test in test1_basic test2_if test3_while test4_compound test5_float test12_comments; do
    echo "--- Running $test.cmini (should succeed) ---"
    $COMPILER $TEST_DIR/${test}.cmini
    echo ""
done

# Tests that should FAIL (syntax or semantic errors)
for test in test6_invalid_undeclared test7_invalid_type_mismatch test8_invalid_missing_semi \
            test9_invalid_missing_paren test10_invalid_duplicate_decl test11_invalid_bad_char; do
    echo "--- Running $test.cmini (should FAIL) ---"
    $COMPILER $TEST_DIR/${test}.cmini
    echo ""
done
