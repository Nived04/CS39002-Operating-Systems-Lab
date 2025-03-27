all:
	g++ -Wall -g -o resource -pthread resource.cpp
	g++ -Wall -g -D_DLAVOID -o resource_nodeadlock -pthread resource.cpp

allow: all
	./resource

avoid: all
	./resource_nodeadlock

db: geninput.c
	gcc -Wall -o geninput geninput.c
	./geninput 10 20

clean:
	-rm -f resource resource_nodeadlock geninput