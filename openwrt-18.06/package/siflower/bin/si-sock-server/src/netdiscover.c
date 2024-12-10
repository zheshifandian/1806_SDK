/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2015年09月02日 20时12分00秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  kevin.feng , kevin.feng@siflower.com.cn
 *        Company:  Siflower
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include <malloc.h>

#include <sys/wait.h>


#include "netdiscover.h"
#include "utils.h"


pthread_mutex_t g_scan_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t g_scan_cond = PTHREAD_COND_INITIALIZER;


/* Forge Arp Packet, using libnet */
void scan_process(devinfo *device)
{
    char *dest_ip = NULL;
    char *iface = device->interface;
    int i = 0;
    char lnet_error[100];
    struct libnet_ether_addr *src_mac = NULL;
    u_char sip[IP_ALEN];
    u_char dip[IP_ALEN];
    u_int32_t otherip, myip;

    libnet_ptag_t arp=0, eth=0;
    libnet_t *libnet = NULL;

    u_char dmac[ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    u_int8_t smac[ETH_ALEN];

    pthread_mutex_lock(&g_scan_mutex);
    pthread_cond_wait(&g_scan_cond, &g_scan_mutex);
    pthread_mutex_unlock(&g_scan_mutex);

    int k = 0;
    for(k = 0; k < SCAN_COUNT; k++)
    {
        for(i = 0; i < device->ipnum; i++)
        {
            libnet = libnet_init(LIBNET_LINK, iface, lnet_error);
            if(!libnet)
            {
                LOG("init error lnet_error = %s", lnet_error);
            }

            src_mac = libnet_get_hwaddr(libnet);

            memcpy(smac, src_mac->ether_addr_octet, ETH_ALEN);



            myip = libnet_name2addr4(libnet, "192.168.12.67", LIBNET_RESOLVE);
            memcpy(sip, (char*)&myip, IP_ALEN);
            /* get src & dst ip address */


            dest_ip = device->iplist[i].ipaddr;
            otherip = libnet_name2addr4(libnet, dest_ip, LIBNET_RESOLVE);
            memcpy(dip, (char*)&otherip, IP_ALEN);
            eth = 0;


            /* forge arp data */
            libnet_build_arp(
              ARPHRD_ETHER,
              ETHERTYPE_IP,
              ETH_ALEN, IP_ALEN,
              ARPOP_REQUEST,
              smac, sip,
              dmac, dip,
              NULL, 0,
              libnet,
              arp);

            /* forge ethernet header */
            eth = libnet_build_ethernet(
              dmac, smac,
              ETHERTYPE_ARP,
              NULL, 0,
              libnet,
              eth);


            /* Inject the packet */
            libnet_write(libnet);
            LOG( "function : %s    device->macaddr = %s", __func__, device->macaddr);
            libnet_destroy(libnet);
        }
    }
}


void arp_sniff(devinfo *device)
{
    char *interface = NULL;
    interface = device->interface;
    pcap_t *descr;
    struct bpf_program fp;
    char errbuf[100];

    LOG( "function : %s    device->macaddr = %s", __func__, device->macaddr);

    /* Open interface */
    descr = pcap_open_live(interface, BUFSIZ, 1, SNIFF_TIMEOUT, errbuf);
    if(descr == NULL)
    {
        LOG("pcap_open_live(): %s\n", errbuf);
        exit(1);
    }

    /* Set pcap filter for arp only */
    pcap_compile(descr, &fp, "arp", 0, 0);
    pcap_setfilter(descr, &fp);


    pthread_mutex_lock(&g_scan_mutex);
    pthread_cond_signal(&g_scan_cond);
    pthread_mutex_unlock(&g_scan_mutex);


    /* Start loop for packet capture */
    pcap_dispatch(descr, -1, (pcap_handler)proccess_packet, (u_char *)device);
    pcap_close(descr);

}

void *sniff_process(void *args)
{
    devinfo *device = (devinfo *)args;
    arp_sniff(device);
    return NULL;
}


/* Handle packets recived from pcap_loop */
static void proccess_packet(u_char *args, struct pcap_pkthdr *pkthdr,const u_char *packet)
{
    int i = 0;
    devinfo *device = (devinfo *)args;
    char sip[20];
    char smacaddr[20];
    sprintf(smacaddr, "%02x:%02x:%02x:%02x:%02x:%02x", packet[6],packet[7],packet[8],packet[9],packet[10],packet[11]);
    LOG( "function : %s    smacaddr = %s", __func__, smacaddr);
    LOG( "function : %s    device->macaddr = %s", __func__, device->macaddr);

    LOG( "function : %s    type = %02x%02x", __func__, packet[20], packet[21]);
    if(packet[21] == 2 && packet[20] == 0 && strcmp(device->macaddr, smacaddr) == 0)
    {
        LOG("running 1111111111");
        sprintf(sip, "%d.%d.%d.%d", packet[28],packet[29],packet[30],packet[31]);
        for(i = 0; i < device->ipnum; i++)
        {
            LOG("running 22222222222");
            if(strcmp(device->iplist[i].ipaddr, sip) == 0)
            {
                LOG("running 333333333333");
                device->iplist[i].ipavailable = 1;
            }
        }
    }
}


void ping_process(devinfo *device)
{
    char cmd[50];
    memset(cmd, 0, 50);
    sprintf(cmd, "ping %s -w 1", device->iplist[device->curipnum].ipaddr);

    FILE *pp = popen(cmd, "r");
    int iRet = 0;
    if (!pp){
        return;
    }

    char result[100];
    while (fgets(result, sizeof(result), pp) != NULL)
    {
            DB_LOG( "ping result = %s", result);
    }

    iRet = pclose(pp);
    if(WIFEXITED(iRet))
    {
        if(WEXITSTATUS(iRet) == 0)
        {
            DB_LOG( "WEXITSTATUS(iRet) == %d", WEXITSTATUS(iRet));
            device->iplist[device->curipnum].ipavailable = 1;
        }
        else
        {
            DB_LOG( "WEXITSTATUS(iRet) == %d", WEXITSTATUS(iRet));
            device->iplist[device->curipnum].ipavailable = 0;
        }
    }
}

int netdiscover(devinfo *device)
{
    int i = 0;
    int try_count = 0;
    for(try_count=0; try_count<5;try_count++){
        for(i = 0; i < device->ipnum; i++)
        {
            device->curipnum = i;
			DB_LOG( "function:%s device->curipnum = %d", __func__, device->curipnum);
            ping_process(device);
            if(device->iplist[device->curipnum].ipavailable){
                return 0;
            }
        }
        if(strcmp(device->online, "0") == 0){
            return 0;
        }
        sleep(1);
    }
    return 0;
}
