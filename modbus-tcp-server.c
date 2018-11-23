/*
 * Copyright © 2008-2010 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#include "modbus.h"
#include "modbus-private.h"
#include "modbus-tcp-private.h"

#include "unit-test.h"

#include "buildtime.h"

enum {
    TCP,
    TCP_PI,
    RTU
};

#define TEMPERATURE_NOT_DETECTED_YET	0
#define DEFAULT_TEMPERATURE TEMPERATURE_NOT_DETECTED_YET

modbus_t *modbus_clone_tcp(modbus_t *ctx)
{	
	modbus_t *new_ctx = (modbus_t *)malloc(sizeof(modbus_t));
	memcpy(new_ctx, ctx, sizeof(modbus_t));
	new_ctx->backend_data = (modbus_tcp_t *) malloc(sizeof(modbus_tcp_t));
	memcpy(new_ctx->backend_data, ctx->backend_data, sizeof(modbus_tcp_t));

	//modbus_tcp_t *mb_tcp = (modbus_tcp_t*)ctx->backend_data;
	printf("Socket copied %d, should be %d\n", new_ctx->s, ctx->s);

	return new_ctx;
}

typedef struct {
	modbus_t *ctxt;
	modbus_mapping_t *mm;
	int id;
	int header_length;
} SA;

void *serveClient(void *threadarg)
{
	int i, rc;
	SA *a;
	a = (SA *) threadarg;
	modbus_t *ctx = a->ctxt;
	modbus_mapping_t *mb_mapping = a->mm;	
	int id = a->id;
	int header_length = a->header_length;

	uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
	int ctrl_bd_id = 1;
	int hotest_asic_id1 = 31;
	int hotest_temper1 = 97;
	int error_id1 = 0;
	int hotest_asic_id2 = 17;
	int hotest_temper2 = 63;
	int error_id2 = 0;
#define BD_ENABLED 1
	int stat_bd1 = BD_ENABLED;
	int stat_bd2 = BD_ENABLED;

	/* TODO: get ctrl_bd_id */
	/* TODO: get stat_bd1, stat_bd2 */

	printf("header_length = %d\n", header_length);
	for (;;) {
		rc = modbus_receive(ctx, query);
		if (rc == -1) {
			/* Connection closed by the client or error */
			break;
		}

		if (query[header_length] != 0x04) {
				modbus_reply_exception(ctx, query,
									   MODBUS_EXCEPTION_NOT_DEFINED);
				printf("query[%d] = 0x%02x\n", header_length, query[header_length]);
				continue;
		}

		int starting_addr = 0;
		int number = 0;
		int idx = 0;

		starting_addr = MODBUS_GET_INT16_FROM_INT8(query, header_length + 1);
		number = MODBUS_GET_INT16_FROM_INT8(query, header_length + 3);

		printf("starting_addr = %d\n", starting_addr);
		printf("number = %d\n", number);
		starting_addr += 1;
#define	MODBUS_ADDR_ID_OF_BD1	1
#define MODBUS_ADDR_ID_OF_HOTEST_ASIC_IN_BD1	2
#define MODBUS_ADDR_TEMPER_OF_HOTEST_ASIC_IN_BD1	3
#define MODBUS_ADDR_ERROR_OF_BD1	4
#define	MODBUS_ADDR_ID_OF_BD2	5
#define MODBUS_ADDR_ID_OF_HOTEST_ASIC_IN_BD2	6
#define MODBUS_ADDR_TEMPER_OF_HOTEST_ASIC_IN_BD2	7
#define MODBUS_ADDR_ERROR_OF_BD2	8
#define MODBUS_ADDR_TEMPER_OF_CPU	9

		if( ( starting_addr         <= MODBUS_ADDR_ID_OF_BD1) 
		  &&((starting_addr+number) >  MODBUS_ADDR_TEMPER_OF_HOTEST_ASIC_IN_BD1)
		  )
		{
			// board number
			mb_mapping->tab_input_registers[idx++] = (ctrl_bd_id<<1) - 1;
			if(stat_bd1 == BD_ENABLED) {
				/* TODO: get hotest_asic_id1 */
				/* TODO: get hotest_temper1 */
				// hotest asic id
				mb_mapping->tab_input_registers[idx++] = hotest_asic_id1;
				// hotest temper
				mb_mapping->tab_input_registers[idx++] = hotest_temper1;
				// error state
				mb_mapping->tab_input_registers[idx++] = error_id1;
				printf("bd1 info : O ,");
			}
			else {
				// asic id
				mb_mapping->tab_input_registers[idx++] = hotest_asic_id1;
				// hotest temper
				mb_mapping->tab_input_registers[idx++] = hotest_temper1;
				// error state
				mb_mapping->tab_input_registers[idx++] = error_id1;
				printf("bd1 info : X ,");
			}
		}
		if( ( starting_addr         <= MODBUS_ADDR_ID_OF_BD2) 
		  &&((starting_addr+number) >  MODBUS_ADDR_TEMPER_OF_HOTEST_ASIC_IN_BD2)
		  )
		{
			// board number
			mb_mapping->tab_input_registers[idx++] = (ctrl_bd_id<<1);
			if(stat_bd2 == BD_ENABLED) {
				/* TODO: get hotest_asic_id2 */
				/* TODO: get hotest_temper2 */
				// hotest asic id
				mb_mapping->tab_input_registers[idx++] = hotest_asic_id2;
				// hotest temper
				mb_mapping->tab_input_registers[idx++] = hotest_temper2;
				// error state
				mb_mapping->tab_input_registers[idx++] = error_id2;
				printf("bd2 info : O ,");
			}
			else {
				// asic id
				mb_mapping->tab_input_registers[idx++] = hotest_asic_id2;
				// hotest temper
				mb_mapping->tab_input_registers[idx++] = hotest_temper2;
				// error state
				mb_mapping->tab_input_registers[idx++] = error_id2;
				printf("bd2 info : X ,");
			}
		}
		if( ( starting_addr         <= MODBUS_ADDR_TEMPER_OF_CPU) 
		  &&((starting_addr+number) >  MODBUS_ADDR_TEMPER_OF_CPU)
		  )
		{
			int cpu_tp_fd;
			int cpu_temper = TEMPERATURE_NOT_DETECTED_YET, rd_len;
			char cpu_temper_array[16];
			cpu_tp_fd = open("/sys/class/thermal/thermal_zone0/temp", O_RDONLY);
			memset(cpu_temper_array, 0, 16);
			rd_len = read(cpu_tp_fd, cpu_temper_array, 16);
			if(rd_len) {
				cpu_temper = strtol(cpu_temper_array, NULL, 0);
				printf("cpu temper : %d ", cpu_temper);
				cpu_temper /= 100;
			}
			else
				printf("cpu temper : X ");
			close(cpu_tp_fd);
			mb_mapping->tab_input_registers[idx++] = cpu_temper;
		}
		printf("\n");
		{
			for(i=0; i<idx; i++) {
				printf("%04x ", mb_mapping->tab_input_registers[i]);
				printf("\n");
			}
		}

		rc = modbus_reply(ctx, query, rc, mb_mapping);
		if (rc == -1) {
			break;
		}
	}

	//clean-up
	free(ctx);
	free(a);
}

int main(int argc, char*argv[])
{
    int rc;
    int i;
	int id = 0;
	int header_length;
	modbus_t *ctx = NULL;
	modbus_t *new_ctx;
	modbus_mapping_t *mb_mapping;
	int server_socket = -1;
	pthread_t tId;

	pthread_attr_t attr;

	printf("modbus-server (buidtime : %s)\n", BUILDTIME);

	/* Initialize and set thread detached attribute */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	ctx = modbus_new_tcp("127.0.0.1", 502);
    header_length = modbus_get_header_length(ctx);

    modbus_set_debug(ctx, TRUE);

	// 1: first board number
	// 2: number of hottest asic in first board
	// 3: temperature of hottest asic in first board
	// 4: error state of first board
	// 5: second board number
	// 6: number of hottest asic in second board
	// 7: temperature of hottest asic in second board
	// 8: error state of first board
	// 9: temperature of s5p6818
    mb_mapping = modbus_mapping_new(
        0, //UT_BITS_ADDRESS + UT_BITS_NB,
        0, //UT_INPUT_BITS_ADDRESS + UT_INPUT_BITS_NB,
        0, //UT_REGISTERS_ADDRESS + UT_REGISTERS_NB,
        9); //UT_INPUT_REGISTERS_ADDRESS + UT_INPUT_REGISTERS_NB);
    if (mb_mapping == NULL) {
        fprintf(stderr, "Failed to allocate the mapping: %s\n",
                modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }

    /** INPUT REGISTERS **/
    for (i=0; i < 9; i++) {
        mb_mapping->tab_input_registers[i] = DEFAULT_TEMPERATURE;
    }

	server_socket = modbus_tcp_listen(ctx, 5);
	if (server_socket == -1) {
		fprintf(stderr, "Unable to listen TCP connection\n");
		modbus_free(ctx);
		return -1;
	}

	for(;;) {
		id++;

		modbus_tcp_accept(ctx, &server_socket);
		new_ctx = modbus_clone_tcp(ctx);

		SA *sa = (SA*)malloc(sizeof(SA));
		sa->ctxt = new_ctx;
		sa->mm = mb_mapping;
		sa->id = id;
		sa->header_length = header_length;

		rc = pthread_create(&tId, &attr, serveClient, (void *)sa); 
		if (rc) {
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			break;
		}
	}
	if(server_socket != -1)
		close(server_socket);
    modbus_mapping_free(mb_mapping);
    modbus_close(ctx);
    modbus_free(ctx);

    return 0;
}

