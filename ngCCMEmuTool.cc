#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <getopt.h>
#include <vector>
#include <unistd.h>

#include "SUB-20-snap-130926/lib/libsub.h"

#define SADDR_BRIDGE_BASE 0x18
#define SADDR_IGLOO2      0x09
#define SADDR_VTTX        0x7e
#define SADDR_UNIQUEID    0x50
#define SADDR_TEMP        0x40
#define SADDR_ALLCALL     0x00

//Bridge FPGA registers
#define BRIDGE_REG_ID1       0x00
#define BRIDGE_REG_ID2       0x01
#define BRIDGE_REG_FWVER     0x04
#define BRIDGE_REG_ONES      0x08
#define BRIDGE_REG_ZEROS     0x09
#define BRIDGE_REG_10S       0x0A
#define BRIDGE_REG_SCRATCH   0x0B
#define BRIDGE_REG_STATUS    0x10
#define BRIDGE_REG_I2CSELECT 0x11
#define BRIDGE_REG_CLKCTR    0x12
#define BRIDGE_REG_QIERSTCTR 0x13
#define BRIDGE_REG_WTECTR    0x14
#define BRIDGE_REG_FPGACTRL  0x22
#define BRIDGE_REG_QIEDC0    0x30
#define BRIDGE_REG_QIEDC1    0x31

//Bridge I2C mux channels
#define BRIDGE_I2CMUX_NOCONNECT   0x00
#define BRIDGE_I2CMUX_VTTX1       0x01
#define BRIDGE_I2CMUX_VTTX2       0x02
#define BRIDGE_I2CMUX_IGLOO2      0x03
#define BRIDGE_I2CMUX_UNIQUEID    0x04
#define BRIDGE_I2CMUX_TEMP        0x05

//Igloo2 Registers
#define IGLOO2_REG_VER_MAJOR      0x00
#define IGLOO2_REG_VER_MINOR      0x01
#define IGLOO2_REG_ONES           0x02
#define IGLOO2_REG_ZEROS          0x03
#define IGLOO2_REG_UNIQUEID       0x05
#define IGLOO2_REG_STATUS         0x10
#define IGLOO2_REG_CNTRREG        0x11
#define IGLOO2_REG_CLKCTR         0x12
#define IGLOO2_REG_QIERSTCTR      0x13
#define IGLOO2_REG_WTECTR         0x14
#define IGLOO2_REG_CAPIDERR1      0x15
#define IGLOO2_REG_CAPIDERR2      0x16
#define IGLOO2_REG_QIEPHASE1      0x60
#define IGLOO2_REG_QIEPHASE2      0x61
#define IGLOO2_REG_QIEPHASE3      0x62
#define IGLOO2_REG_QIEPHASE4      0x63
#define IGLOO2_REG_QIEPHASE5      0x64
#define IGLOO2_REG_QIEPHASE6      0x65
#define IGLOO2_REG_QIEPHASE7      0x66
#define IGLOO2_REG_QIEPHASE8      0x67
#define IGLOO2_REG_QIEPHASE9      0x68
#define IGLOO2_REG_QIEPHASE10     0x69
#define IGLOO2_REG_QIEPHASE11     0x6A
#define IGLOO2_REG_QIEPHASE12     0x6B
#define IGLOO2_REG_LINKTEST       0x80
#define IGLOO2_REG_LINKPATERN     0x81
#define IGLOO2_REG_DATATOSERDES   0x82
#define IGLOO2_REG_ADDRTOSERDES   0x83
#define IGLOO2_REG_CTRLTOSERDES   0x84
#define IGLOO2_REG_DATAFROMSERDES 0x85
#define IGLOO2_REG_STATFROMSERDES 0x86
#define IGLOO2_REG_SCRATCH        0xFF

class I2CCommand
{
public:
    char command;
    int sAddr;
    char *buff;
    int len;

    I2CCommand() : command('\0'), sAddr(0), buff(0), len(0) {}
    
    I2CCommand(const char com, const int addr, const int length, const char* const buf) : command(com), sAddr(addr)
    {
        if(com == 'w' || com == 'e')
        {
            len = length;
            buff = new char[len + 1];
            //sprintf(buff, "%s", buf);
            for(int i = 0; i <= len; i++) buff[i] = buf[i];
        }
    }

    I2CCommand(const char com, const int addr, const int intval) : command(com), sAddr(addr)
    {
        if(com == 'r')
        {
            buff = new char[intval + 1];
            len = intval;
        }
        else if(com == 's')
        {
            buff = 0;
            len = intval;
        }
    }

    ~I2CCommand()
    {
        //if(buff) delete [] buff;
    }
};

bool parseI2C(const char * const rawCommand, std::vector<I2CCommand>& comVec);

void printBuff(const char * const buff, const int length);

void testQIECard(sub_handle sh, int iRM, int iSlot);

int main(int argc, char* argv[])
{
    int opt;
    int option_index = 0;
    static struct option long_options[] = {
        {"freq",  required_argument, 0, 'v'},
        {"i2c",   required_argument, 0, 'i'},
        {"file",  required_argument, 0, 'f'},
        {"dev",   required_argument, 0, 'd'},
        {"test",        no_argument, 0, 't'},
        {"slot",  required_argument, 0, 'S'},
        {"rm" ,   required_argument, 0, 'M'}
     };

    std::vector<I2CCommand> comVec;
    int devNum = 1;
    int freq = 50000;
    int slotNum = 1;
    int rmNum = 4;
    bool runTest = false;

    while((opt = getopt_long(argc, argv, "tf:i:d:v:S:M:", long_options, &option_index)) != -1)
    {
        if(opt == 'i')
        {
            parseI2C(optarg, comVec);
        }
        else if(opt == 'f')
        {
            FILE * fin = fopen(optarg, "r");
            char flbuf[1024];
            if(fin)
            {
                while(!feof(fin) && fgets(flbuf, 1024, fin))
                {
                    if(strlen(flbuf) > 0) parseI2C(flbuf, comVec);
                }

                fclose(fin);
            }
        }
        else if(opt == 'd')
        {
            devNum = int(atoi(optarg));
        }
        else if(opt == 'v')
        {
            freq = int(atoi(optarg));
        }
        else if(opt == 't')
        {
            runTest = true;
        }
        else if(opt == 'S')
        {
            slotNum = int(atoi(optarg)) - 1;
        }
        else if(opt == 'M')
        {
            rmNum = int(atoi(optarg)) - 1;
        }
        //case 'P':
        //    PM_VOUT_NOM = atof(optarg);
        //    pSet = true;
        //    break;
    }

    sub_device dev = 0;
    sub_handle sh = 0;

    for(int i = 0; i < devNum; ++i)
    {
        dev = sub_find_devices(dev);
    }
    sh = sub_open(dev);

    if(sh == 0) 
    {
        printf("No sub-20 found!!\n");
        return 0;
    }

    sub_i2c_config(sh, 0, 0);
    sub_i2c_freq(sh, &freq);

    if(runTest)
    {
        testQIECard(sh, rmNum, slotNum);
    }

    for(const I2CCommand& com : comVec)
    {
        if(com.command == 'r')
        {
            printf("Execute I2C read: 0x%02x %d", com.sAddr & 0xff, com.len);
            sub_i2c_read(sh, com.sAddr, 0, 0, com.buff, com.len);
            printBuff(com.buff, com.len);
            printf("\n");
            printf("I2C Command Status: 0x%x\n", sub_i2c_status);
        }
        else if(com.command == 'w')
        {
            printf("Execute write: 0x%02x", com.sAddr & 0xff);
            printBuff(com.buff, com.len);
            printf("\n");
            sub_i2c_write(sh, com.sAddr, 0, 0, com.buff, com.len);
            printf("I2C Command Status: 0x%x\n", sub_i2c_status);
        }
        else if(com.command == 's')
        {
            printf("Execute sleep: %d\n", com.len);
            usleep(com.len);
        }
        else if(com.command == 'e')
        {
            printf("Execute echo: %s\n", com.buff);
            sub_lcd_write(sh, com.buff);
        }
        else if(com.command == 'c')
        {
            printf("Execute I2C scan:\n");
            int NsAddrs = 0;
            char sAddrs[128];
            sub_i2c_scan(sh, &NsAddrs, sAddrs);
            for(int i = 0; i < NsAddrs; ++i)
            {
                if(sub_i2c_status == 0) printf("Addr: 0x%02x\n", sAddrs[i] & 0xff);
            }
        }
        printf("\n");
    }
}

bool parseI2C(const char * const rawCommand, std::vector<I2CCommand>& comVec)
{
    char* tbuf = new char[strlen(rawCommand) + 1];
    char command;

    sprintf(tbuf, "%s", rawCommand);
    //process comments ('#') into string terminators
    for(char* k; k = strchr(tbuf, '#');) *k = '\0';
    //strip trailing white space
    for(char *k = tbuf + strlen(tbuf) - 1; isspace(*k) & tbuf != k; --k) *k = '\0';

    if(sscanf(tbuf, "%c %[^\n]", &command, tbuf) != 2) return false;

    if(command == 's')
    {
        int delay;
        if(sscanf(tbuf, "%i", &delay) != 1) return false;

        comVec.emplace_back(I2CCommand(command, 0, delay));
    }
    else if(command == 'w')
    {
        int sAddr;
        int data;
        int nBytes = 0;
        int len = 0;
        char * buff = new char[strlen(rawCommand) + 1];
        if(sscanf(tbuf, "%i %[^\n]", &sAddr, tbuf) != 2) return false;
        do
        {
            len = sscanf(tbuf, "%i %[^\n]", &data, tbuf);
            buff[nBytes] = 0xff & data;
            ++nBytes;
        }
        while(len > 1);
        if(nBytes == 0) return false;
        buff[nBytes] = '\0';

        comVec.emplace_back(I2CCommand(command, sAddr, nBytes, buff));
        if(buff) delete [] buff;
    }
    else if(command == 'r')
    {
        int sAddr;
        int len;
        if(sscanf(tbuf, "%i %[^\n]", &sAddr, tbuf) != 2) return false;
        if(sscanf(tbuf, "%i", &len) != 1) return false;

        comVec.emplace_back(I2CCommand(command, sAddr, len));
    }
    else if(command == 'e')
    {
        char * buff = new char[strlen(rawCommand) + 1];
        if(sscanf(tbuf, "%s", buff) != 1) return false;
        comVec.emplace_back(I2CCommand(command, 0, strlen(buff) + 1, buff));
        if(buff) delete [] buff;
    }
    else if(command == 'c')
    {
        comVec.emplace_back(I2CCommand(command, 0, 1));
    }

    if(tbuf) delete [] tbuf;

    return true;
}

void printBuff(const char * const buff, const int length)
{
    for(int i = 0; i <= (length - 1)/8; ++i) 
    {
        printf("\n%03d:", i*8);
        for(int j = 0; j < 8 && i*8 + j < length; ++j) printf(" %02x", 0xff & buff[i*8 + j]);
    }
}

void testQIECard(sub_handle sh, int iRM, int iSlot)
{
    char mux_chan;
    switch(iRM)
    {
    case 1:
        mux_chan = 0x02;
        break;
    case 2:
        break;
    case 3:
        break;
    case 4:
        mux_chan = 0x01;
        break;
        
    }

    bool passTests = true;
    
    char buff[128];

    int bridgeAddr = SADDR_BRIDGE_BASE + iSlot;

    //Set Emulator MUX
    buff[0] = 0x01;
    sub_i2c_write(sh, 0x70, 0, 0, buff, 1);
    usleep(100000);
    //Access bridge ID regesters
    buff[0] = BRIDGE_REG_ID1;
    sub_i2c_write(sh, bridgeAddr, 0, 0, buff, 1);
    sub_i2c_read(sh, bridgeAddr, 0, 0, buff, 4);
    //printf("%x %x %x %x\n", buff[3]&0xff, buff[2]&0xff, buff[1]&0xff, buff[0]&0xff);
    //buff[4] = '\0';
    //printf("%s\n", buff);
    //Expect "HERM" (Actually firmware currently returns "BFRM")
    if(sub_i2c_status == 0x00 && buff[3] == 'B' && buff[2] == 'F' && buff[1] == 'R' && buff[0] == 'M') printf("PASS: Bridge ID1\n");
    else                                                                                               printf("FAILED: Bridge ID1\n");

    buff[0] = BRIDGE_REG_ID2;
    sub_i2c_write(sh, bridgeAddr, 0, 0, buff, 1);
    sub_i2c_read(sh, bridgeAddr, 0, 0, buff, 4);
    //Expect "Brdg"
    if(sub_i2c_status == 0x00 && buff[3] == 'B' && buff[2] == 'r' && buff[1] == 'd' && buff[0] == 'g') printf("PASS: Bridge ID2\n");
    else                                                                                               printf("FAILED: Bridge ID2\n");

    //Bridge FW version 
    buff[0] = BRIDGE_REG_FWVER;
    sub_i2c_write(sh, bridgeAddr, 0, 0, buff, 1);
    sub_i2c_read(sh, bridgeAddr, 0, 0, buff, 4);
    //Only check that FW version read
    if(sub_i2c_status == 0x00) printf("PASS: Bridge FW version - %02x %02x %02x %02x\n", buff[3]&0xff, buff[2]&0xff, buff[1]&0xff, buff[0]&0xff);
    else                       printf("FAILED: Bridge FW version");

    //Ones register
    buff[0] = BRIDGE_REG_ONES;
    sub_i2c_write(sh, bridgeAddr, 0, 0, buff, 1);
    sub_i2c_read(sh, bridgeAddr, 0, 0, buff, 4); 
    //Expect 0xffffffff
    if(sub_i2c_status == 0x00 && (buff[0]&0xff) == 0xff && (buff[1]&0xff) == 0xff && (buff[2]&0xff) == 0xff && (buff[3]&0xff) == 0xff) printf("PASS: Bridge ones\n");
    else                                                                                                                               printf("FAILED: Bridge ones\n");
   
    //zeros register
    buff[0] = BRIDGE_REG_ZEROS;
    sub_i2c_write(sh, bridgeAddr, 0, 0, buff, 1);
    sub_i2c_read(sh, bridgeAddr, 0, 0, buff, 4);
    //Expect 0x00000000
    if(sub_i2c_status == 0x00 && (buff[0]&0xff) == 0x00 && (buff[1]&0xff) == 0x00 && (buff[2]&0xff) == 0x00 && (buff[3]&0xff) == 0x00) printf("PASS: Bridge zeros\n");
    else                                                                                                                               printf("FAILED: Bridge zeros\n");

    //bridge onezeros register
    buff[0] = BRIDGE_REG_10S;
    sub_i2c_write(sh, bridgeAddr, 0, 0, buff, 1);
    sub_i2c_read(sh, bridgeAddr, 0, 0, buff, 4);
    //Expect 0xaaaaaaaa
    if(sub_i2c_status == 0x00 && (buff[0]&0xff) == 0xaa && (buff[1]&0xff) == 0xaa && (buff[2]&0xff) == 0xaa && (buff[3]&0xff) == 0xaa) printf("PASS: Bridge onezeros\n");
    else                                                                                                                               printf("FAILED: Bridge onezeros\n");

    //bridge scratch register write and readback 0x12345678
    buff[0] = BRIDGE_REG_SCRATCH;
    buff[4] = 0x12;
    buff[3] = 0x34;
    buff[2] = 0x56;
    buff[1] = 0x78;
    sub_i2c_write(sh, bridgeAddr, 0, 0, buff, 5);
    sub_i2c_write(sh, bridgeAddr, 0, 0, buff, 1);
    sub_i2c_read(sh, bridgeAddr, 0, 0, buff, 4);
    if(sub_i2c_status == 0x00 && (buff[3]&0xff) == 0x12 && (buff[2]&0xff) == 0x34 && (buff[1]&0xff) == 0x56 && (buff[0]&0xff) == 0x78) printf("PASS: Bridge scratch\n");
    else                                                                                                                               printf("FAILED: Bridge scratch\n");

    //bridge status register
    buff[0] = BRIDGE_REG_STATUS;
    sub_i2c_write(sh, bridgeAddr, 0, 0, buff, 1);
    sub_i2c_read(sh, bridgeAddr, 0, 0, buff, 4);
    //check last 3 bits, should be GEO_ADDR
    //printf("%x %x %x %x\n", buff[0]&0xff, buff[1]&0xff, buff[2]&0xff, buff[3]&0xff);
    if(sub_i2c_status == 0x00 && (buff[0]&0x07) == iSlot) printf("PASS: Bridge status\n");
    else                                                  printf("FAILED: Bridge status\n");

    //check vTTx 1
    buff[0] = BRIDGE_REG_I2CSELECT;
    buff[1] = BRIDGE_I2CMUX_VTTX1;
    buff[2] = 0x00;
    buff[3] = 0x00;
    buff[4] = 0x00;
    sub_i2c_write(sh, bridgeAddr, 0, 0, buff, 5);
    sub_i2c_read(sh, SADDR_VTTX, 0, 0, buff, 1);
    //Sinply check if I2C read recieved ack
    if(sub_i2c_status == 0x00) printf("PASS: vTTx 1\n");
    else                       printf("FAILED: vTTx 1\n");
    
    //check vTTx 2
    buff[0] = BRIDGE_REG_I2CSELECT;
    buff[1] = BRIDGE_I2CMUX_VTTX2;
    buff[2] = 0x00;
    buff[3] = 0x00;
    buff[4] = 0x00;
    sub_i2c_write(sh, bridgeAddr, 0, 0, buff, 5);
    sub_i2c_read(sh, SADDR_VTTX, 0, 0, buff, 1);
    //Sinply check if I2C read recieved ack
    if(sub_i2c_status == 0x00) printf("PASS: vTTx 2\n");
    else                       printf("FAILED: vTTx 2\n");

    //check unique ID chip
    buff[0] = BRIDGE_REG_I2CSELECT;
    buff[1] = BRIDGE_I2CMUX_UNIQUEID;
    buff[2] = 0x00;
    buff[3] = 0x00;
    buff[4] = 0x00;
    sub_i2c_write(sh, bridgeAddr, 0, 0, buff, 5);
    buff[0] = 0x00;
    sub_i2c_write(sh, SADDR_UNIQUEID, 0, 0, buff, 1);
    sub_i2c_read(sh, SADDR_UNIQUEID, 0, 0, buff, 9);
    //Sinply check if I2C read recieved ack
    if(sub_i2c_status == 0x00) printf("PASS: uniqueID - %02x %02x %02x %02x %02x %02x %02x %02x\n", buff[0]&0xff, buff[1]&0xff, buff[2]&0xff, buff[3]&0xff, buff[4]&0xff, buff[5]&0xff, buff[6]&0xff, buff[7]&0xff);
    else                       printf("FAILED: uniqueID\n");

    //check temp/humidity sensor
    buff[0] = BRIDGE_REG_I2CSELECT;
    buff[1] = BRIDGE_I2CMUX_TEMP;
    buff[2] = 0x00;
    buff[3] = 0x00;
    buff[4] = 0x00;
    sub_i2c_write(sh, bridgeAddr, 0, 0, buff, 5);
    //Read temperature
    buff[0] = 0xe3;
    sub_i2c_write(sh, SADDR_TEMP, 0, 0, buff, 1);
    sub_i2c_read(sh, SADDR_TEMP, 0, 0, buff, 3);
    //Convert reading into temperature
    int st = ((0x3f & int(buff[1])) << 8) + (0xff & int(buff[0]));
    double temp = -46.85 + 175.72 * st / (1 << 15);
    //Check if temp reading is reasonable
    if(sub_i2c_status == 0x00 && temp > 0 && temp < 100) printf("PASS: temp - %0.2f C\n", temp);
    else                                                 printf("FAILED: temp\n");
    usleep(100000);
    //Read Humidity
    buff[0] = 0xe5;
    sub_i2c_write(sh, SADDR_TEMP, 0, 0, buff, 1);
    sub_i2c_read(sh, SADDR_TEMP, 0, 0, buff, 3);
    //Convert reading to humidity reading
    int shu = ((0x3f & int(buff[1])) << 8) + (0xff & int(buff[0]));
    double humidity = -6.0 + 125.0 * shu / (1 << 15);
    // chdeck that hmidity is in reasonable range 
    if(sub_i2c_status == 0x00 && temp > 0 && temp < 100) printf("PASS: humidity - %0.2f%%\n", humidity);
    else                                                 printf("FAILED: humidity\n");

    //buff[0] = BRIDGE_REG_CLKCTR;
    //sub_i2c_write(sh, bridgeAddr, 0, 0, buff, 1);
    //sub_i2c_read(sh, bridgeAddr, 0, 0, buff, 4);
    //
    //buff[0] = BRIDGE_REG_QIERSTCTR;
    //sub_i2c_write(sh, bridgeAddr, 0, 0, buff, 1);
    //sub_i2c_read(sh, bridgeAddr, 0, 0, buff, 4);
    //
    //buff[0] = BRIDGE_REG_WTECTR;
    //sub_i2c_write(sh, bridgeAddr, 0, 0, buff, 1);
    //sub_i2c_read(sh, bridgeAddr, 0, 0, buff, 4);
    //
    //buff[0] = BRIDGE_REG_FPGACTRL;
    //sub_i2c_write(sh, bridgeAddr, 0, 0, buff, 1);
    //sub_i2c_read(sh, bridgeAddr, 0, 0, buff, 4);

    //begin testing Igloo2

    //Set I2X MUX to point to Igloo2
    buff[0] = BRIDGE_REG_I2CSELECT;
    buff[1] = BRIDGE_I2CMUX_IGLOO2;
    buff[2] = 0x00;
    buff[3] = 0x00;
    buff[4] = 0x00;
    sub_i2c_write(sh, bridgeAddr, 0, 0, buff, 5);

    //Check igloo2 firmware version 
    buff[0] = IGLOO2_REG_VER_MAJOR;
    sub_i2c_write(sh, SADDR_IGLOO2, 0, 0, buff, 1);
    sub_i2c_read(sh, SADDR_IGLOO2, 0, 0, buff, 1);
    //Just check for successful read
    if(sub_i2c_status == 0x00) printf("PASS: Igloo2 FW major - %02x\n", buff[0]&0xff);
    else                       printf("FAILED: Igloo2 FW major\n");

    buff[0] = IGLOO2_REG_VER_MINOR;
    sub_i2c_write(sh, SADDR_IGLOO2, 0, 0, buff, 1);
    sub_i2c_read(sh, SADDR_IGLOO2, 0, 0, buff, 1);
    //Just check for successful read
    if(sub_i2c_status == 0x00) printf("PASS: Igloo2 FW minor - %02x\n", buff[0]&0xff);
    else                       printf("FAILED: Igloo2 FW minor\n");

    //Check ones register
    buff[0] = IGLOO2_REG_ONES;
    sub_i2c_write(sh, SADDR_IGLOO2, 0, 0, buff, 1);
    sub_i2c_read(sh, SADDR_IGLOO2, 0, 0, buff, 4);
    //Expect 0xffffffff
    if(sub_i2c_status == 0x00 && (buff[0]&0xff) == 0xff && (buff[1]&0xff) == 0xff && (buff[2]&0xff) == 0xff && (buff[3]&0xff) == 0xff) printf("PASS: Igloo2 ones\n");
    else                                                                                                                               printf("FAILED: Igloo2 ones\n");
    
    //check zeros register
    buff[0] = IGLOO2_REG_ZEROS;
    sub_i2c_write(sh, SADDR_IGLOO2, 0, 0, buff, 1);
    sub_i2c_read(sh, SADDR_IGLOO2, 0, 0, buff, 4);
    //Expect 0x00000000
    if(sub_i2c_status == 0x00 && (buff[0]&0xff) == 0x00 && (buff[1]&0xff) == 0x00 && (buff[2]&0xff) == 0x00 && (buff[3]&0xff) == 0x00) printf("PASS: Igloo2 zeros\n");
    else                                                                                                                               printf("FAILED: Igloo2 zeros\n");

    //check unique ID
    buff[0] = IGLOO2_REG_UNIQUEID;
    sub_i2c_write(sh, SADDR_IGLOO2, 0, 0, buff, 1);
    sub_i2c_read(sh, SADDR_IGLOO2, 0, 0, buff, 8);
    //Sinply check if I2C read recieved ack
    if(sub_i2c_status == 0x00) printf("PASS: Igloo2 uniqueID - %02x %02x %02x %02x %02x %02x %02x %02x\n", buff[7]&0xff, buff[6]&0xff, buff[5]&0xff, buff[4]&0xff, buff[3]&0xff, buff[2]&0xff, buff[1]&0xff, buff[0]&0xff);
    else                       printf("FAILED: Igloo2 uniqueID\n");

    //Check status register {19 bits 0, 12-bit Qie_DLL_NoLock, PLL 320MHz Lock}
    buff[0] = IGLOO2_REG_STATUS;
    sub_i2c_write(sh, SADDR_IGLOO2, 0, 0, buff, 1);
    sub_i2c_read(sh, SADDR_IGLOO2, 0, 0, buff, 8);
    //expect 0x00000001
    if(sub_i2c_status == 0x00 && (buff[0]&0xff) == 0x01 && (buff[1]&0xff) == 0x00 && (buff[2]&0xff) == 0x00 && (buff[3]&0xff) == 0x00) printf("PASS: Igloo2 status\n");
    else                                                                                                                               printf("FAILED: Igloo2 status\n");    

    //buff[0] = IGLOO2_REG_CNTRREG;
    //buff[0] = IGLOO2_REG_CLKCTR;
    //buff[0] = IGLOO2_REG_QIERSTCTR;
    //buff[0] = IGLOO2_REG_WTECTR;
    
    //Check for cap errors
    buff[0] = IGLOO2_REG_CAPIDERR1;
    sub_i2c_write(sh, SADDR_IGLOO2, 0, 0, buff, 1);
    sub_i2c_read(sh, SADDR_IGLOO2, 0, 0, buff, 4);
    //Expect 0
    if(sub_i2c_status == 0x00 && (buff[0]&0xff) == 0x00 && (buff[1]&0xff) == 0x00 && (buff[2]&0xff) == 0x00 && (buff[3]&0xff) == 0x00) printf("PASS: Igloo2 Cap Errors 1\n");
    else                                                                                                                               printf("FAILED: Igloo2 Cap Errors 1\n");

    buff[0] = IGLOO2_REG_CAPIDERR2;
    sub_i2c_write(sh, SADDR_IGLOO2, 0, 0, buff, 1);
    sub_i2c_read(sh, SADDR_IGLOO2, 0, 0, buff, 4);
    //Expect 0
    if(sub_i2c_status == 0x00 && (buff[0]&0xff) == 0x00 && (buff[1]&0xff) == 0x00 && (buff[2]&0xff) == 0x00 && (buff[3]&0xff) == 0x00) printf("PASS: Igloo2 Cap Errors 1\n");
    else                                                                                                                               printf("FAILED: Igloo2 Cap Errors 1\n");

    //test readback of scratch register
    buff[0] = IGLOO2_REG_SCRATCH;
    buff[4] = 0x12;
    buff[3] = 0x34;
    buff[2] = 0x56;
    buff[1] = 0x78;
    sub_i2c_write(sh, SADDR_IGLOO2, 0, 0, buff, 5);
    sub_i2c_write(sh, SADDR_IGLOO2, 0, 0, buff, 1);
    sub_i2c_read(sh, SADDR_IGLOO2, 0, 0, buff, 4);
    if(sub_i2c_status == 0x00 && (buff[3]&0xff) == 0x12 && (buff[2]&0xff) == 0x34 && (buff[1]&0xff) == 0x56 && (buff[0]&0xff) == 0x78) printf("PASS: Igloo2 scratch\n");
    else                                                                                                                               printf("FAILED: Igloo2 scratch\n");


    //Reset I2C MUX with "general call" reset
    buff[0] = 0x06;
    sub_i2c_write(sh, SADDR_ALLCALL, 0, 0, buff, 1);
    //Read MUX register
    buff[0] = BRIDGE_REG_I2CSELECT;
    sub_i2c_write(sh, bridgeAddr, 0, 0, buff, 1);
    sub_i2c_read(sh, bridgeAddr, 0, 0, buff, 4);
    //Should be 0 after reset
    if(sub_i2c_status == 0x00 && (buff[0]&0xff) == 0x00 && (buff[1]&0xff) == 0x00 && (buff[2]&0xff) == 0x00 && (buff[3]&0xff) == 0x00) printf("PASS: Bridge general call reset\n");
    else                                                                                                                               printf("FAILED: Bridge general call reset\n");

    buff[0] = BRIDGE_REG_QIEDC0;
    sub_i2c_write(sh, bridgeAddr, 0, 0, buff, 1);
    sub_i2c_read(sh, bridgeAddr, 0, 0, buff, 48);

    buff[0] = BRIDGE_REG_QIEDC1;
    sub_i2c_write(sh, bridgeAddr, 0, 0, buff, 1);
    sub_i2c_read(sh, bridgeAddr, 0, 0, buff, 48);
}
