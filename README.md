# Binary Generator program

This repository contains a program to generate a generic binary file with 0x55 and 0xAA with a particulary format. Implement the function to send this file to Server TCP and implement the functionality to print the table format generated.

## Built With

  - [VSCode](https://code.visualstudio.com/download)
  - gcc binary_generator.c -o binary_generator

## Use
  - You can execute few commands with binary generator.
    ## Generate a generic table
    
    To get a file .bin with a table with 0x55 and 0xAA of data fill you should to do:
    
    ./binary_generator -f <file> <index>

    E.g: ./binary_generator -f test.bin 0
    
    Where <file> is the name file that you asign to the generated file and <index> is the number of the board asigned related with the table. It Can be 0, 1 and 2.
    
    ## Send a binary file to Server TCP

   	In a terminal you should to do: 

	./binary_generator -tcp <IP> <PORT> <full_file_name_generated>

    E.g: ./binary_generator -tcp 10.255.255.210 7 test.bin

    ## Show generated table
    
    To print a test of a table you should to do: 

    ./binary_generator -g <rows> <columns>

    E.g: ./binary_generator -g 256 20

## Authors

  - [Gonzalo Rivera](gonzaloriveras90@gmail.com)

## License

© 2023 grivera. All rights reserved.