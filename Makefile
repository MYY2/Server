EXE=allocate


$(EXE): main.c
	cc -Wall -o $(EXE) $< -lm

format:
	clang-format -style=file -i *.c

clean:
	rm -f $(EXE)
