/* Copyright (C) Uppsala University
 *
 * This file is distributed under the terms of the GNU general Public
 * License (GPL), see the file LICENSE
 *
 * Author: Erik Nordström, <erikn@it.uu.se>
 */
#ifndef _DSR_PKT_H
#define _DSR_PKT_H

#ifdef NS2
#include <packet.h>
#include <ip.h>
#else
#include <linux/in.h>
#endif

#define MAX_RREP_OPTS 10
#define MAX_RERR_OPTS 10
#define MAX_ACK_OPTS  10

#define DEFAULT_TAILROOM 128

/* Internal representation of a packet. For portability */
struct dsr_pkt {//dsr_pkt()数据包结构定义：在26-29行，分别定义了源端IP，目的端IP，下一跳IP和上一跳IP
	struct in_addr src;	/* IP level data *///源端IP地址定义
	struct in_addr dst;                        //目的端IP地址定义
	struct in_addr nxt_hop;                    //下一跳IP
	struct in_addr prv_hop;                    //上一跳IP
	int flags;                                 //标志位
	int salvage;                               //保留位
#ifdef NS2//在32-52行，根据是否是在NS2平台进行协议模拟定义了：
	union {
		struct hdr_mac *ethh;
		unsigned char *raw;
	} mac;//定义可以访问MAC协议头部的mac联合体
	struct hdr_ip ip_data;
	union {
		struct hdr_ip *iph;
		char *raw;
	} nh;//定义可以访问IP协议头部的nh联合体
#else//同理
	union {
		struct ethhdr *ethh;
		char *raw;
	} mac;
	union {
		struct iphdr *iph;
		char *raw;
	} nh;
	char ip_data[60];//定义IP数据包的ip_data
#endif
	struct {
		union {
			struct dsr_opt_hdr *opth;
			char *raw;
		};		
		char *tail, *end;  
	} dh;//在53-59行，定义了可以访问dsr选项的结构体dh。
	     //在62-68行，分别定义了
	int num_rrep_opts, num_rerr_opts, num_rreq_opts, num_ack_opts;
	struct dsr_srt_opt *srt_opt;     //定义了指向源路由选项
	struct dsr_rreq_opt *rreq_opt;	/* Can only be one *///路由请求选项（可能不止一个）
	struct dsr_rrep_opt *rrep_opt[MAX_RREP_OPTS];//路由回复选项
	struct dsr_rerr_opt *rerr_opt[MAX_RERR_OPTS];//路由错误选项
	struct dsr_ack_opt *ack_opt[MAX_ACK_OPTS];   //确认选项
	struct dsr_ack_req_opt *ack_req_opt;         //确认请求选项
	struct dsr_srt *srt;	/* Source route */   //源路由结构体的指针

//在71-78行，定义了负载、负载长度及一个分片结构体。
	int payload_len;    //负载长度
#ifdef NS2
	AppData *payload;
	Packet *p;
#else
	char *payload;
	struct sk_buff *skb; //定义一个分片结构体
#endif

};

/* Flags: */
#define PKT_PROMISC_RECV 0x01
#define PKT_REQUEST_ACK  0x02
#define PKT_PASSIVE_ACK  0x04
#define PKT_XMIT_JITTER  0x08

/* Packet actions: */
#define DSR_PKT_NONE           1
#define DSR_PKT_SRT_REMOVE     (DSR_PKT_NONE << 2)
#define DSR_PKT_SEND_ICMP      (DSR_PKT_NONE << 3)
#define DSR_PKT_SEND_RREP      (DSR_PKT_NONE << 4)
#define DSR_PKT_SEND_BUFFERED  (DSR_PKT_NONE << 5)
#define DSR_PKT_SEND_ACK       (DSR_PKT_NONE << 6)
#define DSR_PKT_FORWARD        (DSR_PKT_NONE << 7)
#define DSR_PKT_FORWARD_RREQ   (DSR_PKT_NONE << 8)
#define DSR_PKT_DROP           (DSR_PKT_NONE << 9)
#define DSR_PKT_ERROR          (DSR_PKT_NONE << 10)
#define DSR_PKT_DELIVER        (DSR_PKT_NONE << 11)
#define DSR_PKT_ACTION_LAST    (12)

static inline int dsr_pkt_opts_len(struct dsr_pkt *dp)
{
	return dp->dh.tail - dp->dh.raw;
}

static inline int dsr_pkt_tailroom(struct dsr_pkt *dp)
{
	return dp->dh.end - dp->dh.tail;
}

#ifdef NS2
struct dsr_pkt *dsr_pkt_alloc(Packet * p);
#else
struct dsr_pkt *dsr_pkt_alloc(struct sk_buff *skb);
#endif
char *dsr_pkt_alloc_opts(struct dsr_pkt *dp, int len);
char *dsr_pkt_alloc_opts_expand(struct dsr_pkt *dp, int len);
void dsr_pkt_free(struct dsr_pkt *dp);
int dsr_pkt_free_opts(struct dsr_pkt *dp);

#endif				/* _DSR_PKT_H */
/*在对数据包结构体的定义代码分析
