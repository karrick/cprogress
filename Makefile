ansi: ansi.c
	$(CC) -o $@ $<

nc: nc.c
	$(CC) -o $@ $< -lncurses
