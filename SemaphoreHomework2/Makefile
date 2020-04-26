all: balloon threejobs band pri_sem or_sem

balloon: balloon.c
	gcc -pthread -o $@ $^

threejobs: threeJobs.c
	gcc -pthread -o $@ $^

band: band.c
	gcc -pthread -o $@ $^

pri_sem: pri_sem.c
	gcc -pthread -o $@ $^

or_sem: or_sem.c
	gcc -pthread -o $@ $^

clean:
	rm balloon threejobs band pri_sem or_sem
