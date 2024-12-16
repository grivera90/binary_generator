/**
*******************************************************************************
* @file           : binary_generator.c
* @brief          : Description of C implementation module
* @author         : Gonzalo Rivera
* @date           : 06/12/2024
*******************************************************************************
* @attention
*
* Copyright (c) <date> grivera. All rights reserved.
*
*/
/******************************************************************************
    Includes
******************************************************************************/
#include <unistd.h>  

/* sockets include */
#include <netdb.h>      
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

/* output and strings */
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
/******************************************************************************
    Defines and constants
******************************************************************************/
#define SERVER_ADDRESS "10.255.255.210" /* Server IP */
#define PORT 7 
#define MAX_RAW_DATA_TABLE_LENGHT			(20 + 2) // ((256 * 20) + 2)
/******************************************************************************
    Data types
******************************************************************************/
typedef struct
{
	uint16_t len;
	uint16_t row;
	uint16_t col;
	uint8_t idx_board;
} table_t;

typedef enum
{
    PRINT_HEX = 0,
    PRINT_BIN   
} format_print_t;
/******************************************************************************
    Local variables
******************************************************************************/
static char file_path[128] = {0};
static char file_data[MAX_RAW_DATA_TABLE_LENGHT] = {0};
static char data_from_server[MAX_RAW_DATA_TABLE_LENGHT] = {0};

// Tablas fuente
static uint8_t source_table_0[256 * 20] = {0};
static uint8_t source_table_1[256 * 20] = {0};
static uint8_t source_table_2[256 * 20] = {0};

// Tabla resultante
static uint8_t result_table[256][240] = {0};

static table_t raw_table_type = {0};
/******************************************************************************
    Local function prototypes
******************************************************************************/
static void fill_source_tables(void);
static uint16_t generate_cpld_table(uint8_t *result_table, const uint8_t *table_0, const uint8_t *table_1, const uint8_t *table_2, table_t *raw_table);
static void format_table_1(uint8_t *data_dst, uint8_t *data_0, uint8_t *data_1, uint8_t *data_2, table_t *ptr_table);
static void format_table_2(uint8_t *data_dst, uint8_t *data_0, uint8_t *data_1, uint8_t *data_2, table_t *ptr_table);
static void print_result_table(void);
static void print_row_result_table(uint16_t row, format_print_t format);
static void print_binary(uint8_t byte);
/******************************************************************************
    Local function definitions
******************************************************************************/

/******************************************************************************
    Public function definitions
******************************************************************************/
int main(int argc, char **argv)
{
    uint8_t toggle = 0;
    char data = 0; 

    // ./binary_generator -tcp ip port file
    // ./binary_generator -f file index
    switch(argc)
    {
        case 4:
            if (!strcmp(argv[1], "-f"))
            {
                // Armo el path completo del archivo.
                sprintf(file_path, "workingdir/%s", argv[2]);
                FILE *fp = fopen(file_path, "wb");
                if (NULL == fp)
                {
                    return EXIT_FAILURE;
                }

                char index = *argv[3];
                char table_type = 'D';
                fwrite(&table_type, sizeof(table_type), 1, fp);     // indicador del tipo de tabla.
                fwrite(&index, sizeof(index), 1, fp);               // Indice que apunta a la placa (RX1, RX2 y TX).

                for (int i = 0; i < MAX_RAW_DATA_TABLE_LENGHT - 2; i++)
                {
                    data = (toggle == 1)? 0x55 : 0xAA;
                    fwrite(&data, sizeof(data), 1, fp);
                    toggle ^= 1;
                }

                fclose(fp);
            }
            else if (!strcmp(argv[1], "-g"))
            {   
                // genera una tabla generica con 0xAA y 0x55 de salida para los CPLD.
                uint16_t row = (uint16_t) atoi(argv[2]);
                uint16_t col = (uint16_t) atoi(argv[3]);

                fill_source_tables();
                raw_table_type.row = row;
                raw_table_type.col = col;
                uint16_t bytes = generate_cpld_table((uint8_t *) result_table, source_table_0, source_table_1, source_table_2, &raw_table_type);
                print_row_result_table(0, PRINT_BIN);
                printf("Total bytes: %d\r\n", bytes);
                
                return EXIT_SUCCESS;
            }
            else
            {
                printf("Bad arguments\n");
                return EXIT_FAILURE;
            }
        break;

        case 5:
            if (!strcmp(argv[1], "-tcp"))
            {
                int socketfd = 0; 
                struct sockaddr_in server_addr = {0};

                socketfd = socket(AF_INET, SOCK_STREAM, 0);
                if(-1 == socketfd)
                {
                    fprintf(stderr, "[SERVER-error]: socket creation failed. %d: %s \n", errno, strerror( errno ));
                    return EXIT_FAILURE;        
                }

                /* assign IP, PORT */
                server_addr.sin_family = AF_INET; 
                server_addr.sin_addr.s_addr = inet_addr(argv[2]); 
                server_addr.sin_port = htons(atoi(argv[3])); 
                
                if(0 != connect(socketfd, (struct sockaddr *)&server_addr, sizeof(server_addr)))
                {
                    fprintf(stderr, "[SERVER-error]: socket creation failed. %d: %s \n", errno, strerror( errno ));
                    return EXIT_FAILURE;  
                }

                printf("connected to the server..\n"); 

                sprintf(file_path, "workingdir/%s", argv[4]);
                FILE *fp = fopen(file_path, "r");
                if (NULL == fp)
                {
                    return EXIT_FAILURE;
                }  

                fseek(fp, 0L, SEEK_END);
                unsigned long size_in_bytes = ftell(fp);
                printf("%ld bytes read\n", size_in_bytes);
                fseek(fp, 0L, SEEK_SET);

                unsigned long rbytes = fread(&file_data[0], sizeof(char), size_in_bytes, fp);
                if (rbytes != size_in_bytes)
                {
                    printf("read file error\n");
                    return EXIT_FAILURE;       
                }

                size_t total_sent = 0;
                ssize_t sent;
                
                while (total_sent < rbytes) 
                {
                    sent = write(socketfd, file_data + total_sent, rbytes - total_sent);
                    if (sent == -1) 
                    {
                        fprintf(stderr, "[SERVER-error]: write failed. %d: %s\n", errno, strerror(errno));
                        fclose(fp);
                        close(socketfd);
                        return EXIT_FAILURE;
                    }

                    total_sent += sent;
                }

                printf("CLIENT: Sent %ld bytes successfully.\n", total_sent);                
#if 0                
                /* Leer la respuesta del server */
                // ssize_t rxbytes = read(socketfd, data_from_server, rbytes);
                // printf("CLIENT:Received %ld bytes %s \n", rxbytes, data_from_server);
#endif                
                /* close the socket */
                close(socketfd);
            }
            else
            {
                printf("Bad arguments\n");
                return EXIT_FAILURE;                
            } 

        break;
        
        default:
            printf("Bad arguments\n");
            return EXIT_FAILURE;
        break;
    }

    return EXIT_SUCCESS;
}

void fill_source_tables(void) 
{
    for (int i = 0; i < 256 * 20; i++) {
        source_table_0[i] = (i % 2 == 0) ? 0x55 : 0xAA;
        source_table_1[i] = (i % 2 == 0) ? 0xAA : 0x55;
        source_table_2[i] = (i % 2 == 0) ? 0x55 : 0xAA;
    }
}

uint16_t generate_cpld_table(uint8_t *result_table, const uint8_t *table_0, const uint8_t *table_1, const uint8_t *table_2, table_t *raw_table)
{
	uint16_t row = 0;
	uint16_t col = 0;
	uint8_t bit = 0;
	uint16_t result_index = 0;
    uint16_t bytes = 0; 

	for (row = 0; row < raw_table->row; row++)
    {
        for (col = 0; col < raw_table->col; col++)
        {
            uint8_t byte0 = table_0[row * raw_table->col + col];
            uint8_t byte1 = table_1[row * raw_table->col  + col];
            uint8_t byte2 = table_2[row * raw_table->col  + col];

            for (bit = 0; bit < 6; bit++)
            {
                uint8_t data_clock_1 = ((byte2 >> bit) & 1) << 3 | ((byte1 >> bit) & 1) << 2 | ((byte0 >> bit) & 1) << 1 | 1;
                uint8_t data_clock_0 = ((byte2 >> bit) & 1) << 3 | ((byte1 >> bit) & 1) << 2 | ((byte0 >> bit) & 1) << 1 | 0;

                // Cada "col" contribuye con 12 bytes en la tabla resultante (6 bits * 2 clocks)
                result_index = (row * 240) + (col * 12) + bit * 2;

                result_table[result_index] = data_clock_1;
                result_table[result_index + 1] = data_clock_0;
                bytes += 2;
            }
        }
    }

    return bytes;
}

static void format_table_1(uint8_t *data_dst, uint8_t *data_0, uint8_t *data_1, uint8_t *data_2, table_t *ptr_table)
{
	uint16_t row = 0;
	uint16_t col = 0;
	uint16_t bit = 0;
	uint8_t tmp = 0;

	for (row = 0; row < ptr_table->row; row++)
	{
		for (col = 0; col < ptr_table->col; col++)
		{
			for (bit = 0; bit < 6; bit++)
			{
				tmp = 	((data_2[row * ptr_table->col + col] >> bit) & 0x01 << 3) |
						((data_1[row * ptr_table->col + col] >> bit) & 0x01 << 2) |
						((data_0[row * ptr_table->col + col] >> bit) & 0x01 << 1);

				data_dst[row * ptr_table->col + col] = (tmp | 0x01);
				data_dst[row * ptr_table->col + col + 1] = (tmp & 0b11111110);
			}
		}
	}
}

static void format_table_2(uint8_t *data_dst, uint8_t *data_0, uint8_t *data_1, uint8_t *data_2, table_t *ptr_table)
{
	uint16_t row = 0;
	uint16_t col = 0;
	uint16_t bit = 0;
	uint8_t tmp = 0;

	for (row = 0; row < ptr_table->row; row++)
	{
		for (col = 0; col < ptr_table->col; col++)
		{
			for (bit = 0; bit < 6; bit++)
			{
				tmp = 	((data_2[row * ptr_table->col + col] >> bit) & 0x01 << 3) |
						((data_1[row * ptr_table->col + col] >> bit) & 0x01 << 2) |
						((data_0[row * ptr_table->col + col] >> bit) & 0x01 << 1);

				data_dst[(row * ptr_table->col) + (ptr_table->col * 12) + bit * 2] = (tmp | 0x01);
				data_dst[(row * ptr_table->col) + (ptr_table->col * 12) + bit * 2 + 1] = (tmp & 0b11111110);
			}
		}
	}
}

void print_result_table(void)
{
    for (int row = 0; row < 50; row++) 
    { 
        printf("Row %d:\n", row);
        for (int col = 0; col < 240; col++)
        {
            printf("%02X ", result_table[row][col]);
            if ((col + 1) % 12 == 0)
            {
                printf(" | ");
            }
        }
        printf("\n\n");
    }
}

static void print_row_result_table(uint16_t row, format_print_t format)
{
    int c = 0; 
    int printcol = 0; 

    for (int col = 0; col < 240; col++)
    {
        
        
        if (PRINT_HEX == format)
        {
            printf("0x%02X ", result_table[row][col]);
        }
        else
        {
            print_binary(result_table[row][col]);
        }

        c++;
        printf(" | ");
        if (6 == c)
        {
            c = 0;
            printf("\n\n");
        }
    }

    printf("\n\n");
}

void print_binary(uint8_t byte)
{
    for (int i = 0; i < 8; i++)
    {
        char bin = (((byte << i) & 0x80) == 0x80) ? '1' : '0';
        printf("%c ", bin);
    }
}