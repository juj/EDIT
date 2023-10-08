#include "../network.h"
#include "../vs/vs_network.h"
#include "../line.h"

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

  if (sendto(sendSock, (const char*)src, (int)numBytes, 0, (sockaddr *)&dst_addr, sizeof(dst_addr)) < 0)
    perror("send failed");
}
