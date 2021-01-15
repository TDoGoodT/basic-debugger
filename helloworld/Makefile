helloworld: start.o helloworld.o
	ld $^ -o $@

%.o: %.asm
	as $^ -o $@
