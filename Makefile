GCOV_FLAGS= --coverage -DDEBUG -g3
#ABS path is needed to correct gcov, gcov is used to get test coverage for server side code
ABS_PATH=$(shell pwd)/
#used -std=gnu99 instead -std=c99 to do compile FUSE
FUSE_C99=-std=gnu99 -D_POSIX_C_SOURCE=200809L
FUSE_FLAGS=-D_FILE_OFFSET_BITS=64 -I/usr/local/include/fuse
SQLITE_FLAGS=-DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION
DEBUG=-g -DLOG_ENABLE $(GCOV_FLAGS)
SRCNODE_INCL=-I. -Iclient -Iclient/source -Isqlite
MANNODE_INCL=-I. -Iclient -Iclient/manager -Isqlite

all: createdirs zf-server dst_node src_node man_node test_node tests/zmq_netw_test tests/sqluse_srv_test tests/sqluse_cli_test
	tests/zmq_netw_test
	tests/sqluse_srv_test
	tests/sqluse_cli_test
	
createdirs:
	mkdir -p obj log tests data

gcov: clean all
	lcov --directory . --capture --output-file app.info
	genhtml --output-directory cov_htmp app.info
	@echo run $(ABS_PATH)cov_htmp/index.html

zf-server: obj/main.o obj/fs_inmem.o obj/sqlite3.o obj/sqluse_srv.o obj/zmq_netw.o obj/logfile.o
	@gcc -o zf-server obj/main.o obj/fs_inmem.o obj/sqlite3.o obj/sqluse_srv.o obj/zmq_netw.o obj/logfile.o -I. -I./sqlite \
	-I. -I./server -Wall -lzmq -lfuse $(DEBUG)

man_node: obj/main_man.o obj/sqluse_cli.o obj/comm_man.o obj/histanlz.o obj/sort.o obj/dsort.o obj/sqlite3.o obj/logfile.o
	@gcc -o man_node obj/main_man.o obj/sqluse_cli.o obj/comm_man.o obj/histanlz.o obj/sort.o obj/dsort.o \
	obj/sqlite3.o obj/logfile.o -Wall -std=c99 $(DEBUG)
	
src_node: obj/main_src.o obj/sqluse_cli.o obj/sort.o obj/dsort.o obj/comm_src.o obj/sqlite3.o obj/logfile.o
	@gcc -o src_node obj/main_src.o obj/sqluse_cli.o obj/sort.o obj/dsort.o obj/sqlite3.o obj/comm_src.o  \
	obj/logfile.o $(SRCNODE_INCL) $(DEBUG) -Wall -std=c99 
	
dst_node: obj/main_dst.o obj/sqluse_cli.o obj/sort.o obj/dsort.o obj/comm_dst.o obj/sqlite3.o obj/logfile.o
	@gcc -o dst_node obj/main_dst.o obj/sqluse_cli.o obj/sort.o obj/dsort.o obj/sqlite3.o obj/comm_dst.o  \
	obj/logfile.o $(SRCNODE_INCL) $(DEBUG) -Wall -std=c99 

test_node: obj/main_test.o obj/sqluse_cli.o obj/sqlite3.o obj/logfile.o
	@gcc -o test_node obj/main_test.o obj/sqluse_cli.o obj/sqlite3.o obj/logfile.o -Wall -std=c99 -lzmq $(DEBUG)
	
tests/zmq_netw_test: server/zmq_netw_test.cc obj/zmq_netw.o obj/logfile.o obj/sqluse_srv.o obj/sqlite3.o
	@g++ -o tests/zmq_netw_test $(ABS_PATH)server/zmq_netw_test.cc obj/zmq_netw.o obj/logfile.o obj/sqluse_srv.o \
	obj/sqlite3.o -lzmq -Lgtest -lgtest -I. -Igtest -Iserver $(DEBUG)
	
tests/sqluse_srv_test: server/sqluse_srv_test.cc obj/sqluse_srv.o obj/logfile.o obj/sqlite3.o
	@g++ -o tests/sqluse_srv_test $(ABS_PATH)server/sqluse_srv_test.cc obj/logfile.o obj/sqluse_srv.o \
	obj/sqlite3.o -lpthread -Lgtest -lgtest -I. -Igtest -Iserver $(DEBUG)

tests/sqluse_cli_test: client/sqluse_cli_test.cc obj/sqluse_cli.o obj/logfile.o obj/sqlite3.o
	@g++ -o tests/sqluse_cli_test $(ABS_PATH)client/sqluse_cli_test.cc obj/logfile.o obj/sqluse_cli.o \
	obj/sqlite3.o -lpthread -Lgtest -lgtest -I. -Igtest -Iserver $(DEBUG)


#for all clients and server
obj/sqlite3.o:
	@gcc -c -o obj/sqlite3.o $(ABS_PATH)sqlite/sqlite3.c -I./sqlite $(SQLITE_FLAGS)

obj/logfile.o: logfile.c logfile.h
	@gcc -c -o obj/logfile.o $(ABS_PATH)logfile.c -I. -Wall $(DEBUG)

#server side
obj/fs_inmem.o: server/fs_inmem.c
	@gcc -c -o obj/fs_inmem.o $(ABS_PATH)server/fs_inmem.c -I. -I./server -Wall $(FUSE_FLAGS) $(FUSE_C99) $(DEBUG)

#server side
obj/main.o: server/main.c defines.h
	@gcc -c -o obj/main.o $(ABS_PATH)server/main.c -I. -I./server -Wall $(FUSE_FLAGS) $(FUSE_C99) $(DEBUG)
	
#server side
obj/sqluse_srv.o: server/sqluse_srv.c
	@gcc -c -o obj/sqluse_srv.o $(ABS_PATH)server/sqluse_srv.c -I. -Iserver -Isqlite $(DEBUG) -Wall -std=c99

#server side
obj/zmq_netw.o: server/zmq_netw.c
	@gcc -c -o obj/zmq_netw.o $(ABS_PATH)server/zmq_netw.c -I. -Iserver $(DEBUG) -Wall -std=c99

#client side: any node
obj/sqluse_cli.o: client/sqluse_cli.c defines.h
	@gcc -c -o obj/sqluse_cli.o $(ABS_PATH)client/sqluse_cli.c $(SRCNODE_INCL) $(DEBUG) -Wall -std=c99

#client side source node & dest node
obj/dsort.o: client/dsort.c
	@gcc -c -o obj/dsort.o client/dsort.c $(SRCNODE_INCL) $(DEBUG) -Wall -std=c99

#client side source node & dest node
obj/sort.o: client/sort.c
	@gcc -c -o obj/sort.o client/sort.c -I. -Iclient $(DEBUG) -Wall -std=c99

#client side: destination node
obj/main_dst.o: client/dest/main_dst.c defines.h
	@gcc -c -o obj/main_dst.o client/dest/main_dst.c $(SRCNODE_INCL) $(DEBUG) -Wall -std=c99

#client side: destination node
obj/comm_dst.o: client/dest/comm_dst.c defines.h
	@gcc -c -o obj/comm_dst.o client/dest/comm_dst.c $(MANNODE_INCL) $(DEBUG) -Wall -std=c99

#client side: source node
obj/main_src.o: client/source/main_src.c defines.h
	@gcc -c -o obj/main_src.o client/source/main_src.c $(SRCNODE_INCL) $(DEBUG) -Wall -std=c99

#client side: source node
obj/comm_src.o: client/source/comm_src.c defines.h
	@gcc -c -o obj/comm_src.o client/source/comm_src.c $(MANNODE_INCL) $(DEBUG) -Wall -std=c99

#client side: manager node
obj/main_man.o: client/manager/main_man.c defines.h
	@gcc -c -o obj/main_man.o client/manager/main_man.c  $(MANNODE_INCL) $(DEBUG) -Wall -std=c99

#client side: manager node
obj/comm_man.o: client/manager/comm_man.c defines.h
	@gcc -c -o obj/comm_man.o client/manager/comm_man.c  $(MANNODE_INCL) $(DEBUG) -Wall -std=c99

#client side: manager node
obj/histanlz.o: client/manager/histanlz.c defines.h
	@gcc -c -o obj/histanlz.o client/manager/histanlz.c  $(MANNODE_INCL) $(DEBUG) -Wall -std=c99

#client side: manager node
obj/main_test.o: client/main_test.c
	@gcc -c -o obj/main_test.o client/main_test.c  $(MANNODE_INCL) $(DEBUG) -Wall -std=c99

clean_gcov:
	find -name "*.gcda" | xargs rm -f
	find -name "*.gcno" | xargs rm -f
	rm cov_htmp -f -r

clean: clean_gcov
	rm *.o -f
	rm obj/*.o -f
	rm zf-server test_node man_node src_node dst_node tests/zmq_netw_test -f
	

