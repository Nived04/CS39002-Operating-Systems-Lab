run: demandpaging.cpp
	g++ -Wall -o runsearch demandpaging.cpp
	./runsearch > op_1.txt

vrun: demandpaging.cpp
	g++ -Wall -DVERBOSE -o runsearch demandpaging.cpp
	./runsearch > op_verb.txt

db: gensearch.c
	gcc -Wall -o gensearch gensearch.c
	./gensearch

clean:
	-rm -f runsearch gensearch