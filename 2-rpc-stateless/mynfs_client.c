#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mynfs.h"

#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define RESET "\x1B[0m"

int _my_open(CLIENT* clnt, char* p, my_access ma) {
	my_open_creat_results  *result_1;
	my_open_params  my_open_1_arg;

	my_open_1_arg.path = p;
	my_open_1_arg.my_access_flag = ma;

	result_1 = my_open_1(&my_open_1_arg, clnt);
	if (result_1 == (my_open_creat_results *) NULL) {
		clnt_perror (clnt, "open call failed");
	} else if (result_1->status == -1) {
		errno = result_1->my_open_creat_results_u.my_errno;
		perror(KRED "--SERVER open error" RESET);
	} else {
		printf(KGRN "####\nOPEN DONE, %s\n####\n" RESET, p);
		return result_1->my_open_creat_results_u.success;
	}
	return -1;
}

int _my_creat(CLIENT* clnt, char* p) {
	my_open_creat_results  *result_2;
	my_creat_params  my_creat_1_arg;

	my_creat_1_arg.path = p;

	result_2 = my_creat_1(&my_creat_1_arg, clnt);
	if (result_2 == (my_open_creat_results *) NULL) {
		clnt_perror (clnt, "creat call failed");
	} else if (result_2->status == -1) {
		errno = result_2->my_open_creat_results_u.my_errno;
		perror(KRED "--SERVER creat error--" RESET);
	} else {
		printf(KGRN "####\nCREAT DONE, %s\n####\n" RESET, p);
		return result_2->my_open_creat_results_u.success;
	}
	return -1;
}

int _my_read(CLIENT* clnt, char* p, int o, int c) {
	my_read_results  *result_3;
	my_read_params  my_read_1_arg;

	my_read_1_arg.path = p;
	my_read_1_arg.offset = o;
	my_read_1_arg.count = c;

	result_3 = my_read_1(&my_read_1_arg, clnt);
	if (result_3 == (my_read_results *) NULL) {
		clnt_perror (clnt, "read call failed");
	} else if (result_3->status == -1) {
		errno = result_3->my_read_results_u.my_errno;
		perror(KRED "--SERVER read error" RESET);
	} else {
		printf(KGRN "####\n(%s) READ DONE: %s\n####\n" RESET, p, result_3->my_read_results_u.buf);
		return 0;
	}
	return -1;
}

int _my_write(CLIENT* clnt, char* p, int o, char* b) {
	my_write_results  *result_4;
	my_write_params  my_write_1_arg;

	my_write_1_arg.path = p;
	my_write_1_arg.offset = o;
	my_write_1_arg.buf = b;
	my_write_1_arg.buf_size = strlen(my_write_1_arg.buf);

	result_4 = my_write_1(&my_write_1_arg, clnt);
	if (result_4 == (my_write_results *) NULL) {
		clnt_perror (clnt, "write call failed");
	} else if (result_4->status == -1) {
		errno = result_4->my_write_results_u.my_errno;
		perror(KRED "--SERVER write error" RESET);
	} else {
		printf(KGRN "####\n(%s) WRITE DONE, %s (%dB)\n####\n" RESET, p, b, result_4->my_write_results_u.bytes_written);
		return result_4->status;
	}
	return -1;
}

int _my_lseek(CLIENT* clnt, char* p, int o, int o2, my_whence mw) {
	my_lseek_results  *result_5;
	my_lseek_params  my_lseek_1_arg;

	my_lseek_1_arg.path = p;
	my_lseek_1_arg.offset = o;
	my_lseek_1_arg.offset_to_set = o2;
	my_lseek_1_arg.my_whence_flag = mw;

	result_5 = my_lseek_1(&my_lseek_1_arg, clnt);
	if (result_5 == (my_lseek_results *) NULL) {
		clnt_perror (clnt, "lseek call failed");
	} else if (result_5->status == -1) {
		errno = result_5->my_lseek_results_u.my_errno;
		perror(KRED "--SERVER lseek error" RESET);
	} else {
		printf(KGRN "####\n(%s) LSEEK DONE, current location: %d\n####\n" RESET, p, result_5->my_lseek_results_u.offset_location);
		return result_5->my_lseek_results_u.offset_location;
	}
	return -1;
}

int _my_close(CLIENT* clnt, char* p) {
	my_close_results  *result_6;
	my_close_params  my_close_1_arg;

	my_close_1_arg.path = p;

	result_6 = my_close_1(&my_close_1_arg, clnt);
	if (result_6 == (my_close_results *) NULL) {
		clnt_perror (clnt, "close call failed");
	} else if (result_6->status == -1) {
		errno = result_6->my_close_results_u.my_errno;
		perror(KRED "--SERVER close error" RESET);
	} else {
		printf(KGRN "####\n(%s) CLOSE DONE\n####\n" RESET, p);
		return 0;
	}
	return -1;
}

CLIENT* get_client_from_host(char *host) {
	CLIENT *c;

	#ifndef	DEBUG
	c = clnt_create (host, MY_NFS, MY_NFS_V1, "udp");
	if (c == NULL) {
		clnt_pcreateerror (host);
		exit (1);
	}
	#endif	/* DEBUG */
	return c;

}

void destroy_client(CLIENT *c) {
	#ifndef	DEBUG
	clnt_destroy (c);
	#endif	 /* DEBUG */
}

/*void my_nfs_1(char *host) {
 	CLIENT *clnt = get_client_from_host(host);

	int mo = _my_open(clnt, "test2.txt", _O_RDWR);
	int mcr = _my_creat(clnt, "test3.txt");
	int mr = _my_read(clnt, mo, 25);
	int mw = _my_write(clnt, mo, "qwe+");
	int ml = _my_lseek(clnt, mo, 2, _SEEK_SET);
	int mr2 = _my_read(clnt, mo, 3);
	int mcl = _my_close(clnt, mo);
	int mcl2 = _my_close(clnt, mcr);

	destroy_client(clnt);
}*/


int main (int argc, char *argv[]) {
	system("COLOR FC");
	char *host;
	if (argc < 2) {
		printf ("usage: %s server_host\n", argv[0]);
		exit (1);
	}
	host = argv[1];
	CLIENT* clnt = get_client_from_host(host);
	int main_loop = 0;

	while (main_loop != -1) {
		system ( "clear" );
		printf("Podaj komendę, do wyboru: \n\n");
		printf("[open path access_flag] - otwiera plik ze ścieżki 'path' z flagą (O_RDWR O_RDONLY O_WRONLY)\n\n");
		printf("[creat path] - tworzy plik o ścieżce 'path'\n\n");

		char input[100], command[10], path[50], arg[10];
		int status = -1;
		scanf("%[^\n]%*c", input);
		sscanf( input, "%s %s %s", command, path, arg);

		// open or creat
		if (strcmp(command, "open") == 0) {
			// open file
			printf("command: %s %s %s\n", command, path, arg);
			my_access maf = _O_RDONLY;
			if (strcmp(argv[4], "O_RDWR") == 0) maf = _O_RDWR;
			else if (strcmp(argv[4], "O_WRONLY") == 0) maf = _O_WRONLY;
			else printf("Zły argument access_flag, używam domyślego O_RDONLY\n");
			status = _my_open(clnt, path, maf);

		} else if (strcmp(command, "creat") == 0) {
			// create file
			printf("command: %s %s\n", command, path);
			status = _my_creat(clnt, path);

		} else {
			printf("zła komenda, kończę program\n");
			destroy_client(clnt);
			exit (0);
		}

		system ( "clear" );
		int current_offset = 0;

		while (status != -1) {
			// read, write or lseek
			printf("Plik: %s Offset: %d\n\nPodaj komendę, do wyboru: \n\n", path, current_offset);
			printf("[read count] - czyta 'count' bajtów od bieżącej pozycji\n\n");
			printf("[write text] - pisze 'text' od bieżącej pozycji\n\n");
			printf("[lseek offset whence_flag] - ustawia kursor w odległości 'offset' w zależności od flagi (SEEK_SET SEEK_CUR SEEK_END)\n\n");
			printf("[close] - zamyka plik i przechodzi do menu otwierania/nowego pliku\n\n");

			char input2[100], command2[10], args2[9999];
			scanf("%[^\n]%*c", input2);
			sscanf( input2, "%s %[^\n]%*c", command2, args2);

			if (strcmp(command2, "read") == 0) {
				// read file
				int count;
				sscanf( args2, "%d", &count);
				printf("command: %s %s %d\n", command2, path, count);
				status = _my_read(clnt, path, current_offset, count);
				current_offset += count;

			} else if (strcmp(command2, "write") == 0) {
				// write file
				printf("command: %s %s %s\n", command2, path, args2);
				status = _my_write(clnt, path, current_offset, args2);
				current_offset += status;

			} else if (strcmp(command2, "lseek") == 0) {
				// lseek file
				int offset;
				char my_whence_input[15];
				sscanf( input2, "%s %d %s", command2, &offset, my_whence_input);
				printf("command: %s %s %d %s\n", command2, path, offset, my_whence_input);
				my_whence mw = _SEEK_SET;
				if (strcmp(my_whence_input, "SEEK_CUR") == 0) mw = _SEEK_CUR;
				else if (strcmp(my_whence_input, "SEEK_END") == 0) mw = _SEEK_END;
				else printf("Używam domyślego SEEK_SET\n");
				status = _my_lseek(clnt, path, current_offset, offset, mw);
				current_offset = status;;

			} else if (strcmp(command2, "close") == 0) {
				// write file
				//printf("command: %s %s\n", command2, path);
				//status = _my_close(clnt, path);
				current_offset = 0;
				status = -1;

			} else {
				printf("zła komenda, kończę program\n");
				destroy_client(clnt);
				exit (0);
			}
			printf("\n-----------------------\n\n");
		}
	}

/*if (strcmp(argv[2], "open") == 0) {
	my_access maf = _O_RDWR;
	if (strcmp(argv[4], "_O_RDONLY") == 0) maf = _O_RDONLY;
	if (strcmp(argv[4], "O_WRONLY") == 0) maf = _O_WRONLY;
	_my_open(clnt, argv[3], maf);
}
else if (strcmp(argv[2], "creat") == 0) {
	_my_creat(clnt, argv[3]);
}
else if (strcmp(argv[2], "read") == 0) {
	_my_read(clnt, atoi(argv[3]), atoi(argv[4]));
}
else if (strcmp(argv[2], "write") == 0) {
	_my_write(clnt, atoi(argv[3]), argv[4]);
}
else if (strcmp(argv[2], "lseek") == 0) {
	my_whence mw = _SEEK_SET;
	if (strcmp(argv[4], "SEEK_CUR") == 0) mw = _SEEK_CUR;
	if (strcmp(argv[4], "SEEK_END") == 0) mw = _SEEK_END;
	_my_lseek(clnt, atoi(argv[3]), atoi(argv[4]), mw);
}
else if (strcmp(argv[2], "close") == 0) {
	_my_close(clnt, atoi(argv[3]));
}
else {
	printf("bad command\n");
}*/


	printf("kończę program\n");
	destroy_client(clnt);
	exit (0);
}
