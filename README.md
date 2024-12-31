# Binary Generator program

This repository contains a program to generate somes generic binary files with 0x55 and 0xAA of data fill in a particulary format and the possibility to send this files to the Server with the correct parameters.

# Built With

  - [VSCode](https://code.visualstudio.com/download)
  - gcc binary_generator.c -o binary_generator

# Use
## Generate a generic table (-f)
    
  To get a file .bin with a table with 0x55 and 0xAA of data fill you should to do:

    ./binary_generator -f <file> <index>"

    Eg: ./binary_generator -f test.bin 0

  Where "< file >" is the name file that you asign to the generated file and "< index >" is the number of the board asigned related with the table. It Can be 0, 1 and 2.

## Generate a 3 data tables and 3 calibration tables (-fdc)

  This command generate three data and calibration tables with 0x55 and 0xAA of data fill to data table and 0x01 to calibration tables.

    ./binary_generator -fdc

  Print:
    
    binary generator run
    Data workingdir/data_table_0x55_AA_0.bin file ok
    Data workingdir/data_table_0x55_AA_1.bin file ok
    Data workingdir/data_table_0x55_AA_2.bin file ok
    Calibration workingdir/cal_table_0x01_0.bin file ok
    Calibration workingdir/cal_table_0x01_1.bin file ok
    Calibration workingdir/cal_table_0x01_2.bin file ok

## Print generic data table (-g)

  This command generate three source tables with 0x55 and 0xAA of data fill
    
    static void fill_source_tables(void) 
    {
      for (int i = 0; i < 256 * 20; i++) 
      {
          source_table_0[i] = (i % 2 == 0) ? 0x55 : 0xAA;
          source_table_1[i] = (i % 2 == 0) ? 0xAA : 0x55;
          source_table_2[i] = (i % 2 == 0) ? 0x55 : 0xAA;
      }
    } 
  
  Next it call "generate_cpld_table" to generate a data table to CPLD's and the end print the first row.

    ./binary_generator -g <rows> <columns>
  
  Eg:
  
    ./binary_generator -g 256 20
    
  Print:
    
    binary generator run
    print_generic_cpld_table
    0 0 0 0 1 0 1 1  | 0 0 0 0 1 0 1 0  | 0 0 0 0 0 1 0 1  | 0 0 0 0 0 1 0 0  | 0 0 0 0 1 0 1 1  | 0 0 0 0 1 0 1 0  |

    ...

    0 0 0 0 1 0 1 1  | 0 0 0 0 1 0 1 0  | 0 0 0 0 0 1 0 1  | 0 0 0 0 0 1 0 0  | 0 0 0 0 1 0 1 1  | 0 0 0 0 1 0 1 0  | 



    Total bytes: 61440

## Send a binary file to Server TCP (-tcp)

  This command send files to TCP connection:

    ./binary_generator -tcp <IP> <PORT> <file_name_1> ... <file_name_n>
  
  Eg:

    ./binary_generator -tcp 192.168.56.1 7 test_0.bin test_1.bin
  
  Print:

    binary generator run
    connected to the server..
    files 2
    5122 bytes read
    CLIENT: Sent file 1, 5122 bytes successfully.
    5122 bytes read
    CLIENT: Sent file 2, 5122 bytes successfully.

## Send three data tables and three calibration tables (-send)
  
  This command send three data tables and three calibration tables previusly generated.

    ./binary_generator -send <IP> <PORT>
  
  Eg: 
  
    ./binary_generator -tcp 192.168.56.1 7

  print:

    binary generator run
    Data workingdir/data_table_0x55_AA_0.bin file ok
    Data workingdir/data_table_0x55_AA_1.bin file ok
    Data workingdir/data_table_0x55_AA_2.bin file ok
    Calibration workingdir/cal_table_0x01_0.bin file ok
    Calibration workingdir/cal_table_0x01_1.bin file ok
    Calibration workingdir/cal_table_0x01_2.bin file ok
    connected to the server..
    Sending files to TCP Server
    5122 bytes read
    CLIENT: Sent workingdir/data_table_0x55_AA_0.bin, 5122 bytes successfully.
    5122 bytes read
    CLIENT: Sent workingdir/data_table_0x55_AA_1.bin, 5122 bytes successfully.
    5122 bytes read
    CLIENT: Sent workingdir/data_table_0x55_AA_2.bin, 5122 bytes successfully.
    22 bytes read
    CLIENT: Sent workingdir/cal_table_0x01_0.bin, 22 bytes successfully.
    22 bytes read
    CLIENT: Sent workingdir/cal_table_0x01_1.bin, 22 bytes successfully.
    22 bytes read
    CLIENT: Sent workingdir/cal_table_0x01_2.bin, 22 bytes successfully.  

# Authors

  - [Gonzalo Rivera](gonzaloriveras90@gmail.com)

# License

© 2023 grivera. All rights reserved.