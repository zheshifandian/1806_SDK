/**
 *****************************************************************************************
 * @file gendiags.c
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
#define DELAY (unsigned long long)14286

#define DIAGS_MAC_MAX     48

#define DIAGS_PHY_MAX     32

struct group {
    char name[255];
    char id[15];
    int bank;
    int lsb;
    int msb;
    uint16_t value;
};

struct groups {
    int cnt;
    struct group group[DIAGS_MAC_MAX * 16];
};

/*
 * FUNCTION DEFINITIONS
 *****************************************************************************************
 */
static int int2bin(char *out, uint32_t val, int width)
{
    int status = -1;

    out[0] = '\0';

    uint32_t z = 1 << (width - 1);
    for (; z > 0; z >>= 1)
    {
        strcat(out, ((val & z) == z) ? "1" : "0");
        status = 0;
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

static int add_group(struct groups *groups, char *name, char *id_prefix, int lsb, int msb, int bank)
{
    struct group *g;

    if (groups->cnt == (DIAGS_MAC_MAX * 16))
        return -1;

    if (msb >= 32)
        return -1;

    g = &groups->group[groups->cnt];

    strcpy(g->name, name);
    sprintf(g->id, "%s%d", id_prefix, groups->cnt);
    g->lsb = lsb;
    g->msb = msb;
    g->bank = bank;

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

static FILE *vcd_open(char *path)
{
    FILE *fp;
    char filename[256];

    snprintf(filename, 255, "%s/diags.vcd", path);
    fp = fopen(filename, "w");

    return fp;
}

static int get_phy_diags(uint16_t *diags, char *path)
{
    FILE *fp;
    char filename[256];
    int i;

    snprintf(filename, 255, "%s/phydiags", path);
    fp = fopen(filename, "r");
    if (fp == NULL)
        return -1;

    for (i=0; i<DIAGS_PHY_MAX; i++)
    {
        if (fread(&diags[i], 2, 1, fp) != 1)
            break;
    }

    fclose (fp);
    return 0;
}

static int get_mac_diags(uint16_t *diags, char *path)
{
    FILE *fp;
    char filename[256];
    int i;

    snprintf(filename, 255, "%s/macdiags", path);
    fp = fopen(filename, "r");
    if (fp == NULL)
        return -1;

    for (i=0; i<DIAGS_MAC_MAX; i++)
    {
        if (fread(&diags[i], 2, 1, fp) != 1)
            break;
    }

    fclose (fp);
    return 0;
}

//static int create_swdiag_groups(struct groups *swdiags, char *path)
//{
//    FILE *fp;
//    char filename[256];
//    int i = 0;
//    char line_prev[256] = "";
//
//    printf("Create SW diag groups\n");
//
//    swdiags->cnt = 0;
//
//    snprintf(filename, 255, "%s/swdiags", path);
//    fp = fopen(filename, "r");
//
//    if (fp == NULL)
//        return -1;
//
//    while (1)
//    {
//        char *line;
//        ssize_t read;
//        read = readline(&line, fp);
//
//        if (read <= 0)
//            break;
//
//        // Compare with previous diag name to know if it is part of a group
//        if (strcmp(line_prev, line))
//        {
//            /* Create new group */
//            add_group(swdiags, line, "sw", i, i, 0);
//            strcpy(line_prev, line);
//        }
//        else
//        {
//            enlarge_group(swdiags);
//        }
//
//        free(line);
//        i++;
//    }
//    for (;i < 32; i++)
//    {
//        char name[32];
//
//        sprintf(name, "SWDIAG%d", i);
//
//        /* Add to the SW diags groups */
//        add_group(swdiags, name, "sw", i, i, 0);
//    }
//
//    fclose(fp);
//
//    return 0;
//}
//
static int create_groups(struct groups *groups, char *file, char *path)
{
    int j;
    FILE *names;
    char *line;
    ssize_t read;
    char line_prev[256] = "";
    int lsb = 0;
    int bank;
    char filename[256];

    printf("Create diag groups\n");

    groups->cnt = 0;

    // Open file containing the diag banks
    snprintf(filename, 255, "%s/%s", path, file);
    names = fopen(filename, "r");
    if (names == NULL)
        return -1;

    // Walk through the name file and create the groups for the LSB and MSB config
    while (1)
    {
        // Find the beginning of an HW diag configuration in the name file
        while (1)
        {
            // Read the HW diag configuration
            read = readline(&line, names);
            if (read < 0)
            {
                fclose(names);
                return 0;
            }
            if (!strncmp(line, "0x", 2))
                break;

            free(line);
        }

        // Get the bank index
        if (sscanf(line, "0x%02x", &bank) != 1)
        {
            free(line);
            fclose(names);
            return -1;
        }

        free(line);

        for (j = 0; j < 16; j++)
        {
            read = readline(&line, names);

            if (read <= 0)
                break;

            // Compare with previous diag name to know if it is part of a group
            if (strcmp(line_prev, line))
            {
                /* Create new group */
                add_group(groups, line, "mac", lsb, lsb, bank);
                strcpy(line_prev, line);
            }
            else
            {
                enlarge_group(groups);
            }
            lsb++;
            free(line);
        }
        lsb = 0;
        line_prev[0] = '\0';
    }

    fclose(names);

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

static void init_groups(FILE *vcd, struct groups *groups)
{
    int i;

    for (i = 0; i < groups->cnt; i++)
    {
        struct group *g = &groups->group[i];
        int width = (g->msb - g->lsb) + 1;

        if (width == 1)
        {
            fprintf(vcd, "%d%s\n", g->value, g->id);
        }
        else
        {
            char bin[64];
            int2bin(bin, g->value, width);
            fprintf(vcd, "b%s %s\n", bin, g->id);
        }
    }
//    for (i = 0; i < groups->cnt; i++)
//    {
//        struct group *g = &groups->group[i];
//        int width = (g->msb - g->lsb) + 1;
//
//        if (width == 1)
//        {
//            fprintf(vcd, "%d%s\n", g->value, g->id);
//        }
//        else
//        {
//            char bin[64];
//            if (int2bin(bin, g->value, width) == 0)
//                fprintf(vcd, "b%s %s\n", bin, g->id);
//        }
//    }
}


static void set_values(struct groups *groups, uint16_t *values)
{
    int i;

    for (i = 0; i < groups->cnt; i++)
    {
        struct group *g = &groups->group[i];
        unsigned int width = (g->msb - g->lsb) + 1;
        unsigned int offset = g->lsb;
        uint32_t msk = (((uint32_t)1 << width) - 1) << offset;
        uint16_t value = (values[g->bank] & msk) >> offset;

        g->value = value;
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
    uint16_t mac_diags[DIAGS_MAC_MAX];
    uint16_t phy_diags[DIAGS_PHY_MAX];

    struct groups macgroups;
    struct groups phygroups;

    /* strip off self */
    argc--;
    argv++;

    if (argc == 0)
        return -1;

    /* Initialize diags arrays */
    get_mac_diags(mac_diags, argv[0]);
    get_phy_diags(phy_diags, argv[0]);

    /* create groups of signals */
    create_groups(&macgroups, "mac_hwdiag.txt", argv[0]);
    create_groups(&phygroups, "hdm_hwdiag.txt", argv[0]);

    /* put values to signals */
    set_values(&macgroups, mac_diags);
    set_values(&phygroups, phy_diags);

    printf("Create VCD file\n");

    /* create VCD file */
    vcd = vcd_open(argv[0]);
    if (vcd == NULL)
    {
        printf("Failed opening the VCD file\n");
        return -1;
    }

    /* print file header */
    fprintf(vcd, "$comment\nTOOL: RW BLA\n$end\n$date\n$end\n$timescale\n1ps\n$end\n");

    /* declare groups */
    declare_groups(vcd, &macgroups);
    declare_groups(vcd, &phygroups);

    fprintf(vcd, "\n$enddefinitions\n$dumpvars\n");

    /* initialize groups (first part) */
    init_groups(vcd, &macgroups);
    init_groups(vcd, &phygroups);

    fprintf(vcd, "\n$end\n#0\n");
    /* initialize groups (second part) */
    init_groups(vcd, &macgroups);
    init_groups(vcd, &phygroups);

    /* initialize groups (third part) */
    fprintf(vcd, "#%llu\n", 42 * DELAY);
    init_groups(vcd, &macgroups);
    init_groups(vcd, &phygroups);

    fclose(vcd);

    return 0;
}
