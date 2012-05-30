rm server_dsort.db client_dsort.db -f
/usr/local/bin/sqlite3 server_dsort.db < sql/db_server_dsort-50.sql
/usr/local/bin/sqlite3 client_dsort.db < sql/db_client_dsort-50.sql
ls -l *.db
echo "Databases created."