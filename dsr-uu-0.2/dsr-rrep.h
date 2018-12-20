/* Copyright (C) Uppsala University
 *
 * This file is distributed under the terms of the GNU general Public
 * License (GPL), see the file LICENSE
 *
 * Author: Erik Nordström, <erikn@it.uu.se>
 */
#ifndef _DSR_RREP_H
#define _DSR_RREP_H

#include "dsr.h"
#include "dsr-srt.h"

#ifndef NO_GLOBALS

struct dsr_rrep_opt { //DSR路由回复选项
	u_int8_t type;  //选项类型
	u_int8_t length;//选项长度
#if defined(__LITTLE_ENDIAN_BITFIELD) //小端情况
	u_int8_t res:7;              //保留位
	u_int8_t l:1;                //置1：路由回复中的最后一跳到达了DSR网络的外部
#elif defined (__BIG_ENDIAN_BITFIELD)//大端情况
	u_int8_t l:1;                //置1：路由回复中的最后一跳到达了DSR网络的外部
	u_int8_t res:7;              //保留位
#else
#error  "Please fix <asm/byteorder.h>"
#endif
	u_int32_t addrs[0];        //记录路由恢复信息
};
/*代码第21行，l标志位代表Last Hop，置0表示最后一跳节点在DSR网络内部，置1表示最后一条到达了DSR网络的外部。
在选取路由时应当遵循以下原则：尽量选取仅利用本网络内部节点就可以到达目的节点的路由，尽量不经过外部网络。
即，在路由缓存中优先选择l标志位置0的路由。
*/
#define DSR_RREP_HDR_LEN sizeof(struct dsr_rrep_opt)
#define DSR_RREP_OPT_LEN(srt) (DSR_RREP_HDR_LEN + srt->laddrs + sizeof(struct in_addr))
/* Length of source route is length of option, minus reserved/flags field minus
 * the last source route hop (which is the destination) */
#define DSR_RREP_ADDRS_LEN(rrep_opt) (rrep_opt->length - 1 - sizeof(struct in_addr))

#endif				/* NO_GLOBALS */

#ifndef NO_DECLS

int dsr_rrep_opt_recv(struct dsr_pkt *dp, struct dsr_rrep_opt *rrep_opt);
int dsr_rrep_send(struct dsr_srt *srt, struct dsr_srt *srt_to_me);

void grat_rrep_tbl_timeout(unsigned long data);
int grat_rrep_tbl_add(struct in_addr src, struct in_addr prev_hop);
int grat_rrep_tbl_find(struct in_addr src, struct in_addr prev_hop);
int grat_rrep_tbl_init(void);
void grat_rrep_tbl_cleanup(void);

#endif				/* NO_DECLS */

#endif				/* _DSR_RREP */
