/*
 * netlink interface
 *
 * Copyright (c) 2017 Goodix
 * Copyright (C) 2021 XiaoMi, Inc.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/time.h>
#include <linux/types.h>
#include <net/sock.h>
#include <net/netlink.h>

#define NETLINK_TEST 25
#define MAX_MSGSIZE 32
int stringlength(char *s);
void sendnlmsg_ta(char *message);
static int pid = -1;
struct sock *nl_sk_ta = NULL;

void sendnlmsg_ta(char *message)
{
	struct sk_buff *skb_1;
	struct nlmsghdr *nlh;
	int len = NLMSG_SPACE(MAX_MSGSIZE);
	int slen = 0;
	int ret = 0;

	if (!message || !nl_sk_ta || !pid) {
		return;
	}

	skb_1 = alloc_skb(len, GFP_KERNEL);

	if (!skb_1) {
		pr_err("alloc_skb error\n");
		return;
	}

	slen = strlen(message);
	nlh = nlmsg_put(skb_1, 0, 0, 0, MAX_MSGSIZE, 0);
	NETLINK_CB(skb_1).portid = 0;
	NETLINK_CB(skb_1).dst_group = 0;
	message[slen] = '\0';
	memcpy(NLMSG_DATA(nlh), message, slen + 1);
	ret = netlink_unicast(nl_sk_ta, skb_1, pid, MSG_DONTWAIT);

	if (!ret) {
		/*kfree_skb(skb_1);*/
		pr_err("send msg from kernel to usespace failed ret 0x%x\n", ret);
	}
}


void nl_data_ready_ta(struct sk_buff *__skb)
{
	struct sk_buff *skb;
	struct nlmsghdr *nlh;
	char str[100];
	skb = skb_get(__skb);

	if (skb->len >= NLMSG_SPACE(0)) {
		nlh = nlmsg_hdr(skb);
		memcpy(str, NLMSG_DATA(nlh), sizeof(str));
		pid = nlh->nlmsg_pid;
		kfree_skb(skb);
	}
}


int netlink_init_ta(void)
{
	struct netlink_kernel_cfg netlink_cfg;
	memset(&netlink_cfg, 0, sizeof(struct netlink_kernel_cfg));
	netlink_cfg.groups = 0;
	netlink_cfg.flags = 0;
	netlink_cfg.input = nl_data_ready_ta;
	netlink_cfg.cb_mutex = NULL;
	nl_sk_ta = netlink_kernel_create(&init_net, NETLINK_TEST,
					&netlink_cfg);

	if (!nl_sk_ta) {
		pr_err("create netlink socket error\n");
		return 1;
	}

	return 0;
}

void netlink_exit_ta(void)
{
	if (nl_sk_ta != NULL) {
		netlink_kernel_release(nl_sk_ta);
		nl_sk_ta = NULL;
	}

	pr_info("self module exited\n");
}

