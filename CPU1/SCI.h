/*
 * SCI.h
 *
 *  Created on: Jan 26, 2019
 *      Author: win7 vm
 */

#ifndef SCI_H_
#define SCI_H_

extern void scia_init(void);
extern void scia_fifo_init(void);
extern void scia_xmit(int a);
extern void scia_msg(char *msg);

#endif /* SCI_H_ */
