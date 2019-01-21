APP=gen-expr

$(APP): gen-expr.c
	gcc -fsanitize=address -O2 -Wall -Werror -o $@ $<

.PHONY: clean
clean:
	-rm $(APP) .code.c .expr
