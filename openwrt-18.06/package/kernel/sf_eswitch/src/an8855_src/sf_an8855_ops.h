#ifndef _SF_AN8855_OPS_H_
#define _SF_AN8855_OPS_H_

int an8855_phy_write(unsigned int phyNo, unsigned int phyAddr, unsigned int pRegData);
int an8855_phy_read(unsigned int phyNo, unsigned int phyAddr, unsigned int *pRegData);
#endif /* _SF_AN8855_OPS_H_ */
