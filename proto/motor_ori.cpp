/*******************************************************************************
* Copyright 2017 ROBOTIS CO., LTD.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/
#include "motor_fileio.h"
#include "Control_Table_XL430-W250.h"
#include <assert.h>

// Protocol version
#define PROTOCOL_VERSION1               1.0                 // See which protocol version is used in the Dynamixel
#define PROTOCOL_VERSION2               2.0

// Default setting
#define DEVICENAME                      "/dev/ttyUSB0"      // Check which port is being used on your controller
                                                            // ex) Windows: "COM1"   Linux: "/dev/ttyUSB0" Mac: "/dev/tty.usbserial-*"
#define ENABLE  1
#define DISABLE 0
#define NUM_ID 6
#define INIT_POS 2200

int init_pos_ = (int)INIT_POS;
char *dev_name = (char*)DEVICENAME;

void usage(char *progname)
{
  printf("-----------------------------------------------------------------------\n");
  printf("Usage: %s\n", progname);
  printf(" [-h | --help]........: display this help\n");
  printf(" [-d | --device]......: port to open\n");
  printf(" [-p | --position]......: initial position to set\n");
  printf("-----------------------------------------------------------------------\n");
}

int parsing(int argc, char *argv[])
{
  // parameter parsing
  while(1)
  {
    int option_index = 0, c = 0;
    static struct option long_options[] = {
        {"h", no_argument, 0, 0},
        {"help", no_argument, 0, 0},
        {"d", required_argument, 0, 0},
        {"device", required_argument, 0, 0},
        {"p", required_argument, 0, 0},
        {"position", required_argument, 0, 0},
        {0, 0, 0, 0}
    };

    // parsing all parameters according to the list above is sufficent
    c = getopt_long_only(argc, argv, "", long_options, &option_index);

    // no more options to parse
    if (c == -1) break;

    // unrecognized option
    if (c == '?') {
      usage(argv[0]);
      return 0;
    }

    // dispatch the given options
    switch(option_index) {
    // h, help
    case 0:
    case 1:
      usage(argv[0]);
      return 0;
      break;

    // d, device
    case 2:
    case 3:
      if (strlen(optarg) == 1)
      {
        char tmp[20];
        sprintf(tmp, "/dev/ttyUSB%s", optarg);
        dev_name = strdup(tmp);
      }
      else
        dev_name = strdup(optarg);
      break;

    // p, position
    case 4:
    case 5:
      if ( ((2048 <= atoi(optarg)) & (atoi(optarg)<=2500)) & (strlen(optarg) == 3 || strlen(optarg) == 4) )
      {
        init_pos_ = atoi(optarg);
        printf("Set Init position : %d \n", init_pos_);
      }
      else{
        fprintf(stderr,"[WARNING] : You shoud insert the value between 2048 ~ 2500.\n");
        assert(0);
      }
      break;

    default:
      usage(argv[0]);
      return 0;
    }
  }
}

int MotorOpenPort(dynamixel::PortHandler *portHandler)
{
  // Open port
  if (portHandler->openPort())
  {
    printf("[0]Succeeded to open the port!\n\n");
    printf(" - Device Name : %s\n", dev_name);
    printf(" - Baudrate    : %d\n\n", portHandler->getBaudRate());
  }
  else
  {
    printf("[0]Failed to open the port! [%s]\n", dev_name);
    printf("[0]Press any key to terminate...\n");
    getch();
    return 0;
  }

  return 1;
}

int MotorInit(dynamixel::PortHandler *portHandler, dynamixel::PacketHandler *packetHandler)
{
  // 1.Torque Enable
  // 1-1. groupSyncWriter instance 생성 
  dynamixel::GroupSyncWrite groupTorqueWrite(portHandler, packetHandler, ADDR_TORQUE_ENABLE, LEN_TORQUE_ENABLE);

  bool dxl_result = false;

  // 1-2. Syncwrite storage에 torque enable 값 저장
  for(int id = 0 ; id < NUM_ID ; id++)
  {
    uint8_t enable_ = ENABLE;
    dxl_result = groupTorqueWrite.addParam(id,&enable_);
    if (dxl_result != true)
    {
      fprintf(stderr, "[1-1]MotorInit : [ID:%03d] groupTorqueWrite addparam failed", id);
      return 0;
    }
  }

  // 1-3. Syncwrite 수행
  dxl_result = groupTorqueWrite.txPacket();
  if(dxl_result != COMM_SUCCESS) 
    printf("%s\n", packetHandler->getTxRxResult(dxl_result));
  else
    printf("[1-1]MotorInit : All Motors Torque is Enabled !!\n");

  // 1-4. Clear syncwrite parameter storage
  groupTorqueWrite.clearParam();

  // 2.INIT POSITION 설정
  // 2-1. groupSyncWriter instance 생성 
  dynamixel::GroupSyncWrite groupPosWrite(portHandler, packetHandler, ADDR_GOAL_POSITION, LEN_GOAL_POSITION);

  // 2-2. Syncwrite storage에 Position 값 저장
  for(int id = 0 ; id < NUM_ID ; id++)
  {
    dxl_result = groupPosWrite.addParam(id, (uint8_t*)&init_pos_);
    if (dxl_result != true)
    {
      fprintf(stderr, "[1-2]MotorInit : [ID:%03d] groupPosWrite addparam failed", id);
      return 0;
    }
  }

  // 2-3. Syncwrite 수행
  dxl_result = groupPosWrite.txPacket();
  if(dxl_result != COMM_SUCCESS) 
    printf("%s\n", packetHandler->getTxRxResult(dxl_result));
  else
    printf("[1-2]MotorInit : All Motors Init Position [%d] is Seted !!\n", init_pos_);

  // 2-4. Clear syncwrite parameter storage
  groupPosWrite.clearParam();

  return 1;
}

void help()
{
  printf("\n");
  printf("                 .------------------------------------.\n");
  printf("                 |  KEPCO Motor Monitor Command List  |\n");
  printf("                 '------------------------------------'\n");
  printf(" =========================== Common Commands ===========================\n");
  printf(" \n");
  printf(" help|h|?                    :Displays help information\n");
  printf(" baud [BAUD_RATE]            :Changes baudrate to [BAUD_RATE] \n");
  printf("                               ex) baud 57600 (57600 bps) \n");
  printf("                               ex) baud 1000000 (1 Mbps)  \n");
  printf(" exit                        :Exit this program\n");
  printf(" scan                        :Outputs the current status of all Dynamixels\n");
  printf(" ping [ID] [ID] ...          :Outputs the current status of [ID]s \n");
  printf(" bp                          :Broadcast ping (Dynamixel Protocol 2.0 only)\n");
  printf(" \n");
  printf(" ==================== Commands for Dynamixel Protocol 2.0 ====================\n");
  printf(" \n");
  printf(" wrb2|w2 [ID] [ADDR] [VALUE] :Write byte [VALUE] to [ADDR] of [ID]\n");
  printf(" wrw2 [ID] [ADDR] [VALUE]    :Write word [VALUE] to [ADDR] of [ID]\n");
  printf(" wrd2 [ID] [ADDR] [VALUE]    :Write dword [VALUE] to [ADDR] of [ID]\n");
  printf(" rdb2 [ID] [ADDR]            :Read byte value from [ADDR] of [ID]\n");
  printf(" rdw2 [ID] [ADDR]            :Read word value from [ADDR] of [ID]\n");
  printf(" rdd2 [ID] [ADDR]            :Read dword value from [ADDR] of [ID]\n");
  printf(" r2 [ID] [ADDR] [LENGTH]     :Dumps the control table of [ID]\n");
  printf("                               ([LENGTH] bytes from [ADDR])\n");
  printf(" reboot2|rbt2 [ID]           :reboot the Dynamixel of [ID]\n");
  printf(" reset2|rst2 [ID] [OPTION]   :Factory reset the Dynamixel of [ID]\n");
  printf("                               OPTION: 255(All), 1(Except ID), 2(Except ID&Baud)\n");
  printf(" \n");
  printf(" ==================== ADDR & LEN for XL430-W250 =============================\n");
  printf(" Goal Position ADDR : %d, LENGTH : %d \n", ADDR_GOAL_POSITION, LEN_GOAL_POSITION);
  printf(" Torque Enable ADDR : %d, LENGTH : %d \n", ADDR_TORQUE_ENABLE, LEN_TORQUE_ENABLE);
  printf(" LED ADDR           : %d, LENGTH : %d \n", ADDR_LED, LEN_LED);
  printf(" \n");

  printf("\n");
}
  
int run(dynamixel::PortHandler *portHandler,dynamixel::PacketHandler *packetHandler)
{
  fprintf(stderr, "\n***********************************************************************\n");
  fprintf(stderr,   "*                    KEPCO Motor Monitor                              *\n");
  fprintf(stderr,   "***********************************************************************\n\n");

  char    input[128];
  char    cmd[80];
  char    param[20][30];
  int     num_param;
  char    *token;
  uint8_t dxl_error;

  while(1)
  {
    printf("[CMD] ");
    fgets(input, sizeof(input), stdin);
    char *p;
    if ((p = strchr(input, '\n'))!= NULL) *p = '\0';
    fflush(stdin);

    if (strlen(input) == 0) continue;

    token = strtok(input, " ");

    if (token == 0) continue;

    strcpy(cmd, token);
    token = strtok(0, " ");
    num_param = 0;
    while(token != 0)
    {
      strcpy(param[num_param++], token);
      token = strtok(0, " ");
    }

    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "h") == 0 || strcmp(cmd, "?") == 0)
    {
      help();
    }
    else if (strcmp(cmd, "baud") == 0)
    {
      if (num_param == 1)
      {
        if (portHandler->setBaudRate(atoi(param[0])) == false)
          fprintf(stderr, " Failed to change baudrate! \n");
        else
          fprintf(stderr, " Success to change baudrate! [ BAUD RATE: %d ]\n", atoi(param[0]));
      }
      else
      {
        fprintf(stderr, " Invalid parameters! \n");
        continue;
      }
    }
    else if (strcmp(cmd, "exit") == 0)
    {
      portHandler->closePort();
      return 0;
    }
    else if (strcmp(cmd, "scan") == 0)
    {
      scan(portHandler,packetHandler);
    }
    else if (strcmp(cmd, "ping") == 0)
    {
      uint16_t dxl_model_num;

      if (num_param == 0)
      {
        fprintf(stderr, " Invalid parameters! \n");
        continue;
      }

      fprintf(stderr, "\n");
      fprintf(stderr, "ping Using Protocol 2.0\n");
      for (int i = 0; i < num_param; i++)
      {
        if (packetHandler->ping(portHandler, atoi(param[i]), &dxl_model_num, &dxl_error) == COMM_SUCCESS)
        {
          fprintf(stderr, "\n                                          ... SUCCESS \r");
          fprintf(stderr, " [ID:%.3d] Model No : %.5d \n", atoi(param[i]), dxl_model_num);
        }
        else
        {
          fprintf(stderr, "\n                                          ... FAIL \r");
          fprintf(stderr, " [ID:%.3d] \n", atoi(param[i]));
        }
      }
      fprintf(stderr, "\n");
    }
    else if (strcmp(cmd, "bp") == 0)
    {
      if (num_param == 0)
      {
        std::vector<unsigned char> vec;

        int dxl_result = packetHandler->broadcastPing(portHandler, vec);
        if (dxl_result != COMM_SUCCESS) packetHandler->printTxRxResult(dxl_result);

        for (unsigned int i = 0; i < vec.size(); i++)
        {
          fprintf(stderr, "\n                                          ... SUCCESS \r");
          fprintf(stderr, " [ID:%.3d] \n", vec.at(i));
        }
        printf("\n");
      }
      else
      {
        fprintf(stderr, " Invalid parameters! \n");
      }
    }
    else if (strcmp(cmd, "wrb2") == 0 || strcmp(cmd, "w2") == 0)
    {
      if (num_param == 3)
      {
        write(portHandler, packetHandler, atoi(param[0]), atoi(param[1]), 1, atoi(param[2]));
      }
      else
      {
        fprintf(stderr, " Invalid parameters! \n");
      }
    }
    else if (strcmp(cmd, "wrw2") == 0)
    {
      if (num_param == 3)
      {
        write(portHandler, packetHandler, atoi(param[0]), atoi(param[1]), 2, atoi(param[2]));
      }
      else
      {
        fprintf(stderr, " Invalid parameters! \n");
      }
    }
    else if (strcmp(cmd, "wrd2") == 0)
    {
      if (num_param == 3)
      {
        write(portHandler, packetHandler, atoi(param[0]), atoi(param[1]), 4, atoi(param[2]));
      }
      else
      {
        fprintf(stderr, " Invalid parameters! \n");
      }
    }
    else if (strcmp(cmd, "rdb2") == 0)
    {
      if (num_param == 2)
      {
        read(portHandler, packetHandler, atoi(param[0]), atoi(param[1]), 1);
      }
      else
      {
        fprintf(stderr, " Invalid parameters! \n");
      }
    }
    else if (strcmp(cmd, "rdw2") == 0)
    {
      if (num_param == 2)
      {
        read(portHandler, packetHandler, atoi(param[0]), atoi(param[1]), 2);
      }
      else
      {
        fprintf(stderr, " Invalid parameters! \n");
      }
    }
    else if (strcmp(cmd, "rdd2") == 0)
    {
      if (num_param == 2)
      {
        read(portHandler, packetHandler, atoi(param[0]), atoi(param[1]), 4);
      }
      else
      {
        fprintf(stderr, " Invalid parameters! \n");
      }
    }
    else if (strcmp(cmd, "r2") == 0)
    {
      if (num_param == 3)
      {
        dump(portHandler, packetHandler, atoi(param[0]), atoi(param[1]), atoi(param[2]));
      }
      else
      {
        fprintf(stderr, " Invalid parameters! \n");
      }
    }
    else if (strcmp(cmd, "reboot2") == 0 || strcmp(cmd, "rbt2") == 0)
    {
      if (num_param == 1)
      {
        int dxl_result = packetHandler->reboot(portHandler, atoi(param[0]), &dxl_error);
        if (dxl_result == COMM_SUCCESS)
        {
          if (dxl_error != 0) packetHandler->printRxPacketError(dxl_error);
          fprintf(stderr, "\n Success to reboot! \n\n");
        }
        else
        {
          packetHandler->printTxRxResult(dxl_result);
          fprintf(stderr, "\n Fail to reboot! \n\n");
        }
      }
      else
      {
          fprintf(stderr, " Invalid parameters! \n");
      }
    }
    else if (strcmp(cmd, "reset2") == 0 || strcmp(cmd, "rst2") == 0)
    {
      if (num_param == 2)
      {
        int dxl_result = packetHandler->factoryReset(portHandler, atoi(param[0]), atoi(param[1]), &dxl_error);
        if (dxl_result == COMM_SUCCESS)
        {
          if (dxl_error != 0) packetHandler->printRxPacketError(dxl_error);
          fprintf(stderr, "\n Success to reset! \n\n");
        }
        else
        {
          packetHandler->printTxRxResult(dxl_result);
          fprintf(stderr, "\n Fail to reset! \n\n");
        }
      }
      else
      {
        fprintf(stderr, " Invalid parameters! \n");
      }
    }
    else
    {
      printf(" Bad command! Please input 'help'.\n");
    }
  }
}

int main(int argc, char *argv[])
{

  // Data Pasing
  if(!parsing(argc,argv)) 
    printf("[WARNING] : parsing error\n");

  // Initialize Packethandler instance & PortHandler instance
  dynamixel::PacketHandler *packetHandler = dynamixel::PacketHandler::getPacketHandler(PROTOCOL_VERSION2);
  dynamixel::PortHandler *portHandler = dynamixel::PortHandler::getPortHandler(dev_name);
  
  // Open motor port
  if(MotorOpenPort(portHandler))
    printf("[1]Motor port Open Success !!\n");
  else
    printf("[1]Motor port Open Success !!\n");

  // Set motor torque & init position
  if(MotorInit(portHandler,packetHandler))
    printf("[2]MotorInit : Success !!\n");

  run(portHandler,packetHandler);
}
