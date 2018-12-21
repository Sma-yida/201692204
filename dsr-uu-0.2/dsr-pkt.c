/* Copyright (C) Uppsala University
 *
 * This file is distributed under the terms of the GNU general Public
 * License (GPL), see the file LICENSE
 *
 * Author: Erik Nordström, <erikn@it.uu.se>
 */
#ifdef __KERNEL_
#include <linux/skbuff.h>
#include <linux/if_ether.h>
#endif

#ifdef NS2
#include "ns-agent.h"
#endif

#include "debug.h"
#include "dsr-opt.h"
#include "dsr.h"

char *dsr_pkt_alloc_opts(struct dsr_pkt *dp, int len)
{
	if (!dp)
		return NULL;

	dp->dh.raw = (char *)MALLOC(len + DEFAULT_TAILROOM, GFP_ATOMIC);

	if (!dp->dh.raw)
		return NULL;

	dp->dh.tail = dp->dh.raw + len;
	dp->dh.end = dp->dh.tail + DEFAULT_TAILROOM;

	return dp->dh.raw;
}

char *dsr_pkt_alloc_opts_expand(struct dsr_pkt *dp, int len)
{
	char *tmp;
	int old_len;

	if (!dp || !dp->dh.raw)
		return NULL;

	if (dsr_pkt_tailroom(dp) > len) {
		tmp = dp->dh.tail;
		dp->dh.tail += len;
		return tmp;
	}

	tmp = dp->dh.raw;
	old_len = dsr_pkt_opts_len(dp);

	if (!dsr_pkt_alloc_opts(dp, old_len + len))
		return NULL;

	memcpy(dp->dh.raw, tmp, old_len);

	FREE(tmp);

	return (dp->dh.raw + old_len);
}

int dsr_pkt_free_opts(struct dsr_pkt *dp)
{
	int len;

	if (!dp->dh.raw)
		return -1;

	len = dsr_pkt_opts_len(dp);

	FREE(dp->dh.raw);

	dp->dh.raw = dp->dh.end = dp->dh.tail = NULL;
	dp->srt_opt = NULL;
	dp->rreq_opt = NULL;
	memset(dp->rrep_opt, 0, sizeof(struct dsr_rrep_opt *) * MAX_RREP_OPTS);
	memset(dp->rerr_opt, 0, sizeof(struct dsr_rerr_opt *) * MAX_RERR_OPTS);
	memset(dp->ack_opt, 0, sizeof(struct dsr_ack_opt *) * MAX_ACK_OPTS);
	dp->num_rrep_opts = dp->num_rerr_opts = dp->num_ack_opts = 0;

	return len;
}
//在dsr_pkt_alloc()函数中，实现了生成数据包的功能
#ifdef NS2//同样会根据是否是在NS2平台上进行模拟而分成两种情况，此处对NS2平台上的宏定义进行分析：
struct dsr_pkt *dsr_pkt_alloc(Packet * p)
{
	struct dsr_pkt *dp;//定义了一个数据包结构体
	struct hdr_cmn *cmh;//定义了一个数据包的common头指针
	
	int dsr_opts_len = 0; //为数据包申请了一个内存空间，然后将其置0，方便后续初始化
	
	dp = (struct dsr_pkt *)MALLOC(sizeof(struct dsr_pkt), GFP_ATOMIC);

	if (!dp)
		return NULL;

	memset(dp, 0, sizeof(struct dsr_pkt));

	if (p) {//判断数据包非空的情况下：对其进行初始化操作
		cmh = hdr_cmn::access(p);//address()是接口函数，用来访问数据包的包头部分

		dp->p = p;
		dp->mac.raw = p->access(hdr_mac::offset_);
		dp->nh.iph = HDR_IP(p);

		dp->src.s_addr =
		    Address::instance().get_nodeaddr(dp->nh.iph->saddr());
		dp->dst.s_addr =
		    Address::instance().get_nodeaddr(dp->nh.iph->daddr());
                 //以下根据common头的类型执行不同的操作
		if (cmh->ptype() == PT_DSR) {//PT_DSR类型
			struct dsr_opt_hdr *opth;
			
			opth = hdr_dsr::access(p);

			dsr_opts_len = ntohs(opth->p_len) + DSR_OPT_HDR_LEN;

			if (!dsr_pkt_alloc_opts(dp, dsr_opts_len)) {
				FREE(dp);
				return NULL;
			}

			memcpy(dp->dh.raw, (char *)opth, dsr_opts_len);

			dsr_opt_parse(dp);

			if ((DATA_PACKET(dp->dh.opth->nh) ||
			    dp->dh.opth->nh == PT_PING) && 
				ConfVal(UseNetworkLayerAck))
				dp->flags |= PKT_REQUEST_ACK;
		} else if ((DATA_PACKET(cmh->ptype()) || 
			    cmh->ptype() == PT_PING) && //PT_PING类型
			   ConfVal(UseNetworkLayerAck))
			dp->flags |= PKT_REQUEST_ACK;

		/* A trick to calculate payload length... */
		dp->payload_len = cmh->size() - dsr_opts_len - IP_HDR_LEN;
	}
	return dp;//无论在101行判断的结果如何，最后都会在这行，返回dp结构体指针。
}

#else//根据是否是在NS2平台上进行模拟第二张种情况，此处对非NS2平台上的宏定义进行分析：同上理

struct dsr_pkt *dsr_pkt_alloc(struct sk_buff *skb)//在dsr_pkt_alloc()函数中，实现了生成数据包的功能
{
	struct dsr_pkt *dp;
	int dsr_opts_len = 0;

	dp = (struct dsr_pkt *)MALLOC(sizeof(struct dsr_pkt), GFP_ATOMIC);

	if (!dp)
		return NULL;

	memset(dp, 0, sizeof(struct dsr_pkt));

	if (skb) {
	/* 	skb_unlink(skb); */

		dp->skb = skb;

		dp->mac.raw = skb->mac.raw;
		dp->nh.iph = skb->nh.iph;

		dp->src.s_addr = skb->nh.iph->saddr;
		dp->dst.s_addr = skb->nh.iph->daddr;

		if (dp->nh.iph->protocol == IPPROTO_DSR) {
			struct dsr_opt_hdr *opth;
			int n;

			opth = (struct dsr_opt_hdr *)(dp->nh.raw + (dp->nh.iph->ihl << 2));
			dsr_opts_len = ntohs(opth->p_len) + DSR_OPT_HDR_LEN;

			if (!dsr_pkt_alloc_opts(dp, dsr_opts_len)) {
				FREE(dp);
				return NULL;
			}

			memcpy(dp->dh.raw, (char *)opth, dsr_opts_len);
			
			n = dsr_opt_parse(dp);
			
			DEBUG("Packet has %d DSR option(s)\n", n);
		}

		dp->payload = dp->nh.raw +
		    (dp->nh.iph->ihl << 2) + dsr_opts_len;

		dp->payload_len = ntohs(dp->nh.iph->tot_len) -
		    (dp->nh.iph->ihl << 2) - dsr_opts_len;

		if (dp->payload_len)
			dp->flags |= PKT_REQUEST_ACK;
	}
	return dp;
}

#endif

void dsr_pkt_free(struct dsr_pkt *dp)
{

	if (!dp)
		return;
#ifndef NS2
	if (dp->skb)
		dev_kfree_skb_any(dp->skb);
#endif
	dsr_pkt_free_opts(dp);

	if (dp->srt)
		FREE(dp->srt);

	FREE(dp);

	return;
}
