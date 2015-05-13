#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <getopt.h>
#include <vector>
#include <unistd.h>

#include "SUB-20-snap-130926/lib/libsub.h"

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

int main(int argc, char* argv[])
{
    int opt;
    int option_index = 0;
    static struct option long_options[] = {
        {"freq",  required_argument, 0, 'f'},
        {"i2c",   required_argument, 0, 'i'},
        {"file",  required_argument, 0, 'f'},
        {"dev",   required_argument, 0, 'd'},
     };

    std::vector<I2CCommand> comVec;
    int devNum = 1;

    while((opt = getopt_long(argc, argv, "f:i:d:", long_options, &option_index)) != -1)
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
    int freq = 50000;
    sub_i2c_freq(sh, &freq);

    //for(const I2CCommand& com : comVec)
    for(std::vector<I2CCommand>::const_iterator icom = comVec.begin(); icom != comVec.end(); ++icom)
    {
        const I2CCommand& com = *icom;
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

        comVec.push_back(I2CCommand(command, 0, delay));
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

        comVec.push_back(I2CCommand(command, sAddr, nBytes, buff));
        if(buff) delete [] buff;
    }
    else if(command == 'r')
    {
        int sAddr;
        int len;
        if(sscanf(tbuf, "%i %[^\n]", &sAddr, tbuf) != 2) return false;
        if(sscanf(tbuf, "%i", &len) != 1) return false;

        comVec.push_back(I2CCommand(command, sAddr, len));
    }
    else if(command == 'e')
    {
        char * buff = new char[strlen(rawCommand) + 1];
        if(sscanf(tbuf, "%s", buff) != 1) return false;
        comVec.push_back(I2CCommand(command, 0, strlen(buff) + 1, buff));
        if(buff) delete [] buff;
    }
    else if(command == 'c')
    {
        comVec.push_back(I2CCommand(command, 0, 1));
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
