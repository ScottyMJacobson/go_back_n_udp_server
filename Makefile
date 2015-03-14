all: testclient testserver

testclient: clienta3.c
	gcc -Wall -Wextra clienta3.c -o testclient

testserver: a3.c
	gcc -Wall -Wextra a3.c -o testserver

clean:
	rm testclient testserver