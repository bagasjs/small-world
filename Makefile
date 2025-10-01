CC := clang
CFLAGS := -Wall -Wextra -pedantic -g -fsanitize=address
LFLAGS := -lraylibdll

main.exe: ./small_world.c
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)
