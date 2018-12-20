/* Copyright (C) Uppsala University
 *
 * This file is distributed under the terms of the GNU general Public
 * License (GPL), see the file LICENSE
 *
 * Author: Erik Nordström, <erikn@it.uu.se>
 */
#ifndef _DSR_RREQ_H
#define _DSR_RREQ_H

#include "dsr.h"

#ifdef NS2
#include "endian.h"
#endif

#ifndef NO_GLOBALS

struct dsr_rreq_opt {        //DSR路由请求选项定义
	u_int8_t type;       //选项类型
	u_int8_t length;     //选项长度
	u_int16_t id;        //路由id号
	u_int32_t target;    //目的地址
	u_int32_t addrs[0];  //记录路由信息
};
/*下面对路由id号和记录路由信息的addrs[ ]数组进行详细说明：
1.路由id号是发起路由请求时源节点产生的id号，且每发起一次新的路由请求就会产生一个新的id号，
但需要特别说明的是，id号都是唯一的。如果节点在路由缓存中检测到相同的id号，那么它将直接丢弃这个数据包以防止多次转发。
如果没有发现相同的id，则转发该数据包并把id记录到缓存中的路由请求表中。
2.addrs[ ]数组用来存储路由记录，当数据报到达第i的节点时，会在数组的第i项addrs[i]中记录对应的路由信息。
*/
#define DSR_RREQ_HDR_LEN sizeof(struct dsr_rreq_opt)
#define DSR_RREQ_OPT_LEN (DSR_RREQ_HDR_LEN - 2)
#define DSR_RREQ_TOT_LEN IP_HDR_LEN + sizeof(struct dsr_opt_hdr) + sizeof(struct dsr_rreq_opt)
#define DSR_RREQ_ADDRS_LEN(rreq_opt) (rreq_opt->length - 6)

#endif				/* NO_GLOBALS */

#ifndef NO_DECLS
void rreq_tbl_set_max_len(unsigned int max_len);
int dsr_rreq_opt_recv(struct dsr_pkt *dp, struct dsr_rreq_opt *rreq_opt);
int rreq_tbl_route_discovery_cancel(struct in_addr dst);
int dsr_rreq_route_discovery(struct in_addr target);
int dsr_rreq_send(struct in_addr target, int ttl);
void rreq_tbl_timeout(unsigned long data);
struct rreq_tbl_entry *__rreq_tbl_entry_create(struct in_addr node_addr);
struct rreq_tbl_entry *__rreq_tbl_add(struct in_addr node_addr);
int rreq_tbl_add_id(struct in_addr initiator, struct in_addr target,
		    unsigned short id);
int dsr_rreq_duplicate(struct in_addr initiator, struct in_addr target,
		       unsigned int id);

int rreq_tbl_init(void);
void rreq_tbl_cleanup(void);

#endif				/* NO_DECLS */

#endif				/* _DSR_RREQ */
//剩下的是异常处理特殊情况的考虑啥的
