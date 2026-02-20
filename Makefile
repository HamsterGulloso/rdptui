run: rdptui
	./rdptui

rdptui: main.c
	gcc -o rdptui main.c
