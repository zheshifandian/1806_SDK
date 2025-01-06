/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  call the wireless extension' cmd to make clear how the drivers(mac80211) support
 *                  wext
 *
 *        Version:  1.0
 *        Created:  02/22/2016 03:56:43 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  franklin , franklin.wang@siflower.com.cn
 *        Company:  Siflower Communication Tenology Co.,Ltd
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/wireless.h>
#include <sys/time.h>
//#include <getopt.h>
//#define IFNAME "wlan0"
char ifname[16] = {"wlan0"};

char beacon[192] = {
	0x80, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x16, 0x88, 0x08, 0x07, 0xf9,
	0x00, 0x16, 0x88, 0x08, 0x07, 0xf9, 0x20, 0x5e, 0x5f, 0x20, 0x31, 0x3b, 0x00, 0x00, 0x00, 0x00,
	0x64, 0x00, 0x01, 0x00, 0x00, 0x06, 0x6f, 0x70, 0x65, 0x6e, 0x35, 0x67, 0x01, 0x08, 0x8c, 0x12,
	0x98, 0x24, 0xb0, 0x48, 0x60, 0x6c, 0x03, 0x01, 0x9d, 0x07, 0x06, 0x4e, 0x6f, 0x20, 0x24, 0x0d,
	0x14, 0x05, 0x04, 0x00, 0x01, 0x00, 0x00, 0x2d, 0x1a, 0xec, 0x01, 0x17, 0xff, 0xff, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x3d, 0x16, 0x9d, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xbf, 0x0c, 0xa0, 0x01, 0xc0,
	0x31, 0xfa, 0xff, 0x0c, 0x03, 0xfa, 0xff, 0x0c, 0x03, 0xc0, 0x05, 0x00, 0x00, 0x00, 0xf5, 0xff,
	0x7f, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0xdd, 0x18, 0x00, 0x50, 0xf2, 0x02,
	0x01, 0x01, 0x00, 0x00, 0x03, 0xa4, 0x00, 0x00, 0x27, 0xa4, 0x00, 0x00, 0x42, 0x43, 0x5e, 0x00,
	0x62, 0x32, 0x2f, 0x00, 0xdd, 0x07, 0x00, 0x0c, 0x43, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#define IS_24G 1
#define ETH_P_SFCFG 0x1688
#define SFCFG_MAGIC_NO      0x18181688
#define FAC_MODE 200
#define NOR_MODE 201
#define SLE_MODE 202
#define TMP_MODE 203
#define ANT1 1
#define ANT2 2
#define USE_ONE_CALI_TABLE 0
/*  command id */
#define SFCFG_CMD_ATE_START				 0x0000
#define SFCFG_CMD_ATE_STOP				 0x0001
#define SFCFG_CMD_ATE_TX_START			 0x0002
#define SFCFG_CMD_ATE_TX_STOP			 0x0003
#define SFCFG_CMD_ATE_RX_START			 0x0004
#define SFCFG_CMD_ATE_RX_STOP			 0x0005
#define SFCFG_CMD_ATE_TX_FRAME_START	 0x0006
#define SFCFG_CMD_ATE_RX_FRAME_START	 0x0007
#define SFCFG_CMD_ATE_MACBYPASS_TX_START 0x0008
#define SFCFG_CMD_ATE_MACBYPASS_TX_STOP  0x0009
#define SFCFG_CMD_ATE_TX_TEST_TONE_START 0x000a
#define SFCFG_CMD_ATE_TX_TEST_TONE_STOP  0x000b
#define SFCFG_CMD_ATE_MACBYPASS_RX_START 0x000c
#define SFCFG_CMD_ATE_MACBYPASS_RX_STOP  0x000d

//define 0x20  command_type set tx vector
//define command_id by command_tpye and command_idx
//command_idx  is index in txvector in rwnx_ioctl.c
#define SFCFG_CMD_ATE_TXVECTOR_PARAM 0x0020
#define SFCFG_CMD_ATE_TXVECTOR_PARAM_NSS 0x0021

#define SFCFG_CMD_ATE_SET_BANDWIDTH		0x0100
#define SFCFG_CMD_ATE_SET_CHANNEL		0x0101
#define SFCFG_CMD_ATE_SET_PHY_MODE		0x0102
#define SFCFG_CMD_ATE_SET_RATE			0x0103
#define SFCFG_CMD_ATE_SET_PREAMBLE		0x0104
#define SFCFG_CMD_ATE_SET_GI			0x0105
#define SFCFG_CMD_ATE_SET_TX_POWER		0x0106
#define SFCFG_CMD_ATE_GET_INFO			0x0107
#define SFCFG_CMD_ATE_SAVE_TO_FLASH		0x0108
#define SFCFG_CMD_ATE_SET_CENTER_FREQ1	0x0109
#define SFCFG_CMD_ATE_READ_FROM_FLASH 0X10b
#define SFCFG_CMD_ATE_SAVE_DATA_TO_FLASH 0X10c
#define SFCFG_CMD_ATE_GET_DATA_FROM_FLASH 0x010d

#define SFCFG_CMD_ATE_GET_POWER 0x0110
#define SFCFG_CMD_ATE_WHOLE_FRAME			0x1000
#define SFCFG_CMD_ATE_TX_COUNT				0x1001
#define SFCFG_CMD_ATE_PAYLOAD_LENGTH		0x1002
#define SFCFG_CMD_ATE_TX_FC					0x1003
#define SFCFG_CMD_ATE_TX_DUR				0x1004
#define SFCFG_CMD_ATE_TX_BSSID				0x1005
#define SFCFG_CMD_ATE_TX_DA					0x1006
#define SFCFG_CMD_ATE_TX_SA					0x1007
#define SFCFG_CMD_ATE_TX_SEQC				0x1008
#define SFCFG_CMD_ATE_PAYLOAD				0x1009
#define SFCFG_CMD_ATE_TX_FRAME_BW			0x100a
#define SFCFG_CMD_ATE_MACBYPASS_INTERVAL    0x100b
#define SFCFG_CMD_ATE_XO_VALUE  0X100C
#define SFCFG_CMD_ATE_AET_NUM   0X100D

#define SFCFG_CMD_ATE_READ_REG       0x10000
#define SFCFG_CMD_ATE_WRITE_REG      0x10001
#define SFCFG_CMD_ATE_BULK_READ_REG  0x10002
#define SFCFG_CMD_ATE_BULK_WRITE_REG 0x10003

#define SFCFG_PRIV_IOCTL_ATE	  (SIOCIWFIRSTPRIV + 0x08)


#define true  1
#define false 0
#define PRINT_ATE_LOG 0
typedef unsigned char   bool;
typedef unsigned char   uint8_t;
typedef int    int32_t;
typedef unsigned int   uint32_t;
typedef unsigned short  uint16_t;
typedef void* caddr_t;

enum register_mode {
	MAC_REG,
	PHY_REG,
	RF_REG,
};

static uint8_t  power_table[2048] = { 0 };


bool g_tx_frame_flag = false;
bool g_rx_frame_flag = false;
bool g_stop_tx_frame_flag = false;
bool g_stop_rx_frame_flag = false;
bool g_clock_flag = false;
bool g_start_tx_frame_by_macbypass_flag = false;
bool g_stop_tx_frame_by_macbypass_flag = false;
bool g_start_rx_by_macbypass_flag = false;
bool g_stop_rx_by_macbypass_flag = false;
bool g_tx_test_tone_flag = false;
bool g_stop_tx_test_tone_flag = false;
bool g_agg_tx_frame_flag = false;

unsigned int g_tx_frame_num  = 1;
unsigned int g_tx_chan_freq  = 2412;
unsigned int g_tx_band_width = 0;
unsigned int g_tx_mode       = 0;
unsigned int g_tx_rate_idx   = 0;
unsigned int g_tx_use_sgi    = 0;
unsigned int g_tx_power    = 0;

static int32_t ioctl_socket = -1;
#if 0
static  uint8_t *sa = "16:88:aa:bb:cc:ee";
static  uint8_t *da = "16:88:aa:bb:cc:ee";
static  uint8_t *bssid = "16:88:aa:bb:cc:ee";
#endif


struct tx_frame_param {
	int frame_num;
};

enum ieee80211_band {
	IEEE80211_BAND_2GHZ =0,
	IEEE80211_BAND_5GHZ,
	IEEE80211_BAND_60GHZ,

	/* keep last */
	IEEE80211_NUM_BANDS
};


enum nl80211_chan_width {
	NL80211_CHAN_WIDTH_20_NOHT,
	NL80211_CHAN_WIDTH_20,
	NL80211_CHAN_WIDTH_40,
	NL80211_CHAN_WIDTH_80,
	NL80211_CHAN_WIDTH_80P80,
	NL80211_CHAN_WIDTH_160,
};


enum format_mode {
	NON_HT_MODE = 0,
	NON_HT_DUP_MODE,
	HT_MM_MODE,
	HT_GF_MODE,
	VHT_MODE,
};


struct rwnx_ate_channel_params{
	enum ieee80211_band chan_band;
	unsigned int chan_freq;
	enum nl80211_chan_width chan_width;
};


struct rwnx_ate_rate_params{
	enum format_mode mode;
	unsigned int rate_idx;
	bool use_short_preamble; //only for 2.4G CCK mode rate_idx 1~3
	bool use_short_gi;       //for HT VHT mode but not HT_GF_MODE
};

struct rwnx_ate_packet_params{
	unsigned int frame_num;
	unsigned int frame_len;
	unsigned int tx_power;
};

struct rwnx_ate_address_params{
	unsigned char bssid[6];
	unsigned char da[6];
	unsigned char sa[6];
	unsigned char addr4[6];

};

struct sfcfghdr {
	uint32_t magic_no;
	uint32_t comand_type;
	uint32_t comand_id;
	uint16_t length;
	uint16_t sequence;
	uint32_t status;
	char  data[4096];
}__attribute__((packed));



//back to user space  /sync to user space parameter
struct rwnx_ioctl_ate_dump_info {
	int bandwidth;
	int frame_bandwidth;
	int band;
	int freq;
	int freq1;
	int rate;
	int mode;
	int gi;
	int pre;
	int power;
	int len;
	int rssi;
	int reg_status;
	uint32_t xof1;
	uint32_t xof2;
	uint32_t count;
	uint32_t tx_cont;
	uint32_t tx_successful;
	uint32_t tx_retry;
	uint32_t rec_rx_count;
	uint32_t fcs_err;
	uint32_t per;
	uint32_t phy_err;
	uint32_t fcs_ok;
	uint32_t fcs_ok_for_macaddr;
	uint32_t fcs_group;
	uint32_t reg_val;
	uint8_t  bssid[12];
	uint8_t  da[12];
	uint8_t  sa[12];
}__attribute__((packed));

/* Convert string(char) into decimal int*/
static int siwifi_char_to_int(char *tmp_data)
{
    int tmp_int = 0;

    while (*tmp_data != 0) {
        if (*tmp_data >= '0' && *tmp_data <= '9') {
            tmp_int = tmp_int * 10 + (*tmp_data - '0');
            tmp_data++;
        } else {
            return -1;
        }
    }
    return tmp_int;
}

/* create the socket fd to wext ioctl*/
static int32_t get_ioctl_socket(void)
{
	/*  Prepare socket */
	if (ioctl_socket == -1)
	{
		ioctl_socket = socket(AF_INET, SOCK_DGRAM, 0);
		fcntl(ioctl_socket, F_SETFD, fcntl(ioctl_socket, F_GETFD) | FD_CLOEXEC);
	}

	return ioctl_socket;
}

/* close the ioctl socket fd*/
void close_ioctl_socket(void)
{
	if (ioctl_socket > -1)
		close(ioctl_socket);
	ioctl_socket = -1;
}


/* do the ioctl*/
int32_t do_ioctl(int32_t cmd, struct iwreq *wrq)
{
	int32_t s = get_ioctl_socket();
	if(!wrq){
		printf("%s error:wrq is NULL\n",__func__);
		return -1;
	}
	strncpy(wrq->ifr_name, ifname, IFNAMSIZ);
	if(s < 0)
		return s;
	return ioctl(s, cmd, wrq);
}


/*
  descript : get power offset in factory

 * */
uint32_t get_power_offset(uint32_t channel,uint32_t mode,uint32_t bw,uint32_t rate){

	uint32_t offset  = 0;
    if (channel < 3000) {
        offset = (channel - 2412) / 5 * 28;
        switch (mode) {
        case 0:
            offset += rate;
            break;
        case 2:
            if (bw == 0 || bw ==1)
                offset += 12 + rate;
            else
                offset += 20 + rate;
            break;
        }
    } else {
        switch (channel) {
        case 4940:
            offset = 364 + 53 * 0;
            break;
        case 5040:
            offset = 364 + 53 * 0;
            break;
        case 5180:
            offset = 364 + 53 * 0;
            break;
        case 5200:
            offset = 364 + 53 * 1;
            break;
        case 5220:
            offset = 364 + 53 * 2;
            break;
        case 5240:
            offset = 364 + 53 * 3;
            break;
        case 5260:
            offset = 364 + 53 * 4;
            break;
        case 5280:
            offset = 364 + 53 * 5;
            break;
        case 5300:
            offset = 364 + 53 * 6;
            break;
        case 5320:
            offset = 364 + 53 * 7;
            break;
        case 5500:
            offset = 364 + 53 * 8;
            break;
        case 5520:
            offset = 364 + 53 * 9;
            break;
        case 5540:
            offset = 364 + 53 * 10;
            break;
        case 5560:
            offset = 364 + 53 * 11;
            break;
        case 5580:
            offset = 364 + 53 * 12;
            break;
        case 5600:
            offset = 364 + 53 * 13;
            break;
        case 5620:
            offset = 364 + 53 * 14;
            break;
        case 5640:
            offset = 364 + 53 * 15;
            break;
        case 5660:
            offset = 364 + 53 * 16;
            break;
        case 5680:
            offset = 364 + 53 * 17;
            break;
        case 5700:
            offset = 364 + 53 * 18;
            break;
        case 5720:
            offset = 364 + 53 * 19;
            break;
        case 5745:
            offset = 364 + 53 * 20;
            break;
        case 5765:
            offset = 364 + 53 * 21;
            break;
        case 5785:
            offset = 364 + 53 * 22;
            break;
        case 5805:
            offset = 364 + 53 * 23;
            break;
        case 5825:
            offset = 364 + 53 * 24;
            break;
        case 5905:
            offset = 364 + 53 * 24;
            break;
        case 5925:
            offset = 364 + 53 * 24;
            break;
        }
        switch (mode) {
        case 0:
            offset += rate;
            break;
        case 2:
            if (bw == 0 || bw ==1)
                offset += 8 + rate;
            else
                offset += 16 + rate;
            break;
        case 4:
            if (bw == 0 || bw ==1)
                offset += 24 + rate;
            else if (bw == 2)
                offset += 33 + rate;
            else
                offset += 43 + rate;
            break;
        }
    }
return  offset;
}

int32_t read_gain_table_from_mtd(unsigned char *buffer){
	struct iwreq wrq;
	struct sfcfghdr tmp;
	//uint32_t offset = 0;
	int i ;
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));

	//read gian table from mtd
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_READ_FROM_FLASH;
	tmp.comand_id   = SFCFG_CMD_ATE_READ_FROM_FLASH;
#if USE_ONE_CALI_TABLE
	tmp.length	= 2044;
#else
	tmp.length      = 3382;
#endif
	tmp.sequence    = 1;

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE,&wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_READ_FROM_FLASH failed\n");
		return -1;
	}
#if USE_ONE_CALI_TABLE
	memcpy(buffer,&tmp.data[0],2044);
	printf("read tx power is : \n");
	for(i = 0;i < 1693;i++)
	{
		if( i >= 0 && i <= 3 )
		{
			printf("%d ",buffer[i]);
			if(i == 3)
			 printf("\n");
		}
		else if (i > 3 && i < 368)
		{
			printf("%d ",buffer[i]);
			if(i%28 == 3)
			  printf("\n");
		}
		else {
			printf("%d ",buffer[i]);
			if((i-367)%53 == 0)
			  printf("\n");
		}
	}
#else
	memcpy(buffer,&tmp.data[0],3382);
	printf("read tx power is : \n");
	for(i = 0;i < 3382;i++)
	{
		if( i >= 0 && i <= 3 )
		{
			printf("%d ",buffer[i]);
			if(i == 3)
			 printf("\n");
		}
		else if (i > 3 && i < 368)
		{
			if(i == 4)
			  printf("-----2.4G ant1 calibration power-----\r\n");
			printf("%d ",buffer[i]);
			if(i%28 == 3)
			  printf("\n");
		}
		else if(i >= 368 && i < 1693)
		{
			if(i == 368)
			  printf("-----5.8G ant1 calibration power-----\r\n");
			printf("%d ",buffer[i]);
			if((i-367)%53 == 0)
			  printf("\n");
		}
		else if(i >= 1693 && i < 2057)
		{
			if(i == 1693)
			  printf("-----2.4G ant2 calibration power-----\r\n");
			printf("%d ",buffer[i]);
			if((i-1692)%28 == 0)
			  printf("\n");
		}
		else if(i >= 2057 && i < 3382)
		{
			if(i == 2057)
			  printf("-----5.8G ant2 calibration power-----\r\n");
			printf("%d ",buffer[i]);
			if((i-2056)%53 == 0)
			  printf("\n");
		}
	}
#endif
	return 0;
}

int32_t save_version_to_factory(char *version)
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));

	// cmd ate band
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_SAVE_DATA_TO_FLASH;
	tmp.comand_id   = SFCFG_CMD_ATE_SAVE_DATA_TO_FLASH;
	tmp.length      = 2;
	tmp.sequence    = 1;


	sprintf(tmp.data,"%4d",2048);
	memcpy(&tmp.data[4],version,2);
	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + 10;

  	printf("tmp.data------%s\n",tmp.data);

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		 printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_SAVE_TO_FLASH failed\n");
		  return -1;
		}

	return 0;
}


int32_t save_power_table_to_factory(uint32_t length,uint32_t offset){
	struct iwreq wrq;
	struct sfcfghdr tmp;
	uint32_t offset_tmp = offset;
	FILE *p1,*p2;
	int i;
	char power_text[4096]={ 0 };
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));

	// cmd ate band
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_SAVE_DATA_TO_FLASH;
	tmp.comand_id   = SFCFG_CMD_ATE_SAVE_DATA_TO_FLASH;
#if USE_ONE_CALI_TABLE
	tmp.length 	= 2044;
#else
	tmp.length      = 4092;
#endif
	tmp.sequence    = 1;
#if USE_ONE_CALI_TABLE
	//read from txt
	p1 = fopen("/etc/atetools/power_save_ant1.txt","r");
	for(i=0;i < 1689; i++)
	{
		fscanf(p1,"%hhd",&power_text[i]);
	}

	sprintf(tmp.data,"%4d",offset_tmp + 2048 + 4);//factory form 2048 begin and 4 bytes form 2048 to 2052 is save xo;
	memcpy(&tmp.data[4],power_text,length);
#else
	p1 = fopen("/etc/atetools/power_save_ant1.txt","r");
	for(i=0;i < 1689; i++)
	{
		fscanf(p1,"%hhd",&power_text[i]);
	}

	p2 = fopen("/etc/atetools/power_save_ant2.txt","r");
	for(i=1689;i < 3378; i++)
	{
		fscanf(p2,"%hhd",&power_text[i]);
	}

	sprintf(tmp.data,"%4d",offset_tmp + 2048 + 4);//factory form 2048 begin and 4 bytes form 2048 to 2052 is save xo;
	memcpy(&tmp.data[4],power_text,length);
#endif
	printf("power_text[0] :%d",power_text[0]);
	printf("power_text[1689] :%d",power_text[1689]);
	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	printf("length :%d\n",length);
  	//printf("tmp.data------%s\n",tmp.data);

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		 printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_SAVE_TO_FLASH failed\n");
		  return -1;
		}
#if USE_ONE_CALI_TABLE
	fclose(p1);
#else
        fclose(p1);
	fclose(p2);
#endif
	return 0;
}

struct calibrate_info
{
	uint32_t channel;
	uint32_t mode;
	uint32_t bw;
	uint32_t rate;
};

struct calibrate_info  get_ate_calibrate_info()
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	struct rwnx_ioctl_ate_dump_info *usdrv;
	struct calibrate_info n;

	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));
	// cmd ate band
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_GET_INFO;
	tmp.comand_id   = SFCFG_CMD_ATE_GET_INFO;
	tmp.length      = 10;
	tmp.sequence    = 1;

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE,&wrq) < 0){
		printf("SFCFG_CMD_ATE_GET_INFO ioctl failed\n");
		//return -1;
	}
    	usdrv = (struct rwnx_ioctl_ate_dump_info *)malloc(sizeof(struct rwnx_ioctl_ate_dump_info));
   	if(!usdrv){
        printf("oom!\n");
        //return -2;
    }

	memset(usdrv,0,sizeof(struct rwnx_ioctl_ate_dump_info));
	memcpy(usdrv, wrq.u.data.pointer, sizeof(struct rwnx_ioctl_ate_dump_info));

	printf("bandwidth = %d\n",usdrv->bandwidth);
	printf("freq = %d\n",usdrv->freq);
	printf("rate = %d\n",usdrv->rate);
	printf("mode = %d\n",usdrv->mode);
	n.channel = usdrv->freq;
	n.mode = usdrv->mode;
	n.bw = usdrv->bandwidth;
	n.rate = usdrv->rate;

	free(usdrv);
	return n;
}

int32_t get_ate_info()
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	struct rwnx_ioctl_ate_dump_info *usdrv;

	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));
	// cmd ate band
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_GET_INFO;
	tmp.comand_id   = SFCFG_CMD_ATE_GET_INFO;
	tmp.length      = 10;
	tmp.sequence    = 1;

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE,&wrq) < 0){
		printf("SFCFG_CMD_ATE_GET_INFO ioctl failed\n");
		return -1;
	}
    usdrv = (struct rwnx_ioctl_ate_dump_info *)malloc(sizeof(struct rwnx_ioctl_ate_dump_info));
    if(!usdrv){
        printf("oom!\n");
        return -2;
    }

	memset(usdrv,0,sizeof(struct rwnx_ioctl_ate_dump_info));
	memcpy(usdrv, wrq.u.data.pointer, sizeof(struct rwnx_ioctl_ate_dump_info));

	printf("bandwidth = %d\n",usdrv->bandwidth);
	printf("frame_bandwidth = %d\n",usdrv->frame_bandwidth);
	printf("band = %d\n",usdrv->band);
	printf("freq = %d\n",usdrv->freq);
	printf("freq = %d\n",usdrv->freq1);
	printf("rate = %d\n",usdrv->rate);
	printf("mode = %d\n",usdrv->mode);
	printf("GI = %d\n",usdrv->gi);
	printf("preamble = %d\n",usdrv->pre);
	printf("power = %d\n",usdrv->power);
	printf("length = %d\n",usdrv->len);
	printf("rssi = %d\n",usdrv->rssi);
	printf("reg_status = %d\n",usdrv->reg_status);
	printf("xof1 = %d\n",usdrv->xof1);
	printf("xof2 = %d\n",usdrv->xof2);
	printf("total count = %d\n",usdrv->count);
	printf("tx_cont = %d\n",usdrv->tx_cont);
	printf("tx_successful = %d\n",usdrv->tx_successful);
	printf("tx_retry = %d\n",usdrv->tx_retry);
	printf("rec_rx_count = %d\n",usdrv->rec_rx_count);
	printf("fcs_err = %d\n",usdrv->fcs_err);
	printf("per = %d\n",usdrv->per);
	printf("phy_err = %d\n",usdrv->phy_err);
	printf("reg_val = %d\n",usdrv->reg_val);
	printf("bssid = 0x%02x : 0x%02x : 0x%02x : 0x%02x : 0x%02x : 0x%02x\n",usdrv->bssid[0],usdrv->bssid[1],usdrv->bssid[2],usdrv->bssid[3],usdrv->bssid[4],usdrv->bssid[5]);
	printf("da = 0x%02x : 0x%02x : 0x%02x : 0x%02x : 0x%02x : 0x%02x\n",usdrv->da[0],usdrv->da[1],usdrv->da[2],usdrv->da[3],usdrv->da[4],usdrv->da[5]);
	printf("sa = 0x%02x : 0x%02x : 0x%02x : 0x%02x : 0x%02x : 0x%02x\n",usdrv->sa[0],usdrv->sa[1],usdrv->sa[2],usdrv->sa[3],usdrv->sa[4],usdrv->sa[5]);
    free(usdrv);
	return 0;
}

int32_t read_iw_reg(char *tmpdata)
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));
	// cmd ate read reg
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_READ_REG;
	tmp.comand_id   = SFCFG_CMD_ATE_READ_REG;
	tmp.length      = 20;
	tmp.sequence    = 1;

	memcpy(tmp.data,tmpdata,20);
	printf("tmp.data= %s---------\n",tmp.data);

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_READ_REG failed\n");
		return -1;
	}
#if 0
	//get the results
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));

	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_GET_INFO;
	tmp.comand_id   = SFCFG_CMD_ATE_GET_INFO;
	tmp.length      = 10;
	tmp.sequence    = 1;
	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;
	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE,&wrq) < 0){
		printf("SFCFG_CMD_ATE_GET_INFO ioctl failed\n");
		return -1;
	}
	memset(&user_reg,0,sizeof(struct rwnx_ioctl_ate_dump_info));
	memcpy(&user_reg, wrq.u.data.pointer, sizeof(struct rwnx_ioctl_ate_dump_info));
	printf("register value = 0x%x\n",user_reg.reg_val);
#endif
	return 0;
}

int32_t write_iw_reg(char *tmpdata)
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));
	// cmd ate read reg
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_WRITE_REG;
	tmp.comand_id   = SFCFG_CMD_ATE_WRITE_REG;
	tmp.length      = 21;
	tmp.sequence    = 1;

	memcpy(tmp.data,tmpdata,tmp.length);
	printf("tmp.data= %s---------\n",tmp.data);

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_WRITE_REG failed\n");
		return -1;
	}
#if 0
	//get the results
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));

	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_GET_INFO;
	tmp.comand_id   = SFCFG_CMD_ATE_GET_INFO;
	tmp.length      = 10;
	tmp.sequence    = 1;
	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;
	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE,&wrq) < 0){
		printf("SFCFG_CMD_ATE_GET_INFO ioctl failed\n");
		return -1;
	}
	memset(&user_reg,0,sizeof(struct rwnx_ioctl_ate_dump_info));
	memcpy(&user_reg, wrq.u.data.pointer, sizeof(struct rwnx_ioctl_ate_dump_info));
	if(user_reg.reg_status == 0){
		printf("write register %x done!\n",user_reg.reg_val);
	}
	else{
		printf("write register fail!\n");
	}
#endif
	return 0;
}

int32_t read_iw_bulk_reg(char *tmpdata)
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	struct rwnx_ioctl_ate_dump_info user_reg;
	int num = 0;
	int type = 0;
	int space = 0;
	int i = 0;
	char addrs[1000] = {0};
	char tmp_addr[9] = {0};
	if(tmpdata[0]=='0'){
		type = MAC_REG;
		space = 8;}
	else if(tmpdata[0]=='1'){
		type = PHY_REG;
		space = 8;}
	else{type = 2;
		space = 4;
	}
	num = atoi(tmpdata+2);
	memcpy(addrs,tmpdata+4,sizeof(addrs));
	while(i<num){
		memcpy(tmp_addr,tmpdata+4+i*(space+1),space);
		memset(&wrq, 0, sizeof(struct iwreq));
		memset(&tmp, 0, sizeof(struct sfcfghdr));

		// cmd ate read reg
		tmp.magic_no = SFCFG_MAGIC_NO;
		tmp.comand_type = SFCFG_CMD_ATE_READ_REG;
		tmp.comand_id   = SFCFG_CMD_ATE_READ_REG;
		tmp.length      = 8;
		tmp.sequence    = 1;

		memcpy(tmp.data,tmp_addr,tmp.length);

		wrq.u.data.pointer = (caddr_t)&tmp;
		wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;
		if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
			printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_READ_BULK_REG failed\n");
			return -1;
		}
		//get the results
		memset(&wrq, 0, sizeof(struct iwreq));
		memset(&tmp, 0, sizeof(struct sfcfghdr));

		tmp.magic_no = SFCFG_MAGIC_NO;
		tmp.comand_type = SFCFG_CMD_ATE_GET_INFO;
		tmp.comand_id   = SFCFG_CMD_ATE_GET_INFO;
		tmp.length      = 10;
		tmp.sequence    = 1;
		wrq.u.data.pointer = (caddr_t)&tmp;
		wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;
		if(do_ioctl(SFCFG_PRIV_IOCTL_ATE,&wrq) < 0){
			printf("SFCFG_CMD_ATE_GET_INFO ioctl failed\n");
			return -1;
		}
		memset(&user_reg,0,sizeof(struct rwnx_ioctl_ate_dump_info));
		memcpy(&user_reg, wrq.u.data.pointer, sizeof(struct rwnx_ioctl_ate_dump_info));
		printf("register:0x%s value = 0x%x\n",tmp_addr,user_reg.reg_val);
		i++;
	}
	return 0;
}


int32_t write_iw_bulk_reg(char *tmpdata)
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	struct rwnx_ioctl_ate_dump_info user_reg;
	int num = 0;
	int type = 0;
	int space = 0;
	int i = 0;
	char addrs[1000] = {0};
	char tmp_addr_val[18] = {0};
	char tmp_addr[9] = {0};
	if(tmpdata[0]=='0'){
		type = MAC_REG;
		space = 17;}
	else if(tmpdata[0]=='1'){
		type = PHY_REG;
		space = 17;}
	else{type = RF_REG;
		space = 9;
	}
	num = atoi(tmpdata+2);
	memcpy(addrs,tmpdata+4,sizeof(addrs));
	while(i<num){
		memcpy(tmp_addr_val,tmpdata+4+i*(space+1),space);
		memcpy(tmp_addr,tmp_addr_val,(space-1)/2);
		memset(&wrq, 0, sizeof(struct iwreq));
		memset(&tmp, 0, sizeof(struct sfcfghdr));

		// cmd ate write reg
		tmp.magic_no = SFCFG_MAGIC_NO;
		tmp.comand_type = SFCFG_CMD_ATE_WRITE_REG;
		tmp.comand_id   = SFCFG_CMD_ATE_WRITE_REG;
		tmp.length      = space;
		tmp.sequence    = 1;

		memcpy(tmp.data,tmp_addr_val,tmp.length);

		wrq.u.data.pointer = (caddr_t)&tmp;
		wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;
		if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
			printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_WRITE_BULK_REG failed\n");
			return -1;
		}
		//get the results
		memset(&wrq, 0, sizeof(struct iwreq));
		memset(&tmp, 0, sizeof(struct sfcfghdr));

		tmp.magic_no = SFCFG_MAGIC_NO;
		tmp.comand_type = SFCFG_CMD_ATE_GET_INFO;
		tmp.comand_id   = SFCFG_CMD_ATE_GET_INFO;
		tmp.length      = 10;
		tmp.sequence    = 1;
		wrq.u.data.pointer = (caddr_t)&tmp;
		wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;
		if(do_ioctl(SFCFG_PRIV_IOCTL_ATE,&wrq) < 0){
			printf("SFCFG_CMD_ATE_GET_INFO ioctl failed\n");
			return -1;
		}
		memset(&user_reg,0,sizeof(struct rwnx_ioctl_ate_dump_info));
		memcpy(&user_reg, wrq.u.data.pointer, sizeof(struct rwnx_ioctl_ate_dump_info));
		if(user_reg.reg_status == 0){
			printf("write register:0x%s done!\n",tmp_addr);
		}
		else{
			printf("write register:0x%s fail!\n",tmp_addr);
		}
		i++;
	}
	return 0;
}


int32_t set_phy_bandwidth(char *tmpdata)
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));
	// cmd ate bandwidth
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_SET_BANDWIDTH;
	tmp.comand_id   = SFCFG_CMD_ATE_SET_BANDWIDTH;
	tmp.length      = 10;
	tmp.sequence    = 1;

	memcpy(tmp.data,tmpdata,10);
	//printf("tmp.data= %s---------\n",tmp.data);

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_SET_BANDWIDTH failed\n");
		return -1;
	}

	return 0;
}


int32_t set_phy_chan_freq(char *tmpdata)
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));
	// cmd ate band
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_SET_CHANNEL;
	tmp.comand_id   = SFCFG_CMD_ATE_SET_CHANNEL;
	tmp.length      = 10;
	tmp.sequence    = 1;

	strncpy(tmp.data,tmpdata,10);
	//printf("tmp.data= %s---------\n", tmp.data);

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_SET_CHANNEL failed\n");
		return -1;
	}
	return 0;

}


int32_t set_phy_center_freq1(char *tmpdata)
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));
	// cmd ate band
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_SET_CENTER_FREQ1;
	tmp.comand_id   = SFCFG_CMD_ATE_SET_CENTER_FREQ1;
	tmp.length      = 10;
	tmp.sequence    = 1;

	strncpy(tmp.data,tmpdata,10);
	//printf("tmp.data= %s---------\n", tmp.data);

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_SET_CENTER_FREQ1 failed\n");
		return -1;
	}
	return 0;

}

int32_t set_phy_mode(char *tmpdata)
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));
	// cmd ate band
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_SET_PHY_MODE;
	tmp.comand_id   = SFCFG_CMD_ATE_SET_PHY_MODE;
	tmp.length      = 10;
	tmp.sequence    = 1;

	memcpy(tmp.data,tmpdata, 10);
	//printf("tmp.data= %s---------\n", tmp.data);

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_SET_PHY_MODE failed\n");
		return -1;
	}

	return 0;

}


int32_t set_phy_rate_idx(char *tmpdata)
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));
	// cmd ate band
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_SET_RATE;
	tmp.comand_id   = SFCFG_CMD_ATE_SET_RATE;
	tmp.length      = 10;
	tmp.sequence    = 1;

	memcpy(tmp.data,tmpdata, 10);
	//printf("tmp.data= %s---------\n", tmp.data);

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_SET_RATE failed\n");
		return -1;
	}
	return 0;

}


int32_t set_phy_use_sgi(char *tmpdata)
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));
	// cmd ate band
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_SET_GI;
	tmp.comand_id   = SFCFG_CMD_ATE_SET_GI;
	tmp.length      = 10;
	tmp.sequence    = 1;

	memcpy(tmp.data,tmpdata, 10);
	//printf("tmp.data= %s---------\n", tmp.data);

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_SET_GI failed\n");
		return -1;
	}
	return 0;

}


int32_t set_phy_preamble(char *tmpdata)
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));
	// cmd ate band
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_SET_PREAMBLE;
	tmp.comand_id   = SFCFG_CMD_ATE_SET_PREAMBLE;
	tmp.length      = 10;
	tmp.sequence    = 1;

	memcpy(tmp.data,tmpdata, 10);
	//printf("tmp.data= %s---------\n", tmp.data);

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_SET_PREAMBLE failed\n");
		return -1;
	}
	return 0;

}
/* *
 *gain mode value
 *0-----------use factory table
 *1-----------use low table
 *2-----------use sleep table
 *3-----------use normal tabele
 *band vlaue
 *0-----------undifferentiated antenna
 *1-----------ant1
 *2-----------ant2
 * */
int32_t get_factory_power(char *channel,char *bw,char *mode,char *mcs,char *gain_mode,char *band){
	struct iwreq wrq;
	struct sfcfghdr tmp;
	int power = 0;
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));
	//cmd get factory power;
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_GET_POWER;
	tmp.comand_id   = SFCFG_CMD_ATE_GET_POWER;
	tmp.length      = 20;
	tmp.sequence    = 1;

	sprintf(tmp.data,"%s %s %s %s %s %s",channel,bw,mode,mcs,gain_mode,band);
	printf("tmp.data==== %s--------------\n",tmp.data);
	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
	  printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_GET_POWER failed\n");
		return -1;
	 }
	 memcpy(&power, wrq.u.data.pointer, 4);

	 return power;
}

int32_t get_data_from_factory(int32_t offset){
       struct iwreq wrq;
       struct sfcfghdr tmp;
       int32_t data = 0;
       memset(&wrq, 0, sizeof(struct iwreq));
       memset(&tmp, 0, sizeof(struct sfcfghdr));
       //cmd get factory power;
       tmp.magic_no = SFCFG_MAGIC_NO;
       tmp.comand_type = SFCFG_CMD_ATE_GET_DATA_FROM_FLASH;
       tmp.comand_id   = SFCFG_CMD_ATE_GET_DATA_FROM_FLASH;
       tmp.length      = 10;
       tmp.sequence    = 1;

       sprintf(tmp.data,"%d",offset);
       wrq.u.data.pointer = (caddr_t)&tmp;
       wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

       if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
               printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_GET_DATA_FROM_FLASH failed\n");
               return -1;
       }
       memcpy(&data, wrq.u.data.pointer, 4);
       return data;
}

int32_t get_temp_from_factory(void){
       int32_t offset = 174;
       int32_t data = 0;
       data = get_data_from_factory(offset);
       if(data == -1)
               return -1;
       printf("get temp is: %d\r\n",data);
       return 0;
}

int32_t get_trx_delay_from_factory(void){
       int32_t offset = 178;
       int32_t data[4] = {0};
       char value[4];
       int32_t i = 0;

       for(i = 0; i < 4; i ++){
               data[i] = get_data_from_factory(offset + i);
               if(data[i] == -1)
                       return -1;
               value[i] = toascii(data[i]);
       }
       printf("get tx dalay is:0x%c%c\r\n",value[0],value[1]);
       printf("get rx dalay is:0x%c%c\r\n",value[2],value[3]);
       return 0;
}

int32_t get_XO_from_factory(void){
       int32_t offset = 2050;
       int32_t data[2] = {0};
       int32_t i = 0;

       for(i = 0; i < 2; i ++){
               data[i] = get_data_from_factory(offset + i);
               if(data[i] == -1)
                       return -1;
       }
       printf("get XO value is:%d %d\r\n",data[0],data[1]);
       return 0;
}

int32_t set_phy_power_verfication(char *channel,char *bw,char *mode,char *rate,uint32_t power,char *band)
{
       uint32_t power_ver = 0;
       uint32_t offset = 0;
       FILE *fp3;
       int i;
       uint32_t  channel_tmp = siwifi_char_to_int(channel);
       uint32_t mode_tmp = siwifi_char_to_int(mode);
       uint32_t bw_tmp = siwifi_char_to_int(bw);
       uint32_t rate_tmp = siwifi_char_to_int(rate);
       uint32_t band_tmp = siwifi_char_to_int(band);
       int tmp_power[1689] = {0};

       power_ver = power;

       if(power_ver == FAC_MODE)//-p 200
       {
               power_ver = get_factory_power(channel,bw,mode,rate,"3",band);
               printf("normal power is %d\r\n",power_ver);
       }
       else if(power_ver == NOR_MODE)// -p 201
       {
               power_ver = get_factory_power(channel,bw,mode,rate,"1",band);
               printf("low power is %d\r\n",power_ver);
       }
       else if(power_ver == SLE_MODE)// -p 202
       {
               power_ver = get_factory_power(channel,bw,mode,rate,"2",band);
               printf("sle power is %d\r\n",power_ver);
       }
       else if(power_ver == TMP_MODE)//-p 203
       {
               offset = get_power_offset(channel_tmp,mode_tmp,bw_tmp,rate_tmp);
	       if(band_tmp == ANT2)
                 fp3 = fopen("/etc/atetools/power_save_ant2.txt","r");
	       else
                 fp3 = fopen("/etc/atetools/power_save_ant1.txt","r");

               for(i = 0;i < 1689;i++)
               {
                       fscanf(fp3,"%d",&tmp_power[i]);
               }
               fclose(fp3);
               power_ver = tmp_power[offset];
               printf("tmp  power is %d\r\n",power_ver);
       }
       else
       {
       		printf("use factory value\r\n");
       }

       return power_ver;
}

int32_t set_phy_power(char *tmpdata,char *channel,char *bw,char *mode,char *rate,char *band)
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	uint32_t  power = 0;
	uint32_t  power_ver = 0;
	uint32_t  channel_tmp = siwifi_char_to_int(channel);
	uint32_t mode_tmp = siwifi_char_to_int(mode);
	uint32_t bw_tmp = siwifi_char_to_int(bw);
	uint32_t rate_tmp = siwifi_char_to_int(rate);
	uint32_t band_tmp = siwifi_char_to_int(band);
	uint32_t offset  = 0;
	struct calibrate_info tx_info;
	FILE *fp,*fp2;
	int i;
	int save_power[1689] = {0};
#if USE_ONE_CALI_TABLE
	unsigned char buffer[2048] = {0};
#else
	unsigned char buffer[3382] = {0};
#endif
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));
	// cmd ate band
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_SET_TX_POWER;
	tmp.comand_id   = SFCFG_CMD_ATE_SET_TX_POWER;
	tmp.length      = 20;
	tmp.sequence    = 1;

	if(band_tmp == ANT1)
	{
	  printf("-----now test ant%d tx-----\r\n",band_tmp);
	}
	else if(band_tmp == ANT2)
	{
          printf("-----now test ant%d tx-----\r\n",band_tmp);
 	}
	else
	{
	  printf("-----not diff ant-----\r\n");
	}

	if (!strcmp(tmpdata,"factory")){ // use factory power tx
	power = get_factory_power(channel,bw,mode,rate,"0","0");//-p factory
	printf("power is %d\r\n",power);
	snprintf(tmp.data,4,"%d",power);
	}
	else if(!strcmp(tmpdata,"read")){//read factory power
		power=get_factory_power(channel,bw,mode,rate,"0","0");
		printf("power is :%d\n",power);
		return 0;
	}
	else if(!strcmp(tmpdata,"read_all")){//read all power from factory
		if(!read_gain_table_from_mtd(buffer))
		printf("read_gain_table_over\n");
		return 0;
	}
	else if(!strcmp(tmpdata,"save_all")){//save all power to factory
#if USE_ONE_CALI_TABLE
		save_power_table_to_factory(2044,0);
#else
		save_power_table_to_factory(3378,0);
#endif
		return 0;
	}
	else{
			memcpy(tmp.data,tmpdata, 20);
			power  = siwifi_char_to_int(tmpdata);
			if(power > 159)
			{
				power_ver = set_phy_power_verfication(channel,bw,mode,rate,power,band);
				printf("get verfication power is %d\r\n",power_ver);
                                snprintf(tmp.data,4,"%d",power_ver);
			}
			else
			{
				memcpy(tmp.data,tmpdata, 20);
				if(channel_tmp != 0)
				{
					offset = get_power_offset(channel_tmp,mode_tmp,bw_tmp,rate_tmp);
				}
				else
				{
					tx_info = get_ate_calibrate_info();
					printf("channel_tx is %d\n",tx_info.channel);
					printf("mode_tx is %d\n",tx_info.mode);
					printf("bw_tx is %d\n",tx_info.bw);
					printf("rate_tx is %d\n",tx_info.rate);

					offset = get_power_offset(tx_info.channel,tx_info.mode,tx_info.bw,tx_info.rate);
				}
				power_table[offset] = power & 0xff;
				//printf("power :%d offset :%d power :%d \n",power,offset,power_table[offset]);
				/*read from power_save.txt*/
				if(band_tmp == ANT2)
				{
				  fp2 = fopen("/etc/atetools/power_save_ant2.txt","r");
				}
				else
				{
				  fp2 = fopen("/etc/atetools/power_save_ant1.txt","r");
				}

				for(i=0;i < 1689; i++)
				{
					fscanf(fp2,"%d",&save_power[i]);
				}
				fclose(fp2);
    				/*write to power_save.txt*/
				if(band_tmp == ANT2)
				{
				  fp = fopen("/etc/atetools/power_save_ant2.txt","w");
				}
				else
				{
				  fp = fopen("/etc/atetools/power_save_ant1.txt","w");
				}
				save_power[offset] = power;

				for(i=0;i<1689;i++)
				{
					if (i < 364)
					{
						fprintf(fp,"%d ",save_power[i]);
							if((i+1)%28 == 0){
								if(i == 0)
									continue;
			  					fprintf(fp,"\n");
							}
					}
					else
					{
						fprintf(fp,"%d ",save_power[i]);
							if((i-363)%53 == 0)
			  					fprintf(fp,"\n");
					}
				}
				fclose(fp);
		}
	}
	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_SET_TX_POWER failed\n");
		return -1;
	}
	return 0;

}


int32_t set_pkg_bandwidth(char *tmpdata)
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));
	// cmd ate bandwidth
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_TX_FRAME_BW;
	tmp.comand_id   = SFCFG_CMD_ATE_TX_FRAME_BW;
	tmp.length      = 10;
	tmp.sequence    = 1;

	memcpy(tmp.data,tmpdata,10);
	//printf("tmp.data= %s---------\n",tmp.data);

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_TX_FRAME_BW failed\n");
		return -1;
	}
	return 0;
}


int32_t set_xo_value(char *tmpdata)
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	printf("%s\n",__func__);
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));
	// cmd ate band
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_XO_VALUE;
	tmp.comand_id   = SFCFG_CMD_ATE_XO_VALUE;
	tmp.length      = 10;
	tmp.sequence    = 1;

	memcpy(tmp.data,tmpdata, 10);
	printf("XO_VALUE = %s---------\n", tmp.data);

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_XO_VALUE failed\n");
		return -1;
	}
	return 0;

}
//for 2*2 board set aet num
int32_t set_aet_num(char *tmpdata)
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	printf("%s\n",__func__);
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));
	// cmd ate band
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_AET_NUM;
	tmp.comand_id   = SFCFG_CMD_ATE_AET_NUM;
	tmp.length      = 10;
	tmp.sequence    = 1;

	memcpy(tmp.data,tmpdata, 10);
	printf("AET_SETTING = %s---------\n", tmp.data);

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_AET_NUM failed\n");
		return -1;
	}
	return 0;
}

int32_t set_pkg_frame_length(char *tmpdata)
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
#if PRINT_ATE_LOG
	printf("%s\n",__func__);
#endif
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));
	// cmd ate band
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_PAYLOAD_LENGTH;
	tmp.comand_id   = SFCFG_CMD_ATE_PAYLOAD_LENGTH;
	tmp.length      = 10;
	tmp.sequence    = 1;

	memcpy(tmp.data,tmpdata, 10);
	//printf("tmp.data= %s---------\n", tmp.data);

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_PAYLOAD_LENGTH failed\n");
		return -1;
	}
	return 0;

}

int32_t set_pkg_frame_num(char *tmpdata)
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));
	// cmd ate num
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_TX_COUNT;
	tmp.comand_id   = SFCFG_CMD_ATE_TX_COUNT;
	tmp.length      = 10;
	tmp.sequence    = 1;

	strncpy(tmp.data,tmpdata,10);
	//printf("tmp.data= %s---------\n", tmp.data);

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_SET_TX_COUNT failed\n");
		return -1;
	}
	return 0;
}

int32_t set_pkg_macbypass_interval(char *tmpdata)
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));
	// cmd ate num
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_MACBYPASS_INTERVAL;
	tmp.comand_id   = SFCFG_CMD_ATE_MACBYPASS_INTERVAL;
	tmp.length      = 10;
	tmp.sequence    = 1;

	strncpy(tmp.data,tmpdata,10);
	//printf("tmp.data= %s---------\n", tmp.data);

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_MACBYPASS_INTERVAL failed\n");
		return -1;
	}
	return 0;
}

int32_t set_pkg_fc(char *tmpdata)
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));
	// cmd ate band
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_TX_FC;
	tmp.comand_id   = SFCFG_CMD_ATE_TX_FC;
	tmp.length      = 20;
	tmp.sequence    = 1;

	memcpy(tmp.data,tmpdata, 20);
	//printf("tmp.data= %s---------\n", tmp.data);

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_SET_TX_FC failed\n");
		return -1;
	}
	return 0;

}


int32_t set_pkg_dur(char *tmpdata)
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));
	// cmd ate band
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_TX_DUR;
	tmp.comand_id   = SFCFG_CMD_ATE_TX_DUR;
	tmp.length      = 20;
	tmp.sequence    = 1;

	memcpy(tmp.data,tmpdata, 20);
	//printf("tmp.data= %s---------\n", tmp.data);

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_SET_TX_DUR failed\n");
		return -1;
	}
	return 0;

}


int32_t set_pkg_bssid(char *tmpdata)
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));
	// cmd ate band
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_TX_BSSID;
	tmp.comand_id   = SFCFG_CMD_ATE_TX_BSSID;
	tmp.length      = 20;
	tmp.sequence    = 1;

	memcpy(tmp.data,tmpdata, 20);
	//printf("tmp.data= %s---------\n", tmp.data);

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_TX_BSSID failed\n");
		return -1;
	}
	return 0;

}


int32_t set_pkg_da(char *tmpdata)
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));
	// cmd ate band
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_TX_DA;
	tmp.comand_id   = SFCFG_CMD_ATE_TX_DA;
	tmp.length      = 20;
	tmp.sequence    = 1;

	memcpy(tmp.data,tmpdata, 20);
	//printf("tmp.data= %s---------\n", tmp.data);

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_TX_DA failed\n");
		return -1;
	}
	return 0;

}


int32_t set_pkg_sa(char *tmpdata)
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));
	// cmd ate band
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_TX_SA;
	tmp.comand_id   = SFCFG_CMD_ATE_TX_SA;
	tmp.length      = 20;
	tmp.sequence    = 1;

	memcpy(tmp.data,tmpdata, 20);
	//printf("tmp.data= %s---------\n", tmp.data);

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_TX_SA failed\n");
		return -1;
	}
	return 0;

}
int32_t set_txvector_param(char *tmpdata){
	struct iwreq wrq;
	struct sfcfghdr tmp;
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));
	// cmd ate band
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_TXVECTOR_PARAM;
	tmp.comand_id   = SFCFG_CMD_ATE_TXVECTOR_PARAM_NSS;
	tmp.length      = 20;
	tmp.sequence    = 1;

	memcpy(tmp.data,tmpdata, 20);
	//printf("tmp.data= %s---------\n", tmp.data);

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_TX_SA failed\n");
		return -1;
	}
	return 0;
}

int32_t set_pkg_seqc(char *tmpdata)
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));
	// cmd ate band
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_TX_SEQC;
	tmp.comand_id   = SFCFG_CMD_ATE_TX_SEQC;
	tmp.length      = 20;
	tmp.sequence    = 1;

	memcpy(tmp.data,tmpdata, 20);
	//printf("tmp.data= %s---------\n", tmp.data);

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_SET_PHY_MODE failed\n");
		return -1;
	}
	return 0;
}

int32_t get_rx_info(void)
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	struct rwnx_ioctl_ate_dump_info user;
	//read results from driver
	//printf("show PER, RSSI!\n");
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));
	memset(&user, 0, sizeof(struct rwnx_ioctl_ate_dump_info));

	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_GET_INFO;
	tmp.comand_id   = SFCFG_CMD_ATE_GET_INFO;
	tmp.length      = 10;
	tmp.sequence    = 1;

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;
	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE,&wrq) < 0){
		printf("SFCFG_CMD_ATE_GET_INFO ioctl failed\n");
		return -1;
	}

	memset(&user, 0, sizeof(struct rwnx_ioctl_ate_dump_info));
	memcpy(&user, wrq.u.data.pointer, sizeof(struct rwnx_ioctl_ate_dump_info));

	printf("receive data = %d,fcs ok  = %d,fcs error = %d,phy error = %d,fcs_ok_for_mac_addr = %d,fcs_group = %d\n",user.rec_rx_count, user.fcs_ok, user.fcs_err, user.phy_err, user.fcs_ok_for_macaddr, user.fcs_group);
	return 0;
}


int32_t set_pkg_whole_frame(char beacon[], int length_array)
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	int i = 0;
	//fill beacon mac header
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));
	//cmd ate band
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_WHOLE_FRAME;
	tmp.comand_id   = SFCFG_CMD_ATE_WHOLE_FRAME;
	tmp.length      = length_array;
	tmp.sequence    = 1;

	printf("%s: length_array = %d\n",__func__,length_array);
	memcpy(tmp.data, beacon, length_array);

	for(i = 0; i <= length_array; i++)
	{
		printf("%s: tmp.data[%d] = 0x%02x\n",__func__,i,tmp.data[i]);
	}

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_SET_PHY_MODE failed\n");
		return -1;
	}
	return 0;
}


int32_t set_pkg_payload(char *tmpdata)
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	//int i = 0;
	printf("~~~set_pkg_payload,%u\n",sizeof(tmpdata));
	//fill the frame payload
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));

	//cmd ate band
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_PAYLOAD;
	tmp.comand_id   = SFCFG_CMD_ATE_PAYLOAD;
	tmp.length      = sizeof(tmpdata);
	tmp.sequence    = 1;

	memcpy(tmp.data, tmpdata, tmp.length);
	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;
	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_SET_PAYLOAD failed\n");
		return -1;
	}
	return 0;
}

int32_t send_tx_test_tone_start()
{
	struct iwreq wrq;
	struct sfcfghdr tmp;

	printf("%s>>\n", __func__);

	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));

	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_TX_TEST_TONE_START;
	tmp.comand_id   = SFCFG_CMD_ATE_TX_TEST_TONE_START;
	tmp.length      = 10;
	tmp.sequence    = 1;

	strncpy(tmp.data, "true", 10);

	wrq.u.data.pointer = (caddr_t)&tmp;

	//12 is the sizeof sfcfghdr except data field, 10 is the actual data
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_TX_TEST_TONE_START failed\n");
		return -1;
	}
	return 0;
}

int32_t send_tx_test_tone_stop()
{
	struct iwreq wrq;
	struct sfcfghdr tmp;

	printf("%s>>\n", __func__);

	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));

	tmp.magic_no = SFCFG_MAGIC_NO;
	//first call SFCFG_CMD_ATE_START
	tmp.comand_type = SFCFG_CMD_ATE_TX_TEST_TONE_STOP;
	tmp.comand_id   = SFCFG_CMD_ATE_TX_TEST_TONE_STOP;
	tmp.length      = 10;
	tmp.sequence    = 1;

	strncpy(tmp.data, "true", 10);

	wrq.u.data.pointer = (caddr_t)&tmp;

	//12 is the sizeof sfcfghdr except data field, 10 is the actual data
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_TX_TEST_TONE_STOP failed\n");
		return -1;
	}
	return 0;
}

int32_t send_ate_start()
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	//struct tx_frame_param *tx_frame_param = NULL;
	// struct rwnx_ate_config_params *ate_config_params = NULL;
#if PRINT_ATE_LOG
	printf("%s>>\n", __func__);
#endif
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));

	tmp.magic_no = SFCFG_MAGIC_NO;
	//first call SFCFG_CMD_ATE_START
	tmp.comand_type = SFCFG_CMD_ATE_START;
	tmp.comand_id   = SFCFG_CMD_ATE_START;
	tmp.length      = 10;
	tmp.sequence    = 1;

	strncpy(tmp.data, "true", 10);

	wrq.u.data.pointer = (caddr_t)&tmp;

	//12 is the sizeof sfcfghdr except data field, 10 is the actual data
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_START failed\n");
		return -1;
	}
	return 0;
}

int32_t send_ate_stop()
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	//struct tx_frame_param *tx_frame_param = NULL;
	// struct rwnx_ate_config_params *ate_config_params = NULL;

	printf("%s>>\n", __func__);

	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));

	tmp.magic_no = SFCFG_MAGIC_NO;
	//first call SFCFG_CMD_ATE_START
	tmp.comand_type = SFCFG_CMD_ATE_STOP;
	tmp.comand_id   = SFCFG_CMD_ATE_STOP;
	tmp.length      = 10;
	tmp.sequence    = 1;

	strncpy(tmp.data, "true", 10);

	wrq.u.data.pointer = (caddr_t)&tmp;

	//12 is the sizeof sfcfghdr except data field, 10 is the actual data
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_STOP failed\n");
		return -1;
	}
	return 0;
}


int32_t send_tx_start(char *bufs)
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	//struct tx_frame_param *tx_frame_param = NULL;
	// struct rwnx_ate_config_params *ate_config_params = NULL;
	int bufs_length;

	printf("%s>>\n", __func__);

	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));

	tmp.magic_no = SFCFG_MAGIC_NO;
	//first call SFCFG_CMD_ATE_START
	tmp.comand_type = SFCFG_CMD_ATE_TX_START;
	tmp.comand_id   = SFCFG_CMD_ATE_TX_START;
	tmp.length      = 10;
	tmp.sequence    = 1;

	if(bufs != NULL){
		bufs_length=sizeof(bufs);
		memcpy(tmp.data, bufs, bufs_length);
	}
	else
		strncpy(tmp.data, "true", 10);

	wrq.u.data.pointer = (caddr_t)&tmp;

	//12 is the sizeof sfcfghdr except data field, 10 is the actual data
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_START failed\n");
		return -1;
	}
	return 0;
}


int32_t send_tx_start_by_macbypass()
{
	struct iwreq wrq;
	struct sfcfghdr tmp;

	printf("%s>>\n", __func__);

	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));

	tmp.magic_no = SFCFG_MAGIC_NO;
	//first call SFCFG_CMD_ATE_START
	tmp.comand_type = SFCFG_CMD_ATE_MACBYPASS_TX_START;
	tmp.comand_id   = SFCFG_CMD_ATE_MACBYPASS_TX_START;
	tmp.length      = 10;
	tmp.sequence    = 1;

	strncpy(tmp.data, "true", 10);

	wrq.u.data.pointer = (caddr_t)&tmp;

	//12 is the sizeof sfcfghdr except data field, 10 is the actual data
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_MACBYPASS_TX_START failed\n");
		return -1;
	}
	return 0;
}


int32_t send_rx_start()
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	struct rwnx_ioctl_ate_dump_info user;
	uint32_t tmp1 = 0;
	uint32_t tmp2 = 0;
	uint32_t tmp3 = 0;
//	uint32_t per = 0;
	uint32_t tmp_f = 0;
	uint32_t tmp_p = 0;
	uint32_t tmp_r = 0;
	uint32_t tmp_k =0 ;
#if PRINT_ATE_LOG
	printf("%s>>\n", __func__);
#endif
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));

	tmp.magic_no = SFCFG_MAGIC_NO;
	//first call SFCFG_CMD_ATE_START
	tmp.comand_type = SFCFG_CMD_ATE_RX_START;
	tmp.comand_id   = SFCFG_CMD_ATE_RX_START;
	tmp.length      = 10;
	tmp.sequence    = 1;

	strncpy(tmp.data, "true", 10);
	wrq.u.data.pointer = (caddr_t)&tmp;

	//12 is the sizeof sfcfghdr except data field, 10 is the actual data
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_RX_START failed\n");
		return -1;
	}

	//read results from driver
#if PRINT_ATE_LOG
	printf("show RX result, RSSI!\n");
#endif
	g_clock_flag = 1;
#if 0
	while(g_clock_flag){
		sleep(3);
		memset(&wrq, 0, sizeof(struct iwreq));
		memset(&tmp, 0, sizeof(struct sfcfghdr));

		tmp.magic_no = SFCFG_MAGIC_NO;
		tmp.comand_type = SFCFG_CMD_ATE_GET_INFO;
		tmp.comand_id   = SFCFG_CMD_ATE_GET_INFO;
		tmp.length      = 10;
		tmp.sequence    = 1;

		wrq.u.data.pointer = (caddr_t)&tmp;
		wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;
		if(do_ioctl(SFCFG_PRIV_IOCTL_ATE,&wrq) < 0){
			printf("SFCFG_CMD_ATE_GET_INFO ioctl failed\n");
			return -1;
		}
		memset(&user, 0, sizeof(struct rwnx_ioctl_ate_dump_info));
		memcpy(&user, wrq.u.data.pointer, sizeof(struct rwnx_ioctl_ate_dump_info));

		tmp_f = user.fcs_err - tmp1;
		tmp_p = user.phy_err - tmp2;
		tmp_r = user.rec_rx_count - tmp3;
		tmp_k = user.fcs_ok;
		tmp1 = user.fcs_err;
		tmp2 = user.phy_err;
		tmp3 = user.rec_rx_count;

		printf("receive data = %d,fcs_ok= %d,fcs error = %d,phy error = %d\n",user.rec_rx_count,user.fcs_ok,user.fcs_err, user.phy_err);
	}
#endif
	return 0;
}

int32_t send_rx_start_by_macbypass()
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	struct rwnx_ioctl_ate_dump_info user;
	uint32_t tmp1 = 0;
	uint32_t tmp2 = 0;
	uint32_t tmp3 = 0;
//	uint32_t per = 0;
	uint32_t tmp_f = 0;
	uint32_t tmp_p = 0;
	uint32_t tmp_r = 0;
	uint32_t tmp_k = 0;
	printf("%s>>\n", __func__);

	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));

	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_MACBYPASS_RX_START;
	tmp.comand_id   = SFCFG_CMD_ATE_MACBYPASS_RX_START;
	tmp.length      = 20;
	tmp.sequence    = 1;

	strncpy(tmp.data, "rx start macby", tmp.length);

	wrq.u.data.pointer = (caddr_t)&tmp;

	//12 is the sizeof sfcfghdr except data field, 10 is the actual data
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_MACBYPASS_RX_START failed\n");
		return -1;
	}
	//read results from driver
#if 0
    printf("show PER, RSSI!\n");
	g_clock_flag = 1;
	while(g_clock_flag){
		sleep(3);
		memset(&wrq, 0, sizeof(struct iwreq));
		memset(&tmp, 0, sizeof(struct sfcfghdr));

		tmp.magic_no = SFCFG_MAGIC_NO;
		tmp.comand_type = SFCFG_CMD_ATE_GET_INFO;
		tmp.comand_id   = SFCFG_CMD_ATE_GET_INFO;
		tmp.length      = 10;
		tmp.sequence    = 1;

		wrq.u.data.pointer = (caddr_t)&tmp;
		wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;
		if(do_ioctl(SFCFG_PRIV_IOCTL_ATE,&wrq) < 0){
			printf("SFCFG_CMD_ATE_GET_INFO ioctl failed\n");
			return -1;
		}
		memset(&user, 0, sizeof(struct rwnx_ioctl_ate_dump_info));
		memcpy(&user, wrq.u.data.pointer, sizeof(struct rwnx_ioctl_ate_dump_info));
		printf("-----------------------------------------------\n");
		tmp_f = user.fcs_err - tmp1;
		tmp_p = user.phy_err - tmp2;
		tmp_r = user.rec_rx_count - tmp3;
		tmp_k = user.fcs_ok;

		tmp1 = user.fcs_err;
		tmp2 = user.phy_err;
		tmp3 = user.rec_rx_count;
		printf("receive data = %d,fcs ok = %d, fcs error = %d,phy error = %d\n",user.rec_rx_count, user.fcs_ok,user.fcs_err, user.phy_err);
	}
#endif
	return 0;
}


int32_t send_tx_stop()
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));

	tmp.magic_no    = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_TX_STOP;
	tmp.comand_id   = SFCFG_CMD_ATE_TX_STOP;
	tmp.length      = 10;
	tmp.sequence    = 1;
	strncpy(tmp.data, "false", 10);

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = tmp.length + 16;//12 is the sizeof sfcfghdr except data field, 10 is the actual data

	//call SFCFG_CMD_ATE_TX_STOP
	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_TX_STOP failed\n");
		return -1;
	}
	return 0;
}

int32_t send_tx_stop_by_macbypass()
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));

	tmp.magic_no    = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_MACBYPASS_TX_STOP;
	tmp.comand_id   = SFCFG_CMD_ATE_MACBYPASS_TX_STOP;
	tmp.length      = 10;
	tmp.sequence    = 1;
	strncpy(tmp.data, "false", 10);

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = tmp.length + 16;//12 is the sizeof sfcfghdr except data field, 10 is the actual data

	//call SFCFG_CMD_ATE_TX_STOP
	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_MACBYPASS_TX_STOP failed\n");
		return -1;
	}
	return 0;
}

int32_t send_rx_stop()
{
	struct iwreq wrq;
	struct sfcfghdr tmp;

	g_clock_flag = false;
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));

	tmp.magic_no    = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_RX_STOP;
	tmp.comand_id   = SFCFG_CMD_ATE_RX_STOP;
	tmp.length      = 10;
	tmp.sequence    = 1;
	strncpy(tmp.data, "false", 10);

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = tmp.length + 16;//12 is the sizeof sfcfghdr except data field, 10 is the actual data

	//call SFCFG_CMD_ATE_RX_STOP
	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_RX_FRAME_START failed\n");
		return -1;
	}
	return 0;
}

int32_t send_rx_stop_by_macbypass()
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	g_clock_flag = false;
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));

	tmp.magic_no    = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_MACBYPASS_RX_STOP;
	tmp.comand_id   = SFCFG_CMD_ATE_MACBYPASS_RX_STOP;
	tmp.length      = 20;
	tmp.sequence    = 1;
	strncpy(tmp.data, "rx stop macbypass", tmp.length);

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = tmp.length + 16;//12 is the sizeof sfcfghdr except data field, 10 is the actual data

	//call SFCFG_CMD_ATE_TX_STOP
	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_MACBYPASS_RX_STOP failed\n");
		return -1;
	}
	return 0;
}

int32_t save_temp_to_flash(char *tmpdata)
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	int offset = 174;
	int length = 2;
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));
	// cmd ate band
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_SAVE_DATA_TO_FLASH;
	tmp.comand_id   = SFCFG_CMD_ATE_SAVE_DATA_TO_FLASH;
	tmp.length      = 2;
	tmp.sequence    = 1;

	sprintf(tmp.data,"%d",offset);
	memcpy(&tmp.data[4],tmpdata,length);
	printf("tmpdata= %d---------\n", tmpdata[0]);

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + 10;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_SAVE_DATA_TO_FLASH failed\n");
		return -1;
	}
	return 0;
}

int32_t save_XO_config_to_flash(char *tmpdata)
{
	struct iwreq wrq;
	struct sfcfghdr tmp;
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));
	// cmd ate band
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_SAVE_TO_FLASH;
	tmp.comand_id   = SFCFG_CMD_ATE_SAVE_TO_FLASH;
	tmp.length      = 20;
	tmp.sequence    = 1;

	memcpy(tmp.data,tmpdata, tmp.length);
	printf("tmp.data= %s---------\n", tmp.data);

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_SAVE_TO_FLASH failed\n");
		return -1;
	}
	return 0;
}
//get power from factory and return the power as int param.
/*int32_t get_factory_power(char *channel,char *bw,char *mode,char *mcs){
	struct iwreq wrq;
	struct sfcfghdr tmp;
	int power = 0;
	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));
	//cmd get factory power;
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_GET_POWER;
	tmp.comand_id   = SFCFG_CMD_ATE_GET_POWER;
	tmp.length      = 10;
	tmp.sequence    = 1;

	sprintf(tmp.data,"%s %s %s %s",channel,bw,mode,mcs);
	printf("tmp.data==== %s--------------\n",tmp.data);
	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;

	if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
	  printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_GET_POWER failed\n");
		return -1;
	 }
	 memcpy(&power, wrq.u.data.pointer, 4);

	 return power;
}*/

int32_t set_power_config(void)
{

	FILE *fp1;
	char text[1024];

	printf("start load wifi tx power table config...\n");
	//change tx_power.txt to tx_power.bin
	system("/usr/bin/txpower_calibrate_table.sh");
	//TODO:check bin does it exist
	//reload wifi driver
	system("sfwifi reset fmac");
	sleep(3);
	//printf using wifi tx power table
	printf("the using tx power table is: \n");
	fp1=fopen("/usr/bin/txpower_calibrate_table.txt","r");
	while(fscanf(fp1,"%[^\n]",text)!=EOF)
	{
		getc(fp1);
		puts(text);
		//if need put map to another txt
		//fputs(text,fp2);
	}
	fclose(fp1);
	return 0;
}

int32_t save_power_config_to_flash(uint16_t tmpdata, uint16_t channel, uint16_t bw, uint16_t mode,uint16_t rate){
	struct iwreq wrq;
	struct sfcfghdr tmp;
	uint32_t offset = 0;

	memset(&wrq, 0, sizeof(struct iwreq));
	memset(&tmp, 0, sizeof(struct sfcfghdr));

	// cmd ate band
	tmp.magic_no = SFCFG_MAGIC_NO;
	tmp.comand_type = SFCFG_CMD_ATE_SAVE_TO_FLASH;
	tmp.comand_id   = SFCFG_CMD_ATE_SAVE_TO_FLASH;
	tmp.length      = 20;
	tmp.sequence    = 1;

	if (channel < 2485){
		switch(channel){
			case 2412:offset = (((channel - 2412) / 5)*28);break;
			case 2417:offset = (((channel - 2412) / 5)*28);break;
			case 2422:offset = (((channel - 2412) / 5)*28);break;
			case 2427:offset = (((channel - 2412) / 5)*28);break;
			case 2432:offset = (((channel - 2412) / 5)*28);break;
			case 2437:offset = (((channel - 2412) / 5)*28);break;
			case 2442:offset = (((channel - 2412) / 5)*28);break;
			case 2447:offset = (((channel - 2412) / 5)*28);break;
			case 2452:offset = (((channel - 2412) / 5)*28);break;
			case 2457:offset = (((channel - 2412) / 5)*28);break;
			case 2462:offset = (((channel - 2412) / 5)*28);break;
			case 2467:offset = (((channel - 2412) / 5)*28);break;
			case 2472:offset = (((channel - 2412) / 5)*28);break;
			default:
				  return -1;
			}
			if (mode == 0){
				offset += rate;
			}
			else if (mode == 2){
				switch(bw)
				{
					case 1: offset += 12 + rate;break;
					case 2: offset += 20 + rate;break;
					default: return -1;
				}
			}
			if (mode !=0 && mode != 2)
			      return -1;
	}
	else{
		switch(channel){
            case 4940:offset = 364 + 53*0;break;
			case 5040:offset = 364 + 53*0;break;
			case 5180:offset = 364 + 53*0;break;
			case 5200:offset = 364 + 53*1;break;
			case 5220:offset = 364 + 53*2;break;
			case 5240:offset = 364 + 53*3;break;
			case 5260:offset = 364 + 53*4;break;
			case 5280:offset = 364 + 53*5;break;
			case 5300:offset = 364 + 53*6;break;
			case 5320:offset = 364 + 53*7;break;
			case 5500:offset = 364 + 53*8;break;
			case 5520:offset = 364 + 53*9;break;
			case 5540:offset = 364 + 53*10;break;
			case 5560:offset = 364 + 53*11;break;
			case 5580:offset = 364 + 53*12;break;
			case 5600:offset = 364 + 53*13;break;
			case 5620:offset = 364 + 53*14;break;
			case 5640:offset = 364 + 53*15;break;
			case 5660:offset = 364 + 53*16;break;
			case 5680:offset = 364 + 53*17;break;
			case 5700:offset = 364 + 53*18;break;
			case 5720:offset = 364 + 53*19;break;
			case 5745:offset = 364 + 53*20;break;
			case 5765:offset = 364 + 53*21;break;
			case 5785:offset = 364 + 53*22;break;
			case 5805:offset = 364 + 53*23;break;
			case 5825:offset = 364 + 53*24;break;
            case 5905:offset = 364 + 53*24;break;
			case 5925:offset = 364 + 53*24;break;
			default: return -1;
			}
		if (mode == 0)
		      offset += rate;
		if (mode == 2){
			switch(bw){
				case 1:offset += 8  + rate;break;
				case 2:offset += 16 + rate;break;
				default: return -1;
			}
		}
		if (mode ==4){
			switch(bw){
				case 1:offset += 24 + rate;break;
				case 2:offset += 33 + rate;break;
				case 3:offset += 43 + rate;break;
				default: return -1;
			}
		if (!(mode ==0||mode ==2||mode ==4)){
				return -1;
			}
		}
	}
	printf("offset:%d\n",offset);
	sprintf(tmp.data,"1%02hu0%4d",tmpdata,offset + 2048 + 4);//factory form 2048 begin and 4 bytes form 2048 to 2052  save xo;
	printf("tmp.data= %s---------\n", tmp.data);

	wrq.u.data.pointer = (caddr_t)&tmp;
	wrq.u.data.length = sizeof(struct sfcfghdr) - sizeof(((struct sfcfghdr *)0)->data) + tmp.length;
		if(do_ioctl(SFCFG_PRIV_IOCTL_ATE, &wrq) < 0){
		 printf("SFCFG_PRIV_IOCTL_ATE ioctl SFCFG_CMD_ATE_SAVE_TO_FLASH failed\n");
		  return -1;
		}
		return 0;
}

uint32_t _offset_result(int ant)
{
	FILE *fp,*fp2,*fp3;
	int i,j,k;
	int g_24_channel[13] = {0};
	int g_5_channel[25] = {0};
	int g_one_24g_channel_power[28] = {0};
	int g_one_5g_channel_power[53] = {0};
	int save_power[1689] = {0};
	int gain_offset[1689] = {0};
	/*TODO:open config for reset*/
	if(ant == ANT2)
	{
		fp = fopen("/etc/atetools/power_save_ant2.txt","r");
	}else
	{
		fp = fopen("/etc/atetools/power_save_ant1.txt","r");
	}

	for(i=0;i < 1689; i++)
	{
		fscanf(fp,"%d",&save_power[i]);
	}
	/*how to offset,fill up the array */
	//TODO
	/*1,offset set one 24g channel*/
	/*2,copy to other channel*/
	/*2.4g*/
	for(i=0;i<13;i++)
	{
		g_24_channel[i] = save_power[27+i*28];
		if(i <= 4)
		{
			if(i == 0)
			{
				for(j=0;j<28;j++)
				{
					if(j <= 3)
						g_one_24g_channel_power[j] = save_power[3+i*28];
					else if (j > 3 && j <= 19)
						g_one_24g_channel_power[j] = save_power[19+i*28];
					else
						g_one_24g_channel_power[j] = save_power[27+i*28];
				}
			}
		}
		else if( i > 4 && i <= 9)
		{
			if(i == 5){
				memset(g_one_24g_channel_power,0,28*sizeof(int));
			        for(j=0;j<28;j++)
			        {
				        if(j <= 3)
					        g_one_24g_channel_power[j] = save_power[3+i*28];
				        else if (j > 3 && j <= 19)
					        g_one_24g_channel_power[j] = save_power[19+i*28];
                                        else
					        g_one_24g_channel_power[j] = save_power[27+i*28];
			        }
			}
		}
		else
		{
			if(i == 10){
				memset(g_one_24g_channel_power,0,28*sizeof(int));
			        for(j=0;j<28;j++)
			        {
				        if(j <= 3)
					        g_one_24g_channel_power[j] = save_power[3+i*28];
				        else if (j > 3 && j <= 19)
                                                g_one_24g_channel_power[j] = save_power[19+i*28];
                                        else
                                                g_one_24g_channel_power[j] = save_power[27+i*28];
			        }
			}
		}
        memcpy((void *)&save_power[i*28],(void *)&g_one_24g_channel_power[0],28*sizeof(int));
	}
	/*5g*/

	for(i=0;i<25;i++)
	{
		g_5_channel[i] = save_power[364+52+i*53];
		if(i < 4 )//ch36-ch48 use ch36
		{
			if(i == 0)//use calibrate value to fill ch36 line
			{
				for(j=0;j<53;j++)
				{
					if(j < 16)//11a 11n_20
						g_one_5g_channel_power[j] = save_power[364+32+i*53];
					else if(j >= 16 && j < 24)//11n_40
						g_one_5g_channel_power[j] = save_power[364+42+i*53];
					else if(j >= 24 && j < 33)//11ac_20
						g_one_5g_channel_power[j] = save_power[364+32+i*53];
					else if(j >= 33 && j <43)//11ac_40
						g_one_5g_channel_power[j] = save_power[364+42+i*53];
					else
						g_one_5g_channel_power[j] = save_power[364+52+i*53];
				}
			}
		}
		else if(i >= 4 && i <8)//ch52-ch64  use ch64
		{
			if(i == 7)//use calibrate value to fill ch64 line
			{
				memset(g_one_5g_channel_power,0,53*sizeof(int));
				for(j=0;j<53;j++)
				{
					if(j < 16)//11a 11n_20
						g_one_5g_channel_power[j] = save_power[364+32+i*53];
					else if(j >= 16 && j < 24)//11n_40
						g_one_5g_channel_power[j] = save_power[364+42+i*53];
					else if(j >= 24 && j < 33)//11ac_20
						g_one_5g_channel_power[j] = save_power[364+32+i*53];
					else if(j >= 33 && j <43)//11ac_40
						g_one_5g_channel_power[j] = save_power[364+42+i*53];
					else//11ac_80
						g_one_5g_channel_power[j] = save_power[364+52+i*53];
				}
			}
		}
		else if( i >= 8 && i < 20 )//for 5500,if calibrate use 5500 gain,if not copy 5320 for ch100-ch144
		{
			if(i == 8)
			{
				if(g_5_channel[i] != 0)
				{
				    memset(g_one_5g_channel_power,0,53*sizeof(int));
					for(j=0;j<53;j++)
					{
						if(j < 16)//11a 11n_20
							g_one_5g_channel_power[j] = save_power[364+32+i*53];
						else if(j >= 16 && j < 24)//11n_40
							g_one_5g_channel_power[j] = save_power[364+42+i*53];
						else if(j >= 24 && j < 33)//11ac_20
							g_one_5g_channel_power[j] = save_power[364+32+i*53];
						else if(j >= 33 && j < 43)//11ac_40
							g_one_5g_channel_power[j] = save_power[364+42+i*53];
						else//11ac_80
							g_one_5g_channel_power[j] = save_power[364+52+i*53];
					}
				}else
				{
					for(j=0;j<53;j++)
					{
						if(j < 16)//11a 11n_20
							g_one_5g_channel_power[j] = save_power[364+32+7*53];
						else if(j >= 16 && j < 24)//11n_40
							g_one_5g_channel_power[j] = save_power[364+42+7*53];
						else if(j >= 24 && j < 33)//11ac_20
							g_one_5g_channel_power[j] = save_power[364+32+7*53];
						else if(j >= 33 && j < 43)//11ac_40
							g_one_5g_channel_power[j] = save_power[364+42+7*53];
						else//11ac_80
							g_one_5g_channel_power[j] = save_power[364+52+7*53];
					}
				}
			}
		}
		else //ch149-ch165 use ch149
		{
			if(i == 20)//use calibrate value to fill ch149 line
			{
				memset(g_one_5g_channel_power,0,53*sizeof(int));
				for(j=0;j<53;j++)
				{
					if(j < 16)//11a 11n_20
						g_one_5g_channel_power[j] = save_power[364+32+i*53];
					else if(j >= 16 && j < 24)//11n_40
						g_one_5g_channel_power[j] = save_power[364+42+i*53];
					else if(j >= 24 && j < 33)//11ac_20
						g_one_5g_channel_power[j] = save_power[364+32+i*53];
					else if(j >= 33 && j <43)//11ac_40
						g_one_5g_channel_power[j] = save_power[364+42+i*53];
					else//11ac_80
						g_one_5g_channel_power[j] = save_power[364+52+i*53];
				}
			}
		}
		if(i == 7)
		{
			for(k=4;k<7;k++)
			{
				memcpy((void *)&save_power[364+k*53],(void *)&g_one_5g_channel_power[0],53*sizeof(int));
			}
		}
		memcpy((void *)&save_power[364+i*53],(void *)&g_one_5g_channel_power[0],53*sizeof(int));
	}

	fclose(fp);
/*calculate power_save.txt by gain_offset*/
	fp3 = fopen("/etc/atetools/gain_offset.txt","r");
	for(i=0;i < 1689; i++)
	{
		fscanf(fp3,"%d",&gain_offset[i]);
	}
#if USE_ONE_CALI_TABLE
	for(i=0;i < 1689;i++)
	{
		save_power[i]+=gain_offset[i];
		if(( save_power[i] > 31) && (save_power[i] < 128))
		{
		    save_power[i] = 31;
		}
		else if( save_power[i] > 159)
		{
		    save_power[i] = 159;
		}
	}
#else
	for(i=0;i < 1689;i++)
	{
        /*save power & gain offset all 0.5 step need calculate*/
        if(((save_power[i] > 128) && (save_power[i] < 144)) && ((gain_offset[i] > 128) && (gain_offset[i] < 144)))
        {
            save_power[i] = (save_power[i] & 0x0f) + (gain_offset[i] & 0x0f) -1;
        }else/*save power or gain offset ,only one 0.5 step not calculate*/
        {
		    save_power[i]+=gain_offset[i];
        }
		if(( save_power[i] > 15) && (save_power[i] < 128))
		{
		    save_power[i] = 15;
		}
		else if( save_power[i] > 142)
		{
		    save_power[i] = 143;
		}
	}
#endif
	fclose(fp3);
/*save the final gain to power_save.txt*/
	if(ant == ANT2)
	{
		fp2 = fopen("/etc/atetools/power_save_ant2.txt","w");
	}else
	{
		fp2 = fopen("/etc/atetools/power_save_ant1.txt","w");
	}

	for(i=0;i<1689;i++)
	{
			if (i < 364)
			{
				fprintf(fp2,"%d ",save_power[i]);
					if((i+1)%28 == 0){
						if(i == 0)
							continue;
			  			fprintf(fp2,"\n");
					}
			}
			else
			{
				fprintf(fp2,"%d ",save_power[i]);
					if((i-363)%53 == 0)
			  			fprintf(fp2,"\n");
			}
	}

	fclose(fp2);
	return 0;
}

uint32_t offset_result()
{
	int ret = 0;
	ret = _offset_result(ANT1);
	if(ret)
	{
		printf("offset ant1 table result failed!\r\n");
		return -1;
	}
	else
	{
		printf("offset ant1 table over!\r\n");
	}
	ret = _offset_result(ANT2);
	if(ret)
	{
		printf("offset ant2 table result failed!\r\n");
		return -1;
	}
	else
	{
		printf("offset ant2 table over!\r\n");
	}
	return 0;
}

static void usage()
{
	printf("Usage:\n"
			"  ate_cmd wlan0/1 set/read/write "
			"ATEXXX = XXX \\\n"
			"\n"
		  );
	printf(" ate_cmd wlan0 set ATE = ATESTART, ATE start\n");
	printf(" ate_cmd wlan0 set ATE = ATETXSTART, Tx frame start (-t)\n");
	printf(" ate_cmd wlan0 set ATE = ATETXSTOP, Stop current TX action (-s)\n");
	printf(" ate_cmd wlan0 set ATE = ATERXSTART, RX frame start (-r)\n");
	printf(" ate_cmd wlan0 set ATE = ATERXSTOP, Stop current RX action (-k)\n");
	printf(" ate_cmd wlan0 set ATE = ATESHOW, Show Setting info\n");
	printf(" ate_cmd wlan0 set ATEPHYBW = X,(0~3) Set ATE PHY Bandwidth (-w)\n");
	printf(" ate_cmd wlan0 set ATETXBW = X,(0~3) Set ATE Tx Frame Bandwidth (-u)\n");
	printf(" ate_cmd wlan0 set ATECHAN = X, Set ATE Channel(primmary freq), decimal(-f)\n");
	printf(" ate_cmd wlan0 set ATECENTERFREQ1 = X, Set ATE center freq1, decimal(-c)\n");
	printf(" ate_cmd wlan0 set ATETXNUM = X, Set ATE Tx Frame Number(-n)\n");
	printf(" ate_cmd wlan0 set ATELEN = X, Set ATE Frame Length(-l)\n");
	printf(" ate_cmd wlan0 set ATETXMODE = X,(0~4) Set ATE Tx Mode(-m)\n");
	printf(" ate_cmd wlan0 set ATETXMCS = X, Set ATE Tx MCS type(-i)\n");
	printf(" ate_cmd wlan0 set ATETXGI = X,(0~1) Set ATE Tx Guard Interval(-g)\n");
	printf(" ate_cmd wlan0 set ATETXPOW = XX,(0~31) Set ATE Tx power(-p)\n");
	printf(" ate_cmd wlan0 set ATETXPREAM = X,(0~1) Set ATE Tx pream\n");
	printf(" ate_cmd wlan0 set ATETXDA = XX:XX:XX:XX:XX:XX, Set da\n");
	printf(" ate_cmd wlan0 set ATETXSA = XX:XX:XX:XX:XX:XX, Set sa\n");
	printf(" ate_cmd wlan0 set ATETXBSSID = XX:XX:XX:XX:XX:XX, Set bssid\n");
	printf(" ate_cmd wlan0 read ATEREG = X, Read the Value of One Register, X must be 0xYYYYYYYY or 0xYYYY\n");
	printf(" ate_cmd wlan0 write ATEREG = X(addr)_X(val), Write the Value of One Rergister\n");
	//printf(" ate_cmd wlan0 read ATEBULKREG = type_num_X1_X1_..., Read Many Registers\n");
	//printf(" ate_cmd wlan0 write ATEBULKREG = type_num_addr1_val1_X2_X2..., Write Many Registers\n");
	printf(" ate_cmd wlan0 set ATE = ATEMACHDR -f 0x0008 -t 0x0000 -b 00:aa:bb:cc:dd:ee -d 16:88:aa:bb:cc:dd -s 16:88:aa:bb:cc:dd -q 0x5e20, Set the frame mac header\n");
	printf(" ate_cmd wlan1 fastconfig -v 0xb790b100 //read reg value \n");
	printf(" ate_cmd wlan1 fastconfig -z 0xb7990b100_0x00101010 //write reg value\n" );
	printf(" ate_cmd wlan1 fastconfig -n 0 -l 1024 -f 5500 -c 5530 -w 3 -u 3 -m 4 -i 9 -g 0 -p 28 -t, Transmit frames by fast setting on HB\n");
	printf(" ate_cmd wlan1 fastconfig -l 1024 -f 5500 -c 5530 -w 3 -u 3 -m 4 -i 9 -g 0 -p 28 -b 4096 -y, Transmit frames by macbypass on HB\n");
	printf(" ate_cmd wlan1 fastconfig -q, stop tx by macbypass on HB!!\n");
	printf(" ate_cmd wlan1 fastconfig -f 5180 -c 5180 -w 0 -u 0 -h,Receive frames by macbypass on HB\n");
	printf(" ate_cmd wlan1 fastconfig -j, stop rx by macbypass on HB!!\n");
	printf(" ate_cmd wlan1 fastconfig -f 5500 -c 5500 -w 3 -u 3 -p 28 -o, Transmit test tone on HB\n");
	printf(" ate_cmd wlan1 fastconfig -n 0 -l 1024 -f 5500 -c 5500 -w 1 -u 1 -m 2 -i 7 -g 1 -p 28 -a 16, Transmit aggregated frames by fast setting on HB, 16 is the max agg buffer size\n");
	printf(" ate_cmd save 0x1a1b, Save XO cali caonfig  0x1a1b into falsh\n");
	printf(" ate_cmd wlan1 fastconfig -O 1 , 1  is xo calibrate value from 0 to 255\n");
	printf(" ate_cmd save pow 1 2412 0  0  0 ,the parameters are power channel bw mode rate;\n" );
	printf(" ate_cmd read pow 2412 0  0  0 ,the parameters are channel bw mode rate; \n" );
	printf(" if you use (ate_init lb/hb) to active ate tool, only use wlan0\n");
	printf(" if you use (ate_init) to active ate tool, use wlan0 for 2.4G test and wlan1 for 5G test\n");
	printf(" ate_cmd wlan1 fastconfig -p save_all/read_all ,read all_power or save_all power\n");
}

/*
 * getopt --
 *	Parse argc/argv argument vector.
 */

static char *optarg_tmp=NULL;
static int opting_tmp=0;
static int optopt_tmp=0;


char *strchra(const char *arr ,int optopt_tmp){
		char *p=NULL;
		int size=50;
		int i=0;
		for ( i = 0; i < size - 1; i++) {
				if (arr[i] == optopt_tmp) {
						p =(char *)&arr[i];
						return p;
				}
		}
		return NULL;
}

int getopt_tmp(int nargc,char* const nargv[],const char* ostr)

{
	char *place = "";		/* option letter processing */
	char *oli;				/* option letter list index */
	static int optreset=0;
//	printf("%s\n",__func__);
	if (optreset || !*place) {		/* update scanning pointer */
		optreset = 0;
		if (opting_tmp >= nargc){
			optarg_tmp=NULL;
			opting_tmp=0;
			optopt_tmp=0;
			return -1;
		}
		if(*(place =nargv[opting_tmp]) != '-') {
					place = "";
					++opting_tmp;
					return 0;
			}
			if (place[1] && *++place == '-'	/* found "--" */
									&& place[1] == '\0') {
					++opting_tmp;
					place = "";
					return (-1);
			}
	}	/* option letter okay? */
	if ((optopt_tmp = (int)*place++) == (int)':' ||
							!(oli = strchra(ostr, optopt_tmp))) {
			/*
			 * if the user didn't specify '-' as an option,
			 * assume it means -1.
			 */
			if (optopt_tmp == (int)'-')
				  return (-1);
			if (!*place)
				  ++opting_tmp;
			return (-1);
	}
	if (*++oli != ':') {			/* don't need argument */
			optarg_tmp = NULL;
			if (!*place)
				  ++opting_tmp;
	}
	else {					/* need an argument */
			if (*place)			/* no white space */
				  optarg_tmp = place;
			else if (nargc <= ++opting_tmp) {	/* no arg */
					place = "";
					if (*ostr == ':')
						  return (-1);
					return (-1);
			}
			else				/* white space */
				  optarg_tmp = nargv[opting_tmp];
			place = "";
			++opting_tmp;
	}
//	printf("opting %d opt %c optopt_arg %s\n",opting_tmp,optopt_tmp,optarg_tmp);
	return (optopt_tmp);			/* dump back option letter */
}

int32_t main(int32_t argc, char *argv[])
{
	uint8_t j = 1;
	char *bufs="\0";
	char *channel = "\0";
	char *mode= "\0";
	char *rate="\0";
	char *bw ="\0";
	char *band = "\0";
	char temp[2] = "0";
    struct timeval start;
	struct timeval end;
	float use_time;
	gettimeofday(&start,NULL);
    if(argc < 2){
        usage();
        return -1;
    }

	if(strcmp(argv[1],"set_power") == 0){
		set_power_config();
		return 0;
	}

	if(strcmp(argv[1],"tx_calibrate_over") == 0)
	{
		if(offset_result()==0)
			printf("offset calibration value over! now you can wirte to flash..\n");
		return 0;
	}
	if(strcmp(argv[1],"init_power_save") == 0)
	{
		system("cp /usr/bin/power_save.txt /etc/atetools/");
		system("cp /usr/bin/power_save_ant1.txt /etc/atetools/");
		system("cp /usr/bin/power_save_ant2.txt /etc/atetools/");
		return 0;
	}
	send_ate_start();
	if(strcmp(argv[1],"save") == 0){
		if (strcmp(argv[2],"pow") == 0){
				uint16_t value;//power
				uint16_t value2;//channel
				uint16_t value3;//bw
				uint16_t value4;//mode
				uint16_t value5;//rate
				value = atoi(argv[3]);
				value2 = atoi(argv[4]);
				value3 = atoi(argv[5]);
				value4 = atoi(argv[6]);
				value5 = atoi(argv[7]);
				if (save_power_config_to_flash(value,value2,value3,value4,value5) == -1){
				      printf("save pow fail\r\n");
					return -1;
				}
				goto DONE;
					}
		else if (strcmp(argv[2],"ver") == 0)
		{
			char value[] = "0";
			strcpy(value, argv[3]);
			printf("version is %s\n",value);
			if(save_version_to_factory(value) == -1)
			{
		 		printf("save version fail\r\n");
				return -1;
			}
	         	 goto DONE;
		}
		/* *ate_cmd save temp [value]
		 * *value: use decimal number
		 * */
		else if (strcmp(argv[2],"temp") == 0)
		{
			//get temp
			temp[0] = atoi(argv[3]);
			if(temp[0] < 0)
			{
			   printf("----temperature is %d degrees below zero!!----\n",temp[0]);
			  temp[0] = ~temp[0] + 1;
			  //if use temp below zero ,set the flag 1,default 0;
			  temp[1] = 1;
			}
			if(!save_temp_to_flash(temp))
				printf("----save temperature ok!----\n");
			goto DONE;
		}
		else{
		char value[] = "0";
		strcpy(value, argv[2]);
		if(save_XO_config_to_flash(value))
			return -3;
		goto DONE;
		}
	}

	if(strcmp(argv[1],"read") == 0){
		if (strcmp(argv[2],"pow") == 0){
			if (get_factory_power(argv[3],argv[4],argv[5],argv[6],"0","0") == -1){
				printf("read pow fail\r\n");
				return -1;
			}
			goto DONE;
			}
        if (strcmp(argv[2],"temp") == 0){
            if (get_temp_from_factory() == -1){
				printf("read temp fail\r\n");
                return -1;
            }
            goto DONE;
        }
		if (strcmp(argv[2],"trx_delay") == 0){
            if (get_trx_delay_from_factory() == -1){
                printf("read trx delay fail\r\n");
                return -1;
            }
            goto DONE;
            }
		if (strcmp(argv[2],"XO_value") == 0){
            if (get_XO_from_factory() == -1){
                printf("read XO value fail\r\n");
                return -1;
            }
            goto DONE;
        }
	}

	if(strcmp(argv[1],"fastconfig") != 0)
	{
		strcpy(ifname, argv[1]);
		j=2;
	}
	if(strcmp(argv[j],"fastconfig") == 0)
	{
		int opt;
		char options[] = "trskyqoxhjRa:n:l:f:c:w:u:m:i:g:B:p:v:z:e:db:V:O:T:";
#if 0
		set_pkg_da(da);
		set_pkg_sa(sa);
		set_pkg_bssid(bssid);
#endif
		// parse parameters
		//./ate_cmd fastconfig -t -n 10 -l 1024 -f 5520 -w 0 -m 0 -i 0 -g 0

	//	printf("getopt------argc %d  argv %s\n",argc_tmp,argv[j+1]);
		while ((opt = getopt_tmp (argc, argv, options)) != -1)
		{
	//		printf("switch--------\n");
			switch (opt)
			{
				case 't':
					g_tx_frame_flag = true;
					printf("%s -t param g_tx_frame_flag=%d\n", __func__, g_tx_frame_flag);
					break;
				case 'r':
					g_rx_frame_flag = true;
					printf("%s -r param g_rx_frame_flag=%d\n", __func__, g_rx_frame_flag);
					break;
				case 's':
					g_stop_tx_frame_flag = true;
					printf("%s -s param g_stop_tx_frame_flag=%d\n", __func__, g_stop_tx_frame_flag);
					break;
				case 'k':
					g_stop_rx_frame_flag = true;
					printf("%s -k param g_stop_rx_frame_flag=%d\n", __func__, g_stop_rx_frame_flag);
					break;
				case 'y':
					g_start_tx_frame_by_macbypass_flag = true;
					printf("%s -y param g_start_tx_frame_by_macbypass_flag=%d\n", __func__, g_start_tx_frame_by_macbypass_flag);
					break;
				case 'q':
					g_stop_tx_frame_by_macbypass_flag = true;
					printf("%s -y param g_stop_tx_frame_by_macbypass_flag=%d\n", __func__, g_stop_tx_frame_by_macbypass_flag);
					break;
				case 'o':
					g_tx_test_tone_flag = true;
					printf("%s -o param g_tx_test_tone_flag=%d\n", __func__, g_tx_test_tone_flag);
					break;
				case 'x':
					g_stop_tx_test_tone_flag = true;
					printf("%s -x param g_stop_tx_test_tone_flag=%d\n", __func__, g_stop_tx_test_tone_flag);
					break;
				case 'h':
					g_start_rx_by_macbypass_flag = true;
					printf("%s -h param g_start_rx_by_macbypass_flag=%d\n", __func__, g_start_rx_by_macbypass_flag);
					break;
				case 'j':
					g_stop_rx_by_macbypass_flag = true;
					printf("%s -j param g_stop_rx_by_macbypass_flag=%d\n", __func__, g_stop_rx_by_macbypass_flag);
					break;
				case 'a':
					g_agg_tx_frame_flag = true;
					bufs = optarg_tmp;
					printf("%s -a param g_agg_tx_frame_flag=%d\n", __func__, g_agg_tx_frame_flag);
					break;
				case 'n':
					if(set_pkg_frame_num(optarg_tmp))
						return -1;
					break;
				case 'b':
					if(set_pkg_macbypass_interval(optarg_tmp))
						return -1;
					break;
				case 'l':
					if(set_pkg_frame_length(optarg_tmp))
						return -1;
					break;
				case 'f':
					channel = optarg_tmp;
					if(set_phy_chan_freq(optarg_tmp))
						return -1;
					break;
				case 'c':
					if(set_phy_center_freq1(optarg_tmp))
						return -1;
					break;
				case 'w':
					bw = optarg_tmp;
					if(set_phy_bandwidth(optarg_tmp))
						return -1;
					break;
				case 'u':
					if(set_pkg_bandwidth(optarg_tmp))
						return -1;
					break;
				case 'm':
					mode  = optarg_tmp;
					if(set_phy_mode(optarg_tmp))
						return -1;
					break;
				case 'i':
					rate = optarg_tmp;
					if(set_phy_rate_idx(optarg_tmp))
						return -1;
					break;
				case 'g':
					if(set_phy_use_sgi(optarg_tmp))
						return -1;
					break;
				case 'B':
					band = optarg_tmp;
					printf("####test band%s####\r\n",band);
					break;
				case 'p':
					if(set_phy_power(optarg_tmp,channel,bw,mode,rate,band))
						return -1;
					break;
				case 'v':
					if(read_iw_reg(optarg_tmp))
						return -1;
					break;
				case 'z':
					if(write_iw_reg(optarg_tmp))
						return -1;
					break;
				case 'e':
					if(read_iw_bulk_reg(optarg_tmp))
						return -1;
					break;
				case 'd':
					if(write_iw_bulk_reg(optarg_tmp))
						return -1;
					break;
				case 'V':
					if(set_txvector_param(optarg_tmp))
					      return -1;
					break;
				case 'O':
					if(set_xo_value(optarg_tmp))
					  return -1;
					break;
				case 'R':
					//ioctl get rx result 0/1
					if(get_rx_info())
						return -1;
					break;
				case 'T':
					if(set_aet_num(optarg_tmp))
						return -1;
					break;
				case '?':
				case ':':
					usage();
					break;
			}
		}
		if (g_tx_frame_flag) {
			if(send_tx_start(NULL))
				return -1;
			goto DONE;
		}

		if (g_rx_frame_flag) {
			if(send_rx_start())
				return -1;
			goto DONE;
		}

		if (g_stop_tx_frame_flag) {
			if(send_tx_stop())
				return -1;
			goto DONE;
		}

		if (g_stop_rx_frame_flag) {
			if(send_rx_stop())
				return -1;
			goto DONE;
		}

		if (g_start_tx_frame_by_macbypass_flag) {
			if(send_tx_start_by_macbypass())
				return -1;
			goto DONE;
		}

		if (g_stop_tx_frame_by_macbypass_flag) {
			if(send_tx_stop_by_macbypass())
				return -1;
			goto DONE;
		}

		if (g_start_rx_by_macbypass_flag) {
			if(send_rx_start_by_macbypass())
				return -1;
			goto DONE;
		}

		if (g_stop_rx_by_macbypass_flag) {
			if(send_rx_stop_by_macbypass())
				return -1;
			goto DONE;
		}

		if (g_tx_test_tone_flag) {
			if(send_tx_test_tone_start())
				return -1;
			g_tx_test_tone_flag = false;
			goto DONE;
		}

		if (g_stop_tx_test_tone_flag) {
			if(send_tx_test_tone_stop())
				return -1;
			g_stop_tx_test_tone_flag = false;
			goto DONE;
		}

		if (g_agg_tx_frame_flag) {
			if(send_tx_start(bufs))
				return -1;
			g_agg_tx_frame_flag = false;
			goto DONE;
		}
		goto DONE;
	}

	// ./ate_cmd wlan0 set XXX = XXX
	if(argc > 5 && strcmp(argv[0],"ate_cmd") == 0 && strcmp(argv[4],"=") == 0)
	{
		char *tmpdata;
	    int length_array = sizeof(beacon);
		// ./ate_cmd wlanx set XXX = XXX
		tmpdata=(char *)malloc(200);

		tmpdata = argv[5];

		if(strcmp(argv[3],"ATE") == 0 && strcmp(argv[5],"ATESTART") == 0){
			if(send_ate_start())
				return -1;
		}
		if(strcmp(argv[3],"ATE") == 0 && strcmp(argv[5],"ATESTOP") == 0){
			if(send_ate_stop())
				return -1;
		}
		//command id SFCFG_CMD_ATE_START 0x0000

		if(strcmp(argv[3],"ATE") == 0 && strcmp(argv[5],"ATETXSTART") == 0){
			if(send_tx_start(NULL))
				return -1;
		}
		//command id SFCFG_CMD_ATE_TX_START 0x0002

		if(strcmp(argv[3],"ATE") == 0 && strcmp(argv[5],"ATETXSTOP") == 0){
			if(send_tx_stop())
				return -1;
		}
		if(strcmp(argv[3],"ATE") == 0 && strcmp(argv[5],"ATERXSTART") == 0){
			if(send_rx_start())
				return -1;
		}
		if(strcmp(argv[3],"ATE") == 0 && strcmp(argv[5],"ATERXSTOP") == 0){
			if(send_rx_stop())
				return -1;
		}

		if(strcmp(argv[3],"ATE") == 0 && strcmp(argv[5],"ATESHOW") == 0){
			if(get_ate_info())
				return -1;
		}
		//config the frame mac head content
		//./ate_cmd wlan0 set ATE = ATEMACHDR -f 0x0080 -t 0x0000 -b 16:88:aa:bb:cc:dd -d 16:88:aa:bb:cc:dd -s 16:88:aa:bb:cc:dd -q 0x5e20
		if( strcmp(argv[5],"ATEMACHDR") == 0)
		{
			int opt;
			char options[] ="f:t:b:d:s:q:";
			while ((opt = getopt_tmp (argc, argv, options)) != -1)
			{
				switch (opt)
				{
					case 'f':
						set_pkg_fc(optarg_tmp);
						break;
					case 't':
						set_pkg_dur(optarg_tmp);
						break;
					case 'b':
						set_pkg_bssid(optarg_tmp);
						break;
					case 'd':
						set_pkg_da(optarg_tmp);
						break;
					case 's':
						set_pkg_sa(optarg_tmp);
						break;
					case 'q':
						set_pkg_seqc(optarg_tmp);
						break;
					case ':':
						usage();
						break;
				}
			}
		}

		//./ate_cmd wlan0 set ATETXFC = 0x0080
		if(strcmp(argv[3],"ATETXFC") == 0){
			if(set_pkg_fc(tmpdata))
				return -1;
		}
		//./ate_cmd wlan0 set ATETXDUR = 0x0000
		if(strcmp(argv[3],"ATETXDUR") == 0){
			if(set_pkg_dur(tmpdata))
				return -1;
		}
		//./ate_cmd wlan0 set ATETXDA = 00:16:88:08:07:f9
		if(strcmp(argv[3],"ATETXDA") == 0){
			if(set_pkg_da(tmpdata))
				return -1;
		}
		if(strcmp(argv[3],"ATETXSA") == 0){
			if(set_pkg_sa(tmpdata))
				return -1;
		}
		if(strcmp(argv[3],"ATETXBSSID") == 0){
			if(set_pkg_bssid(tmpdata))
				return -1;
		}
		//./ate_cmd wlan0 set ATETXSEQC = 0x5e20
		if(strcmp(argv[3],"ATETXSEQC") == 0){
			if(set_pkg_seqc(tmpdata))
				return -1;
		}
		if(strcmp(argv[3],"ATEPHYBW") == 0){
			if(set_phy_bandwidth(tmpdata))
				return -1;
		}
		if(strcmp(argv[3],"ATETXBW") == 0){
			if(set_pkg_bandwidth(tmpdata))
				return -1;
		}
		if(strcmp(argv[3],"ATETXNUM") == 0){
			if(set_pkg_frame_num(tmpdata))
				return -1;
		}
		if(strcmp(argv[3],"ATECHAN") == 0){
			if(set_phy_chan_freq(tmpdata))
				return -1;
		}
		if(strcmp(argv[3],"ATECENTERFREQ1") == 0){
			if(set_phy_center_freq1(tmpdata))
				return -1;
		}
		if(strcmp(argv[3],"ATETXMODE") == 0){
			if(set_phy_mode(tmpdata))
				return -1;
		}
		if(strcmp(argv[3],"ATETXMCS") == 0){
			if(set_phy_rate_idx(tmpdata))
				return -1;
		}
		if(strcmp(argv[3],"ATETXGI") == 0){
			if(set_phy_use_sgi(tmpdata))
				return -1;
		}
		if(strcmp(argv[3],"ATETXPREAM") == 0){
			if(set_phy_preamble(tmpdata))
				return -1;
		}
		if(strcmp(argv[3],"ATETXPOW") == 0){
			if(set_phy_power(tmpdata,channel,bw,mode,rate,"0"))
				return -1;
		}
		if(strcmp(argv[3],"ATELEN") == 0){
			if(set_pkg_frame_length(tmpdata))
				return -1;
		}
		// ./ate_cmd wlan0 set ATE = ATEWHOLEFRAME
		if(strcmp(argv[5],"ATEWHOLEFRAME") == 0){
			if(set_pkg_whole_frame(beacon,length_array))
				return -1;
		}
		if(strcmp(argv[3],"ATEPAYLOAD") == 0){
			if(set_pkg_payload(tmpdata))
				return -1;
		}
		if(strcmp(argv[3],"ATEREG") == 0 && strcmp(argv[2],"read") ==0){
			if(read_iw_reg(tmpdata))
				return -1;
		}
		if(strcmp(argv[3],"ATEREG") == 0 && strcmp(argv[2],"write") == 0){
			if(write_iw_reg(tmpdata))
				return -1;
		}
		if(strcmp(argv[3],"ATEBULKREG") == 0 && strcmp(argv[2],"read") ==0){
			if(read_iw_bulk_reg(tmpdata))
				return -1;
		}
		if(strcmp(argv[3],"ATEBULKREG") == 0 && strcmp(argv[2],"write") == 0){
			if(write_iw_bulk_reg(tmpdata))
				return -1;
		}
	}else{
		usage();
    }

DONE:
	close_ioctl_socket();
	gettimeofday(&end,NULL);
	use_time=(end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec);
//	printf("time_use is %.10f\n",use_time);
	return 0;
}
