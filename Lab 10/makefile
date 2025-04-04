run: lru.cpp
	g++ -Wall -o runsearch lru.cpp
	./runsearch > op.txt

vrun: lru.cpp
	g++ -Wall -DVERBOSE -o runsearch lru.cpp
	./runsearch > op_verb.txt

db: gensearch.c
	gcc -Wall -o gensearch gensearch.c
	./gensearch

clean:
	-rm -f runsearch gensearch op.txt op_verb.txt