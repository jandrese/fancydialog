CC=clang
CFLAGS=-g -Wall -pedantic --std=gnu17 `pkg-config --cflags gtk+-3.0`
LDLIBS=`pkg-config --libs gtk+-3.0`

ALL: fancydialog
