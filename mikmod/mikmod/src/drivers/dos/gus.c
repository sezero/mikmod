/*

Name: GUS.C

Description:
  Basic GUS driver routines used by all GUS modes.

Portability:
 MSDOS:  BC(y)   Watcom(y)       DJGPP(y)
 Win95:  n
 Os2:    n
 Linux:  n

 (y) - yes
 (n) - no (not possible or not useful)
 (?) - may be possible, but not tested

*/

#include <dos.h>
#include <conio.h>
#include "gus.h"
#include "mirq.h"

UWORD GUS_DRAM_DMA;

/***************************************************************************
>>>>>>>>>>>>>>>>>>>>>>>>> Lowlevel GUS code <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
***************************************************************************/

static UWORD GUS_PORT;
static UBYTE GUS_VOICES;
static UBYTE GUS_TIMER_CTRL;
static UBYTE GUS_TIMER_MASK;
static UBYTE GUS_MIX_IMAGE;

static UWORD GUS_ADC_DMA;
static UWORD GUS_GF1_IRQ;
static UWORD GUS_MIDI_IRQ;
static ULONG GUS_POOL;           // dram address of first gusmem pool node

static UBYTE GUS_SELECT;         // currently selected GF1 register

static void (*GUS_TIMER1_FUNC)(void);
static void (*GUS_TIMER2_FUNC)(void);
static void (*GUS_DMA_FUNC)(void);

#define UltraSelect(x) outportb(GF1_REG_SELECT,GUS_SELECT=x)

/***************************************************************
 * This function will convert the value read from the GF1 registers
 * back to a 'real' address.
 ***************************************************************/

#define MAKE_MS_SWORD( x )       ((unsigned long)((unsigned long)(x)) << 16)

static ULONG make_physical_address(UWORD low,UWORD high,UBYTE mode)
{
        UWORD lower_16, upper_16;
        ULONG ret_address, bit_19_20;

        upper_16 = high >> 9;
        lower_16 = ((high & 0x01ff) << 7) | ((low >> 9) & 0x007f);

        ret_address = MAKE_MS_SWORD(upper_16) + lower_16;

        if (mode & VC_DATA_TYPE)
        {   bit_19_20 = ret_address & 0xC0000;
            ret_address <<= 1;
            ret_address &= 0x3ffff;
            ret_address |= bit_19_20;
        }

        return( ret_address );
}

/***************************************************************
 * This function will translate the address if the dma channel
 * is a 16 bit channel. This translation is not necessary for
 * an 8 bit dma channel.
 ***************************************************************/

static ULONG convert_to_16bit(ULONG address)
// unsigned long address;               // 20 bit ultrasound dram address
{
    ULONG hold_address;

    hold_address = address;

    // Convert to 16 translated address.
    address = address >> 1;

    // Zero out bit 17.
    address &= 0x0001ffffL;

    // Reset bits 18 and 19.
    address |= (hold_address & 0x000c0000L);

    return(address);
}


static void GF1OutB(UBYTE x, UBYTE y)
{
    UltraSelect(x);
    outportb(GF1_DATA_HI,y);
}


static void GF1OutW(UBYTE x, UWORD y)
{
    UltraSelect(x);
    outport(GF1_DATA_LOW,y);
}


static UBYTE GF1InB(UBYTE x)
{
    UltraSelect(x);
    return inportb(GF1_DATA_HI);
}


static UWORD GF1InW(UBYTE x)
{
    UltraSelect(x);
    return inport(GF1_DATA_LOW);
}


static void gf1_delay(void)
{
    inportb(GF1_DRAM);  inportb(GF1_DRAM);
    inportb(GF1_DRAM);  inportb(GF1_DRAM);
    inportb(GF1_DRAM);  inportb(GF1_DRAM);
    inportb(GF1_DRAM);
}


UBYTE UltraPeek(ULONG address)
{
    GF1OutW(SET_DRAM_LOW,address);
    GF1OutB(SET_DRAM_HIGH,(address>>16)&0xff);      // 8 bits
    return(inportb(GF1_DRAM));
}


void UltraPoke(ULONG address,UBYTE data)
{
    GF1OutW(SET_DRAM_LOW,address);
    GF1OutB(SET_DRAM_HIGH,(address>>16)&0xff);
    outportb(GF1_DRAM,data);
}


static void UltraPokeFast(ULONG address, UBYTE *src, ULONG size)

//  [address,size> doesn't cross 64k page boundary

{
    if(!size) return;

    UltraSelect(SET_DRAM_HIGH);
    outportb(GF1_DATA_HI,(address>>16)&0xff);       // 8 bits
    UltraSelect(SET_DRAM_LOW);

    while(size--)
    {   outport(GF1_DATA_LOW,address);
        outportb(GF1_DRAM,*src);
        address++;
        src++;
    }
}


void UltraPokeChunk(ULONG address, UBYTE *src, ULONG size)
{
    ULONG todo;

    // first 'todo' is number of bytes 'till first 64k boundary

    todo=0x10000-(address&0xffff);
    if(todo>size) todo=size;

    do
    {   UltraPokeFast(address,src,todo);
        address += todo;
        src     += todo;
        size    -= todo;

        // next 'todo' is in chunks of max 64k at once
        todo=(size>0xffff) ? 0x10000 : size;

    } while(todo);
}


static ULONG UltraPeekLong(ULONG address)
{
    ULONG data;
    UBYTE *s = (UBYTE *)&data;

    s[0] = UltraPeek(address);
    s[1] = UltraPeek(address+1);
    s[2] = UltraPeek(address+2);
    s[3] = UltraPeek(address+3);
    return data;
}


static void UltraPokeLong(ULONG address,ULONG data)
{
    UltraPokeChunk(address,(UBYTE *)&data,4);
}


void UltraEnableOutput(void)
{
    GUS_MIX_IMAGE &= ~ENABLE_OUTPUT;
    outportb(GF1_MIX_CTRL,GUS_MIX_IMAGE);
}

void UltraDisableOutput(void)
{
    GUS_MIX_IMAGE |= ENABLE_OUTPUT;
    outportb(GF1_MIX_CTRL,GUS_MIX_IMAGE);
}

/*
static void UltraEnableLineIn(void)
{
    GUS_MIX_IMAGE &= ~ENABLE_LINE_IN;
    outportb(GF1_MIX_CTRL,GUS_MIX_IMAGE);
}
*/
static void UltraDisableLineIn(void)
{
    GUS_MIX_IMAGE |= ENABLE_LINE_IN;
    outportb(GF1_MIX_CTRL,GUS_MIX_IMAGE);
}

/*
static void UltraEnableMicIn(void)
{
    GUS_MIX_IMAGE |= ENABLE_MIC_IN;
    outportb(GF1_MIX_CTRL,GUS_MIX_IMAGE);
}
*/

static void UltraDisableMicIn(void)
{
    GUS_MIX_IMAGE &= ~ENABLE_MIC_IN;
    outportb(GF1_MIX_CTRL,GUS_MIX_IMAGE);
}


static void UltraReset(int voices)
{
    int v;

    if(voices<14) voices = 14;
    if(voices>32) voices = 32;

    GUS_VOICES     = voices;
    GUS_TIMER_CTRL = 0;
    GUS_TIMER_MASK = 0;

    UltraPokeLong(0,0);

    GF1OutB(MASTER_RESET,0x00);
    for(v=0;v<10;v++) gf1_delay();

    // Release Reset and wait
    GF1OutB(MASTER_RESET,GF1_MASTER_RESET);
    for (v=0;v<10;v++) gf1_delay();

    // Reset the MIDI port also
    outportb(GF1_MIDI_CTRL,MIDI_RESET);
    for (v=0;v<10;v++) gf1_delay();
    outportb(GF1_MIDI_CTRL,0x00);

    // Clear all interrupts.
    GF1OutB(DMA_CONTROL,0x00);
    GF1OutB(TIMER_CONTROL,0x00);
    GF1OutB(SAMPLE_CONTROL,0x00);

    // Set the number of active voices
    GF1OutB(SET_VOICES,((voices-1) | 0xC0));

    // Clear interrupts on voices.
    // Reading the status ports will clear the irqs.

    inportb(GF1_IRQ_STAT);

    GF1InB(DMA_CONTROL);
    GF1InB(SAMPLE_CONTROL);
    GF1InB(GET_IRQV);

    for(v=0; v<voices; v++)
    {   // Select the proper voice
        outportb(GF1_PAGE,v);

        // Stop the voice and volume
        GF1OutB(SET_CONTROL,VOICE_STOPPED|STOP_VOICE);
        GF1OutB(SET_VOLUME_CONTROL,VOLUME_STOPPED|STOP_VOLUME);
         
        gf1_delay(); // Wait 4.8 micos. or more

        // Initialize each voice specific registers. This is not
        // really necessary, but is nice for completeness sake ..
        // Each application will set up these to whatever values
        // it needs.

        GF1OutW(SET_FREQUENCY,0x0400);
        GF1OutW(SET_START_HIGH,0);
        GF1OutW(SET_START_LOW,0);
        GF1OutW(SET_END_HIGH,0);
        GF1OutW(SET_END_LOW,0);
        GF1OutB(SET_VOLUME_RATE,0x01);
        GF1OutB(SET_VOLUME_START,0x10);
        GF1OutB(SET_VOLUME_END,0xe0);
        GF1OutW(SET_VOLUME,0x0000);

        GF1OutW(SET_ACC_HIGH,0);
        GF1OutW(SET_ACC_LOW,0);
        GF1OutB(SET_BALANCE,0x07);
    }

    inportb(GF1_IRQ_STAT);

    GF1InB(DMA_CONTROL);
    GF1InB(SAMPLE_CONTROL);
    GF1InB(GET_IRQV);

    // Set up GF1 Chip for interrupts & enable DACs.
/*      GF1OutB(MASTER_RESET,GF1_MASTER_RESET|GF1_OUTPUT_ENABLE); */
    GF1OutB(MASTER_RESET,GF1_MASTER_RESET|GF1_OUTPUT_ENABLE|GF1_MASTER_IRQ);
}


static BOOL UltraProbe(void)
{
    UBYTE s1,s2,t1,t2;

    // Pull a reset on the GF1
    GF1OutB(MASTER_RESET,0x00);

    // Wait a little while ...
    gf1_delay();
    gf1_delay();

    // Release Reset
    GF1OutB(MASTER_RESET,GF1_MASTER_RESET);

    gf1_delay();
    gf1_delay();

    s1=UltraPeek(0);   s2 = UltraPeek(1);
    UltraPoke(0,0xaa); t1 = UltraPeek(0);
    UltraPoke(1,0x55); t2 = UltraPeek(1);
    UltraPoke(0,s1);   UltraPoke(1,s2);

    return((t1==0xaa) && (t2==0x55));
}



BOOL UltraDetect(void)
{
    CHAR *ptr;

    if((ptr=getenv("ULTRASND"))==NULL) return 0;

    if(sscanf(ptr,"%hx,%hd,%hd,%hd,%hd",
                  &GUS_PORT,
                  &GUS_DRAM_DMA,
                  &GUS_ADC_DMA,
                  &GUS_GF1_IRQ,
                  &GUS_MIDI_IRQ)!=5) return 0;

    return(UltraProbe());
}




static UBYTE dmalatch[8]      = { 0,1,0,2,0,3,4,5 };
static UBYTE irqlatch[16]     = { 0,0,1,3,0,2,0,4,0,0,0,5,6,0,0,7 };


static void UltraSetInterface(int dram,int adc,int gf1,int midi)
/* int dram;    // dram dma chan */
/* int adc;             // adc dma chan */
/* int gf1;             // gf1 irq # */
/* int midi;    // midi irq # */
{
    UBYTE gf1_irq, midi_irq,dram_dma,adc_dma;
    UBYTE irq_control,dma_control;
    UBYTE mix_image;

    /* Don't need to check for 0 irq #. Its latch entry = 0 */
    gf1_irq =irqlatch[gf1];
    midi_irq=irqlatch[midi];
    midi_irq<<=3;

    dram_dma = dmalatch[dram];
    adc_dma  = dmalatch[adc];
    adc_dma<<=3;

    irq_control = dma_control = 0x0;

    mix_image   = GUS_MIX_IMAGE;

    irq_control|=gf1_irq;

    if((gf1==midi) && (gf1!=0))
       irq_control|=0x40;
    else
      irq_control|=midi_irq;

    dma_control|=dram_dma;

    if((dram==adc) && (dram!=0))
       dma_control|=0x40;
    else
       dma_control|=adc_dma;

    // Set up for Digital ASIC
    outportb(GUS_PORT+0x0f,0x5);
    outportb(GF1_MIX_CTRL,mix_image);
    outportb(GF1_IRQ_CTRL,0x0);
    outportb(GUS_PORT+0x0f,0x0);

    // First do DMA control register
    outportb(GF1_MIX_CTRL,mix_image);
    outportb(GF1_IRQ_CTRL,dma_control|0x80);

    // IRQ CONTROL REG
    outportb(GF1_MIX_CTRL,mix_image|0x40);
    outportb(GF1_IRQ_CTRL,irq_control);

    // First do DMA control register
    outportb(GF1_MIX_CTRL,mix_image);
    outportb(GF1_IRQ_CTRL,dma_control);

    // IRQ CONTROL REG
    outportb(GF1_MIX_CTRL,mix_image|0x40);
    outportb(GF1_IRQ_CTRL,irq_control);

    // IRQ CONTROL, ENABLE IRQ
    // just to Lock out writes to irq\dma register ...
    outportb(GF1_VOICE_SELECT,0);

    // enable output & irq, disable line & mic input
    mix_image|=0x09;
    outportb(GF1_MIX_CTRL,mix_image);

    // just to Lock out writes to irq\dma register ...
    outportb(GF1_VOICE_SELECT,0x0);

    // put image back ....
    GUS_MIX_IMAGE=mix_image;
}


static BOOL UltraPP(ULONG address)
{
    UBYTE s,t;
    
    s = UltraPeek(address);
    UltraPoke(address,0xaa);
    t = UltraPeek(address);
    UltraPoke(address,s);

    return(t==0xaa);
}


static UWORD UltraSizeDram(void)
{
    if(!UltraPP(0))      return 0;
    if(!UltraPP(262144)) return 256;
    if(!UltraPP(524288)) return 512;
    if(!UltraPP(786432)) return 768;
    return 1024;
}


ULONG UltraMemTotal(void)
{
    ULONG node = GUS_POOL,nsize,total=0;

    while(node!=0)
    {   nsize  = UltraPeekLong(node);
        total += nsize;
        node   = UltraPeekLong(node+4);
    }
    return total;
}


static BOOL Mergeable(ULONG a,ULONG b)
{
    return(a && b && (a+UltraPeekLong(a))==b);
}


static ULONG Merge(ULONG a,ULONG b)
{
    UltraPokeLong(a,UltraPeekLong(a)+UltraPeekLong(b));
    UltraPokeLong(a+4,UltraPeekLong(b+4));
    return a;
}


void UltraFree(ULONG size, ULONG location)
{
    ULONG pred=0,succ=GUS_POOL;

    if(!size) return;
    size+=31;
    size&=-32L;

    UltraPokeLong(location,size);

    while(succ!=0 && succ<=location)
    {  pred = succ;
       succ = UltraPeekLong(succ+4);
    }

    if(pred)
       UltraPokeLong(pred+4,location);
    else
       GUS_POOL = location;

    UltraPokeLong(location+4,succ);

    if(Mergeable(pred,location))
       location = Merge(pred,location);

    if(Mergeable(location,succ))
       Merge(location,succ);
}


/*
void DumpPool(void)
{
    ULONG node=GUS_POOL;

    while(node!=0)
    {   printf("Node %ld, size %ld, next %ld\n",node,UltraPeekLong(node),UltraPeekLong(node+4));
        node = UltraPeekLong(node+4);
    }
}
*/

ULONG UltraMalloc(ULONG reqsize)
{
    ULONG curnode = GUS_POOL,cursize,newnode,newsize,pred,succ;

    if(reqsize==0) return 0;

    // round size to 32 bytes

    reqsize+=31;
    reqsize&=-32L;

    // as long as there are nodes:

    pred = 0;

    while(curnode!=0)
    {   succ = UltraPeekLong(curnode+4);

        // get current node size
        cursize = UltraPeekLong(curnode);

        // requested block fits?
        if(cursize>=reqsize)
        {   // it fits, so we're allocating the first
            // 'size' bytes of this node

            // find new node position and size
            newnode = curnode+reqsize;
            newsize = cursize-reqsize;

            // create a new freenode if needed:
            if(newsize>=8)
            {   UltraPokeLong(newnode,newsize);
                UltraPokeLong(newnode+4,succ);
                succ = newnode;
            }

            // link prednode & succnode
            if(pred)
               UltraPokeLong(pred+4,succ);
            else
               GUS_POOL = succ;

            // store size of allocated memory block in block itself:
            UltraPokeLong(curnode,reqsize);
            return curnode;
        }

        // doesn't fit, try next node
        pred = curnode;
        curnode = succ;
    }
    return 0;
}



ULONG UltraMalloc16(ULONG reqsize)
//  Allocates a free block of gus memory, suited for 16 bit samples i.e.
//  smaller than 256k and doesn't cross a 256k page.
{
    ULONG p,spage,epage;

    if(reqsize>262144) return 0;

    // round size to 32 bytes
    reqsize+=31;
    reqsize&=-32L;

    p     = UltraMalloc(reqsize);
    spage = p>>18;
    epage = (p+reqsize-1)>>18;

    if(p && spage!=epage)
    {   ULONG newp, esize;

        // free the second part of the block, and try again
        esize = (p+reqsize)-(epage<<18);
        UltraFree(esize,epage<<18);

        newp = UltraMalloc16(reqsize);

        // free first part of the previous block
        UltraFree(reqsize-esize, p);

        p = newp;
    }

    return p;
}


static void UltraMemInit(void)
{
    UWORD memsize;

    GUS_POOL = 32;
    memsize  = UltraSizeDram();
    UltraPokeLong(GUS_POOL,((ULONG)memsize<<10)-32);
    UltraPokeLong(GUS_POOL+4,0);
}


void UltraNumVoices(int voices)
{
    UltraDisableLineIn();
    UltraDisableMicIn();
    UltraDisableOutput();
    UltraReset(voices);
    UltraSetInterface(GUS_DRAM_DMA,GUS_ADC_DMA,GUS_GF1_IRQ,GUS_MIDI_IRQ);
}


static UBYTE dma_dram_control;
static volatile BOOL dma_dram_active=0;
static PVI oldhandler;


static void interrupt gf1handler(MIRQARGS)
{
    UBYTE irq_source;
    UBYTE oldselect = GUS_SELECT;

    while((irq_source = inportb(GF1_IRQ_STAT)))
    {   if(irq_source & DMA_TC_IRQ)
        {   GF1InB(DMA_CONTROL);    // reset the IRQ pending bit
            dma_dram_active = 0;
            if(GUS_DMA_FUNC!=NULL) GUS_DMA_FUNC();
        }

/*              if(irq_source & (MIDI_TX_IRQ|MIDI_RX_IRQ)){
                    no provisions for MIDI-ready irq yet
            }
*/
         if(irq_source & GF1_TIMER1_IRQ)
         {   GF1OutB(TIMER_CONTROL,GUS_TIMER_CTRL & ~0x04);
             GF1OutB(TIMER_CONTROL,GUS_TIMER_CTRL);
             if(GUS_TIMER1_FUNC!=NULL) GUS_TIMER1_FUNC();
         }

         if(irq_source & GF1_TIMER2_IRQ)
         {   GF1OutB(TIMER_CONTROL,GUS_TIMER_CTRL & ~0x08);
             GF1OutB(TIMER_CONTROL,GUS_TIMER_CTRL);
             if(GUS_TIMER2_FUNC!=NULL) GUS_TIMER2_FUNC();
         }

/*              if (irq_source & (WAVETABLE_IRQ | ENVELOPE_IRQ)){
                    no wavetable or envelope irq provisions yet
            }
*/
    }

    MIrq_EOI(GUS_GF1_IRQ);
    UltraSelect(oldselect);
}


PFV UltraTimer1Handler(PFV handler)
{
    PFV old = GUS_TIMER1_FUNC;
    GUS_TIMER1_FUNC = handler;
    return old;
}


PFV UltraDmaHandler(PFV handler)
{
    PFV old = GUS_DMA_FUNC;
    GUS_DMA_FUNC = handler;
    return old;
}


/*static PFV UltraTimer2Handler(PFV handler)
{
    PFV old=GUS_TIMER2_FUNC;
    GUS_TIMER2_FUNC=handler;
    return old;
}
*/

void UltraSetDramDma(UBYTE control)
{
    dma_dram_control = control;
}


void UltraWaitDramDma(void)
{
    while(dma_dram_active);
}


BOOL UltraDramDmaActive(void)
{
    return dma_dram_active;
}


void UltraStartDramDma(void)
{
    GF1OutB(DMA_CONTROL, dma_dram_control | DMA_ENABLE);
    dma_dram_active = 1;
}


void UltraStopDramDma(void)
{
    GF1OutB(DMA_CONTROL, dma_dram_control & ~DMA_ENABLE);
    dma_dram_active = 0;
}


void UltraDownloadDma(ULONG dest)

// Uploads the given amount of data to the given location.
// If the PC's DMA is not yet programmed, the GUS will start the transfer
// as soon as it is.

{
    dma_dram_control &= ~DMA_READ;
    if(GUS_DRAM_DMA >= 4) dest = convert_to_16bit(dest);
    GF1OutW(SET_DMA_ADDRESS,dest>>4);

    UltraStartDramDma();
}


void UltraOpen(int voices)
{
    GUS_MIX_IMAGE   = 0x0b;
    GUS_TIMER1_FUNC = NULL;
    GUS_TIMER2_FUNC = NULL;
    GUS_DMA_FUNC    = NULL;

    dma_dram_active = 0;

    UltraDisableLineIn();
    UltraDisableMicIn();
    UltraDisableOutput();

    UltraReset(voices);
    UltraSetInterface(GUS_DRAM_DMA,GUS_ADC_DMA,GUS_GF1_IRQ,GUS_MIDI_IRQ);
    UltraMemInit();
    oldhandler = MIrq_SetHandler(GUS_GF1_IRQ,gf1handler);
    MIrq_OnOff(GUS_GF1_IRQ,1);    
}


void UltraClose(void)
{
    UltraStopDramDma();
    UltraDisableOutput();
    UltraDisableLineIn();
    UltraDisableMicIn();
    UltraReset(14);
}

void UltraSelectVoice(UBYTE voice)
{
    // Make sure was are talking to proper voice
    outportb(GF1_VOICE_SELECT,voice);
}


/*
static void UltraSetVoiceEnd(ULONG end)
{
        ULONG phys_end;
        UBYTE data;

        data=GF1InB(GET_CONTROL);

        phys_end=(data&VC_DATA_TYPE)?convert_to_16bit(end):end;

        // Set end address of buffer
        GF1OutW(SET_END_LOW,ADDR_LOW(phys_end));
        GF1OutW(SET_END_HIGH,ADDR_HIGH(phys_end));

        data&=~(VC_IRQ_PENDING|VOICE_STOPPED|STOP_VOICE);

        GF1OutB(SET_CONTROL,data);
        gf1_delay();

        GF1OutB(SET_CONTROL,data);
}
*/

/* The formula for this table is:
        1,000,000 / (1.619695497 * # of active voices)

        The 1.619695497 is calculated by knowing that 14 voices
                gives exactly 44.1 Khz. Therefore,
                1,000,000 / (X * 14) = 44100
                X = 1.619695497
*/

static UWORD freq_divisor[19] =
{       44100,          // 14 active voices
        41160,          // 15 active voices
        38587,          // 16 active voices
        36317,          // 17 active voices
        34300,          // 18 active voices
        32494,          // 19 active voices
        30870,          // 20 active voices
        29400,          // 21 active voices
        28063,          // 22 active voices
        26843,          // 23 active voices
        25725,          // 24 active voices
        24696,          // 25 active voices
        23746,          // 26 active voices
        22866,          // 27 active voices
        22050,          // 28 active voices
        21289,          // 29 active voices
        20580,          // 30 active voices
        19916,          // 31 active voices
        19293           // 32 active voices
};

void UltraSetFrequency(ULONG speed_khz)
{
        UWORD fc;
        ULONG temp;

        // FC is calculated based on the # of active voices ...
        temp=freq_divisor[GUS_VOICES-14];

        fc=(((speed_khz<<9L)+(temp>>1L))/temp);
        GF1OutW(SET_FREQUENCY,fc<<1);
}

/*
static void UltraSetLoopMode(UBYTE mode)
{
        UBYTE data;
        UBYTE vmode;

        // set/reset the rollover bit as per user request

        vmode=GF1InB(GET_VOLUME_CONTROL);

        if(mode&USE_ROLLOVER)
                vmode|=VC_ROLLOVER;
        else
                vmode&=~VC_ROLLOVER;

        GF1OutB(SET_VOLUME_CONTROL,vmode);
        gf1_delay();
        GF1OutB(SET_VOLUME_CONTROL,vmode);

        data=GF1InB(GET_CONTROL);

        data&=~(VC_WAVE_IRQ|VC_BI_LOOP|VC_LOOP_ENABLE); // isolate the bits 
        mode&=VC_WAVE_IRQ|VC_BI_LOOP|VC_LOOP_ENABLE;    // no bad bits passed in
        data|=mode;             // turn on proper bits ... 

        GF1OutB(SET_CONTROL,data);
        gf1_delay();
        GF1OutB(SET_CONTROL,data);
}
*/

ULONG UltraReadVoice(void)
{
        UWORD count_low,count_high;
        ULONG acc;
        UBYTE mode;

        // Get the high & low portion of the accumulator
        count_high = GF1InW(GET_ACC_HIGH);
        count_low = GF1InW(GET_ACC_LOW);

        // convert from UltraSound's format to a physical address
        mode = GF1InB(GET_CONTROL);

        acc = make_physical_address(count_low,count_high,mode);
        acc&=0xfffffL;          // Only 20 bits please ...

        return(acc);
}



/*static void UltraSetVoice(ULONG location)
{
        ULONG phys_loc;
        UBYTE data;

        data=GF1InB(GET_CONTROL);

        phys_loc=(data&VC_DATA_TYPE)?convert_to_16bit(location):location;

        // First set accumulator to beginning of data
        GF1OutW(SET_ACC_HIGH,ADDR_HIGH(phys_loc));
        GF1OutW(SET_ACC_LOW,ADDR_LOW(phys_loc));
}
*/


static UBYTE UltraPrimeVoice(ULONG begin,ULONG start,ULONG end,UBYTE mode)
{
        ULONG phys_start,phys_end;
        ULONG phys_begin;
        ULONG temp;
        UBYTE vmode;

        // if start is greater than end, flip 'em and turn on
        // decrementing addresses

        if(start > end)
        {   temp  = start;
            start = end;
            end   = temp;
            mode |= VC_DIRECT;
        }

        // if 16 bit data, must convert addresses

        if(mode & VC_DATA_TYPE)
        {   phys_begin = convert_to_16bit(begin);
            phys_start = convert_to_16bit(start);          
            phys_end   = convert_to_16bit(end);
        } else
        {   phys_begin = begin;
            phys_start = start;
            phys_end   = end;
        }

        // set/reset the rollover bit as per user request

        vmode = GF1InB(GET_VOLUME_CONTROL);
        vmode &= ~VC_ROLLOVER;

        GF1OutB(SET_VOLUME_CONTROL,vmode);
        gf1_delay();
        GF1OutB(SET_VOLUME_CONTROL,vmode);

        // First set accumulator to beginning of data
        GF1OutW(SET_ACC_LOW,ADDR_LOW(phys_begin));
        GF1OutW(SET_ACC_HIGH,ADDR_HIGH(phys_begin));

        // Set start loop address of buffer
        GF1OutW(SET_START_HIGH,ADDR_HIGH(phys_start));
        GF1OutW(SET_START_LOW,ADDR_LOW(phys_start));

        // Set end address of buffer
        GF1OutW(SET_END_HIGH,ADDR_HIGH(phys_end));
        GF1OutW(SET_END_LOW,ADDR_LOW(phys_end));
        return(mode);
}


static void UltraGoVoice(UBYTE mode)
{
        mode &= ~(VOICE_STOPPED | STOP_VOICE);   // turn 'stop' bits off ...

        // NOTE: no irq's from the voice ...

        GF1OutB(SET_CONTROL,mode);
        gf1_delay();
        GF1OutB(SET_CONTROL,mode);
}


/**********************************************************************
 *
 * This function will start playing a wave out of DRAM. It assumes
 * the playback rate, volume & balance have been set up before ...
 *
 *********************************************************************/

void UltraStartVoice(ULONG begin,ULONG start,ULONG end,UBYTE mode)
{
        mode = UltraPrimeVoice(begin,start,end,mode);
        UltraGoVoice(mode);
}



/***************************************************************
 * This function will stop a given voices output. Note that a delay
 * is necessary after the stop is issued to ensure the self-
 * modifying bits aren't a problem.
 ***************************************************************/

void UltraStopVoice(void)
{
    UBYTE data;

    // turn off the roll over bit first ...

    data=GF1InB(GET_VOLUME_CONTROL);
    data&=~VC_ROLLOVER;

    GF1OutB(SET_VOLUME_CONTROL,data);
    gf1_delay();
    GF1OutB(SET_VOLUME_CONTROL,data);

    // Now stop the voice 

    data=GF1InB(GET_CONTROL);
    data&=~VC_WAVE_IRQ;                // disable irq's & stop voice ..
    data|=VOICE_STOPPED|STOP_VOICE;

    GF1OutB(SET_CONTROL,data);         // turn it off
    gf1_delay();
    GF1OutB(SET_CONTROL,data);
}


int UltraVoiceStopped(void)
{
    return(GF1InB(GET_CONTROL) & (VOICE_STOPPED|STOP_VOICE));
}


void UltraSetBalance(UBYTE pan)
{
    GF1OutB(SET_BALANCE,pan & 0xf);
}


static void UltraSetVolume(UWORD volume)
{
        GF1OutW(SET_VOLUME,volume<<4);
}


UWORD UltraReadVolume(void)
{
        return(GF1InW(GET_VOLUME)>>4);
}


static void UltraStopVolume(void)
{
        UBYTE vmode;

        vmode=GF1InB(GET_VOLUME_CONTROL);
        vmode|=(VOLUME_STOPPED|STOP_VOLUME);

        GF1OutB(SET_VOLUME_CONTROL,vmode);
        gf1_delay();
        GF1OutB(SET_VOLUME_CONTROL,vmode);
}


static void UltraRampVolume(UWORD start,UWORD end,UBYTE rate,UBYTE mode)
{
        UWORD begin;
        UBYTE vmode;

        if(start==end) return;
/*********************************************************************
 * If the start volume is greater than the end volume, flip them and
 * turn on decreasing volume. Note that the GF1 requires that the
 * programmed start volume MUST be less than or equal to the end
 * volume.
 *********************************************************************/
        // Don't let bad bits thru ...
        mode&=~(VL_IRQ_PENDING|VC_ROLLOVER|STOP_VOLUME|VOLUME_STOPPED);

        begin = start;

        if(start>end)
        {   // flip start & end if decreasing numbers ...
            start = end;
            end   = begin;
            mode |= VC_DIRECT;              // decreasing volumes
        }

        // looping below 64 or greater that 4032 can cause strange things
        if(start<64) start = 64;
        if(end>4032) end   = 4032;

        GF1OutB(SET_VOLUME_RATE,rate);
        GF1OutB(SET_VOLUME_START,start>>4);
        GF1OutB(SET_VOLUME_END,end>>4);

        // Also MUST set the current volume to the start volume ...
        UltraSetVolume(begin);

        vmode = GF1InB(GET_VOLUME_CONTROL);

        if(vmode&VC_ROLLOVER) mode|=VC_ROLLOVER;

        // start 'er up !!!
        GF1OutB(SET_VOLUME_CONTROL,mode);
        gf1_delay();
        GF1OutB(SET_VOLUME_CONTROL,mode);
}


/*static void UltraVectorVolume(UWORD end,UBYTE rate,UBYTE mode)
{
        UltraStopVolume();
        UltraRampVolume(UltraReadVolume(),end,rate,mode);
}
*/

/*static int UltraVolumeStopped(void)
{
   return(GF1InB(GET_VOLUME_CONTROL) & (VOLUME_STOPPED|STOP_VOLUME));
}*/


/*
static UWORD vol_rates[19]={
23,24,26,28,29,31,32,34,36,37,39,40,42,44,45,47,49,50,52
};
*/

/*static UBYTE UltraCalcRate(UWORD start,UWORD end,ULONG mil_secs)
{
        ULONG gap,mic_secs;
        UWORD i,range,increment;
        UBYTE rate_val;
        UWORD value;

        gap = (start>end) ? (start-end) : (end-start);
        mic_secs = (mil_secs * 1000L)/gap;

// OK. We now have the # of microseconds for each update to go from
// A to B in X milliseconds. See what the best fit is in the table

        range = 4;
        value = vol_rates[GUS_VOICES-14];

        for(i=0;i<3;i++){
                if(mic_secs<value){
                        range = i;
                        break;
                }
                else value<<=3;
        }

        if(range==4){
                range = 3;
                increment = 1;
        }
        else{
                // calculate increment value ... (round it up ?)
                increment=(unsigned int)((value + (value>>1)) / mic_secs);
        }

        rate_val=range<<6;
        rate_val|=(increment&0x3F);
        return(rate_val);
}
*/

static UWORD _gf1_volumes[512] =
{  0x0000,
   0x0700, 0x07ff, 0x0880, 0x08ff, 0x0940, 0x0980, 0x09c0, 0x09ff, 0x0a20,
   0x0a40, 0x0a60, 0x0a80, 0x0aa0, 0x0ac0, 0x0ae0, 0x0aff, 0x0b10, 0x0b20,
   0x0b30, 0x0b40, 0x0b50, 0x0b60, 0x0b70, 0x0b80, 0x0b90, 0x0ba0, 0x0bb0,
   0x0bc0, 0x0bd0, 0x0be0, 0x0bf0, 0x0bff, 0x0c08, 0x0c10, 0x0c18, 0x0c20,
   0x0c28, 0x0c30, 0x0c38, 0x0c40, 0x0c48, 0x0c50, 0x0c58, 0x0c60, 0x0c68,
   0x0c70, 0x0c78, 0x0c80, 0x0c88, 0x0c90, 0x0c98, 0x0ca0, 0x0ca8, 0x0cb0,
   0x0cb8, 0x0cc0, 0x0cc8, 0x0cd0, 0x0cd8, 0x0ce0, 0x0ce8, 0x0cf0, 0x0cf8,
   0x0cff, 0x0d04, 0x0d08, 0x0d0c, 0x0d10, 0x0d14, 0x0d18, 0x0d1c, 0x0d20,
   0x0d24, 0x0d28, 0x0d2c, 0x0d30, 0x0d34, 0x0d38, 0x0d3c, 0x0d40, 0x0d44,
   0x0d48, 0x0d4c, 0x0d50, 0x0d54, 0x0d58, 0x0d5c, 0x0d60, 0x0d64, 0x0d68,
   0x0d6c, 0x0d70, 0x0d74, 0x0d78, 0x0d7c, 0x0d80, 0x0d84, 0x0d88, 0x0d8c,
   0x0d90, 0x0d94, 0x0d98, 0x0d9c, 0x0da0, 0x0da4, 0x0da8, 0x0dac, 0x0db0,
   0x0db4, 0x0db8, 0x0dbc, 0x0dc0, 0x0dc4, 0x0dc8, 0x0dcc, 0x0dd0, 0x0dd4,
   0x0dd8, 0x0ddc, 0x0de0, 0x0de4, 0x0de8, 0x0dec, 0x0df0, 0x0df4, 0x0df8,
   0x0dfc, 0x0dff, 0x0e02, 0x0e04, 0x0e06, 0x0e08, 0x0e0a, 0x0e0c, 0x0e0e,
   0x0e10, 0x0e12, 0x0e14, 0x0e16, 0x0e18, 0x0e1a, 0x0e1c, 0x0e1e, 0x0e20,
   0x0e22, 0x0e24, 0x0e26, 0x0e28, 0x0e2a, 0x0e2c, 0x0e2e, 0x0e30, 0x0e32,
   0x0e34, 0x0e36, 0x0e38, 0x0e3a, 0x0e3c, 0x0e3e, 0x0e40, 0x0e42, 0x0e44,
   0x0e46, 0x0e48, 0x0e4a, 0x0e4c, 0x0e4e, 0x0e50, 0x0e52, 0x0e54, 0x0e56,
   0x0e58, 0x0e5a, 0x0e5c, 0x0e5e, 0x0e60, 0x0e62, 0x0e64, 0x0e66, 0x0e68,
   0x0e6a, 0x0e6c, 0x0e6e, 0x0e70, 0x0e72, 0x0e74, 0x0e76, 0x0e78, 0x0e7a,
   0x0e7c, 0x0e7e, 0x0e80, 0x0e82, 0x0e84, 0x0e86, 0x0e88, 0x0e8a, 0x0e8c,
   0x0e8e, 0x0e90, 0x0e92, 0x0e94, 0x0e96, 0x0e98, 0x0e9a, 0x0e9c, 0x0e9e,
   0x0ea0, 0x0ea2, 0x0ea4, 0x0ea6, 0x0ea8, 0x0eaa, 0x0eac, 0x0eae, 0x0eb0,
   0x0eb2, 0x0eb4, 0x0eb6, 0x0eb8, 0x0eba, 0x0ebc, 0x0ebe, 0x0ec0, 0x0ec2,
   0x0ec4, 0x0ec6, 0x0ec8, 0x0eca, 0x0ecc, 0x0ece, 0x0ed0, 0x0ed2, 0x0ed4,
   0x0ed6, 0x0ed8, 0x0eda, 0x0edc, 0x0ede, 0x0ee0, 0x0ee2, 0x0ee4, 0x0ee6,
   0x0ee8, 0x0eea, 0x0eec, 0x0eee, 0x0ef0, 0x0ef2, 0x0ef4, 0x0ef6, 0x0ef8,
   0x0efa, 0x0efc, 0x0efe, 0x0eff, 0x0f01, 0x0f02, 0x0f03, 0x0f04, 0x0f05,
   0x0f06, 0x0f07, 0x0f08, 0x0f09, 0x0f0a, 0x0f0b, 0x0f0c, 0x0f0d, 0x0f0e,
   0x0f0f, 0x0f10, 0x0f11, 0x0f12, 0x0f13, 0x0f14, 0x0f15, 0x0f16, 0x0f17,
   0x0f18, 0x0f19, 0x0f1a, 0x0f1b, 0x0f1c, 0x0f1d, 0x0f1e, 0x0f1f, 0x0f20,
   0x0f21, 0x0f22, 0x0f23, 0x0f24, 0x0f25, 0x0f26, 0x0f27, 0x0f28, 0x0f29,
   0x0f2a, 0x0f2b, 0x0f2c, 0x0f2d, 0x0f2e, 0x0f2f, 0x0f30, 0x0f31, 0x0f32,
   0x0f33, 0x0f34, 0x0f35, 0x0f36, 0x0f37, 0x0f38, 0x0f39, 0x0f3a, 0x0f3b,
   0x0f3c, 0x0f3d, 0x0f3e, 0x0f3f, 0x0f40, 0x0f41, 0x0f42, 0x0f43, 0x0f44,
   0x0f45, 0x0f46, 0x0f47, 0x0f48, 0x0f49, 0x0f4a, 0x0f4b, 0x0f4c, 0x0f4d,
   0x0f4e, 0x0f4f, 0x0f50, 0x0f51, 0x0f52, 0x0f53, 0x0f54, 0x0f55, 0x0f56,
   0x0f57, 0x0f58, 0x0f59, 0x0f5a, 0x0f5b, 0x0f5c, 0x0f5d, 0x0f5e, 0x0f5f,
   0x0f60, 0x0f61, 0x0f62, 0x0f63, 0x0f64, 0x0f65, 0x0f66, 0x0f67, 0x0f68,
   0x0f69, 0x0f6a, 0x0f6b, 0x0f6c, 0x0f6d, 0x0f6e, 0x0f6f, 0x0f70, 0x0f71,
   0x0f72, 0x0f73, 0x0f74, 0x0f75, 0x0f76, 0x0f77, 0x0f78, 0x0f79, 0x0f7a,
   0x0f7b, 0x0f7c, 0x0f7d, 0x0f7e, 0x0f7f, 0x0f80, 0x0f81, 0x0f82, 0x0f83,
   0x0f84, 0x0f85, 0x0f86, 0x0f87, 0x0f88, 0x0f89, 0x0f8a, 0x0f8b, 0x0f8c,
   0x0f8d, 0x0f8e, 0x0f8f, 0x0f90, 0x0f91, 0x0f92, 0x0f93, 0x0f94, 0x0f95,
   0x0f96, 0x0f97, 0x0f98, 0x0f99, 0x0f9a, 0x0f9b, 0x0f9c, 0x0f9d, 0x0f9e,
   0x0f9f, 0x0fa0, 0x0fa1, 0x0fa2, 0x0fa3, 0x0fa4, 0x0fa5, 0x0fa6, 0x0fa7,
   0x0fa8, 0x0fa9, 0x0faa, 0x0fab, 0x0fac, 0x0fad, 0x0fae, 0x0faf, 0x0fb0,
   0x0fb1, 0x0fb2, 0x0fb3, 0x0fb4, 0x0fb5, 0x0fb6, 0x0fb7, 0x0fb8, 0x0fb9,
   0x0fba, 0x0fbb, 0x0fbc, 0x0fbd, 0x0fbe, 0x0fbf, 0x0fc0, 0x0fc1, 0x0fc2,
   0x0fc3, 0x0fc4, 0x0fc5, 0x0fc6, 0x0fc7, 0x0fc8, 0x0fc9, 0x0fca, 0x0fcb,
   0x0fcc, 0x0fcd, 0x0fce, 0x0fcf, 0x0fd0, 0x0fd1, 0x0fd2, 0x0fd3, 0x0fd4,
   0x0fd5, 0x0fd6, 0x0fd7, 0x0fd8, 0x0fd9, 0x0fda, 0x0fdb, 0x0fdc, 0x0fdd,
   0x0fde, 0x0fdf, 0x0fe0, 0x0fe1, 0x0fe2, 0x0fe3, 0x0fe4, 0x0fe5, 0x0fe6,
   0x0fe7, 0x0fe8, 0x0fe9, 0x0fea, 0x0feb, 0x0fec, 0x0fed, 0x0fee, 0x0fef,
   0x0ff0, 0x0ff1, 0x0ff2, 0x0ff3, 0x0ff4, 0x0ff5, 0x0ff6, 0x0ff7, 0x0ff8,
   0x0ff9, 0x0ffa, 0x0ffb, 0x0ffc, 0x0ffd, 0x0ffe, 0x0fff
};


/*
static void UltraSetLinearVolume(UWORD index)
{
        UltraSetVolume(_gf1_volumes[index]);
}


static void UltraRampLinearVolume(UWORD start_idx,UWORD end_idx,ULONG msecs,UBYTE mode)
{
        UWORD start,end;
        UBYTE rate;

        // Ramp from start to end in x milliseconds

        start = _gf1_volumes[start_idx];
        end   = _gf1_volumes[end_idx];

        // calculate a rate to get from start to end in msec milliseconds ..
        rate=UltraCalcRate(start,end,msecs);
        UltraRampVolume(start,end,rate,mode);
}
*/

void UltraVectorLinearVolume(UWORD end_idx,UBYTE rate,UBYTE mode)
{
    UltraStopVolume();
    UltraRampVolume(UltraReadVolume(),_gf1_volumes[end_idx],rate,mode);
}


static void UltraStartTimer(UBYTE timer,UBYTE time)
{
        UBYTE temp;

        if (timer == 1)
        {   GUS_TIMER_CTRL |= 0x04;
            GUS_TIMER_MASK |= 0x01;
            temp = TIMER1;
        } else
        {   GUS_TIMER_CTRL |= 0x08;
            GUS_TIMER_MASK |= 0x02;
            temp = TIMER2;
        }                        

//      ENTER_CRITICAL;

        time = 256 - time;

        GF1OutB(temp,time);                             // set timer speed
        GF1OutB(TIMER_CONTROL,GUS_TIMER_CTRL);          // enable timer interrupt on gf1
        outportb(GF1_TIMER_CTRL,0x04);                  // select timer stuff
        outportb(GF1_TIMER_DATA,GUS_TIMER_MASK);        // start the timers

//      LEAVE_CRITICAL;
}


void UltraStopTimer(int timer)
{
        if (timer == 1)
        {   GUS_TIMER_CTRL &= ~0x04;
            GUS_TIMER_MASK &= ~0x01;
        } else
        {   GUS_TIMER_CTRL &= ~0x08;
            GUS_TIMER_MASK &= ~0x02;
        }

//      ENTER_CRITICAL;

        GF1OutB(TIMER_CONTROL,GUS_TIMER_CTRL);          // disable timer interrupts
        outportb(GF1_TIMER_CTRL,0x04);                  // select timer stuff
        outportb(GF1_TIMER_DATA,GUS_TIMER_MASK | 0x80);

//      LEAVE_CRITICAL;
}

/*
static BOOL UltraTimerStopped(UBYTE timer)
{
        UBYTE value;
        UBYTE temp;

        if (timer == 1)
                temp = 0x40;
        else
                temp = 0x20;

        value = (inportb(GF1_TIMER_CTRL)) & temp;

        return(value);
}
*/

/***************************************************************************
>>>>>>>>>>>>>>>>>>>>>>>>> The actual GUS driver <<<<<<<<<<<<<<<<<<<<<<<<<<<<
***************************************************************************/


UBYTE timeskip;
UBYTE timecount;
UBYTE GUS_BPM;


void UltraSetBPM(UBYTE bpm)
{
   // The player routine has to be called (bpm*50)/125 times a second,
   // so the interval between calls takes 125/(bpm*50) seconds (amazing!).

   // The Timer1 handler has a resolution of 80 microseconds.
   // So the timer value to program:

   // (125/(bpm*50)) / 80e-6 = 31250/bpm
   
   UWORD rate = 31250/bpm;

   timeskip  = 1;
   timecount = 1;

   while(rate > 255)
   {  rate     >>= 1;
      timeskip <<= 1;
   }
   UltraStartTimer(1,rate);
}


BOOL GUS_IsThere(void)
{
    return(UltraDetect());
}


ULONG GUS_SampleSpace(int type)
{
    return (type==MD_HARDWARE) ? (UltraMemTotal()>>10)-16 : VC_SampleSpace(type);
}


ULONG GUS_SampleLength(int type, SAMPLE *s)
{
    int retval;

    if(type==MD_HARDWARE)
    {   retval = s->length + 64;
        if(s->flags&SF_16BITS)
        {   if(s->length < 131000)
                retval = (retval * 2) + (16*1024);
        }
    } else
        retval = VC_SampleLength(type, s);

    return retval;
}


void GUS_Dummy(void)
{
}


