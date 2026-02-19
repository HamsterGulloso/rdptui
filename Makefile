run: rdpconnect
	./rdpconnect

rdpconnect: main.c
	gcc -o rdpconnect main.c
