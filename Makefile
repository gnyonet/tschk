#
# TSデータのチェックプログラム
#
CFLAGS	+= -g
LDFLAGS	+= -g
PROG	+= tschk

all: tschk

tschk: tschk.o
	$(CC) -o $@ $(LDFLAGS) $+

%.o:%.c
	$(CC) -c $(CFLAGS) $<

clean:
	$(RM) $(PROG) $(PROG:=.o)
