/**
 *****************************************************************************************
 * @file genvcd_a28.c
 *
 * @brief Main file for la application, sending DBG commands to LMAC layer
 *
 * Copyright (C) Siflower 2018-2025
 *
 *****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * DEFINES
 *****************************************************************************************
 */
//the unit is ps, our LA always run in 150MHz
#define DELAY (unsigned long long)6667

ssize_t extern getline(char **lineptr, size_t *size, FILE *fp);

struct group {
    char name[255];
    char id[15];
    int lsb;
    int msb;
    uint32_t prev;
};

struct groups {
    int cnt;
    struct group group[32]; // Up to 32 groups per 32-bit bank
};

/*
 * FUNCTION DEFINITIONS
 *****************************************************************************************
 */
static int int2bin(char *out, uint32_t val, int width, uint32_t smp_msk)
{
    int status = -1;

    out[0] = '\0';

    uint32_t z = 1 << (width - 1);
    for (; z > 0; z >>= 1)
    {
        if (z & smp_msk)
        {
            strcat(out, ((val & z) == z) ? "1" : "0");
            status = 0;
        }
        else
            strcat(out, "x");
    }

    return status;
}

static ssize_t readline(char **lineptr, FILE *fp)
{
    *lineptr = NULL;
    char *line;
    size_t len = 0;
    ssize_t read;
    int i;

    read = getline(lineptr, &len, fp);

    if (read <= 0)
        return read;

    line = *lineptr;

    // Remove the end of line character
    if (line[read - 1] == '\n')
        line[read - 1] = '\0';

    // Keep only the characters up to a * or a [
    for (i = 0; i < strlen(line); i++)
    {
        if ((line[i] == '*') || (line[i] == '['))
        {
            line[i] = '\0';
            break;
        }
    }

    return strlen(line);
}

static int add_group(struct groups *groups, char *name, char *id_prefix, int lsb, int msb, uint32_t val)
{
    struct group *g;

    if (groups->cnt == 32)
        return -1;

    if (msb >= 32)
        return -1;

    g = &groups->group[groups->cnt];

    strcpy(g->name, name);
    sprintf(g->id, "%s%d", id_prefix, groups->cnt);
    g->lsb = lsb;
    g->msb = msb;
    g->prev = val;

    groups->cnt++;

    return 0;
}

static int enlarge_group(struct groups *groups)
{
    struct group *g;

    if (groups->cnt == 0)
        return -1;

    g = &groups->group[groups->cnt - 1];

    if (g->msb >= 32)
        return -1;

    g->msb++;

    return 0;
}

static FILE *hwdiag_name_open(char *path, char *name)
{
    FILE *fp;
    char filename[256];

    snprintf(filename, 255, "%s/%s", path, name);
    fp = fopen(filename, "r");

    return fp;
}

static FILE *hwdiag_config_open(char *path)
{
    FILE *fp;
    char filename[256];

    snprintf(filename, 255, "%s/mac_cur_diag", path);
    fp = fopen(filename, "r");

    return fp;
}

static FILE *plfdiag_config_open(char *path)
{
    FILE *fp;
    char filename[256];

    snprintf(filename, 255, "%s/plfdiags", path);
    fp = fopen(filename, "r");

    return fp;
}

static FILE *plfdiag1_config_open(char *path)
{
    FILE *fp;
    char filename[256];

    snprintf(filename, 255, "%s/plfdiags1", path);
    fp = fopen(filename, "r");

    return fp;
}

static FILE *plf_la_clk_open(char *path)
{
    FILE *fp;
    char filename[256];

    snprintf(filename, 255, "%s/la_clk", path);
    fp = fopen(filename, "r");

    return fp;
}

static FILE *siwifiplat_open(char *path)
{
    FILE *fp;
    char filename[256];

    snprintf(filename, 255, "%s/siwifiplat", path);
    fp = fopen(filename, "r");

    return fp;
}

static FILE *fpgabdiag_config_open(char *path)
{
    FILE *fp;
    char filename[256];

    snprintf(filename, 255, "%s/fpgabdiags", path);
    fp = fopen(filename, "r");

    return fp;
}

static FILE *riudiag_config_open(char *path)
{
    FILE *fp;
    char filename[256];

    snprintf(filename, 255, "%s/riu_cur_diag", path);
    fp = fopen(filename, "r");

    return fp;
}

static FILE *hdmdiag_config_open(char *path)
{
    FILE *fp;
    char filename[256];

    snprintf(filename, 255, "%s/hdm_cur_diag", path);
    fp = fopen(filename, "r");

    return fp;
}

static FILE *vcd_open(char *path)
{
    FILE *fp;
    char filename[256];

    snprintf(filename, 255, "%s/trace.vcd", path);
    fp = fopen(filename, "w");

    return fp;
}

static FILE *trace_open(char *path)
{
    FILE *fp;
    char filename[256];

    snprintf(filename, 255, "%s/la0", path);
#ifdef USE_SIMULATION_TXT_DEBUG
	fp = fopen("/etc/trace.txt", "r");
#else
    fp = fopen(filename, "r");
#endif

    return fp;
}

static int get_la_conf(uint32_t *sampling, uint32_t *version, char *path, char *name)
{
    FILE *fp;
    char filename[256];
    int i;

    snprintf(filename, 255, "%s/%s", path, name);
    fp = fopen(filename, "r");
    if (fp == NULL)
        return -1;

    // Get sampling mask
    for (i=0; i<2; i++)
    {
        if (fread(&sampling[i], 4, 1, fp) != 1)
            break;
    }

    // Get unused words
    for (i=0; i<7; i++)
    {
        uint32_t dummy;
        if (fread(&dummy, 4, 1, fp) != 1)
            break;
    }

    // Get version
    if (fread(version, 4, 1, fp) != 1)
        *version = 0;

    fclose (fp);

    return 0;
}

static int create_swdiag_groups(struct groups *swdiags, char *path, int sw_idx, int idx)
{
    FILE *fp;
    char filename[256];
    int i = 0;
    char line_prev[256] = "";
    char grouptag[32];

    printf("Create SW diag groups %d\n",sw_idx);

    swdiags->cnt = 0;

    snprintf(filename, 255, "%s/swdiags", path);
    fp = fopen(filename, "r");

    if (fp == NULL)
        return -1;

    while (1)
    {
        char *line;
        ssize_t read;

        read = readline(&line, fp);

        if ((read <= 0) || (i >= (16 * (sw_idx + 1))))
            break;

		if(i < 16 * sw_idx)
		{
			i++;
			continue;
		}

        // Compare with previous diag name to know if it is part of a group
        if (strcmp(line_prev, line))
        {
			sprintf(grouptag, "nx%d", idx);
            /* Create new group */
            add_group(swdiags, line, grouptag, i - 16 * sw_idx, i - 16 * sw_idx, 0);
            strcpy(line_prev, line);
        }
        else
        {
            enlarge_group(swdiags);
        }

        free(line);
        i++;
    }
    for (;i < 16 * (sw_idx + 1); i++)
    {
        char name[32];

        sprintf(name, "SWDIAG%d", i);

		sprintf(grouptag, "nx%d", idx);
        /* Add to the SW diags groups */
        add_group(swdiags, name, grouptag, i, i, 0);
    }

    fclose(fp);

    return 0;
}


static int create_mpif_groups(struct groups *mpif)
{
    printf("Create MPIF groups\n");

    mpif->cnt = 0;

    add_group(mpif, "mpif_rifsDetected",     "mp", 0,  0,  0);
    add_group(mpif, "mpif_keepRfOn",         "mp", 1,  1,  0);
    add_group(mpif, "mpif_phyErr",           "mp", 2,  2,  0);
    add_group(mpif, "mpif_rxEnd_p",          "mp", 3,  3,  0);
    add_group(mpif, "mpif_rxErr_p",          "mp", 4,  4,  0);
    add_group(mpif, "mpif_rxEndForTiming_p", "mp", 5,  5,  0);
    add_group(mpif, "mpif_ccaSecondary",     "mp", 6,  6,  0);
    add_group(mpif, "mpif_ccaPrimary",       "mp", 7,  7,  0);
    add_group(mpif, "mpif_rxReq",            "mp", 16, 16, 0);
    add_group(mpif, "mpif_txEnd_p",          "mp", 17, 17, 0);
    add_group(mpif, "mpif_txReq",            "mp", 28, 28, 0);
    add_group(mpif, "mpif_rfshutdown",       "mp", 29, 29, 0);
    add_group(mpif, "mpif_phyRdy_u1",        "mp", 30, 30, 0); //will be always 0 for SF16A18 PLATFORM
    add_group(mpif, "mpif_macDataValid_u1",  "mp", 31, 31, 0); //will be always 0 for SF16A18 PLATFORM
    add_group(mpif, "mpif_rxData",           "mp", 8,  15, 0);
    add_group(mpif, "mpif_phyRdy",           "mp", 18, 18, 0);
    add_group(mpif, "mpif_macDataValid",     "mp", 19, 19, 0);
    add_group(mpif, "mpif_txData",           "mp", 20, 27, 0);

    return 0;
}

static int isphyorswdiag (char *path)
{
    FILE *config;
    char *line;
    ssize_t read;


    // First open the platform configuration file to check if the MAC HW or
    // platform diags are output
    config = plfdiag_config_open(path);
    if (config != NULL)
    {
        // Read the platform diag configuration
        read = readline(&line, config);
        if (read != 8)
            return -1;

        //printf("LINE %c\n",line[4]);

        //PHY diag are selected when plfdiag_config[15] is set
        if (line[4]>=56)  // 56 dec = 8 ASCII
        {
            printf("PHY Diag selected\n");
            return 1;

        } else {
            printf("SW Diag selected\n");
            return 0;
        }
        // Free line and close configuration file
        free(line);
        fclose(config);
    } else {
        return -1;
    }
}

static int get_la_clk(char *path)
{
   FILE *la_clk;
   char *line;
   ssize_t read;
   int tmp = 0;

   la_clk = plf_la_clk_open(path);
   if (la_clk != NULL)
   {
       read = readline(&line, la_clk);
       if (!read)
       {
            printf("can not read value from ./la_clk!\n");
            return 0;
       }
       tmp = atoi(line);
       free(line);
       fclose(la_clk);
   }

   return tmp;
}

static int isv7orv6board (char *path)
{
#if defined(SIFLOWER_SF16A18) || defined(SIFLOWER_SF19A28)
    printf("SIFLOWER SF16A18 platform!\n");
    return 0;
#else
    FILE *platform;
    char *line;
    ssize_t read;

    // Open the platform file to know v6/v7.
    // If no file, this is a v6 platform
    platform = siwifiplat_open(path);
    if (platform != NULL)
    {
        // Read the platform type
        read = readline(&line, platform);
        if (read != 2)
            return -1;

        //printf("LINE %c\n",line[1]);

        if (line[1]==54) // 54 dec = 6 ASCII
        {
            printf("Dini platform\n");
            return 1;

        } else if (line[1]==55) { // 55 dec = 7 ASCII
            printf("SIFLOWER FPGA v7 platform\n");
            return 0;
        } else {
            return -1;
        }
        // Free line and close configuration file
        free(line);
        fclose(platform);
    } else {
        printf("Dini platform\n");
        return 1;
    }
#endif
}

static int create_hwdiag_fpgaa_groups(struct groups *hwdiags, char *path)
{
    int i, j;
    FILE *names;
    FILE *config;
    char hwconfig[2][5] = {"0xXX", "0xYY"};
    char *line;
    ssize_t read;
    char line_prev[256] = "";
    int lsb = 0;
    int diag_cnt = 2;
    int bank_len = 16;
    char hwdiag_name[32] = "mac_hwdiag.txt";
    int read_hw_diags = 1;

    printf("Create HW diag for FPGA A groups ");

    hwdiags->cnt = 0;

    // First open the platform configuration file to check if the MAC HW or
    // platform diags are output
    config = plfdiag_config_open(path);
    if (config != NULL)
    {
        // Read the platform diag configuration
        read = readline(&line, config);
        if (read != 8)
            return -1;

        if (memcmp("0C", &line[6], 2) != 0)
        {
            memcpy(&hwconfig[0][2], &line[6], 2);
            diag_cnt = 1;
            bank_len = 32;
            strcpy(hwdiag_name, "fpgaa_hwdiag.txt");
            read_hw_diags = 0;
            printf("(FPGA A diag port)\n");
        }

        // Free line and close configuration file
        free(line);
        fclose(config);
    }

    if (read_hw_diags)
    {
        config = hwdiag_config_open(path);
        if (config == NULL)
            return -1;

        // Read the HW diag configuration
        read = readline(&line, config);
        if (read != 8)
            return -1;

        // Fill-in the LSB and MSB configurations
        memcpy(&hwconfig[0][2], &line[6], 2);
        memcpy(&hwconfig[1][2], &line[4], 2);

        printf("(MAC HW diag port)\n");
        // Free line and close configuration file
        free(line);
        fclose(config);
    }

    names = hwdiag_name_open(path, hwdiag_name);
    if (names == NULL)
    {
        printf("Unable to create HW diags for %s in %s\n",hwdiag_name,path);
        return -1;
    }

    // Walk through the name file and create the groups for the LSB and MSB config
    for (i = 0; i < diag_cnt; i++)
    {
        // Find the beginning of an HW diag configuration in the name file
        while (1)
        {
            // Read the HW diag configuration
            read = readline(&line, names);
            if (read < 0)
                return -1;

            if (!strcmp(line, hwconfig[i]))
            {
                printf(" Diag : %s\n",line_prev);
                free(line);
                break;
            }

            strcpy(line_prev, line);
            free(line);
        }

        for (j = 0; j < bank_len; j++)
        {
            read = readline(&line, names);

            if (read <= 0)
                break;

            // Compare with previous diag name to know if it is part of a group
            if (strcmp(line_prev, line))
            {
                /* Create new group */
                add_group(hwdiags, line, "hwa", lsb, lsb, 0);
                strcpy(line_prev, line);
            }
            else
            {
                enlarge_group(hwdiags);
            }
            lsb++;
            free(line);
        }
        fseek(names, 0, SEEK_SET);
    }

    fclose(names);

    return 0;
}

static int create_siwifidiag_groups(struct groups *hwdiags, char *path, int idx)
{
    int j;
    FILE *names;
    FILE *config;
    char hwconfig[5] = {"0xXX"};
    char *line;
    ssize_t read;
    char line_prev[256] = "";
    int lsb = 0;
    // By default, decode mux setting to fetch 2 banks of 16 bits
    int bank_len = 16;
    char hwdiag_name[32] = "mac_hwdiag.txt";
    int read_machw_diags = 0;
    int read_phy0hw_diags = 0;
    //int read_phy1hw_diags = 0;
    int read_riu0hw_diags = 0;
    //int read_riu1hw_diags = 0;
    int read_nx_top_diags = 0;
    char grouptag[32];
	int diag_port_index = 0;
	int debug_port_index = 0;
	int is_sw_prof[4] = {0};

    printf("Create HW diag for RWNX%d group ", idx);

    hwdiags->cnt = 0;

    // First open the platform configuration file to check which diags are output
	if(idx < 2)
		config = plfdiag_config_open(path);
	else if(idx < 3)
		config = plfdiag1_config_open(path);
	else
		return -1;

    if (config != NULL)
    {
        // Read the platform diag configuration
        read = readline(&line, config);
        if (read != 8)
            return -1;

		if(idx == 0 || idx == 2)
		{
			debug_port_index = 4;
			diag_port_index = 7;
		}
		else if(idx == 1)
		{
			debug_port_index = 0;
			diag_port_index = 3;
		}

        if (memcmp("1", &line[diag_port_index], 1) == 0)
            read_machw_diags = 1;
        else if (memcmp("2", &line[diag_port_index], 1) == 0)
            read_phy0hw_diags = 1;
        else if (memcmp("3", &line[diag_port_index], 1) == 0)
            read_riu0hw_diags = 1;
        else
            read_nx_top_diags = 1;
        memcpy(&hwconfig[2], &line[debug_port_index], 2);

        // Free line and close configuration file
        free(line);
        fclose(config);
    }

    if (read_machw_diags)
    {
        printf(": Create machw diag groups\n");
        strcpy(hwdiag_name, "mac_hwdiag.txt");

        printf("(MAC HW diag port)\n");
    }

    if (read_phy0hw_diags)
    {
        printf(": Create phy0hw diag groups\n");
        strcpy(hwdiag_name, "hdm_hwdiag.txt");

        printf("(PHY0 HW diag port)\n");
    }

    if (read_riu0hw_diags)
    {
        printf(": Create riu0hw diag groups\n");
        strcpy(hwdiag_name, "riu_hwdiag.txt");

        printf("(RIU0 HW diag port)\n");
    }

    if (read_nx_top_diags)
    {
        printf(": Create top diag groups\n");

        //bank_len = 32;
        strcpy(hwdiag_name, "nxtop_hwdiag.txt");
        printf("(rw_nx_top diag port)\n");
    }

    names = hwdiag_name_open(path, hwdiag_name);
    if (names == NULL)
    {
        printf("Unable to create HW diags for %s in %s\n",hwdiag_name,path);
        return -1;
    }

    // Walk through the name file and create the groups for the LSB and MSB config
    //for (i = 0; i < diag_cnt; i++)
    {
        // Find the beginning of an HW diag configuration in the name file
        while (1)
        {
            // Read the HW diag configuration
            read = readline(&line, names);
            if (read < 0)
                return -1;

            if (!strcmp(line, hwconfig))
            {
                printf(" Diag : %s\n",line_prev);
				if(read_machw_diags)
				{
					//if(!strcmp(line, "0x1E") || !strcmp(line, "0x1F") || !strcmp(line, "0x3B") || !strcmp(line, "0x3C"))
					if(!strcmp(line, "0x1E"))
						is_sw_prof[0] = 1;
					else if(!strcmp(line, "0x1F"))
						is_sw_prof[1] = 1;
					else if(!strcmp(line, "0x3B"))
						is_sw_prof[2] = 1;
					else if(!strcmp(line, "0x3C"))
						is_sw_prof[3] = 1;
				}
                free(line);
                break;
            }

            strcpy(line_prev, line);
            free(line);
        }

		//sw diag is special
		if(is_sw_prof[0] == 1)
			create_swdiag_groups(hwdiags, path, 0, idx);
		else if(is_sw_prof[1] == 1)
			create_swdiag_groups(hwdiags, path, 1, idx);
		else if(is_sw_prof[2] == 1)
			create_swdiag_groups(hwdiags, path, 2, idx);
		else if(is_sw_prof[3] == 1)
			create_swdiag_groups(hwdiags, path, 3, idx);
		else
		{
			for (j = 0; j < bank_len; j++)
			{
				read = readline(&line, names);

				if (read <= 0)
					break;

				// Compare with previous diag name to know if it is part of a group
				if (strcmp(line_prev, line))
				{
					/* Create new group */
					sprintf(grouptag, "nx%d",idx);
					add_group(hwdiags, line, grouptag, lsb, lsb, 0);
					strcpy(line_prev, line);
				}
				else
				{
					enlarge_group(hwdiags);
				}
				lsb++;
				free(line);
			}
		}
        fseek(names, 0, SEEK_SET);
    }

    fclose(names);

    return 0;
}

static int create_hwdiag_fpgab_groups(struct groups *hwdiags, char *path)
{
    int i, j;
    FILE *names;
    FILE *config;
    char hwconfig[2][5] = {"0xXX", "0xYY"};
    char *line;
    ssize_t read;
    char line_prev[256] = "";
    int lsb = 0;
    int diag_cnt = 2;
    int bank_len = 16;
    char hwdiag_name[32] = "hdm_hwdiag.txt";
    int read_hw_diags = 1;

    printf("Create HW diag for FPGA B groups");

    hwdiags->cnt = 0;

    // First open the platform configuration file to check if the MAC HW or
    // platform diags are output
    config = fpgabdiag_config_open(path);
    if (config != NULL)
    {
        // Read the platform diag configuration
        read = readline(&line, config);
        if (read != 8)
            return -1;


        if (memcmp("14", &line[0], 2) != 0)
        {
            memcpy(&hwconfig[0][2], &line[0], 2);
            diag_cnt = 1;
            bank_len = 32;
            strcpy(hwdiag_name, "fpgab_hwdiag.txt");
            read_hw_diags = 0;
            printf("(FPGA B diag port)\n");
        }

        // Free line and close configuration file
        free(line);
        fclose(config);
    }

    if (read_hw_diags)
    {
        config = hdmdiag_config_open(path);
        if (config == NULL)
            return -1;

        // Read the HW diag configuration
        read = readline(&line, config);
        if (read != 8)
            return -1;

        // Fill-in the LSB and MSB configurations
        memcpy(&hwconfig[0][2], &line[6], 2);
        memcpy(&hwconfig[1][2], &line[4], 2);

        printf("(HDM diag port)\n");
        // Free line and close configuration file
        free(line);
        fclose(config);
    }

    names = hwdiag_name_open(path, hwdiag_name);
    if (names == NULL)
    {
        printf("Unable to create HW diags for %s in %s\n",hwdiag_name,path);
        return -1;
    }

    // Walk through the name file and create the groups for the LSB and MSB config
    for (i = 0; i < diag_cnt; i++)
    {
        // Find the beginning of an HW diag configuration in the name file
        while (1)
        {
            // Read the HW diag configuration
            read = readline(&line, names);
            if (read < 0)
                return -1;

            if (!strcmp(line, hwconfig[i]))
            {
                printf(" Diag : %s\n",line_prev);
                free(line);
                break;
            }

            strcpy(line_prev, line);
            free(line);
        }

        for (j = 0; j < bank_len; j++)
        {
            read = readline(&line, names);

            if (read <= 0)
                break;

            // Compare with previous diag name to know if it is part of a group
            if (strcmp(line_prev, line))
            {
                /* Create new group */
                add_group(hwdiags, line, "hwb", lsb, lsb, 0);
                strcpy(line_prev, line);
            }
            else
            {
                enlarge_group(hwdiags);
            }
            lsb++;
            free(line);
        }
        fseek(names, 0, SEEK_SET);
    }

    fclose(names);

    return 0;
}

static int create_control_groups(struct groups *control,uint32_t version)
{
    printf("Create Control groups");

    control->cnt = 0;

    if (version < 0x40000)
    {
        printf(" with 1 trigger\n");
        add_group(control, "trigger", "ct", 31,  31,  0);
    } else {
        printf(" with 6 triggers\n");
        add_group(control, "ext_trigger", "ct", 11,  14,  0);
        add_group(control, "sw_trigger", "ct", 10,  10,  0);
        add_group(control, "int_trigger", "ct", 9,  9,  0);
    }
    return 0;
}

static void declare_groups(FILE *vcd, struct groups *groups)
{
    int i;

    for (i = 0; i < groups->cnt; i++)
    {
        struct group *g = &groups->group[i];

        if (g->lsb == g->msb)
        {
            fprintf(vcd, "$var wire 1 %s %s $end\n", g->id, g->name);
        }
        else
        {
            int width = (g->msb - g->lsb) + 1;
            fprintf(vcd, "$var reg %d %s %s [%d:0] $end\n", width, g->id, g->name, width - 1);
        }
    }
}

static void init_groups(FILE *vcd, struct groups *groups, uint32_t sampling)
{
    int i;
    printf("Group %d\n",groups->cnt);
    for (i = 0; i < groups->cnt; i++)
    {
        struct group *g = &groups->group[i];
        int width = (g->msb - g->lsb) + 1;
        uint32_t msk = (((uint32_t)1 << width) - 1);
        uint32_t smp_msk = (sampling >> g->lsb) & msk;

        if (width == 1)
        {
            if (smp_msk)
                fprintf(vcd, "%d%s\n", g->prev, g->id);
            else
                fprintf(vcd, "x%s\n", g->id);
        }
        else
        {
            char bin[64];
            int2bin(bin, g->prev, width, smp_msk);
            fprintf(vcd, "b%s %s\n", bin, g->id);
        }
    }
}


static void put_groups(FILE *vcd, struct groups *groups, uint32_t value, uint32_t sampling)
{
    int i;

    for (i = 0; i < groups->cnt; i++)
    {
        struct group *g = &groups->group[i];
        unsigned int width = (g->msb - g->lsb) + 1;
        unsigned int offset = g->lsb;
        uint32_t msk = (uint32_t)((((uint64_t)1 << width) - 1) << offset);
        uint32_t curr = (value & msk) >> offset;
        uint32_t smp_msk = (sampling & msk) >> offset;

        if (curr != g->prev)
        {
            if (width == 1)
            {
                if (smp_msk)
                    fprintf(vcd, "%d%s\n", curr, g->id);
            }
            else
            {
                char bin[64];
                if (int2bin(bin, curr, width, smp_msk) == 0)
                    fprintf(vcd, "b%s %s\n", bin, g->id);
            }
            g->prev = curr;
        }
    }
}

/*
 *****************************************************************************************
 * @brief Main entry point of the application.
 *
 * @param argc   usual parameter counter
 * @param argv   usual parameter values
 *****************************************************************************************
 */
int main(int argc, char **argv)
{
	FILE *vcd;
	FILE *trace;
	uint32_t value[2] = {0};
	uint32_t pre_value[2] = {0};
#if USE_SIMULATION_TXT_DEBUG
	char buf[17];
	char buf1[9];
	char buf2[9];
	char *pos;
#endif
	//now sampling is 16bits
	//sampling[0] is low 16bits and middle 16bits data
	//sampling[1] is high 16bits data and control data
	uint32_t sampling[2];
	uint32_t la_version_mac;
	unsigned long long time = 0;
	unsigned long long delay = DELAY;
	unsigned long long freq;

	struct groups siwifidiag0;
	struct groups siwifidiag1;
	struct groups siwifidiag2;
	struct groups controls;
	int i = 0;

	/* strip off self */
	argc--;
	argv++;
	if (argc == 0)
		return -1;

	/* Get LA configuration */
	get_la_conf(sampling, &la_version_mac, argv[0], "lamacconf");
	sampling[1] |= 0xFFFF0000;

	/* Get LA clk from platform*/
#if defined(SIFLOWER_SF16A18) || defined(SIFLOWER_SF19A28)
#ifdef USE_SIMULATION_TXT_DEBUG
	freq = 375;
#else
	freq = get_la_clk(argv[0]);
#endif
	if (freq != 0)
	{
		printf("la_clk is %lld\n", freq);
		delay = ((unsigned long long)1000000)/freq;
	}
#else
	/* In case the sampling frequency information is available, recompute the
	 * VCD delay
	 */
	freq = (unsigned long long)(la_version_mac >> 24);
	if (freq != 0)
	{
		delay = ((unsigned long long)1000000)/freq;
	}
#endif

	printf("LA Info.\n Sampling Freq : %i MHz\n Version : %i.%i\n", 1000000/delay, (la_version_mac&0xFF0000) >> 16,(la_version_mac&0xFFFF));

	{   // Siflower FPGA v7 platform
		/* create groups of signals */
		create_siwifidiag_groups(&siwifidiag0, argv[0], 0);
		create_siwifidiag_groups(&siwifidiag1, argv[0], 1);
		create_siwifidiag_groups(&siwifidiag2, argv[0], 2);
	}

	create_control_groups(&controls,la_version_mac & 0xFFFFF);

	printf("Create VCD file\n");

	/* create VCD file */
	vcd = vcd_open(argv[0]);
	if (vcd == NULL)
	{
		printf("Failed opening the VCD file\n");
		return -1;
	}

	/* print file header */
	fprintf(vcd, "$comment\nTOOL: RW LA\n$end\n$date\n$end\n$timescale\n1ps\n$end\n");

	/* declare groups */
	declare_groups(vcd, &controls);
	declare_groups(vcd, &siwifidiag2);
	declare_groups(vcd, &siwifidiag0);
	declare_groups(vcd, &siwifidiag1);

	fprintf(vcd, "\n$enddefinitions\n$dumpvars\n");

	/* initialize groups (first part) */
	init_groups(vcd, &controls, (sampling[1] & 0xFFFF0000) >> 16);
	init_groups(vcd, &siwifidiag2, sampling[1] & 0x0000FFFF);
	init_groups(vcd, &siwifidiag0, sampling[0] & 0x0000FFFF);
	init_groups(vcd, &siwifidiag1, (sampling[0] & 0xFFFF0000) >> 16);

	fprintf(vcd, "\n$end\n#0\n");

	/* initialize groups (second part) */
	init_groups(vcd, &controls, (sampling[1] & 0xFFFF0000) >> 16);
	init_groups(vcd, &siwifidiag2, sampling[1] & 0x0000FFFF);
	init_groups(vcd, &siwifidiag0, sampling[0] & 0x0000FFFF);
	init_groups(vcd, &siwifidiag1, (sampling[0] & 0xFFFF0000) >> 16);

	/* open trace file from Dini */
	trace = trace_open(argv[0]);
	if (trace == NULL)
	{
		printf("Failed opening the la0 file\n");
		return -1;
	}

	/* go through the whole file */
	while(1)
	{
#if USE_SIMULATION_TXT_DEBUG
		if(fgets(&buf[0], 17, trace) == NULL)
			break;
		i++;
		if(i % 2 == 0)
			continue;
		buf[16] = 0;
		memcpy(buf1, &buf[0], 8);
		memcpy(buf2, &buf[8], 8);
		buf1[8] = 0;
		buf2[8] = 0;
		pos = NULL;
		value[1] = strtoul(buf1, &pos,16);
		value[0] = strtoul(buf2, &pos,16);
#else
		for (i=0; i<2; i++)
		{
			if (fread(&value[i], 4, 1, trace) != 1)
				break;
		}
		if (i < 2)
			break;
#endif
		//bit 31 is 1 represent only time,sample due to time wrap
		//only increase time,do not write vcd file
		if((value[1] & 0x80000000))
		{
			//64bits: sample_type(1) + reserved(39) + time_cnt(24)
			time += (unsigned long long)(value[0] & 0x00FFFFFF) * delay;
		}

		if(pre_value[0] != 0 || pre_value[1] != 0)
		{
			//64bits: sample_type(1) + trigger_status(6) + time_cnt(9) + data(48)
			time += (unsigned long long)((pre_value[1] & 0x01FF0000) >> 16) * delay;
			put_groups(vcd, &controls, (pre_value[1] & 0x7E000000) >> 16, (sampling[1] & 0xFFFF0000) >> 16);
			put_groups(vcd, &siwifidiag2, pre_value[1] & 0xFFFF, sampling[1] & 0x0000FFFF);
			put_groups(vcd, &siwifidiag1, pre_value[0] >> 16, (sampling[0] & 0xFFFF0000) >> 16);
			put_groups(vcd, &siwifidiag0, pre_value[0] & 0xFFFF, sampling[0] & 0x0000FFFF);

			fprintf(vcd, "#%llu\n", time);
		}
		else
		{
			pre_value[0] = value[0];
			pre_value[1] = value[1];
		}

		if((value[1] & 0x80000000))
		{
			pre_value[0] = 0;
			pre_value[1] = 0;
		}
		else
		{
			pre_value[0] = value[0];
			pre_value[1] = value[1];
		}
	}

	if(pre_value[0] != 0 || pre_value[1] != 0)
	{
		//64bits: sample_type(1) + trigger_status(6) + time_cnt(9) + data(48)
		//time_cnt is duration of this data = interval with next data,so write to file after data
		time += (unsigned long long)((pre_value[1] & 0x01FF0000) >> 16) * delay;
		put_groups(vcd, &controls, (pre_value[1] & 0x7E000000) >> 16, (sampling[1] & 0xFFFF0000) >> 16);
		put_groups(vcd, &siwifidiag2, pre_value[1] & 0xFFFF, sampling[1] & 0x0000FFFF);
		put_groups(vcd, &siwifidiag1, pre_value[0] >> 16, (sampling[0] & 0xFFFF0000) >> 16);
		put_groups(vcd, &siwifidiag0, pre_value[0] & 0xFFFF, sampling[0] & 0x0000FFFF);

		fprintf(vcd, "#%llu\n", time);
	}

	fclose(vcd);
	fclose(trace);

	return 0;
}
