1. Make sure queue.h, sut.c, sut.h, and the test files are in the same directory.
2. To test file test1.c, run: gcc test1.c sut.c -pthread && ./a.out
3. To change from 1 to 2 C-EXECs find the variable: two_c_exec_threads

mkdir output
gcc test1.c sut.c -pthread && ./a.out > output/test1-stdout.txt
gcc test2.c sut.c -pthread && ./a.out > output/test2-stdout.txt
gcc test3.c sut.c -pthread && ./a.out > output/test3-stdout.txt
gcc test4.c sut.c -pthread && ./a.out > output/test4-stdout.txt
gcc test5.c sut.c -pthread && ./a.out > output/test5-stdout.txt

mkdir output-2-c-execs
gcc test1.c sut.c -pthread && ./a.out > output-2-c-execs/test1-stdout.txt
gcc test2.c sut.c -pthread && ./a.out > output-2-c-execs/test2-stdout.txt
gcc test3.c sut.c -pthread && ./a.out > output-2-c-execs/test3-stdout.txt
gcc test4.c sut.c -pthread && ./a.out > output-2-c-execs/test4-stdout.txt
gcc test5.c sut.c -pthread && ./a.out > output-2-c-execs/test5-stdout.txt