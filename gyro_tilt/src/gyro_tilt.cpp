#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <iostream>

#define BMI160_CMD_REG 0x7E
#define BMI160_CHIP_ID_REG 0x00
#define BMI160_GYRO_REG 0x0C
#define BMI160_STATUS_REG 0x1B
#define BMI160_READ_BIT 0x80

#define BMI160_GYRO_SENS 16.4

#define SPI_DEVICE "/dev/spidev0.0"
#define SPI_SPEED 1000000 // 1 MHz
#define SPI_BITS_PER_WORD 8

#define MAXBUFSIZE 32

int8_t fd = -1;
uint8_t buffer[MAXBUFSIZE] = {0};
struct spi_ioc_transfer tx[1] = {0};

// Initialize SPI bus
void initSPI(void) {
  fd = open(SPI_DEVICE, O_RDWR);
  if (fd < 0) {
    perror("open()");
    exit(EXIT_FAILURE);
  }
  uint8_t mode = SPI_MODE_0;
  uint8_t bits = SPI_BITS_PER_WORD;
  uint32_t speed = SPI_SPEED;
  ioctl(fd, SPI_IOC_RD_MODE, &mode);
  ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
  ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
  ioctl(fd, SPI_IOC_WR_MODE, &mode);
  ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
  ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
}

// Read from register
int readReg(uint8_t reg, uint8_t nbytes) {
  if (nbytes > MAXBUFSIZE) {
    return -1;
  }

  uint8_t buf[nbytes+1] = {0};

  buf[0] = reg | BMI160_READ_BIT;

  tx[0].tx_buf = (__u64)buf;
  tx[0].rx_buf = (__u64)buf;
  tx[0].len = (__u32)nbytes + 1;
  tx[0].cs_change = 0;
  tx[0].delay_usecs = 0;
  tx[0].speed_hz = SPI_SPEED;
  tx[0].bits_per_word = SPI_BITS_PER_WORD;

  if (ioctl(fd, SPI_IOC_MESSAGE(1), &tx) < 0) {
    perror("ioctl()");
    close(fd);
    exit(EXIT_FAILURE);
  }

  for (uint8_t i = 0; i < nbytes; ++i) {
    buffer[i] = buf[i+1];
  }

  return 0;
}

// Write to register
int writeReg(uint8_t reg, uint8_t data) {

  uint8_t buf[2] = {0};

  buf[0] = reg;
  buf[1] = data;

  tx[0].tx_buf = (__u64)buf;
  tx[0].rx_buf = (__u64)buf;
  tx[0].len = (__u32)sizeof(buf);
  tx[0].cs_change = 0;
  tx[0].delay_usecs = 0;
  tx[0].speed_hz = SPI_SPEED;
  tx[0].bits_per_word = SPI_BITS_PER_WORD;

  if (ioctl(fd, SPI_IOC_MESSAGE(1), &tx) < 0) {
    perror("ioctl()");
    close(fd);
    exit(EXIT_FAILURE);
  }

  return 0;
}

// Turn on gyro
void startGyro(void) {
  memset(&tx, 0, sizeof(tx));
  memset(&buffer, 0, sizeof(buffer));

  if (writeReg(BMI160_CMD_REG, 0x15) < 0) {
    std::cerr << "writeReg() failed\n";
    close(fd);
    exit(EXIT_FAILURE);
  }
  // Sleep to allow gyro to start up
  usleep(100000);
}

// Get gyro status
bool isGyroDataAvailable(void) {
  memset(&tx, 0, sizeof(tx));
  memset(&buffer, 0, sizeof(buffer));

  if (readReg(BMI160_STATUS_REG, 1) < 0) {
    std::cerr << "readReg() failed\n";
    close(fd);
    exit(EXIT_FAILURE);
  }
  return (buffer[0] & 0x40) == 0x40;
}

// Read gyro data
void readGyro(void) {
  memset(&tx, 0, sizeof(tx));
  memset(&buffer, 0, sizeof(buffer));

  if (readReg(BMI160_GYRO_REG, 6) < 0) {
    std::cerr << "readReg() failed\n";
    close(fd);
    exit(EXIT_FAILURE);
  }
}

// Main
int main(int argc, char* argv[]) {
  initSPI();

  startGyro();

  while ( isGyroDataAvailable() ) {
    readGyro();

    double gx = (double)(((int8_t)buffer[1] << 8 | (int8_t)buffer[0]) / BMI160_GYRO_SENS);
    double gy = (double)(((int8_t)buffer[3] << 8 | (int8_t)buffer[2]) / BMI160_GYRO_SENS);
    double gz = (double)(((int8_t)buffer[5] << 8 | (int8_t)buffer[4]) / BMI160_GYRO_SENS);

    std::cout << "gx: " << gx << "        \t";
    std::cout << "gy: " << gy << "        \t";
    std::cout << "gz: " << gz << "\n";

    usleep(100000);
  }

  close(fd);

  return EXIT_SUCCESS;
}
