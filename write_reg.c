/*This connection-progam tries to start a connection with the goodwe-inverter

importent: the SLAVE_ID for the inverter is 247; baudate: 9600; Bits 8N1; one register has 16bit or 4byte
 
execution: ./connection <register> <size in bytes>*/

#include <stdio.h>
#include <modbus.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int fd_272;
int fd_273;

void set_rts_custom(modbus_t*, int);
void config_gpio_pins();
void close_gpio_pins();

void print_bits(uint16_t val, uint16_t offset)
{
  for (int i = 15; 0 <= i; i--) {
    // if (i % 4 == 3) printf (" ");
    // printf("%c", (val & (1 << i)) ? '1' : '0');
    if (val & (1 << i)) {
       printf("%d ", i+offset);
    }
  }
}

int main(int argc, char* argv[]){

    if (argc != 3){
        printf("usage: %s <register> <val>\n", argv[0]);
        return -1;
    }


    const int REMOTE_ID = 247;
    int reg = atoi(argv[1]);
    int val = atoi(argv[2]);

    modbus_t *ctx;
    uint16_t tab_reg[64]={0};

    config_gpio_pins();

    ctx = modbus_new_rtu("/dev/ttyS5", 9600, 'N', 8, 1);
    modbus_rtu_set_serial_mode(ctx, MODBUS_RTU_RS232);
    modbus_rtu_set_rts(ctx, MODBUS_RTU_RTS_UP);
    modbus_set_slave(ctx, REMOTE_ID);

    //Set custom function
    if (modbus_rtu_set_custom_rts(ctx, &set_rts_custom) == -1){
        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        modbus_free(ctx);
        close_gpio_pins();
        return -1;
    }

    if (ctx == NULL) {
        fprintf(stderr, "Unable to create the libmodbus context\n");
        close_gpio_pins();
        return -1;
    }

    if (modbus_connect(ctx) == -1) {
        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        modbus_free(ctx);
        close_gpio_pins();
        return -1;
    }

    //Write register 
    if (modbus_write_register(ctx, reg, val) == -1){
        fprintf(stderr, "Connection failed: %s\n; reg: %d; val: %d", modbus_strerror(errno),reg, val);
        modbus_free(ctx);
        close_gpio_pins();
        return -1;
    }

    modbus_free(ctx);
    close_gpio_pins();

    return 0;
}

void set_rts_custom(modbus_t *ctx, int on){
    if (on){
        if (write(fd_272, "1", 1) != 1) {
            perror("Error writing to /sys/class/gpio/gpio272/value");
            exit(1);
        }
        if (write(fd_273, "1", 1) != 1) {
            perror("Error writing to /sys/class/gpio/gpio273/value");
            exit(1);
        }
    }else{
        if (write(fd_272, "0", 1) != 1) {
            perror("Error writing to /sys/class/gpio/gpio272/value");
            exit(1);
        }
        if (write(fd_273, "0", 1) != 1) {
            perror("Error writing to /sys/class/gpio/gpio273/value");
            exit(1);
        }
    }
}

void config_gpio_pins(){
     // Export the desired pin by writing to /sys/class/gpio/export
    int fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd == -1) {
        perror("Unable to open /sys/class/gpio/export");
        exit(1);
    }

    if (write(fd, "272", 3) != 3) {
        perror("Error writing to /sys/class/gpio/export");
        exit(1);
    }

    close(fd);

    // Set the pin to be an output by writing "out" to /sys/class/gpio/gpio24/direction

    fd = open("/sys/class/gpio/gpio272/direction", O_WRONLY);
    if (fd == -1) {
        perror("Unable to open /sys/class/gpio/gpio272/direction");
        exit(1);
    }

    if (write(fd, "out", 3) != 3) {
        perror("Error writing to /sys/class/gpio/gpio272/direction");
        exit(1);
    }

    close(fd);

    fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd == -1) {
        perror("Unable to open /sys/class/gpio/export");
        exit(1);
    }

    if (write(fd, "273", 3) != 3) {
        perror("Error writing to /sys/class/gpio/export");
        exit(1);
    }

    close(fd);

    // Set the pin to be an output by writing "out" to /sys/class/gpio/gpio24/direction

    fd = open("/sys/class/gpio/gpio273/direction", O_WRONLY);
    if (fd == -1) {
        perror("Unable to open /sys/class/gpio/gpio273/direction");
        exit(1);
    }

    if (write(fd, "out", 3) != 3) {
        perror("Error writing to /sys/class/gpio/gpio273/direction");
        exit(1);
    }

    close(fd);

    fd_273 = open("/sys/class/gpio/gpio273/value", O_WRONLY);
    if (fd_273 == -1) {
        perror("Unable to open /sys/class/gpio/gpio273/value");
        exit(1);
    }

    fd_272 = open("/sys/class/gpio/gpio272/value", O_WRONLY);
    if (fd_272 == -1) {
        perror("Unable to open /sys/class/gpio/gpio272/value");
        exit(1);
    }
}

void close_gpio_pins(){
    close(fd_272);
    close(fd_273);

    int fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (fd == -1) {
        perror("Unable to open /sys/class/gpio/unexport");
        exit(1);
    }
    if (write(fd, "273", 3) != 3) {
        perror("Error writing to /sys/class/unexport");
        exit(1);
    }
    if (write(fd, "272", 3) != 3) {
        perror("Error writing to /sys/class/unexport");
        exit(1);
    }

    close(fd);
}
