#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ostream>
#include <iostream>
#include "spi.h"
#include "gpio.h"
#include <unistd.h>

// SETUP -> c-periphery : 

//  git clone https://github.com/vsergeev/c-periphery.git
//  cd c-periphery/
//  mkdir build
//  cd build/
//  cmake ..
//  make

// SETUP -> Compile

// cd ../../
// g++ -I./c-periphery/src eeg_reader.cpp ./c-periphery/build/libperiphery.a -o reader

// Function to write to a register
void write_reg(spi_t *spi, uint8_t reg_address, uint8_t val_hex) {
    uint8_t zero3[3] = {0x00, 0x00, 0x00};
    uint8_t reg_address_shift = 0x40 | reg_address;
    uint8_t write[3] = {reg_address_shift, 0x00, val_hex};
    spi_transfer(spi, write, zero3, 3);
}

// Function to send a command
void send_command(spi_t *spi, uint8_t command) {
    uint8_t zero1[1] = {0x00};
    uint8_t write_command[1] = {command};
    spi_transfer(spi, write_command, zero1, 1);
}

int main(void) {
    gpio_t *gpio_in = gpio_new();
    gpio_open(gpio_in, "/dev/gpiochip0", 26, GPIO_DIR_IN);

    spi_t *spi = spi_new();
    int spi_open_res = spi_open_advanced(spi, "/dev/spidev0.0", 0x01, 1000000, MSB_FIRST, 8, 1);

    if (spi_open_res < 0) {
        fprintf(stderr, "Failed to open SPI device.\n");
        return 1;
    }

    write_reg(spi, 0x14, 0x80); // LED
    write_reg(spi, 0x05, 0x00); // CH1
    write_reg(spi, 0x06, 0x00); // CH2
    write_reg(spi, 0x07, 0x00); // CH3
    write_reg(spi, 0x08, 0x00); // CH4
    write_reg(spi, 0x09, 0x00); // CH5
    write_reg(spi, 0x0A, 0x00); // CH6
    write_reg(spi, 0x0B, 0x00); // CH7
    write_reg(spi, 0x0C, 0x00); // CH8
    write_reg(spi, 0x15, 0x20); // MICS
    write_reg(spi, 0x01, 0x96); // REG1
    write_reg(spi, 0x02, 0xD4); // REG2
    write_reg(spi, 0x03, 0xFF); // REG3

    send_command(spi, 0x10); // SDATC
    send_command(spi, 0x08); // START

    uint8_t buf[27] = {0};
    float package[8] = {0};
    uint32_t data_test = 0x7FFFFF;
    uint32_t data_check = 0xFFFFFF;
    uint8_t zero27[27] = {0x00};

    int gpio_edge = gpio_set_edge(gpio_in, GPIO_EDGE_FALLING);
    int timeout_ms = 1000;

    FILE *file = fopen("data.txt", "w");
    fprintf(file, "ch1 ch2 ch3 ch4 ch5 ch6 ch7 ch8\n");

    int flag_counter = 0;
    while (1) {
        usleep(100);
        int gpio_res = gpio_poll(gpio_in, timeout_ms);
        flag_counter++;

        if (gpio_res == 1) {
            gpio_edge_t edge = GPIO_EDGE_NONE;
            gpio_res = gpio_read_event(gpio_in, &edge, NULL);
            spi_transfer(spi, zero27, buf, 27);


            for (int i = 1; i < 9; i++) {
                int offset = 3 * i;
                uint32_t voltage = (buf[offset] << 8) | buf[offset + 1];
                voltage = (voltage << 8) | buf[offset + 2];
                uint32_t voltage_test = voltage | data_test;
                if (voltage_test == data_check) {
                    voltage = 16777214 - voltage;
                }

                package[i - 1] = 0.27 * voltage; // (4.5*1000000/16777214) = 0.27

                if (flag_counter == 25) {
                    std::cout << package[i - 1] << " ";
                }
                fprintf(file, "%.1f ", package[i - 1]);
            }

            if (flag_counter == 25) {
                flag_counter = 0;
                std::cout << std::endl;
            }
            fprintf(file, "\n");
        } else {
            printf("ADS1299 didn't send data\n");
        }
    }

    spi_close(spi);
    spi_free(spi);
    gpio_close(gpio_in);
    gpio_free(gpio_in);
    fclose(file);

    return 0;
}
