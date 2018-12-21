/* Copyright (C) Uppsala University
 *
 * This file is distributed under the terms of the GNU general Public
 * License (GPL), see the file LICENSE
 *
 * Author: Erik Nordström, <erikn@it.uu.se>
 */
#ifndef _DSR_SRT_H
#define _DSR_SRT_H

#include "dsr.h"
#include "debug.h"

#ifdef NS2
#include "endian.h"
#endif

#ifndef NO_GLOBALS

/* Source route options header */
/* TODO: This header is not byte order correct... is there a simple way to fix
 * it? */
struct dsr_srt_opt {//DSR路由维护选项
	u_int8_t type;
	u_int8_t length;
#if defined(__LITTLE_ENDIAN_BITFIELD)	
/* TODO: Fix bit/byte order */
	u_int16_t f:1;      //first置1：路由的第一条路由到达的是DSR网络的外部
	u_int16_t l:1;      //last置1：路由信息的最后一跳到达了DSR网络的外部
	u_int16_t res:4;    //保留位
	u_int16_t salv:4;   //Salvage:该数据包被重发的次数
	u_int16_t sleft:6;  //Segments Left:路由剩余的跳数
#elif defined (__BIG_ENDIAN_BITFIELD)//大端同理定义
	u_int16_t f:1;
	u_int16_t l:1;
	u_int16_t res:4;
	u_int16_t salv:4;     //Salvage:该数据包被重发的次数
	u_int16_t sleft:6;  //Segments Left:路由剩余的跳数
#else
#error  "Please fix <asm/byteorder.h>"
#endif
	u_int32_t addrs[0];    //维护的源路由路径
};
/*salv标志位占4bits，其含义是salvage，用来表示这个数据包被重发的次数，根据这个数值来决定该数据包是否应该被丢弃。
  
  sleft标志位占4bits，其含义是segments Left，用来表示路由剩余的跳数。
  
  路由维护选项，也可以被称为源路由选项，当一个节点想到达某一目的节点时，会从路由缓存中取得路由，
  并将源路由选项加入DSR选项头，具体存入数组addrs[ ]中，利用其中的路由信息到达目的节点。*/
/* Flags: */
#define SRT_FIRST_HOP_EXT 0x1
#define SRT_LAST_HOP_EXT  0x2


#define DSR_SRT_HDR_LEN sizeof(struct dsr_srt_opt)
#define DSR_SRT_OPT_LEN(srt) (DSR_SRT_HDR_LEN + srt->laddrs)

/* Flags */
#define SRT_BIDIR 0x1

/* Internal representation of a source route */
struct dsr_srt {
	struct in_addr src;
	struct in_addr dst;
	unsigned short flags;
	unsigned short index;
	unsigned int laddrs;	/* length in bytes if addrs */
	struct in_addr addrs[0];	/* Intermediate nodes */
};

static inline char *print_srt(struct dsr_srt *srt)
{
#define BUFLEN 256
	static char buf[BUFLEN];
	unsigned int i, len;

	if (!srt)
		return NULL;

	len = sprintf(buf, "%s<->", print_ip(srt->src));

	for (i = 0; i < (srt->laddrs / sizeof(u_int32_t)) &&
	     (len + 16) < BUFLEN; i++)
		len += sprintf(buf + len, "%s<->", print_ip(srt->addrs[i]));

	if ((len + 16) < BUFLEN)
		len = sprintf(buf + len, "%s", print_ip(srt->dst));
	return buf;
}
struct in_addr dsr_srt_next_hop(struct dsr_srt *srt, int sleft);
struct in_addr dsr_srt_prev_hop(struct dsr_srt *srt, int sleft);
struct dsr_srt_opt *dsr_srt_opt_add(char *buf, int len, int flags, int salvage, struct dsr_srt *srt);
struct dsr_srt *dsr_srt_new(struct in_addr src, struct in_addr dst,
			    unsigned int length, char *addrs);
struct dsr_srt *dsr_srt_new_rev(struct dsr_srt *srt);
void dsr_srt_del(struct dsr_srt *srt);
struct dsr_srt *dsr_srt_concatenate(struct dsr_srt *srt1, struct dsr_srt *srt2);int dsr_srt_check_duplicate(struct dsr_srt *srt);
struct dsr_srt *dsr_srt_new_split(struct dsr_srt *srt, struct in_addr addr);

#endif				/* NO_GLOBALS */

#ifndef NO_DECLS

int dsr_srt_add(struct dsr_pkt *dp);
int dsr_srt_opt_recv(struct dsr_pkt *dp, struct dsr_srt_opt *srt_opt);

#endif				/* NO_DECLS */

#endif				/* _DSR_SRT_H */
//在dsr_srt.h头文件中定义了路由维护选项，具体代码分析
