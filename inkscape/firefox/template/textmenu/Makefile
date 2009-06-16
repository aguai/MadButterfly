SRC=main.c %n.c
OBJ=$(SRC:.c=.o)
CFLAGS+=`pkg-config --cflags libmbfly` `pkg-config --cflags pangocairo` 
LDFLAGS=`pkg-config --libs libmbfly`

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

%.so:%.mbsvg
	svg2code.py $< $(<:.mbsvg=.mb)
	m4 -I /usr/local/share/mb mb_c_header.m4 $(<:.mbsvg=.mb) > $(<:.mbsvg=.h)
	m4 -I /usr/local/share/mb mb_c_source.m4 $(<:.mbsvg=.mb) > $(<:.mbsvg=.c)
	$(CC) $(CFLAGS) -I../../include  -I ../../include -shared -o $@ $(<:.mbsvg=.c)
	
all: list.so textmenu

textmenu : $(OBJ)
	$(CC) -o textmenu $(LDFLAGS) $(OBJ) 
