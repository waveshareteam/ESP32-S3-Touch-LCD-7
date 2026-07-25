/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* cmd_i2ctools.c

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include "argtable3/argtable3.h"
#include "driver/i2c_master.h"
#include "esp_console.h"
#include "esp_log.h"

static const char *TAG = "cmd_i2ctools"; // Tag for logging

#define I2C_TOOL_TIMEOUT_VALUE_MS (50) // Timeout value for I2C operations
static uint32_t i2c_frequency = 100 * 1000; // Default I2C frequency
i2c_master_bus_handle_t tool_bus_handle; // Handle for I2C master bus

// Function to get the I2C port number
static esp_err_t i2c_get_port(int port, i2c_port_t *i2c_port)
{
    if (port >= I2C_NUM_MAX) {
        ESP_LOGE(TAG, "Wrong port number: %d", port); // Log error for wrong port
        return ESP_FAIL; // Return failure
    }
    *i2c_port = port; // Assign port number
    return ESP_OK; // Return success
}

// Argument structure for I2C configuration command
static struct {
    struct arg_int *port;     // Argument for I2C port number
    struct arg_int *freq;     // Argument for I2C frequency
    struct arg_int *sda;      // Argument for I2C SDA GPIO pin
    struct arg_int *scl;      // Argument for I2C SCL GPIO pin
    struct arg_end *end;      // Argument for end of command
} i2cconfig_args;

// Command to configure the I2C bus
static int do_i2cconfig_cmd(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&i2cconfig_args); // Parse command arguments
    i2c_port_t i2c_port = I2C_NUM_0; // Default I2C port
    int i2c_gpio_sda = 0; // Default SDA GPIO pin
    int i2c_gpio_scl = 0; // Default SCL GPIO pin
    if (nerrors != 0) {
        arg_print_errors(stderr, i2cconfig_args.end, argv[0]); // Print argument parsing errors
        return 0; // Exit command
    }

    /* Check "--port" option */
    if (i2cconfig_args.port->count) {
        if (i2c_get_port(i2cconfig_args.port->ival[0], &i2c_port) != ESP_OK) {
            return 1; // Return error if port number is invalid
        }
    }
    /* Check "--freq" option */
    if (i2cconfig_args.freq->count) {
        i2c_frequency = i2cconfig_args.freq->ival[0]; // Update I2C frequency
    }
    /* Check "--sda" option */
    i2c_gpio_sda = i2cconfig_args.sda->ival[0]; // Get SDA GPIO pin
    /* Check "--scl" option */
    i2c_gpio_scl = i2cconfig_args.scl->ival[0]; // Get SCL GPIO pin

    // Re-initialize the I2C bus
    if (i2c_del_master_bus(tool_bus_handle) != ESP_OK) {
        return 1; // Return error if bus deletion fails
    }

    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT, // Set clock source
        .i2c_port = i2c_port, // Set I2C port
        .scl_io_num = i2c_gpio_scl, // Set SCL GPIO pin
        .sda_io_num = i2c_gpio_sda, // Set SDA GPIO pin
        .glitch_ignore_cnt = 7, // Glitch ignore count
        .flags.enable_internal_pullup = true, // Enable internal pull-up resistors
    };

    // Create new I2C master bus with the configured parameters
    if (i2c_new_master_bus(&i2c_bus_config, &tool_bus_handle) != ESP_OK) {
        return 1; // Return error if bus creation fails
    }

    return 0; // Successful command execution
}

// Function to register the I2C configuration command
static void register_i2cconfig(void)
{
    i2cconfig_args.port = arg_int0(NULL, "port", "<0|1>", "Set the I2C bus port number");
    i2cconfig_args.freq = arg_int0(NULL, "freq", "<Hz>", "Set the frequency(Hz) of I2C bus");
    i2cconfig_args.sda = arg_int1(NULL, "sda", "<gpio>", "Set the gpio for I2C SDA");
    i2cconfig_args.scl = arg_int1(NULL, "scl", "<gpio>", "Set the gpio for I2C SCL");
    i2cconfig_args.end = arg_end(2); // Define end of argument table
    const esp_console_cmd_t i2cconfig_cmd = {
        .command = "i2cconfig", // Command name
        .help = "Config I2C bus", // Command help text
        .hint = NULL,
        .func = &do_i2cconfig_cmd, // Command execution function
        .argtable = &i2cconfig_args // Argument table
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&i2cconfig_cmd)); // Register command
}

// Function to detect I2C devices on the bus
static int do_i2cdetect_cmd(int argc, char **argv)
{
    uint8_t address; // Variable to store the I2C address
    printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\r\n");
    for (int i = 0; i < 128; i += 16) { // Loop through address space
        printf("%02x: ", i);
        for (int j = 0; j < 16; j++) { // Check each address in the block
            fflush(stdout); // Flush output buffer
            address = i + j; // Compute address
            esp_err_t ret = i2c_master_probe(tool_bus_handle, address, I2C_TOOL_TIMEOUT_VALUE_MS); // Probe address
            if (ret == ESP_OK) {
                printf("%02x ", address); // Address detected
            } else if (ret == ESP_ERR_TIMEOUT) {
                printf("UU "); // Address unresponsive (timeout)
            } else {
                printf("-- "); // Address not found
            }
        }
        printf("\r\n"); // New line after each block
    }

    return 0; // Successful command execution
}

// Function to register the I2C detection command
static void register_i2cdetect(void)
{
    const esp_console_cmd_t i2cdetect_cmd = {
        .command = "i2cdetect", // Command name
        .help = "Scan I2C bus for devices", // Command help text
        .hint = NULL,
        .func = &do_i2cdetect_cmd, // Command execution function
        .argtable = NULL // No arguments for this command
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&i2cdetect_cmd)); // Register command
}

// Argument structure for I2C get command
static struct {
    struct arg_int *chip_address; // Argument for chip address
    struct arg_int *register_address; // Argument for register address
    struct arg_int *data_length; // Argument for data length to read
    struct arg_end *end; // Argument for end of command
} i2cget_args;

// Command to get data from an I2C device
static int do_i2cget_cmd(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&i2cget_args); // Parse command arguments
    if (nerrors != 0) {
        arg_print_errors(stderr, i2cget_args.end, argv[0]); // Print argument parsing errors
        return 0; // Exit command
    }

    /* Check chip address: "-c" option */
    int chip_addr = i2cget_args.chip_address->ival[0]; // Get chip address from command arguments
    /* Check register address: "-r" option */
    int data_addr = -1; // Default register address
    if (i2cget_args.register_address->count) {
        data_addr = i2cget_args.register_address->ival[0]; // Get register address if provided
    }
    /* Check data length: "-l" option */
    int len = 1; // Default data length
    if (i2cget_args.data_length->count) {
        len = i2cget_args.data_length->ival[0]; // Get data length from command arguments
    }
    uint8_t *data = malloc(len); // Allocate memory to store the read data

    // Configure I2C device settings
    i2c_device_config_t i2c_dev_conf = {
        .scl_speed_hz = i2c_frequency, // Set the I2C clock speed
        .device_address = chip_addr, // Set the I2C device address
    };
    i2c_master_dev_handle_t dev_handle; // Handle for the I2C device

    // Add I2C device to the bus
    if (i2c_master_bus_add_device(tool_bus_handle, &i2c_dev_conf, &dev_handle) != ESP_OK) {
        return 1; // Return error if device addition fails
    }

    // Perform I2C read operation
    esp_err_t ret = i2c_master_transmit_receive(dev_handle, (uint8_t*)&data_addr, 1, data, len, I2C_TOOL_TIMEOUT_VALUE_MS);
    if (ret == ESP_OK) {
        // Print the read data in hexadecimal format
        for (int i = 0; i < len; i++) {
            printf("0x%02x ", data[i]); // Print each byte of data
            if ((i + 1) % 16 == 0) {
                printf("\r\n"); // New line after every 16 bytes
            }
        }
        if (len % 16) {
            printf("\r\n"); // New line if the last line is not complete
        }
    } else if (ret == ESP_ERR_TIMEOUT) {
        ESP_LOGW(TAG, "Bus is busy"); // Log a warning if the bus is busy
    } else {
        ESP_LOGW(TAG, "Read failed"); // Log a warning for read failure
    }

    free(data); // Free allocated memory
    // Remove the I2C device from the bus
    if (i2c_master_bus_rm_device(dev_handle) != ESP_OK) {
        return 1; // Return error if device removal fails
    }
    return 0; // Successful execution
}

// Function to register the i2cget command
static void register_i2cget(void)
{
    i2cget_args.chip_address = arg_int1("c", "chip", "<chip_addr>", "Specify the address of the chip on that bus"); // Define chip address argument
    i2cget_args.register_address = arg_int0("r", "register", "<register_addr>", "Specify the address on that chip to read from"); // Define register address argument
    i2cget_args.data_length = arg_int0("l", "length", "<length>", "Specify the length to read from that data address"); // Define data length argument
    i2cget_args.end = arg_end(1); // End argument parsing
    const esp_console_cmd_t i2cget_cmd = {
        .command = "i2cget", // Command name
        .help = "Read registers visible through the I2C bus", // Help description
        .hint = NULL, // No hint
        .func = &do_i2cget_cmd, // Function to call for this command
        .argtable = &i2cget_args // Argument table for the command
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&i2cget_cmd)); // Register the command with the console
}

static struct {
    struct arg_int *chip_address; // Argument for chip address
    struct arg_int *register_address; // Argument for register address
    struct arg_int *data; // Argument for data to write
    struct arg_end *end; // End of argument parsing
} i2cset_args;

// Command to set data on an I2C device
static int do_i2cset_cmd(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&i2cset_args); // Parse command arguments
    if (nerrors != 0) {
        arg_print_errors(stderr, i2cset_args.end, argv[0]); // Print argument parsing errors
        return 0; // Exit command
    }

    /* Check chip address: "-c" option */
    int chip_addr = i2cset_args.chip_address->ival[0]; // Get chip address from command arguments
    /* Check register address: "-r" option */
    int data_addr = 0; // Default register address
    if (i2cset_args.register_address->count) {
        data_addr = i2cset_args.register_address->ival[0]; // Get register address if provided
    }
    /* Check data: "-d" option */
    int len = i2cset_args.data->count; // Get the number of data bytes to write

    // Configure I2C device settings
    i2c_device_config_t i2c_dev_conf = {
        .scl_speed_hz = i2c_frequency, // Set the I2C clock speed
        .device_address = chip_addr, // Set the I2C device address
    };
    i2c_master_dev_handle_t dev_handle; // Handle for the I2C device

    // Add I2C device to the bus
    if (i2c_master_bus_add_device(tool_bus_handle, &i2c_dev_conf, &dev_handle) != ESP_OK) {
        return 1; // Return error if device addition fails
    }

    uint8_t *data = malloc(len + 1); // Allocate memory for the data to be written
    data[0] = data_addr; // Set the first byte as the register address
    for (int i = 0; i < len; i++) {
        data[i + 1] = i2cset_args.data->ival[i]; // Fill the data array with provided values
    }

    // Perform I2C write operation
    esp_err_t ret = i2c_master_transmit(dev_handle, data, len + 1, I2C_TOOL_TIMEOUT_VALUE_MS);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Write OK"); // Log success message
    } else if (ret == ESP_ERR_TIMEOUT) {
        ESP_LOGW(TAG, "Bus is busy"); // Log a warning if the bus is busy
    } else {
        ESP_LOGW(TAG, "Write Failed"); // Log a warning for write failure
    }

    free(data); // Free allocated memory
    // Remove the I2C device from the bus
    if (i2c_master_bus_rm_device(dev_handle) != ESP_OK) {
        return 1; // Return error if device removal fails
    }
    return 0; // Successful execution
}

// Function to register the i2cset command
static void register_i2cset(void)
{
    i2cset_args.chip_address = arg_int1("c", "chip", "<chip_addr>", "Specify the address of the chip on that bus"); // Define chip address argument
    i2cset_args.register_address = arg_int0("r", "register", "<register_addr>", "Specify the address on that chip to read from"); // Define register address argument
    i2cset_args.data = arg_intn(NULL, NULL, "<data>", 0, 256, "Specify the data to write to that data address"); // Define data argument
    i2cset_args.end = arg_end(2); // End argument parsing
    const esp_console_cmd_t i2cset_cmd = {
        .command = "i2cset", // Command name
        .help = "Set registers visible through the I2C bus", // Help description
        .hint = NULL, // No hint
        .func = &do_i2cset_cmd, // Function to call for this command
        .argtable = &i2cset_args // Argument table for the command
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&i2cset_cmd)); // Register the command with the console
}

// Structure to hold command line arguments for the i2cdump command
static struct {
    struct arg_int *chip_address; // Chip address on the I2C bus
    struct arg_int *size;          // Size of data to read (1, 2, or 4 bytes)
    struct arg_end *end;           // End of argument table
} i2cdump_args;

// Function to handle the i2cdump command
static int do_i2cdump_cmd(int argc, char **argv)
{
    // Parse command arguments
    int nerrors = arg_parse(argc, argv, (void **)&i2cdump_args);
    if (nerrors != 0) {
        // Print argument parsing errors if any
        arg_print_errors(stderr, i2cdump_args.end, argv[0]);
        return 0; // Exit command
    }

    // Check chip address: "-c" option
    int chip_addr = i2cdump_args.chip_address->ival[0];
    // Check read size: "-s" option
    int size = 1; // Default read size
    if (i2cdump_args.size->count) {
        size = i2cdump_args.size->ival[0]; // Get specified size if provided
    }
    // Validate read size (must be 1, 2, or 4)
    if (size != 1 && size != 2 && size != 4) {
        ESP_LOGE(TAG, "Wrong read size. Only support 1, 2, 4");
        return 1; // Exit if the size is invalid
    }

    // Configure I2C device with the specified chip address and SCL speed
    i2c_device_config_t i2c_dev_conf = {
        .scl_speed_hz = i2c_frequency,
        .device_address = chip_addr,
    };
    i2c_master_dev_handle_t dev_handle;
    // Add the I2C device to the bus
    if (i2c_master_bus_add_device(tool_bus_handle, &i2c_dev_conf, &dev_handle) != ESP_OK) {
        return 1; // Exit on failure
    }

    uint8_t data_addr; // Variable to hold the data address for reading
    uint8_t data[4];   // Buffer to hold data read from the I2C device
    int32_t block[16]; // Buffer to hold formatted data for display
    printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f"
           "    0123456789abcdef\r\n");
    
    // Loop to read and display data from the I2C device
    for (int i = 0; i < 128; i += 16) {
        printf("%02x: ", i); // Print the current address
        for (int j = 0; j < 16; j += size) {
            fflush(stdout); // Flush output buffer
            data_addr = i + j; // Calculate the current data address
            // Read data from the I2C device
            esp_err_t ret = i2c_master_transmit_receive(dev_handle, &data_addr, 1, data, size, I2C_TOOL_TIMEOUT_VALUE_MS);
            if (ret == ESP_OK) {
                // Print read data if successful
                for (int k = 0; k < size; k++) {
                    printf("%02x ", data[k]);
                    block[j + k] = data[k]; // Store read data in block buffer
                }
            } else {
                // Indicate read failure by printing "XX"
                for (int k = 0; k < size; k++) {
                    printf("XX ");
                    block[j + k] = -1; // Mark as failure
                }
            }
        }
        printf("   "); // Print spacing between hexadecimal and ASCII output
        for (int k = 0; k < 16; k++) {
            // Print ASCII representation of the read data
            if (block[k] < 0) {
                printf("X"); // Mark failed reads
            }
            if ((block[k] & 0xff) == 0x00 || (block[k] & 0xff) == 0xff) {
                printf("."); // Print dot for null or full bytes
            } else if ((block[k] & 0xff) < 32 || (block[k] & 0xff) >= 127) {
                printf("?"); // Print question mark for non-printable characters
            } else {
                printf("%c", (char)(block[k] & 0xff)); // Print printable characters
            }
        }
        printf("\r\n"); // New line after each row of data
    }

    // Remove the I2C device from the bus
    if (i2c_master_bus_rm_device(dev_handle) != ESP_OK) {
        return 1; // Exit on failure
    }
    return 0; // Success
}

// Function to register the i2cdump command
static void register_i2cdump(void)
{
    // Define command arguments
    i2cdump_args.chip_address = arg_int1("c", "chip", "<chip_addr>", "Specify the address of the chip on that bus");
    i2cdump_args.size = arg_int0("s", "size", "<size>", "Specify the size of each read (1, 2, or 4 bytes)");
    i2cdump_args.end = arg_end(1);
    
    // Register the i2cdump command with the console
    const esp_console_cmd_t i2cdump_cmd = {
        .command = "i2cdump", // Command name
        .help = "Examine registers visible through the I2C bus", // Help description
        .hint = NULL, // No hint
        .func = &do_i2cdump_cmd, // Function to handle command
        .argtable = &i2cdump_args // Argument table for command
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&i2cdump_cmd)); // Register command and check for errors
}

// Function to register all I2C tool commands
void register_i2ctools(void)
{
    register_i2cconfig(); // Register I2C configuration command
    register_i2cdetect(); // Register I2C detection command
    register_i2cget();    // Register I2C get command
    register_i2cset();    // Register I2C set command
    register_i2cdump();   // Register I2C dump command
}
