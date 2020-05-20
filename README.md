# cprogress

two progress bars for c

A couple of small C libraries for a progress bar. The ANSI one is
working, although I still need to deal with the case where the length
of the message is too long for the progress bar window.

The ncurses one is working but is not as full featured as the ANSI
version is. Namely I would like to be able to use ncurses without
creating a window, but I am not yet sure if that is even possible,
which is why I then just made the ANSI version.
