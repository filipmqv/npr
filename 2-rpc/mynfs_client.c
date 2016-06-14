/*
 * This is sample code generated by rpcgen.
 * These are only templates and you can use them
 * as a guideline for developing your own functions.
 */

#include <errno.h>
#include "mynfs.h"


void
my_nfs_1(char *host)
{
	CLIENT *clnt;
	my_open_creat_results  *result_1;
	my_open_params  my_open_1_arg;
	my_open_creat_results  *result_2;
	my_creat_params  my_creat_1_arg;
	my_read_results  *result_3;
	my_read_params  my_read_1_arg;
	my_write_results  *result_4;
	my_write_params  my_write_1_arg;
	my_lseek_results  *result_5;
	my_lseek_params  my_lseek_1_arg;
	my_close_results  *result_6;
	my_close_params  my_close_1_arg;

#ifndef	DEBUG
	clnt = clnt_create (host, MY_NFS, MY_NFS_V1, "udp");
	if (clnt == NULL) {
		clnt_pcreateerror (host);
		exit (1);
	}
#endif	/* DEBUG */


	//my_open_1_arg.path = "/home/filipmqv/npr/2-rpc/test2.txt";
	my_open_1_arg.path = "/tmp/test_rpc.txt";
	my_open_1_arg.my_access_flag = _O_RDONLY;
	result_1 = my_open_1(&my_open_1_arg, clnt);
	if (result_1 == (my_open_creat_results *) NULL) {
		clnt_perror (clnt, "open call failed");
	} else if (result_1->status == -1) {
		errno = result_1->my_open_creat_results_u.my_errno;
		perror("--custom open error--");
	} else {
		printf("%d\n", result_1->my_open_creat_results_u.fd);
	}


	my_creat_1_arg.path = "adasd";
	result_2 = my_creat_1(&my_creat_1_arg, clnt);
	if (result_2 == (my_open_creat_results *) NULL) {
		clnt_perror (clnt, "creat call failed");
	} else if (result_2->status == -1) {
		errno = result_2->my_open_creat_results_u.my_errno;
		perror("--custom creat error--");
	} else {
		printf("%d\n", result_2->my_open_creat_results_u.fd);
	}


	result_3 = my_read_1(&my_read_1_arg, clnt);
	if (result_3 == (my_read_results *) NULL) {
		clnt_perror (clnt, "read call failed");
	} else if (result_3->status == -1) {
		errno = result_3->my_read_results_u.my_errno;
		perror("--custom read error--");
	} else {
		printf("%s\n", result_3->my_read_results_u.buf);
	}


	my_write_1_arg.buf = "adasdasdasdasd";
	result_4 = my_write_1(&my_write_1_arg, clnt);
	if (result_4 == (my_write_results *) NULL) {
		clnt_perror (clnt, "write call failed");
	}


	result_5 = my_lseek_1(&my_lseek_1_arg, clnt);
	if (result_5 == (my_lseek_results *) NULL) {
		clnt_perror (clnt, "lseek call failed");
	}

	my_close_1_arg.fd = 6;
	result_6 = my_close_1(&my_close_1_arg, clnt);
	if (result_6 == (my_close_results *) NULL) {
		clnt_perror (clnt, "close call failed");
	}


#ifndef	DEBUG
	clnt_destroy (clnt);
#endif	 /* DEBUG */
}


int
main (int argc, char *argv[])
{
	char *host;

	if (argc < 2) {
		printf ("usage: %s server_host\n", argv[0]);
		exit (1);
	}
	host = argv[1];
	my_nfs_1 (host);
exit (0);
}
