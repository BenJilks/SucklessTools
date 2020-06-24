CC=c++
AR=ar
CXXFLAGS=-std=c++17 -g
OBJ=main.o \
    prompt.o \
    cleaner.o \
    database.o \
    chunk.o \
    dynamicdata.o \
    table.o \
    column.o \
    row.o \
    entry.o \
    sql/lexer.o \
    sql/parser.o \
    sql/select.o \
    sql/insert.o \
    sql/createtable.o \
    sql/update.o \
    sql/delete.o \
    sql/value.o

HEADER=cleaner.hpp \
    database.hpp \
    chunk.hpp \
    dynamicdata.hpp \
    table.hpp \
    column.hpp \
    row.hpp \
    entry.hpp \
    forward.hpp

SQL_HEADER=sql/sql.hpp \
    sql/lexer.hpp \
    sql/parser.hpp \
    sql/select.hpp \
    sql/insert.hpp \
    sql/createtable.hpp \
    sql/update.hpp \
    sql/delete.hpp \
    sql/value.hpp

ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

%.o: %.cpp
	$(CC) -c -o $@ $< $(CXXFLAGS)

all: $(OBJ)
	$(CC) -o database $^ $(CXXFLAGS)
#	$(AR) rs libdatabase.a $(OBJ)

install:
	mkdir -p $(PREFIX)/include/database
	mkdir -p $(PREFIX)/include/database/sql
	install -m 644 libdatabase.a $(PREFIX)/lib/
	install -m 644 $(HEADER) $(PREFIX)/include/database/
	install -m 644 $(SQL_HEADER) $(PREFIX)/include/database/sql/

uninstall:
	$(RM) -r $(PREFIX)/include/libdatabase
	$(RM) $(PREFIX)/lib/libdatabase.a

clean:
	$(RM) $(OBJ) 
	$(RM) database libdatabase.a
