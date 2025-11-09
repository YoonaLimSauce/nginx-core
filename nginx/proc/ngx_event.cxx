// 开启子进程相关

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include "ngx_func.h"
#include "ngx_macro.h"
#include "ngx_c_conf.h"

// 处理网络事件和定时器事件
void NgxProcessEventsAndTimers()
{
    global_socket.NgxEpollProcessEvents(-1);        // -1表示阻塞等待

    // TODO: nothing
}