ansi: ansi.c
	cc -o $@ $<

nc: nc.c
	cc -o $@ $< -lncurses
