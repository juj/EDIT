#include "../network.h"
#include "../vs/vs_network.h"
#include "../line.h"
#include "../file.h"
#include "../screen.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

static SOCKET recvSock, sendSock;

void init_network()
{
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2, 2), &wsaData);

  recvSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  int bAllow = 1;
  int err = setsockopt(recvSock, SOL_SOCKET, SO_BROADCAST, (char *)&bAllow, sizeof(bAllow));

  sockaddr_in src_addr = {};
  src_addr.sin_family = AF_INET;
  src_addr.sin_port = htons(RECVFROM_PORT); // if the port is specified as zero, the service provider assigns a unique port to the application from the dynamic client port range. On Windows Vista and later, the dynamic client port range is a value between 49152 and 65535. 
  src_addr.sin_addr.s_addr = INADDR_ANY;
  //  inet_pton(AF_INET, "127.0.0.1", &src_addr.sin_addr.s_addr);
  if (bind(recvSock, (sockaddr*)&src_addr, sizeof(src_addr)) == SOCKET_ERROR)
  {
    printf("bind failed with error %u\n", WSAGetLastError());
    closesocket(recvSock);
    return;
  }

  unsigned long nonblocking = 1;
  err = ioctlsocket(recvSock, FIONBIO, &nonblocking);
  if (err != NO_ERROR) printf("ioctlsocket failed with error: %ld\n", err);

  sendSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sendSock == INVALID_SOCKET)
  {
    perror("socket creation");
    return;
  }

  BOOL enabled = TRUE;
  if (setsockopt(sendSock, SOL_SOCKET, SO_BROADCAST, (char*)&enabled, sizeof(BOOL)) < 0)
  {
    perror("broadcast options");
    closesocket(sendSock);
    return;
  }
  // TODO: Do I need two sockets even?
}

void deinit_network()
{
  closesocket(recvSock);
  closesocket(sendSock);
}

size_t recv_datagram(void *dst, size_t maxBytes)
{
  sockaddr_in sock_in = {};
  sock_in.sin_family = AF_INET;
  sock_in.sin_port = htons(RECVFROM_PORT);
  sock_in.sin_addr.s_addr = INADDR_ANY;

  int length = sizeof(sockaddr_in);
  int numBytes = recvfrom(recvSock, (char*)dst, (int)maxBytes, 0, (sockaddr*)&sock_in, &length);
  if (numBytes < 0)
  {
    int error = WSAGetLastError();
    if (error != WSAEWOULDBLOCK)
      printf("Error: %d\n", WSAGetLastError());
    return 0;
  }
  /*
    printf("Received broadcast of %d bytes.\n", (int)length);
    for(int i = 0; i < length; ++i)
      printf("%02Xh ", buf[i]);
    printf("\n");
    */
  return numBytes;
}

void send_datagram(const void *src, size_t numBytes)
{
  sockaddr_in dst_addr = {};
  dst_addr.sin_family = AF_INET;
  dst_addr.sin_port = htons(SENDTO_PORT);
  dst_addr.sin_addr.s_addr = INADDR_BROADCAST;
  //inet_pton(AF_INET, "192.168.0.144", &dst_addr.sin_addr.s_addr);

  if (sendto(sendSock, (const char*)src, (int)numBytes, 0, (sockaddr *)&dst_addr, sizeof(dst_addr)) < 0)
    perror("send failed");
}

void send_line_to_network_server(uint8_t editType, File *f, Line *l, line_num lineno)
{
  UpdateLineMsg *msg = (UpdateLineMsg*)malloc(sizeof(UpdateLineMsg) + MAX_FILENAME_LEN + MAX_CHARS_PER_LINE);
  msg->magic = MAGIC_UPDATE_LINE_MSG;
  msg->line_no = lineno - 1; // Convert 1-based line number to 0-based
  msg->edit_type = editType;
  msg->filename_len = (uint8_t)strlen(f->filename);
  msg->line_len = l ? l->length : 0;
  uint8_t *data = (uint8_t*)msg + sizeof(UpdateLineMsg);
  memcpy(data, f->filename, msg->filename_len);
  data += msg->filename_len;
  if (l) memcpy(data, l->text, l->length);
  send_datagram(msg, sizeof(UpdateLineMsg) + msg->filename_len + msg->line_len);
  free(msg);
}

static uint8_t data[1400];

void receive_data_from_network()
{
  for(;;)
  {
    size_t recvd = recv_datagram(data, sizeof(data));
    if (recvd == 0) break;

    BuildErrorMsg *msg = (BuildErrorMsg*)data;
    if (msg->magic == MAGIC_BUILD_ERROR_MSG && msg->msg_len < SCREEN_WIDTH)
    {
      char *str = (char *)data + sizeof(BuildErrorMsg);
      str[msg->msg_len] = 0;
      put_string_on_screen(str, 0, SCREEN_HEIGHT-1, msg->success ? LTGREEN : LTRED);
      for(int x = msg->msg_len; x < SCREEN_WIDTH; ++x)
        put_screen((SCREEN_HEIGHT-1)*SCREEN_WIDTH + x, ' ');
    }
  }
}
