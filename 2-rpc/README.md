my_open_creat_results 	my_open(my_open_params) = 1; 		// path, access_flag
my_open_creat_results 	my_creat(my_creat_params) = 2; 		// path
my_read_results 		my_read(my_read_params) = 3; 		// fd, count
my_write_results 		my_write(my_write_params) = 4; 		// fd, buf, buf_size
my_lseek_results 		my_lseek(my_lseek_params) = 5;		// fd, offset, whence_flag
my_close_results 		my_close(my_close_params) = 6;		// fd

rpcgen mynfs.x -a
make
sudo ./mynfs_server
./mynfs_client localhost -- tryb demo

./mynfs_client localhost open path access_flag	-- O_RDONLY O_WRONLY O_RDWR
./mynfs_client localhost creat path
./mynfs_client localhost read fd count
./mynfs_client localhost write fd text
./mynfs_client localhost lseek fd offset whence_flag  -- SEEK_SET SEEK_CUR SEEK_END
./mynfs_client localhost close fd