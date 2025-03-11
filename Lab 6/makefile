all:
	gcc -Wall -g -o cook cook.c
	gcc -Wall -g -o waiter waiter.c
	gcc -Wall -g -o customer customer.c
db:
	gcc -Wall -o gencustomers gencustomers.c
	./gencustomers > customers.txt

run:
	gnome-terminal -- bash -c "./cook; exec bash";
	gnome-terminal -- bash -c "./waiter; exec bash";
	gnome-terminal -- bash -c "./customer; exec bash";

clean:
	-rm -f cook waiter customer gencustomers