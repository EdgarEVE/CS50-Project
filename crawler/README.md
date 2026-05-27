
Compile:
gcc -Wall -pedantic -std=c11 -ggdb -I../common -I../libcs50 crawler.c ../common/pagedir.o ../libcs50/libcs50.a -o crawler -Debug

gcc -Wall -pedantic -std=c11 -ggdb -I../common -I../libcs50 -c ../common/pagedir.c -o ../common/pagedir.o