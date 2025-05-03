all:
	gcc client.c -o client 
	gcc reactor.c -o reactor 

clean:
	rm -rf reactor client