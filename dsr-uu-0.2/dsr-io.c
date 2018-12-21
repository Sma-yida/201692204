/* Copyright (C) Uppsala University
 *
 * This file is distributed under the terms of the GNU general Public
 * License (GPL), see the file LICENSE
 *
 * Author: Erik Nordström, <erikn@it.uu.se>
 */
#ifdef NS2
#include "ns-agent.h"
#else
#include "dsr-dev.h"
#endif

#include "dsr.h"
#include "dsr-pkt.h"
#include "dsr-rreq.h"
#include "dsr-rrep.h"
#include "dsr-srt.h"
#include "dsr-ack.h"
#include "dsr-rtc.h"
#include "dsr-ack.h"
#include "maint-buf.h"
#include "neigh.h"
#include "dsr-opt.h"
#include "link-cache.h"
#include "debug.h"
#include "send-buf.h"
//在dsr_recv() 函数中，实现了节点处理接收到的数据包的功能。
int NSCLASS dsr_recv(struct dsr_pkt *dp)
{
	int i = 0, action;
	int mask = DSR_PKT_NONE;

/* Process DSR Options处理DSR选项 */
	action = dsr_opt_recv(dp);
//在35行，调用dsr_opt_recv() 函数，检查数据包的选项，并返回数据包的状态，将值赋给action，方便后续处理
	/* Add mac address of previous hop to the neighbor table */
//然后在39-42行，对接收到的数据包的状态值和PKT_PROMISC_RECV进行与操作，
	if (dp->flags & PKT_PROMISC_RECV) {//若不为0
		dsr_pkt_free(dp);          //那么调用dsr_pkt_free() 函数，将数据包的空间释放掉，丢弃该数据包
		return 0;                  //然后返回
	}//若为0，那么在43-112行，进入for循环
	for (i = 0; i < DSR_PKT_ACTION_LAST; i++) {
	//使用switch语句，根据数据包的不同状态，进行相应处理。
		switch (action & mask) {
		case DSR_PKT_NONE:
			break;
		case DSR_PKT_DROP:
		case DSR_PKT_ERROR:
			DEBUG("DSR_PKT_DROP or DSR_PKT_ERROR\n");
			dsr_pkt_free(dp);
			return 0;
		case DSR_PKT_SEND_ACK:
			/* Moved to dsr-ack.c */
			break;
		case DSR_PKT_SRT_REMOVE:
			//DEBUG("Remove source route\n");
			// Hmm, we remove the DSR options when we deliver a
			//packet
			//dsr_opt_remove(dp);
			break;
		case DSR_PKT_FORWARD://在62-80行，如果是需要转发的数据包，会检查其TTL

#ifdef NS2
			if (dp->nh.iph->ttl() < 1)
#else
			if (dp->nh.iph->ttl < 1)  
#endif
			{//若ttl为0，则丢弃该数据包
				DEBUG("ttl=0, dropping!\n");
				dsr_pkt_free(dp);
				return 0;
			} else {//否则：
				DEBUG("Forwarding %s %s nh %s\n",
				      print_ip(dp->src),     //打印源端IP地址
				      print_ip(dp->dst),     //打印目的端IP地址
				      print_ip(dp->nxt_hop));//打印下一跳IP地址
				XMIT(dp);                    //然后执行XMIT()传输数据包
				return 0;                    
			}
			break;
		case DSR_PKT_FORWARD_RREQ:
			XMIT(dp);
			return 0;
		case DSR_PKT_SEND_RREP:
			/* In dsr-rrep.c */
			break;
		case DSR_PKT_SEND_ICMP:
			DEBUG("Send ICMP\n");
			break;
		case DSR_PKT_SEND_BUFFERED:
			if (dp->rrep_opt) {
				struct in_addr rrep_srt_dst;
				int i;
				
				for (i = 0; i < dp->num_rrep_opts; i++) {
					rrep_srt_dst.s_addr = dp->rrep_opt[i]->addrs[DSR_RREP_ADDRS_LEN(dp->rrep_opt[i]) / sizeof(struct in_addr)];
					
					send_buf_set_verdict(SEND_BUF_SEND, rrep_srt_dst);
				}
			}
				break;
		case DSR_PKT_DELIVER:
			DEBUG("Deliver to DSR device\n");
			DELIVER(dp);
			return 0;
		case 0:
			break;
		default:
			DEBUG("Unknown pkt action\n");
		}
		mask = (mask << 1);
	}

	dsr_pkt_free(dp);
//在最后，释放该数据包地址空间，丢弃该数据包，完成函数的执行。
	return 0;
}

void NSCLASS dsr_start_xmit(struct dsr_pkt *dp)
{
	int res;

	if (!dp) {
		DEBUG("Could not allocate DSR packet\n");
		return;
	}

	dp->srt = dsr_rtc_find(dp->src, dp->dst);

	if (dp->srt) {

		if (dsr_srt_add(dp) < 0) {
			DEBUG("Could not add source route\n");
			goto out;
		}
		/* Send packet */

		XMIT(dp);

		return;

	} else {
#ifdef NS2
		res = send_buf_enqueue_packet(dp, &DSRUU::ns_xmit);
#else
		res = send_buf_enqueue_packet(dp, &dsr_dev_xmit);
#endif
		if (res < 0) {
			DEBUG("Queueing failed!\n");
			goto out;
		}
		res = dsr_rreq_route_discovery(dp->dst);

		if (res < 0)
			DEBUG("RREQ Transmission failed...");

		return;
	}
      out:
	dsr_pkt_free(dp);
}
