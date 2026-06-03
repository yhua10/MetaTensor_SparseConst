lcov -c -d ./ -o tests.info
genhtml --title "lov_result" --legend -o coverage tests.info
