/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include <memory.h> /* for memset */
#include "mynfs.h"

/* Default timeout can be changed using clnt_control() */
static struct timeval TIMEOUT = { 25, 0 };

my_open_creat_results *
my_open_1(my_open_params *argp, CLIENT *clnt)
{
	static my_open_creat_results clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, my_open,
		(xdrproc_t) xdr_my_open_params, (caddr_t) argp,
		(xdrproc_t) xdr_my_open_creat_results, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

my_open_creat_results *
my_creat_1(my_creat_params *argp, CLIENT *clnt)
{
	static my_open_creat_results clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, my_creat,
		(xdrproc_t) xdr_my_creat_params, (caddr_t) argp,
		(xdrproc_t) xdr_my_open_creat_results, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

my_read_results *
my_read_1(my_read_params *argp, CLIENT *clnt)
{
	static my_read_results clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, my_read,
		(xdrproc_t) xdr_my_read_params, (caddr_t) argp,
		(xdrproc_t) xdr_my_read_results, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

my_write_results *
my_write_1(my_write_params *argp, CLIENT *clnt)
{
	static my_write_results clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, my_write,
		(xdrproc_t) xdr_my_write_params, (caddr_t) argp,
		(xdrproc_t) xdr_my_write_results, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

my_lseek_results *
my_lseek_1(my_lseek_params *argp, CLIENT *clnt)
{
	static my_lseek_results clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, my_lseek,
		(xdrproc_t) xdr_my_lseek_params, (caddr_t) argp,
		(xdrproc_t) xdr_my_lseek_results, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

my_close_results *
my_close_1(my_close_params *argp, CLIENT *clnt)
{
	static my_close_results clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, my_close,
		(xdrproc_t) xdr_my_close_params, (caddr_t) argp,
		(xdrproc_t) xdr_my_close_results, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}