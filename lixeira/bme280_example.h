#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#include <main/inc/bme280.h>
#include <main/inc/bme280_example.h>

/* Structure that contains identifier details used in example */
struct identifier
{
    /* Variable to hold device address */
    uint8_t dev_addr;

    /* Variable that contains file descriptor */
    int8_t fd;
};

int8_t user_i2c_read(uint8_t reg_addr, uint8_t *data, uint32_t len, void *intf_ptr);
void user_delay_us(uint32_t period, void *intf_ptr);
int8_t user_i2c_write(uint8_t reg_addr, const uint8_t *data, uint32_t len, void *intf_ptr);
void print_sensor_data(struct bme280_data *comp_data);
int8_t stream_sensor_data_forced_mode(struct bme280_dev *dev);

// USED TO GET THE DATA WE NEED
struct bce280_data *get_bce280_temp_press_humidity(char *ic2_bus);
