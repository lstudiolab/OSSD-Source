/*
 * Copyright (c) 2000-2007 Apple Inc. All rights reserved.
 */
/*
 * Copyright (c) 1987, 1988 NeXT, Inc.
 *
 * HISTORY 7-Jan-93  Mac Gillon (mgillon) at NeXT Integrated POSIX support
 *
 * 12-Aug-87  John Seamons (jks) at NeXT Ported to NeXT.
 */

  
                               
   
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/uio.h>

struct tty      *constty;                                           

  
                                                                    
                                                                        
   
extern struct tty *km_tty[1];

  
                                                 
   
int cnopen(__unused dev_t dev, int flag, int devtype, proc_t pp);
int cnclose(__unused dev_t dev, int flag, int mode, proc_t pp);
int cnread(__unused dev_t dev, struct uio *uio, int ioflag);
int cnwrite(__unused dev_t dev, struct uio *uio, int ioflag);
int cnioctl(__unused dev_t dev, u_long cmd, caddr_t addr, int flg, proc_t p);
int cnselect(__unused dev_t dev, int flag, void * wql, proc_t p);

static dev_t
cndev(void)
{
	if (constty) {
		return constty->t_dev;
	} else {
		return km_tty[0]->t_dev;
	}
}

int
cnopen(__unused dev_t dev, int flag, int devtype, struct proc *pp)
{
	dev = cndev();
	return (*cdevsw[major(dev)].d_open)(dev, flag, devtype, pp);
}


int
cnclose(__unused dev_t dev, int flag, int mode, struct proc *pp)
{
	dev = cndev();
	return (*cdevsw[major(dev)].d_close)(dev, flag, mode, pp);
}


int
cnread(__unused dev_t dev, struct uio *uio, int ioflag)
{
	dev = cndev();
	return (*cdevsw[major(dev)].d_read)(dev, uio, ioflag);
}


int
cnwrite(__unused dev_t dev, struct uio *uio, int ioflag)
{
	dev = cndev();
	return (*cdevsw[major(dev)].d_write)(dev, uio, ioflag);
}


int
cnioctl(__unused dev_t dev, u_long cmd, caddr_t addr, int flag, struct proc *p)
{
	dev = cndev();

	  
                                                                     
                                                                    
                                                                
   
                                                             
                                      
    
	if ((unsigned) cmd == TIOCCONS && constty) {
		int             error = proc_suser(p);
		if (error) {
			return error;
		}
		constty = NULL;
		return 0;
	}
	return (*cdevsw[major(dev)].d_ioctl)(dev, cmd, addr, flag, p);
}


int
cnselect(__unused dev_t dev, int flag, void *wql, struct proc *p)
{
	dev = cndev();
	return (*cdevsw[major(dev)].d_select)(dev, flag, wql, p);
}
