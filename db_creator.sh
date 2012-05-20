rm server_dsort.db client_dsort.db -f
~/sqlite-autoconf-3071200/sqlite3 server_dsort.db < sql/db_server_dsort.sql
~/sqlite-autoconf-3071200/sqlite3 client_dsort.db < sql/db_client_dsort.sql
ls -l *.db
echo "Databases created."