#ifndef PRINTF_OVRD_H
#define PRINTF_OVRD_H

#include  <errno.h>
#include  <sys/unistd.h> // STDOUT_FILENO, STDERR_FILENO

extern UART_HandleTypeDef huart2;

int _write(int file, char *data, int len)
{
   if ((file != STDOUT_FILENO) && (file != STDERR_FILENO))
   {
      errno = EBADF;
      return -1;
   }

   // arbitrary timeout 1000
   HAL_StatusTypeDef status =
      HAL_UART_Transmit(&huart2, (uint8_t*)data, len, 0xFFFF);

   // return # of bytes written - as best we can tell
   return (status == HAL_OK ? len : 0);
}

#endif
