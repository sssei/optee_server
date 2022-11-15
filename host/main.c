/*
 * Copyright (c) 2016, Linaro Limited
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <err.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* OP-TEE TEE client API (built by optee_client) */
#include <tee_client_api.h>

/* For the UUID (found in the TA's h-file(s)) */
#include <server_ta.h>
#include "server.h"

/* #include <tee_isocket.h> */
// #include <tee_tcpsocket.h>

struct socket_handle {
	uint64_t buf[2];
	size_t blen;
};

static TEEC_Result socket_tcp_open(TEEC_Session *session, uint32_t ip_vers, 
									const char *addr, uint16_t port, 
									struct socket_handle *handle, 
									uint32_t *error, uint32_t *ret_orig)
{
	TEEC_Result res = TEEC_ERROR_GENERIC;
	TEEC_Operation op = {};

	memset(handle, 0, sizeof(*handle));

	op.params[0].value.a = ip_vers;
	op.params[0].value.b = port;
	op.params[1].tmpref.buffer = (void *)addr;
	op.params[1].tmpref.size = strlen(addr) + 1;
	op.params[2].tmpref.buffer = handle->buf;
	op.params[2].tmpref.size = sizeof(handle->buf);

	op.paramTypes =  TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
					 TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_OUTPUT,
					 TEEC_VALUE_OUTPUT);

	res = TEEC_InvokeCommand(session, TA_SOCKET_CMD_TCP_OPEN,
							&op, ret_orig);

	handle->blen = op.params[2].tmpref.size; 
	*error = op.params[3].value.a;
	return res;

}	

static TEEC_Result socket_close(TEEC_Session *session,
			      struct socket_handle *handle, uint32_t *ret_orig)
{
	TEEC_Operation op = {};

	op.params[0].tmpref.buffer = handle->buf;
	op.params[0].tmpref.size = handle->blen;

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_NONE, TEEC_NONE, TEEC_NONE);

	return TEEC_InvokeCommand(session, TA_SOCKET_CMD_CLOSE, &op, ret_orig);
}

static TEEC_Result socket_listen(TEEC_Session *session, uint32_t ip_vers, 
									const char *addr, uint16_t port, 
									struct socket_handle *handle, 
									uint32_t *error, uint32_t *ret_orig)
{
	TEEC_Result res = TEEC_ERROR_GENERIC;
	TEEC_Operation op = {};

	memset(handle, 0, sizeof(*handle));

	op.params[0].value.a = ip_vers;
	op.params[0].value.b = port;
	op.params[1].tmpref.buffer = (void *)addr;
	op.params[1].tmpref.size = strlen(addr) + 1;
	op.params[2].tmpref.buffer = handle->buf;
	op.params[2].tmpref.size = sizeof(handle->buf);

	op.paramTypes =  TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
					 TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_OUTPUT,
					 TEEC_VALUE_OUTPUT);

	res = TEEC_InvokeCommand(session, TA_SOCKET_CMD_LISTEN,
							&op, ret_orig);

	handle->blen = op.params[2].tmpref.size; 
	*error = op.params[3].value.a;
	return res;

}	

static TEEC_Result socket_accept(TEEC_Session *session,
			      struct socket_handle *handle, uint32_t *ret_orig)
{
	TEEC_Result res = TEEC_ERROR_GENERIC;	
	TEEC_Operation op = {};

	op.params[0].tmpref.buffer = handle->buf;
	op.params[0].tmpref.size = handle->blen;
	op.params[1].tmpref.buffer = handle->buf;
	op.params[1].tmpref.size = sizeof(handle->buf);	

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_OUTPUT, 
					 TEEC_NONE, TEEC_NONE);

	res = TEEC_InvokeCommand(session, TA_SOCKET_CMD_ACCEPT, &op, ret_orig);

	handle->blen = op.params[1].tmpref.size;
	return res;
}

int main(void)
{
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = TA_SERVER_UUID;
	uint32_t err_origin;
/* 	struct timespec start, end;
	long long elapsed_time;
 */	struct socket_handle sh = { };
	struct socket_handle ac_sh = { };
	uint32_t ret_orig = 0;
	uint32_t proto_error = 0;
	struct server_info si = {
		.addr = "127.0.0.1",
		.port = 8089,
	};


	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);


	res = TEEC_OpenSession(&ctx, &sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
			res, err_origin);

	memset(&op, 0, sizeof(op));

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT, TEEC_NONE,
					 TEEC_NONE, TEEC_NONE);
	op.params[0].value.a = 42;

/* 	
	printf("Invoking TA to increment %d\n", op.params[0].value.a);
	clock_gettime(CLOCK_MONOTONIC, &start);

	for(int i = 0; i < 10000; i++){
		res = TEEC_InvokeCommand(&sess, TA_HELLO_WORLD_CMD_INC_VALUE, &op,
					&err_origin);
		if (res != TEEC_SUCCESS)
			errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
				res, err_origin);
	}

	clock_gettime(CLOCK_MONOTONIC, &end);

	elapsed_time = 1000 * 1000 * 1000 * (end.tv_sec - start.tv_sec);
	elapsed_time += end.tv_nsec - start.tv_nsec;

	printf("%lld nsec\n", elapsed_time);
	printf("TA incremented value to %d\n", op.params[0].value.a);
 */
	printf("Invoking TA to open tcp\n");
	
	res = socket_listen(&sess, 1, si.addr, si.port, 
					&sh, &proto_error, &ret_orig);

	if (res != TEEC_SUCCESS){
		printf("host: socket_listen failed with code 0x%x\n", res);
	}else{
		printf("host: listen success\n");
	}

/* 	memcpy(&ac_sh.buf, &sh.buf, 2 * sizeof(uint64_t));
	ac_sh.blen = sh.blen;
 */	
	res = socket_accept(&sess, &sh, &ret_orig);

	if (res != TEEC_SUCCESS){
		printf("host: socket_accept failed with code 0x%x\n", res);
	}else{
		printf("host: accept success\n");
	}

	res = socket_close(&sess, &sh, &ret_orig);
	if (res != TEEC_SUCCESS) {
		printf("socket_close failed\n");
	}


	TEEC_CloseSession(&sess);

	TEEC_FinalizeContext(&ctx);

	return 0;
}
