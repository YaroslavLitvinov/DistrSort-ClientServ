rm server_dsort.db client_dsort.db -f
/usr/local/bin/sqlite3 server_dsort.db < sql/db_server_dsort.sql
/usr/local/bin/sqlite3 client_dsort.db < sql/db_client_dsort.sql
ls -l *.db
echo "Databases created."