EXEC1 = threeAndTwo
EXEC2 = abcd
EXEC3 = producerConsumer
EXEC4 = littleRedHen

all: $(EXEC1) $(EXEC2) $(EXEC3) $(EXEC4)
 
$(EXEC1): $(EXEC1).c
	gcc -pthread -o $@ $^

$(EXEC2): $(EXEC2).c
	gcc -pthread -o $@ $^

$(EXEC3): $(EXEC3).c
	gcc -pthread -o $@ $^

$(EXEC4): $(EXEC4).c
	gcc -pthread -o $@ $^

clean:
	rm $(EXEC1) $(EXEC2) $(EXEC3) $(EXEC4) 
