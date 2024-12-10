#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uci.h>

#include "netdiscover.h"
#include "device_info.h"
#include "utils.h"

int strsub(char *str, char start, char end)
{
	*(str+end) = '\0';
	strcpy(str, str+start);
    return 0;
}

int regex_match(char *str, const char *pattern)
{

	regmatch_t pm[1];
	regex_t preg;
	if(regcomp(&preg, pattern, REG_EXTENDED | REG_NEWLINE) != 0)
	{
		fprintf(stderr, "Cannot regex compile!");
		return -1;
	}

	regexec(&preg, str, 1, pm, REG_NOTEOL);

	strsub(str, pm[0].rm_so, pm[0].rm_eo);

	regfree(&preg);
	return 0;
}

int GetDevNotify(devinfo *device)
{
    struct uci_context *ctx = uci_alloc_context();
    struct uci_package *p = NULL;
    int ret = -1;
    uci_set_confdir(ctx, "/etc/config");
    if(uci_load(ctx, WLDEVLIST, &p) == UCI_OK)
    {
        struct uci_section *device_section = uci_lookup_section(ctx, p, device->macaddr);
        //lookup values
        if(device_section != NULL){
            const char *notify_value = uci_lookup_option_string(ctx, device_section, "notify");
            if(notify_value != NULL){
                ret = 0;
                device->notify = atoi(notify_value);
            }
        }
        uci_unload(ctx,p);
    }
    uci_free_context(ctx);
    return ret;
}


int GetDevIp(devinfo *device)
{
	char arp_item[ARP_ITEM_MAX_LENGTH];
	FILE *fd = NULL;

    DB_LOG( "run into GetDevIp++++++------");

	fd = fopen(ARP_PATH, "r");

	if(fd)
	{
		while( fgets(arp_item, ARP_ITEM_MAX_LENGTH, fd) )
		{
			DB_LOG( "arp_item = %s", arp_item);
			DB_LOG( "dev->macaddr = %s", device->macaddr);

			if(strstr(arp_item, device->macaddr))
			{
				DB_LOG( "arp_item = %s", arp_item);
				regex_match(arp_item, IP_PATTERN);
				strcpy(device->iplist[device->ipnum].ipaddr, arp_item);
				device->ipnum++;
			}
		}
		fclose(fd);
	}
	else
	{
		fprintf(stderr, "open file failed!");
		return -1;
	}
	return 0;
}

int check_ip_available(devinfo *device)
{
    int ret = 0;
    ret = netdiscover(device);
    return ret;
}

int32_t GetDevName(devinfo *device)
{
    memset(device->hostname, 0, sizeof(device->hostname));
    char dhcp_item[DHCP_ITEM_MAX_LENGTH];
    FILE *fd = NULL;
	char *devname = NULL;
	char *ipaddr = NULL;

    fd = fopen(DHCP_PATH, "r");
    if(fd)
    {
        while(fgets(dhcp_item, DHCP_ITEM_MAX_LENGTH, fd))
        {
            DB_LOG( "=function:%s=======dhcp_item ====%s===", __func__, dhcp_item);
			if(strstr(dhcp_item, device->macaddr))
			{
            	regex_match(dhcp_item, NAME_PATTERN);
				ipaddr = strtok(dhcp_item, " ");
				devname = strtok(NULL, " ");
                DB_LOG( "======devname===%s===", devname);
                strcpy(device->hostname, devname);

				break;
			}
        }
        fclose(fd);
    }
    else
    {
        fprintf(stderr, "Open /tmp/dhcp.leases failed!");
        return -1;
    }
    return 0;
}



int GetDevLowInfo(devinfo *device)
{
    int i = 0;
    GetDevNotify(device);

    int try_count = 0;
    for(try_count=0; try_count<20; try_count++){
        GetDevIp(device);
        if(device->ipnum){
            break;
        }
        sleep(1);
    }
//    sleep(2);
	check_ip_available(device);

	GetDevName(device);

    DB_LOG("hostname = %s\n", device->hostname);
    DB_LOG("ipnum = %d\n", device->ipnum);
    for(i=0;i<device->ipnum;i++)
    {
        DB_LOG( "iplist[%d] = %s, ipavailable = %d, dhcp = %d\n", i, device->iplist[i].ipaddr, device->iplist[i].ipavailable, device->iplist[i].dhcp);
    }
    return 0;
}
