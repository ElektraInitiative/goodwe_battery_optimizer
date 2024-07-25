/*
This progamm charges a goodwe-inverter, in a way to be friendly to the battery&grid.
*/

#include <stdio.h>
#include <modbus.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <syslog.h>

int fd_272;
int fd_273;

void set_rts_custom(modbus_t*, int);
void config_gpio_pins();
void close_gpio_pins();

int main(int argc, char* argv[]){
    if (argc != 2){
        printf("usage: %s <percentage_to_reach>\n", argv[0]);
        return -1;
    }

    const int REMOTE_ID = 247;
    int percentage_to_reach = atoi(argv[1]);

    modbus_t *ctx;
    uint16_t tab_reg[64]={0};

    config_gpio_pins();
    openlog(argv[0], 0, LOG_USER);

    ctx = modbus_new_rtu("/dev/ttyS5", 9600, 'N', 8, 1);

    if (ctx == NULL) {
        fprintf(stderr, "Unable to create the libmodbus context\n");
        close_gpio_pins();
        return -1;
    }

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

    if (modbus_connect(ctx) == -1) {
        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        modbus_free(ctx);
        close_gpio_pins();
        return -1;
    }

    int battery_SOC = 0;
    //Read current SOC
    if (modbus_read_registers(ctx, 37007, 1, tab_reg) == -1){
        fprintf(stderr, "Could not read current SOC: %s\n", modbus_strerror(errno));
        modbus_free(ctx);
        close_gpio_pins();
        return -1;
    }
    battery_SOC = tab_reg[0];

    int hours_to_charge = 6;
    int capacity = 330; // 16600 / 500 in 0.1 Ah
    int current_to_set = capacity * (percentage_to_reach - battery_SOC) / (100 * hours_to_charge);

    int cut_off = 120; // in 0.1 A
    if (current_to_set > cut_off ) {
       // to be easy on the battery
       current_to_set = cut_off;
    }

    if (current_to_set < 0 ) {
       // nothing to do here, battery charged enough
       goto cleanup;
    }

    syslog(LOG_NOTICE, "will charge battery with %.1f A", ((float)current_to_set)/10.0f);
    // printf("%d\n", current_to_set);
    // goto cleanup;

    //Write register of max. current battery charge
    if (modbus_write_register(ctx, 45353, current_to_set) == -1){
        fprintf(stderr, "Could not write BattChargeCurrMax: %s\n", modbus_strerror(errno));
        modbus_free(ctx);
        close_gpio_pins();
        return -1;
    }

cleanup:
    closelog();
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
