enum my_access {
	_O_RDONLY = 0,
	_O_WRONLY = 1,
	_O_RDWR = 2
};

enum my_whence {
	_SEEK_SET = 0,
	_SEEK_CUR = 1,
	_SEEK_END = 2
};

struct my_open_params {
	string path<>;
	my_access my_access_flag;
};

struct my_creat_params {
	string path<>;
};

struct my_read_params {
	string path<>;
	int offset;
	int count;
};

struct my_write_params {
	string path<>;
	int offset;
	string buf<>;
	int buf_size;
};

struct my_lseek_params {
	string path<>;
	int offset;
	int offset_to_set;
	my_whence my_whence_flag;
};

struct my_close_params {
	string path<>;
};


union my_open_creat_results switch (int status) {
	case -1: int my_errno;
	default: int success;
};

union my_read_results switch (int status) {
	case -1: int my_errno;
	default: string buf<>;
};

union my_write_results switch (int status) {
	case -1: int my_errno;
	default: int bytes_written;
};

union my_lseek_results switch (int status) {
	case -1: int my_errno;
	default: int offset_location;
};

union my_close_results switch (int status) {
	case -1: int my_errno;
	default: int success;
};


program MY_NFS {
	version MY_NFS_V1 {
		my_open_creat_results 	my_open(my_open_params) = 1;
		my_open_creat_results 	my_creat(my_creat_params) = 2;
		my_read_results 		my_read(my_read_params) = 3;
		my_write_results 		my_write(my_write_params) = 4;
		my_lseek_results 		my_lseek(my_lseek_params) = 5;
		my_close_results 		my_close(my_close_params) = 6;
	} = 1;  
} = 0x20109765;
