
WHERE = ../cudd-2.3.1

INCLUDE = $(WHERE)/include

LIBS	= $(WHERE)/cudd/libcudd.a $(WHERE)/mtr/libmtr.a \
	$(WHERE)/st/libst.a $(WHERE)/util/libutil.a $(WHERE)/epd/libepd.a
#$(WHERE)/obj/libobj.a
net_nand: main.cc print.h node.h
	g++ -march=nocona -Wno-deprecated -g -o net_nand main.cc $(LIBS) -I$(INCLUDE)
