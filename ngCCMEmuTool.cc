#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <getopt.h>
#include <vector>
#include <unistd.h>
#include <cmath> 

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
#define IGLOO2_REG_INPUTSPY       0x33
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

void readQIECard(sub_handle sh, int iRM, int iSlot);
void printData(char * buff, int pedArray[12][4]);
int interleave(char c0, char c1);

void setHV(sub_handle sh, int iRM, int chan, double voltage);
void getHVCurrent(sub_handle sh, int iRM, int chan);

int main(int argc, char* argv[])
{
    int opt;
    int option_index = 0;
    static struct option long_options[] = {
        {"freq",    required_argument, 0, 'p'},
        {"i2c",     required_argument, 0, 'i'},
        {"file",    required_argument, 0, 'f'},
        {"dev",     required_argument, 0, 'd'},
        {"test",          no_argument, 0, 't'},
        {"read",          no_argument, 0, 'r'},
        {"slot",    required_argument, 0, 'S'},
        {"voltage", required_argument, 0, 'V'},
        {"setHV",   required_argument, 0, 'v'},
        {"getHVI",  required_argument, 0, 'c'},
        {"rm" ,     required_argument, 0, 'M'}
     };

    std::vector<I2CCommand> comVec;
    int devNum = 1;
    int freq = 50000;
    int slotNum = 1;
    int rmNum = 4;
    bool runTest = false;
    bool readData = false;
    int hvChan = 0;
    int hvIChan = 0;
    double voltage = 70.0;

    while((opt = getopt_long(argc, argv, "trf:i:d:v:S:M:p:V:c:", long_options, &option_index)) != -1)
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
        else if(opt == 'p')
        {
            freq = int(atoi(optarg));
        }
        else if(opt == 't')
        {
            runTest = true;
        }
        else if(opt == 'r')
        {
            readData = true;
        }
        else if(opt == 'S')
        {
            slotNum = int(atoi(optarg)) - 1;
        }
        else if(opt == 'M')
        {
            rmNum = int(atoi(optarg));
        }
        else if(opt == 'V')
        {
            voltage = atof(optarg);
        }
        else if(opt == 'v')
        {
            hvChan = int(atoi(optarg));
        }
        else if(opt == 'c')
        {
            hvIChan = int(atoi(optarg));
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

    if(hvChan != 0)
    {
        setHV(sh, rmNum, hvChan, voltage);
    }

    if(hvIChan)
    {
        getHVCurrent(sh, rmNum, hvIChan);
    }

    if(runTest)
    {
        testQIECard(sh, rmNum, slotNum);
    }

    if(readData)
    {
        readQIECard(sh, rmNum, slotNum);
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
        mux_chan = 0x20;
        break;
    case 3:
        break;
    case 4:
        mux_chan = 0x01;
        break;
    case 5:
        mux_chan = 0x10;
        break;
    }

    bool passTests = true;
    
    char buff[128];

    int bridgeAddr = SADDR_BRIDGE_BASE + iSlot;

    //Set Emulator MUX
    buff[0] = mux_chan;
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
    if(sub_i2c_status == 0x00 && (buff[0]&0xff) == 0x00 && (buff[1]&0xff) == 0x00 && (buff[2]&0xff) == 0x00 && (buff[3]&0xff) == 0x00) printf("PASS: Igloo2 Cap Errors 2\n");
    else                                                                                                                               printf("FAILED: Igloo2 Cap Errors 2\n");

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

void readQIECard(sub_handle sh, int iRM, int iSlot)
{
    char mux_chan;
    switch(iRM)
    {
    case 1:
        mux_chan = 0x02;
        break;
    case 2:
        mux_chan = 0x20;
        break;
    case 3:
        break;
    case 4:
        mux_chan = 0x01;
        break;        
    }

    char buff[128];

    int bridgeAddr = SADDR_BRIDGE_BASE + iSlot;

    //Set Emulator MUX
    buff[0] = mux_chan;
    sub_i2c_write(sh, 0x70, 0, 0, buff, 1);
    usleep(100000);

    //Set I2X MUX to point to Igloo2
    buff[0] = BRIDGE_REG_I2CSELECT;
    buff[1] = BRIDGE_I2CMUX_IGLOO2;
    buff[2] = 0x00;
    buff[3] = 0x00;
    buff[4] = 0x00;
    sub_i2c_write(sh, bridgeAddr, 0, 0, buff, 5);

    //Activate spy bugger fill
    buff[0] = IGLOO2_REG_CNTRREG;
    buff[1] = 0x02; //fill spy buffer
    buff[2] = 0x00;
    buff[3] = 0x00;
    buff[4] = 0x00;
    sub_i2c_write(sh, SADDR_IGLOO2, 0, 0, buff, 5);

    usleep(10);

    buff[0] = IGLOO2_REG_CNTRREG;
    buff[1] = 0x00;
    buff[2] = 0x00;
    buff[3] = 0x00;
    buff[4] = 0x00;
    sub_i2c_write(sh, SADDR_IGLOO2, 0, 0, buff, 5);

    //wait for fifo to fill
    usleep(100000);
    
    //ped array
    int pedArray[12][4];
    for(int i = 0; i < 12; ++i) for(int j = 0; j < 4; ++j) pedArray[i][j] = 0;

    //read spy buffer (512 entries)
    for(int iEntry = 0; iEntry < 512; ++iEntry)
    {
        //Set spy register address 
        buff[0] = IGLOO2_REG_INPUTSPY;
        sub_i2c_write(sh, SADDR_IGLOO2, 0, 0, buff, 1);

        //read spy register
        sub_i2c_read(sh, SADDR_IGLOO2, 0, 0, buff, 25);

        printf("Event %d\n", iEntry);
        printData(buff, pedArray);
    }

    printf("Pedestal tuning summary (Meaningless for non-pedestal data)\n");
    char qieReg[2][7][8];
    for(int qieSet = 0; qieSet < 2; ++qieSet)
    {
        for(int i = 0; i < 6; ++i)
        {
            int iChan = i + qieSet*6;
            double averagePeds[4] = {pedArray[iChan][0]*4.0/512.0, pedArray[iChan][1]*4.0/512.0, pedArray[iChan][2]*4.0/512.0, pedArray[iChan][3]*4.0/512.0};
            double average = (averagePeds[0] + averagePeds[1] + averagePeds[2] + averagePeds[3])/4.0;
            printf("Average: %lf\n", average);
            int corr[4];
            for(int i = 0; i < 4; ++i)
            {
                double tmpCorr = (average - averagePeds[i])/0.42;
                if(tmpCorr >= 0.0) corr[i] = floor(tmpCorr + 0.5);
                else               corr[i] = ceil(tmpCorr - 0.5);
            }
            printf("Channel: %d\n", iChan + 1);
            printf("%10s %10s %10s %10s\n", "Cap0", "Cap1", "Cap2", "Cap3");
            printf("%10.2lf %10.2lf %10.2lf %10.2lf\n", averagePeds[0], averagePeds[1], averagePeds[2], averagePeds[3]);
            printf("%10d %10d %10d %10d\n", corr[0], corr[1], corr[2], corr[3]);
            char c[3] = {0x00, 0x00, 0x00};
        
            unsigned int ucorr[4];
            unsigned int scorr[4];
            //corr[0] = 5;
            //corr[1] = 5;
            //corr[2] = 5;
            //corr[3] = 5;
            for(int i = 0; i < 4; ++i)
            {
                ucorr[i] = abs(corr[i]);
                scorr[i] = (corr[i] > 0)?1:0;
            }
            printf("%d %d %d %d\n", ucorr[0], ucorr[1], ucorr[2], ucorr[3]);
            printf("%d %d %d %d\n", scorr[0], scorr[1], scorr[2], scorr[3]);
            c[0] = 0x26 | ((ucorr[0] & 0x3) << 6);
            c[1] = ((ucorr[0] >> 2) & 0x1) | (scorr[0] * 0x02) | ((ucorr[1] << 2) & 0x1c) |  (scorr[1] * 0x20) | ((ucorr[2] << 6) & 0xc0);
            c[2] = ((ucorr[2] >> 2) & 0x1) | (scorr[2] * 0x02) | ((ucorr[3] << 2) & 0x1c) |  (scorr[3] * 0x20);

            printf("config words: e5 1f %02x %02x %02x 00 00 80\n", 0xff & c[0], 0xff & c[1], 0xff & c[2]);

            qieReg[qieSet][(5-i) + 1][0] = 0xe5;
            qieReg[qieSet][(5-i) + 1][1] = 0x1f;
            qieReg[qieSet][(5-i) + 1][2] = c[0];
            qieReg[qieSet][(5-i) + 1][3] = c[1];
            qieReg[qieSet][(5-i) + 1][4] = c[2];
            qieReg[qieSet][(5-i) + 1][5] = 0x00;
            qieReg[qieSet][(5-i) + 1][6] = 0x00;
            qieReg[qieSet][(5-i) + 1][7] = 0x80;
        }
    }
    qieReg[0][0][7]= 0x30;
    sub_i2c_write(sh, bridgeAddr, 0, 0, ((char *)(qieReg[0])) + 7, 49);
    qieReg[1][0][7]= 0x31;
    for(int i = 0; i < 7; ++i) 
    {
        for(int j = 0; j < 8; ++j) printf("%02x ", 0xff & qieReg[1][i][j]);
        printf("\n");
    }
    printf("\n");
    usleep(10000);
    sub_i2c_write(sh, bridgeAddr, 0, 0, ((char *)(qieReg[1])) + 7, 49);
    printf("I2C STatus: %x\n", sub_i2c_status);
}

void printData(char * buff, int pedArray[12][4])
{
//000: f6 70 f6 70 f6 70 f6 70
//008: f6 70 f4 70 f4 72 f6 70
//016: f4 72 f6 70 f4 70 f4 70
//024: 34

    const char BITMASK_TDC = 0x07;  const int OFFSET_TDC = 4;
    const char BITMASK_ADC = 0x07;  const int OFFSET_ADC = 1;
    const char BITMASK_EXP = 0x01;  const int OFFSET_EXP = 0;
    const char BITMASK_CAP = 0x01;  const int OFFSET_CAP = 7;

    bool fifoEmpty = (bool)buff[24] & 0x80;
    bool fifoFull  = (bool)buff[24] & 0x40;
    int  clkctr    =  (int)buff[24] & 0x3f;
    int adc[12], tdc[12], capId[12], range[12];

    for(int i = 0; i < 12; ++i)
    {
        char adc1 = (buff[(11-i)*2 + 1] >> OFFSET_ADC) & BITMASK_ADC;
        char adc0 = (buff[(11-i)*2    ] >> OFFSET_ADC) & BITMASK_ADC;
        char tdc1 = (buff[(11-i)*2 + 1] >> OFFSET_TDC) & BITMASK_TDC;
        char tdc0 = (buff[(11-i)*2    ] >> OFFSET_TDC) & BITMASK_TDC;
        char cap1 = (buff[(11-i)*2 + 1] >> OFFSET_CAP) & BITMASK_CAP;
        char cap0 = (buff[(11-i)*2    ] >> OFFSET_CAP) & BITMASK_CAP;
        char exp1 = (buff[(11-i)*2 + 1] >> OFFSET_EXP) & BITMASK_EXP;
        char exp0 = (buff[(11-i)*2    ] >> OFFSET_EXP) & BITMASK_EXP;

        adc[i] =   interleave(adc0, adc1);
        tdc[i] =   interleave(tdc0, tdc1);
        capId[i] = interleave(cap0, cap1);
        range[i] = interleave(exp0, exp1);

        pedArray[i][0x3 & int(capId[i])] += int(0x3f & adc[i]);
    }

    printf("FIFO empty: %1d   FIFO full: %1d   clk counter: %6d\n", fifoEmpty, fifoFull, clkctr);
    printf("       ");
    for(int i = 0; i < 12; ++i) printf("  QIE %2d  ", i + 1);
    printf("\nCapID: ");
    for(int i = 0; i < 12; ++i) printf("  %6d  ", capId[i] & 0x3);
    printf("\nADC:   ");
    for(int i = 0; i < 12; ++i) printf("  %6d  ", adc[i]);
    printf("\nRANGE: ");
    for(int i = 0; i < 12; ++i) printf("  %6d  ", range[i]);
    printf("\nTDC:   ");
    for(int i = 0; i < 12; ++i) printf("  %6d  ", tdc[i]);
    printf("\n\n");
}

int interleave(char c0, char c1)
{
    int retval = 0;
    for(int i = 0; i < 8; ++i)
    {
        int bitmask = 0x01 << i;
        retval |= ((c0 & bitmask) | ((c1 & bitmask) << 1)) << i;
    }

    return retval;
}

void setHV(sub_handle sh, int iRM, int chan, double voltage)
{
    char buff[16];
    int status = 0;
    int origChan = chan;
    
    char mux_chan;
    switch(iRM)
    {
    case 1:
        mux_chan = 0x04;
        break;
    case 2:
        mux_chan = 0x01;
        break;
    case 3:
        break;
    case 4:
        //mux_chan = 0x01;
        break;        
    }

    int dacAddr = 0x00;

    //map sipm channel number to DAC channel
    if(chan <= 0); //do nothing
    else if(chan <= 32)
    {
        chan -= 1;
        dacAddr = 0x54;
    }
    else if(chan <= 48)
    {
        chan -= 32 + 1;
        dacAddr = 0x55;
    }

    //Set Emulator MUX
    buff[0] = mux_chan;
    status |= sub_i2c_write(sh, 0x70, 0, 0, buff, 1);

    //Set config regester for DAQ (0x0400)
    // 0x0c & reg0,reg1 - 0,0 sets the config register
    buff[0] = 0x0c;
    buff[1] = 0x04;
    buff[2] = 0x00;
    status |= sub_i2c_write(sh, dacAddr, 0, 0, buff, 3);

    //calculate integer voltage
    int iVoltage = int(voltage/(82.0*(68.87/70.0))*4096);
    
    //Set the voltage for the specific channel
    buff[0] = char(0xff & chan);
    buff[1] = char(0xc0 | (0x3f & (iVoltage >> 6)));
    buff[2] = char(0xfc & iVoltage << 2);
    status |= sub_i2c_write(sh, dacAddr, 0, 0, buff, 3);

    if(status)
    {
        printf("setHV failed on channel: %d\n", origChan);
    }
    else
    {
        printf("HV channel %d set to %0.2lfV\n", origChan, voltage);
    }
}

void getHVCurrent(sub_handle sh, int iRM, int chan)
{
    char buff[16];
    int status = 0;
    int origChan = chan;
    float current;
    
    char mux_chan;
    switch(iRM)
    {
    case 1:
        mux_chan = 0x04;
        break;
    case 2:
        mux_chan = 0x01;
        break;
    case 3:
        break;
    case 4:
        //mux_chan = 0x01;
        break;        
    }

    int adcAddr = 0x48;

    //map sipm channel number to ADC channel
    if(chan <= 0); //do nothing
    else
    {
        chan = (chan - 1)/8;
    }

    //Set Emulator MUX
    buff[0] = mux_chan;
    status |= sub_i2c_write(sh, 0x70, 0, 0, buff, 1);

    //Set config regester for ADC includes channel #
    //0x80 - single ended
    //0x0c - internal ref on, AD conversion on
    buff[0] = 0x80 | ((0x7 & chan) << 4) | 0x0c;
    status |= sub_i2c_write(sh, adcAddr, 0, 0, buff, 1);

    //Get the current for the specific channel
    status |= sub_i2c_read(sh, adcAddr, 0, 0, buff, 2);

    int iCurrent = ((0x0f & buff[0]) << 8) | (0xff & buff[1]);
    current = 2.5*float(iCurrent)/4096;

    if(status)
    {
        printf("getHV current failed on channel: %d\n", origChan);
    }
    else
    {
        printf("HV channel %2d current is %0.2f mA\n", origChan, current);
    }
}
