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
#include <ctype.h>

/* output and strings */
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
/******************************************************************************
    Defines and constants
******************************************************************************/
#define SERVER_ADDRESS "10.255.255.210" /* Server IP */
#define PORT 7 
#define MAX_RAW_DATA_TABLE_LENGHT			((256 * 20) + 2)
#define MAX_RAW_CAL_TABLE_LENGHT            (20 + 2)
#define OPTIONS                             (5)
#define DEFAUTL_FILES_COUNT                 (6)
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

typedef struct
{
    const char *cmd;
    int (*cmd_func)(int, char **);
} cmd_t;
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

static cmd_t commands[OPTIONS] = 
{
    { "-f",     NULL},
    { "-fdc",    NULL},
    { "-g",     NULL},
    { "-tcp",   NULL},
    { "-send",   NULL}
};

static const char *files_name[6] = 
{
    "workingdir/data_table_0x55_AA_0.bin",
    "workingdir/data_table_0x55_AA_1.bin",
    "workingdir/data_table_0x55_AA_2.bin",
    "workingdir/cal_table_0x01_0.bin",
    "workingdir/cal_table_0x01_1.bin",
    "workingdir/cal_table_0x01_2.bin"
};
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
static int connect_to_tcp_server(const char *ip, uint16_t port, int *socketfd);
static int tcp_arg_handler(int argc, char **argv);
static int tcp_send_data_cal(int argc, char **argv);
static int file_arg_handler(int argc, char **argv);
static int file_data_cal_generation(int argc, char **argv);
static int print_generic_cpld_table(int argc, char **argv);
static bool is_number_valid(char *str, int n);
static bool is_file_name_valid(char *str, int n);
static void dispatcher(cmd_t *commands, char *opt, int argc, char **argv);
/******************************************************************************
    Local function definitions
******************************************************************************/

/******************************************************************************
    Public function definitions
******************************************************************************/
int main(int argc, char **argv)
{
    printf("binary generator run\r\n");

    commands[0].cmd_func = file_arg_handler;
    commands[1].cmd_func = file_data_cal_generation;
    commands[2].cmd_func = print_generic_cpld_table;
    commands[3].cmd_func = tcp_arg_handler;
    commands[4].cmd_func = tcp_send_data_cal;

    dispatcher(commands, argv[1], argc, argv);

    return EXIT_SUCCESS;
}

static void fill_source_tables(void) 
{
    for (int i = 0; i < 256 * 20; i++) 
    {
        source_table_0[i] = (i % 2 == 0) ? 0x55 : 0xAA;
        source_table_1[i] = (i % 2 == 0) ? 0xAA : 0x55;
        source_table_2[i] = (i % 2 == 0) ? 0x55 : 0xAA;
    }
}

static uint16_t generate_cpld_table(uint8_t *result_table, const uint8_t *table_0, const uint8_t *table_1, const uint8_t *table_2, table_t *raw_table)
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

static void print_result_table(void)
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

static void print_binary(uint8_t byte)
{
    for (int i = 0; i < 8; i++)
    {
        char bin = (((byte << i) & 0x80) == 0x80) ? '1' : '0';
        printf("%c ", bin);
    }
}

static int connect_to_tcp_server(const char *ip, uint16_t port, int *socketfd)
{
    struct sockaddr_in server_addr = {0};
    *socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == *socketfd)
    {
        fprintf(stderr, "[SERVER-error]: socket creation failed. %d: %s \n", errno, strerror( errno ));
        return EXIT_FAILURE;        
    }

    /* assign IP, PORT */
    server_addr.sin_family = AF_INET; 
    server_addr.sin_addr.s_addr = inet_addr(ip); 
    server_addr.sin_port = htons(port); 
    
    if(0 != connect(*socketfd, (struct sockaddr *)&server_addr, sizeof(server_addr)))
    {
        fprintf(stderr, "[SERVER-error]: socket creation failed. %d: %s \n", errno, strerror( errno ));
        return EXIT_FAILURE;  
    }

    printf("connected to the server..\n"); 
    return EXIT_SUCCESS;    
}

static int tcp_arg_handler(int argc, char **argv)
{
    int socketfd = 0;
    int files = 0;
    int c = 0;

    if (0 != connect_to_tcp_server(argv[2], atoi(argv[3]), &socketfd))
    {
        return EXIT_FAILURE;
    }

    files = (argc - 4); 
    printf("files %d\r\n", files);

    for (c = 0; c < files; c++)
    {
        sprintf(file_path, "workingdir/%s", argv[4 + c]);
        FILE *fp = fopen(file_path, "r");
        if (NULL == fp)
        {
            fprintf(stderr, "[FILE-error]: open failed. %d: %s\n", errno, strerror(errno));
            fclose(fp);
            close(socketfd);
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

        fclose(fp);
        printf("CLIENT: Sent file %d, %ld bytes successfully.\n", (c + 1), total_sent); 
        // sleep(1);
        usleep(500000);
    }

    close(socketfd);

    return EXIT_SUCCESS;
}

static int tcp_send_data_cal(int argc, char **argv)
{
    int socketfd = 0;
    int files = 0;
    int c = 0;

    /* Se crean los archivos a mandar */
    file_data_cal_generation(argc, argv);

    if (0 != connect_to_tcp_server(argv[2], atoi(argv[3]), &socketfd))
    {
        return EXIT_FAILURE;
    }

    printf("Sending files to TCP Server\r\n");

    for (c = 0; c < DEFAUTL_FILES_COUNT; c++)
    {
        sprintf(file_path, "%s", files_name[c]);
        FILE *fp = fopen(file_path, "r");
        if (NULL == fp)
        {
            fprintf(stderr, "[FILE-error]: open failed. %d: %s\n", errno, strerror(errno));
            fclose(fp);
            close(socketfd);
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

        fclose(fp);
        printf("CLIENT: Sent %s, %ld bytes successfully.\n", file_path, total_sent); 
        usleep(500000);
    }

    close(socketfd);

    return EXIT_SUCCESS;
}

static int file_arg_handler(int argc, char **argv)
{
    int files = 0;
    int c = 0;
    uint8_t toggle = 0;
    char data = 0; 
    int board = 0;

    while(argv[2 + c] != NULL)
    {
        /* Es un nombre valido? -> crear el archivo */
        if (true != is_file_name_valid(argv[2 + c], strlen(argv[2 + c])))
        {
            return EXIT_FAILURE;
        }

        /* es un indice valido? -> asignarle ese indice al archivo anterior */
        if (true != is_number_valid(argv[2 + c + 1], strlen(argv[2 + c + 1])))
        {
            return EXIT_FAILURE;            
        }

        /* Escribir el archivo con data generica */
        sprintf(file_path, "workingdir/%s", argv[2 + c]);
        FILE *fp = fopen(file_path, "wb");
        if (NULL == fp)
        {
            fprintf(stderr, "[FILE-error]: open failed. %d: %s\n", errno, strerror(errno));
            fclose(fp);
            return EXIT_FAILURE;
        }

        char index = *argv[2 + c + 1];
        char table_type = 'D';
        fwrite(&table_type, sizeof(table_type), 1, fp);     // indicador del tipo de tabla.
        fwrite(&index, sizeof(index), 1, fp);               // Indice que apunta a la placa (RX1, RX2 y TX).

        for (int i = 0; i < MAX_RAW_DATA_TABLE_LENGHT - 2; i++)
        {
            data = (toggle == 1)? 0x55 : 0xAA;
            fwrite(&data, sizeof(data), 1, fp);
            toggle ^= 1;
        }

        printf("%s ok\r\n", argv[2 + c]);

        /* Cerrar el archivo */
        fclose(fp);

        c += 2;
    } 

    return EXIT_SUCCESS;
}

static int file_data_cal_generation(int argc, char **argv)
{
    int files = 0;
    uint8_t toggle = 0;
    char data = 0;

    /* Crea 3 tablas de datos genericas */
    for (files = 0; files < 3; files++)
    {
        // sprintf(file_path, "workingdir/data_table_0x55_0xAA_%c.bin", (files + 0x30));
        sprintf(file_path, "%s", files_name[files]);
        FILE *fp = fopen(file_path, "wb");
        if (NULL == fp)
        {
            fprintf(stderr, "[FILE-error]: open failed. %d: %s\n", errno, strerror(errno));
            fclose(fp);
            return EXIT_FAILURE;
        }

        char table_type = 'D';
        char index = '0' + files;
        fwrite(&table_type, sizeof(table_type), 1, fp);     // indicador del tipo de tabla.
        fwrite(&index, sizeof(index), 1, fp);               // Indice que apunta a la placa (RX1, RX2 y TX).

        for (int i = 0; i < MAX_RAW_DATA_TABLE_LENGHT - 2; i++)
        {
            data = (toggle == 1) ? 0x55 : 0xAA;
            fwrite(&data, sizeof(data), 1, fp);
            toggle ^= 1;
        }

        printf("Data %s file ok\r\n", file_path);
        fclose(fp);
    }
    
    /* Crea 3 tablas de calibracion con 0x01 */
    for (files = 0; files < 3; files++)
    {
        sprintf(file_path, "workingdir/cal_table_0x01_%c.bin", (files + 0x30));
        FILE *fp = fopen(file_path, "wb");
        if (NULL == fp)
        {
            fprintf(stderr, "[FILE-error]: open failed. %d: %s\n", errno, strerror(errno));
            fclose(fp);
            return EXIT_FAILURE;
        }

        char table_type = 'C';
        char index = '0' + files;
        fwrite(&table_type, sizeof(table_type), 1, fp);     // indicador del tipo de tabla.
        fwrite(&index, sizeof(index), 1, fp);               // Indice que apunta a la placa (RX1, RX2 y TX).

        for (int i = 0; i < MAX_RAW_CAL_TABLE_LENGHT - 2; i++)
        {
            data = 0x01;
            fwrite(&data, sizeof(data), 1, fp);
        }

        printf("Calibration %s file ok\r\n", file_path);
        fclose(fp);
    }

    /* Fin */
    return EXIT_SUCCESS;
}

static int print_generic_cpld_table(int argc, char **argv)
{
    printf("print_generic_cpld_table\r\n");
    
    int params = argc - 2;
    
    if (params != 2)
    {
        return EXIT_FAILURE;
    }

    if (true != is_number_valid(argv[2], strlen(argv[2])))
    {
        return EXIT_FAILURE;
    }

    if (true != is_number_valid(argv[3], strlen(argv[3])))
    {
        return EXIT_FAILURE;
    }

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

static bool is_number_valid(char *str, int n)
{
    for (int i = 0; i < n - 1; i++)
    {
        if(false == isdigit(str[i]))
        {
            return false;
        }
    }

    return true;    
}

static bool is_file_name_valid(char *str, int n)
{
    // If first character is invalid.
    if (!((str[0] >= 'a' && str[0] <= 'z') || (str[0] >= 'A' && str[0] <= 'Z') || str[0] == '_' || str[0] == '.'))
    {
        return false;
    }
        
    // Traverse the string for the rest of the characters.
    for (int i = 1; i < n; i++) 
    {
        if (!((str[i] >= 'a' && str[i] <= 'z') || (str[i] >= 'A' && str[i] <= 'Z') || (str[i] >= '0' && str[i] <= '9') || str[i] == '_'  || str[i] == '.'))
        {
            printf("%d", i);
            return false;
        }
    }
 
    return true;
}

static void dispatcher(cmd_t *commands, char *opt, int argc, char **argv)
{
    int i = 0; 

    for (i = 0; i < OPTIONS; i++)
    {
        if(!strcmp(commands[i].cmd, opt))
        {
            if (NULL != commands[i].cmd_func)
            {
                commands[i].cmd_func(argc, &argv[0]);
            }
        }
    }
}