

CFLAGS = -I../include

objs	:	complex.o	dslash.o

%.o	:	%.c
	$(CC) -c $< $(CFLAGS) -o $@



#test	:	test.c
#	gcc -g -o test -fprofile-arcs -ftest-coverage test.c ../src/*.c ../plugins/*.c ../targets/*.c -lgc -I ../include

clean	:
	rm -rf *.o test *.gcda *.gcno *.c.gcov *.info ./coverage dump.c

# 添加的新规则以使用 GDB 启动调试
debug : test
	    gdb ./$<


.PHONY: all clean debug
