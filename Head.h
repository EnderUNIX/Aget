#ifndef HEAD_H
#define HEAD_H

#include "Data.h"

#ifndef h_addr
#define h_addr h_addr_list[0]
#endif

void http_head_req(struct request *);

#endif
