/*
 * MIT License
 * C-GSX Copyright 2026 John Elliott
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "cgsximpl.h"

static int gdos_opnwk(PGDOS self, gword *contrl, gword *intin, gword *ptsin,
			gword *intout, gword *ptsout);
static int gdos_clswk(PGDOS self);

static void gdos_scale_ptsin(PGDOS self, gword *ptsin, gword count);
static void gdos_scale_ptsout(PGDOS self, gword *ptsout, gword count);

int gdos_create(PGDOS *ptr)
{
	int n;

	*ptr = malloc(sizeof(struct GDOS));

	if (!*ptr) 	
	{
		return -1;
	}
	memset(*ptr, 0, sizeof(struct GDOS));
	(*ptr)->curdev = 0;
	(*ptr)->width  = 0;
	(*ptr)->height = 0;
	for (n = 0; n < MAXDEVICE; n++)
	{
		(*ptr)->create[n] = NULL;
		(*ptr)->param[n]  = NULL;
		(*ptr)->device[n] = NULL;
	}
	return 0;
}

void gdos_destroy(PGDOS *ptr)
{
	if (*ptr)
	{
		if ((*ptr)->curdev)	
		{
			gdos_clswk(*ptr);
		}
		free(*ptr);
		*ptr = NULL;
	}
}

int gdos_register_raster(PGDOS self, gword device, RASTERSURFACE *s)
{
	return gdos_register(self, device, gios_raster_create, s);
}

int gdos_register(PGDOS self, gword device, GIOSCREATEFUNC create, void *param)
{
	if (device == 0 || device >= MAXDEVICE)
	{
		return -1;
	}
	/* Unregister any existing device in that slot */
	gdos_unregister(self, device);

	/* And store the create function */
	self->create[device] = create;
	self->param [device] = param;
	self->device[device] = NULL;
	return 0;
}


void gdos_unregister(PGDOS self, gword device)
{
	if (device == 0 || device >= MAXDEVICE)
	{
		return;
	}
	/* If the device is currently open, close it */
	if (self->curdev == device)
	{
		gdos_clswk(self);
	}
	if (self->device[device] && self->device[device]->destroy)
	{
		(*self->device[device]->destroy)(&self->device[device]);
	}
	self->create[device] = NULL;
	self->param [device] = NULL;
	self->device[device] = NULL;

}


int gdos_dispatch(PGDOS self, gword *contrl, gword *intin, gword *ptsin,
			gword *intout, gword *ptsout)
{
	check_contrl(contrl);
	contrl[NINTOUT] = 0;
	contrl[NPTSOUT] = 0;

	/* It should always be the case that if curdev is nonzero, 
	 * self->device[curdev] is valid */
	assert(self->curdev == 0 || self->device[self->curdev] != NULL);

 	if (contrl[0] == 1)
	{
		return gdos_opnwk(self, contrl, intin, ptsin, intout, ptsout);	
	}
	/* If the function wasn't "open" and no device is open, there's 
	 * nothing that can be done */
	if (self->curdev == 0 || self->device[self->curdev] == NULL)
	{
		return -1;
	}
	if (contrl[0] == 2)
	{
		return gdos_clswk(self);
	}
	gdos_scale_ptsin(self, ptsin, contrl[NPTSIN]);
	(*self->device[self->curdev]->dispatch)(self->device[self->curdev], 
			contrl, intin, ptsin, intout, ptsout);
	gdos_scale_ptsout(self, ptsout, contrl[NPTSOUT]);

	/* It should always be the case that if curdev is nonzero, 
	 * self->device[curdev] is valid */
	assert(self->curdev == 0 || self->device[self->curdev] != NULL);
	return 0;
}



static int gdos_opnwk(PGDOS self, gword *contrl, gword *intin, gword *ptsin,
			gword *intout, gword *ptsout)
{
	gword dev = intin[0];
	GIOSDEVICE *newdev = NULL;

	if (dev == 0 || dev >= MAXDEVICE || !self->create[dev])
	{
fprintf(stderr, "GDOS: Device %d does not exist\n", dev);
		return 0;	/* Device number out of range */
	}
	if ((*self->create[dev])(self, &newdev, self->param[dev]))
	{
fprintf(stderr, "GDOS: Device %d create failed\n", dev);
		return 0;	/* Couldn't instantiate GIOS device */
	}
	/* The new device has been created. If there's an existing one 
	 * tear it down */
	if (self->curdev)
	{
		gdos_clswk(self);
	}
	self->curdev = dev;
	self->device[dev] = newdev;
	if (newdev)	/* Which should always be the case */
	{
		(*newdev->dispatch)(newdev, contrl, intin, ptsin, 
				intout, ptsout);
	}
	self->width  = intout[0] + 1;
	self->height = intout[1] + 1;
	gdos_scale_ptsout(self, ptsout, contrl[NPTSOUT]);
	return 0;
}


static int gdos_clswk(PGDOS self)
{
	gword dev = self->curdev;
	static gword contrl[10] = { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	gword dummy;

	/* It should always be the case that if curdev is nonzero, 
	 * self->device[curdev] is valid */
	assert(self->curdev == 0 || self->device[self->curdev] != NULL);
	if (dev == 0 || dev >= MAXDEVICE)
	{
		return -1;	/* Device number out of range */
	}
	/* Call the device's close function */
	(*self->device[dev]->dispatch)(self->device[dev], 
		contrl, &dummy, &dummy, &dummy, &dummy);

	/* If it has a destructor call that */
	if (self->device[dev]->destroy)
	{
		(*self->device[dev]->destroy)(&self->device[dev]);
	}
	self->device[dev] = NULL;
	self->curdev = 0;
	/* It should always be the case that if curdev is nonzero, 
	 * self->device[curdev] is valid */
	assert(self->curdev == 0 || self->device[self->curdev] != NULL);
	return 0;
}

static gword scale_to_dev(gword w, gword scale)
{
	w = (w * scale) / 32768;
	if (w > scale) w = scale;
	return w;
}


static gword scale_from_dev(gword w, gword scale)
{
	w = (w * 32768) / scale;
	if (w > 32767) w = 32767;
	return w;
}


static void gdos_scale_ptsin(PGDOS self, gword *ptsin, gword count)
{
	gword n;

	for (n = 0; n < count; n++)
	{
		ptsin[n*2  ] = scale_to_dev(ptsin[n*2],   self->width);
		ptsin[n*2+1] = scale_to_dev(ptsin[n*2+1], self->height);
	}
}


static void gdos_scale_ptsout(PGDOS self, gword *ptsout, gword count)
{
	gword n;

	for (n = 0; n < count; n++)
	{
		ptsout[n*2  ] = scale_from_dev(ptsout[n*2  ], self->width);
		ptsout[n*2+1] = scale_from_dev(ptsout[n*2+1], self->height);
	}
}


