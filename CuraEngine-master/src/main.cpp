/** Copyright (C) 2013 David Braam - Released under terms of the AGPLv3 License */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#include <stddef.h>
#include <vector>

#include "utils/gettime.h"
#include "utils/logoutput.h"
#include "utils/string.h"
#include "sliceDataStorage.h"

#include "modelFile/modelFile.h"
#include "settings.h"
#include "settingRegistry.h"
#include "multiVolumes.h"
#include "polygonOptimizer.h"
#include "slicer.h"
#include "layerPart.h"
#include "inset.h"
#include "skin.h"
#include "infill.h"
#include "bridge.h"
#include "support.h"
#include "pathOrderOptimizer.h"
#include "skirt.h"
#include "raft.h"
#include "comb.h"
#include "gcodeExport.h"
#include "fffProcessor.h"

void print_usage()
{
    cura::logError("usage: CuraEngine [-h] [-v] [-m 3x3matrix] [-c <config file>] [-s <settingkey>=<value>] -o <output.gcode> <model.stl>\n");
}

//Signal handler for a "floating point exception", which can also be integer division by zero errors.
void signal_FPE(int n)
{
    (void)n;
    cura::logError("Arithmetic exception.\n");
    exit(1);
}

using namespace cura;

int main(int argc, char **argv)
{


#ifndef DEBUG
    //Register the exception handling for arithmic exceptions, this prevents the "something went wrong" dialog on windows to pop up on a division by zero.
    signal(SIGFPE, signal_FPE);
#endif

    fffProcessor processor;
    std::vector<std::string> files;

   
    CommandSocket* commandSocket = NULL;
    std::string ip;
    int port = 49674;

    for(int argn = 1; argn < argc; argn++)	//usage: CuraEngine [-h] [-v] [-m 3x3matrix] [-c <config file>] [-s <settingkey>=<value>] -o <output.gcode> <model.stl>
    {
        char* str = argv[argn];
        if (str[0] == '-')
        {
            if (str[1] == '-')
            {
                if (stringcasecompare(str, "--connect") == 0)
                {
                    commandSocket = new CommandSocket(&processor);

                    std::string ip_port(argv[argn + 1]);
                    if (ip_port.find(':') != std::string::npos)
                    {
                        ip = ip_port.substr(0, ip_port.find(':'));
                        port = std::stoi(ip_port.substr(ip_port.find(':') + 1).data());    //端口号
                    }

                    argn += 1;
                }
                else if (stringcasecompare(str, "--") == 0)
                {
                    try {
                        //Catch all exceptions, this prevents the "something went wrong" dialog on windows to pop up on a thrown exception.
                        // Only ClipperLib currently throws exceptions. And only in case that it makes an internal error.
                        if (files.size() > 0)
                            processor.processFiles(files);
                        files.clear();
                    }catch(...){
                        cura::logError("Unknown exception\n");
                        exit(1);
                    }
                    break;
                }else{
                    cura::logError("Unknown option: %s\n", str);
                }
            }
			else{
                for(str++; *str; str++)
                {
                    switch(*str)
                    {
                    case 'h':
                        print_usage();
                        exit(1);
                    case 'v':
                        cura::increaseVerboseLevel();
                        break;
                    case 'j':
                        argn++;
                        if (!SettingRegistry::getInstance()->loadJSON(argv[argn]))
                        {
                            cura::logError("ERROR: Failed to load json file: %s\n", argv[argn]);
                        }
                        break;
                    case 'p':
                        cura::enableProgressLogging();
                        break;
                    case 'o':
                        argn++;
                        if (!processor.setTargetFile(argv[argn]))//设置输出文件
                        {
                            cura::logError("Failed to open %s for output.\n", argv[argn]);            
                            exit(1);
                        }
                        break;
                    case 's':
                        {
                            //Parse the given setting and store it.
                            argn++;
                            char* valuePtr = strchr(argv[argn], '=');
                            if (valuePtr)
                            {
                                *valuePtr++ = '\0';

                                processor.setSetting(argv[argn], valuePtr);//另行配置文件
                            }
                        }
                        break;
                    default:
                        cura::logError("Unknown option: %c\n", *str);
                        break;
                    }
                }
            }
        }else{
            files.push_back(argv[argn]);    //保存stl文件名
        }
    }

    if (!SettingRegistry::getInstance()->settingsLoaded())
    {
        //If no json file has been loaded, try to load the default.
        if (!SettingRegistry::getInstance()->loadJSON("fdmprinter.json"))
        {
            logError("ERROR: Failed to load json file: fdmprinter.json\n");
        }
    }
        
    if(commandSocket)
    {
        commandSocket->connect(ip, port);
    }
    else
    {
#ifndef DEBUG
        try {
#endif
            //Catch all exceptions, this prevents the "something went wrong" dialog on windows to pop up on a thrown exception.
            // Only ClipperLib currently throws exceptions. And only in case that it makes an internal error.
            if (files.size() > 0)
                processor.processFiles(files);            //处理stl文件啦！
#ifndef DEBUG
        }catch(...){
            cura::logError("Unknown exception\n");
            exit(1);
        }
#endif
        //Finalize the processor, this adds the end.gcode. And reports statistics.
        processor.finalize();
    }

    return 0;
}
