create table files(nodename text, ftype varchar(3), fmode character(1), fpath text, fd int);
insert into files values('source', 'std', 'r', '/in/stdin', 0);
insert into files values('source', 'std', 'w', '/out/stdout', 1);
insert into files values('source', 'std', 'w', '/out/stderr', 2);
insert into files values('source', 'msq', 'w', '/out/histogram', 3);
insert into files values('source', 'msq', 'r', '/in/detailed_histogram_request', 4);
insert into files values('source', 'msq', 'w', '/out/detailed_histogram_reply', 5);
insert into files values('source', 'msq', 'r', '/in/sequences_request', 6);
insert into files values('source', 'msq', 'w', '/out/ranges0', 7);
insert into files values('source', 'msq', 'w', '/out/ranges1', 8);
insert into files values('source', 'msq', 'w', '/out/ranges2', 9);
insert into files values('source', 'msq', 'w', '/out/ranges3', 10);
insert into files values('source', 'msq', 'w', '/out/ranges4', 11);
insert into files values('source', 'msq', 'r', '/in/ranges0', 12);
insert into files values('source', 'msq', 'r', '/in/ranges1', 13);
insert into files values('source', 'msq', 'r', '/in/ranges2', 14);
insert into files values('source', 'msq', 'r', '/in/ranges3', 15);
insert into files values('source', 'msq', 'r', '/in/ranges4', 16);

insert into files values('dest', 'std', 'r', '/in/stdin', 0);
insert into files values('dest', 'std', 'w', '/out/stdout', 1);
insert into files values('dest', 'std', 'w', '/out/stderr', 2);
insert into files values('dest', 'msq', 'r', '/in/ranges', 3);
insert into files values('dest', 'msq', 'w', '/out/ranges', 4);
insert into files values('dest', 'msq', 'w', '/out/sorted_range', 5);


insert into files values('manager', 'std', 'r', '/in/stdin', 0);
insert into files values('manager', 'std', 'w', '/out/stdout', 1);
insert into files values('manager', 'std', 'w', '/out/stderr', 2);
insert into files values('manager', 'msq', 'r', '/in/histograms', 3);
insert into files values('manager', 'msq', 'w', '/out/detailed_histogram_request0', 4);
insert into files values('manager', 'msq', 'w', '/out/detailed_histogram_request1', 5);
insert into files values('manager', 'msq', 'w', '/out/detailed_histogram_request2', 6);
insert into files values('manager', 'msq', 'w', '/out/detailed_histogram_request3', 7);
insert into files values('manager', 'msq', 'w', '/out/detailed_histogram_request4', 8);
insert into files values('manager', 'msq', 'r', '/in/detailed_histogram_reply0', 9);
insert into files values('manager', 'msq', 'r', '/in/detailed_histogram_reply1', 10);
insert into files values('manager', 'msq', 'r', '/in/detailed_histogram_reply2', 11);
insert into files values('manager', 'msq', 'r', '/in/detailed_histogram_reply3', 12);
insert into files values('manager', 'msq', 'r', '/in/detailed_histogram_reply4', 13);
insert into files values('manager', 'msq', 'w', '/out/range-request0', 14);
insert into files values('manager', 'msq', 'w', '/out/range-request1', 15);
insert into files values('manager', 'msq', 'w', '/out/range-request2', 16);
insert into files values('manager', 'msq', 'w', '/out/range-request3', 17);
insert into files values('manager', 'msq', 'w', '/out/range-request4', 18);
insert into files values('manager', 'msq', 'r', '/in/sorted_range', 19);

insert into files values('test1PUSH', 'msq', 'w', '/out/test1', 10);
insert into files values('test1PULL', 'msq', 'r', '/in/test1', 11);
insert into files values('test2REQ', 'msq', 'w', '/out/test2', 12);
insert into files values('test2REQ', 'msq', 'r', '/in/test2', 13);
insert into files values('test2REP', 'msq', 'r', '/in/test2', 14);
insert into files values('test2REP', 'msq', 'w', '/out/test2', 15);

