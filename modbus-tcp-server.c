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
#include <modbus.h>
#include <pthread.h>

#include "unit-test.h"

#include "buildtime.h"

enum {
    TCP,
    TCP_PI,
    RTU
};

int main(int argc, char*argv[])
{
    int socket;
    modbus_t *ctx;
    modbus_mapping_t *mb_mapping;
    int rc;
    int i;
    int use_backend;
    uint8_t *query;
    int header_length;
	int ctrl_bd_id = 1;
	int hotest_asic_id1 = 33;
	int hotest_temper1 = 92;
	int hotest_asic_id2 = 21;
	int hotest_temper2 = 89;
#define BD_ENABLED 1
	int stat_bd1 = BD_ENABLED;
	int stat_bd2 = BD_ENABLED;

	printf("modbus-server (buidtime : %s)\n", BUILDTIME);
    if (argc > 1) {
        if (strcmp(argv[1], "tcp") == 0) {
            use_backend = TCP;
        } else if (strcmp(argv[1], "tcppi") == 0) {
            use_backend = TCP_PI;
        } else if (strcmp(argv[1], "rtu") == 0) {
            use_backend = RTU;
        } else {
            printf("Usage:\n  %s [tcp|tcppi|rtu] - Modbus server for unit testing\n\n", argv[0]);
            return -1;
        }
    } else {
        /* By default */
        use_backend = TCP;
    }

    if (use_backend == TCP) {
        ctx = modbus_new_tcp("127.0.0.1", 502);
        query = malloc(MODBUS_TCP_MAX_ADU_LENGTH);
    } else if (use_backend == TCP_PI) {
        ctx = modbus_new_tcp_pi("::0", "1502");
        query = malloc(MODBUS_TCP_MAX_ADU_LENGTH);
    } else {
        ctx = modbus_new_rtu("/dev/ttyUSB0", 115200, 'N', 8, 1);
        modbus_set_slave(ctx, SERVER_ID);
        query = malloc(MODBUS_RTU_MAX_ADU_LENGTH);
    }
    header_length = modbus_get_header_length(ctx);

    modbus_set_debug(ctx, TRUE);

	// 1: first board number
	// 2: number of hottest asic in first board
	// 3: temperature of hottest asic in first board
	// 4: second board number
	// 5: number of hottest asic in second board
	// 6: temperature of hottest asic in second board
	// 7: temperature of s5p6818
    mb_mapping = modbus_mapping_new(
        0, //UT_BITS_ADDRESS + UT_BITS_NB,
        0, //UT_INPUT_BITS_ADDRESS + UT_INPUT_BITS_NB,
        0, //UT_REGISTERS_ADDRESS + UT_REGISTERS_NB,
        7); //UT_INPUT_REGISTERS_ADDRESS + UT_INPUT_REGISTERS_NB);
    if (mb_mapping == NULL) {
        fprintf(stderr, "Failed to allocate the mapping: %s\n",
                modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }

    /* Examples from PI_MODBUS_300.pdf.
       Only the read-only input values are assigned. */

    /** INPUT STATUS **/
//    modbus_set_bits_from_bytes(mb_mapping->tab_input_bits,
//                               UT_INPUT_BITS_ADDRESS, UT_INPUT_BITS_NB,
//                               UT_INPUT_BITS_TAB);

    /** INPUT REGISTERS **/
#if 0
    for (i=0; i < UT_INPUT_REGISTERS_NB; i++) {
        mb_mapping->tab_input_registers[UT_INPUT_REGISTERS_ADDRESS+i] =
            UT_INPUT_REGISTERS_TAB[i];;
    }
#else
#define TEMPERATURE_NOT_DETECTED_YET	0
#define DEFAULT_TEMPERATURE TEMPERATURE_NOT_DETECTED_YET
    for (i=0; i < 8; i++) {
        mb_mapping->tab_input_registers[i] = i; //DEFAULT_TEMPERATURE;
    }
#endif

	for(;;) {
    if (use_backend == TCP) {
        socket = modbus_tcp_listen(ctx, 1);
        modbus_tcp_accept(ctx, &socket);
    } else if (use_backend == TCP_PI) {
        socket = modbus_tcp_pi_listen(ctx, 1);
        modbus_tcp_pi_accept(ctx, &socket);
    } else {
        rc = modbus_connect(ctx);
        if (rc == -1) {
            fprintf(stderr, "Unable to connect %s\n", modbus_strerror(errno));
            modbus_free(ctx);
            return -1;
        }
    }

	/* TODO: get ctrl_bd_id */
	/* TODO: get stat_bd1, stat_bd2 */

	printf("header_length = %d\n", header_length);
    for (;;) {
        rc = modbus_receive(ctx, query);
        if (rc == -1) {
            /* Connection closed by the client or error */
            break;
        }

#if 0
        /* Read holding registers */
        if (query[header_length] == 0x03) {
            if (MODBUS_GET_INT16_FROM_INT8(query, header_length + 3)
                == UT_REGISTERS_NB_SPECIAL) {
                printf("Set an incorrect number of values\n");
                MODBUS_SET_INT16_TO_INT8(query, header_length + 3,
                                         UT_REGISTERS_NB_SPECIAL - 1);
            } else if (MODBUS_GET_INT16_FROM_INT8(query, header_length + 1)
                == UT_REGISTERS_ADDRESS_SPECIAL) {
                printf("Reply to this special register address by an exception\n");
                modbus_reply_exception(ctx, query,
                                       MODBUS_EXCEPTION_SLAVE_OR_SERVER_BUSY);
                continue;
            }
        }
#else
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
#define	MODBUS_ADDR_ID_OF_BD2	4
#define MODBUS_ADDR_ID_OF_HOTEST_ASIC_IN_BD2	5
#define MODBUS_ADDR_TEMPER_OF_HOTEST_ASIC_IN_BD2	6
#define MODBUS_ADDR_TEMPER_OF_CPU	7

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
				printf("bd1 info : O ,");
			}
			else {
				// asic id
				mb_mapping->tab_input_registers[idx++] = hotest_asic_id1;
				// hotest temper
				mb_mapping->tab_input_registers[idx++] = hotest_temper1;
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
				printf("bd2 info : O ,");
			}
			else {
				// asic id
				mb_mapping->tab_input_registers[idx++] = hotest_asic_id2;
				// hotest temper
				mb_mapping->tab_input_registers[idx++] = hotest_temper2;
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
#endif

        rc = modbus_reply(ctx, query, rc, mb_mapping);
        if (rc == -1) {
            break;
        }
    }

    printf("Quit the loop: %s\n", modbus_strerror(errno));

    if (use_backend == TCP) {
        close(socket);
    }
	}
    modbus_mapping_free(mb_mapping);
    free(query);
    modbus_free(ctx);

    return 0;
}
