/*
* Copyright 2018 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
 
#include "crypto_stl_reader_writer_python_wrapper.h"
#include "crypto_ledger_reader_writer.h"
#include "app_log.h"
#include <stdio.h>
#include <string.h>
#include "secure_allocator.h"

#include <stdint.h>
#include <inttypes.h>
#include <iostream>

bool encrypt_data(const uint8_t *data, size_t size, uint16_t svn, char **res, const char* p_client_public_key_str, const char *keys_path)
{
	init_log();
	ledger_hex_address_t address = {};
	memset_s(address, sizeof(ledger_hex_address_t)-1,'0',sizeof(ledger_hex_address_t)-1);
	address[sizeof(ledger_hex_address_t)-1] = '\0';
	Ledger_Reader_Writer rw;

	rw.set_svn(svn);
	if (keys_path != NULL)
	{
		rw.set_files_path(keys_path);
	}

	rw.load_keys_from_files();
	rw.set_signing_public_key((const public_ec_key_str_t*)p_client_public_key_str);

	bool ret = rw.encode_secure_data(address, data, size, TYPE_TRANSACTION, res);

	return ret;
}


bool encrypt_address(char *address, uint16_t svn, uint64_t &nonce, uint8_t *secret, char **res, const char* p_client_public_key_str, const char* p_client_private_key_str, const char *keys_path)
{
	return encrypt_request(address, NULL, 0, svn, nonce, secret, res, p_client_public_key_str, p_client_private_key_str, keys_path);
}

bool encrypt_addresses_data(const uint8_t* data, size_t data_size, uint16_t svn, uint64_t &nonce, uint8_t *secret, char **res, const char* p_client_public_key_str, const char* p_client_private_key_str, const char *keys_path)
{
	ledger_hex_address_t address = {};
	memset_s(address, sizeof(ledger_hex_address_t)-1,'0',sizeof(ledger_hex_address_t)-1);
	address[sizeof(ledger_hex_address_t)-1] = '\0';

	return encrypt_request(address.data, data, data_size, svn, nonce, secret, res, p_client_public_key_str, p_client_private_key_str, keys_path);
}

bool encrypt_request(char *address, const uint8_t* data, size_t data_size, uint16_t svn, uint64_t &nonce, uint8_t *secret, char **res, const char* p_client_public_key_str, const char* p_client_private_key_str, const char *keys_path)
{
	init_log();

	Ledger_Reader_Writer rw;

	rw.set_svn(svn);
	if (keys_path != NULL)
	{
		rw.set_files_path(keys_path);
	}
	if (!rw.load_keys_from_files())
	{
		printf("load_keys_from_files failed\n");
		return false;
	}
#if !defined(SKIP_SIGN) && !defined(HSM_SIGN) // signing is done by crypto lib, need also private key
	if (!rw.set_signing_keys((const public_ec_key_str_t*)p_client_public_key_str, (const private_ec_key_str_t*)p_client_private_key_str))
	{
		printf("set_signing_keys failed\n");
		return false;
	}
#else
	if (!rw.set_signing_public_key((const public_ec_key_str_t*)p_client_public_key_str))
	{
		printf("set_signing_public_key failed\n");
		return false;
	}
#endif
	if (!rw.encode_secure_data(*(ledger_hex_address_t*)address, data, data_size, TYPE_READER_REQUEST, res))
	{
		printf("encode_secure_data failed\n");
		return false;
	}
	nonce = rw.get_nonce();
	if (!rw.get_dh_shared_secret((dh_shared_secret_t *)secret))
	{
		printf("get_dh_shared_secret failed\n");
		// free memory
		if (*res != NULL)
		{
			free(*res);
			*res = NULL;
		}
		return false;
	}
	return true;
}

char* decrypt_data(const char *input_data, uint16_t svn, uint64_t nonce, uint8_t *secret, secure_data_content_t **out, size_t *data_size, const char *keys_path)
{
	init_log();
	public_ec_key_str_t client_pub_key_str = {0};

	Ledger_Reader_Writer rw(nonce, (dh_shared_secret_t *)secret);

	rw.set_svn(svn);

	if (keys_path != NULL)
	{
		rw.set_files_path(keys_path);
	}
	if (!rw.load_keys_from_files())
	{
		printf("load_keys_from_files failed\n");
		return (char*)"";
	}

	if (!rw.decode_secure_data(input_data, out, data_size, &client_pub_key_str))
	{
		printf("Error: failed to decode data\n");
		return (char*)"";
	}
	return (char *)((*out)->data);
}

bool free_mem_request(char **request_str)
{
	init_log();
	// allocated in Ledger_Reader_Writer.encode_secure_data
	if (*request_str != NULL)
	{
		free(*request_str);
		*request_str = NULL;
	}
	return true;
}
