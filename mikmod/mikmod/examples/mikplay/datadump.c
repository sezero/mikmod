/*
   Mikmod Pattern Dumper
   By Jake Stine [Air] of Divine Entertainment

   On a warm fall night in 1999!

   Dumps all the pattern data for the loaded module to a poorly formatted text file.
 */

#include "mikmod.h"
#include "mplayer.h"
#include <string.h>
#include <conio.h>

static char *notelookup[] = { "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-" };

void DumpPatternData(MPLAYER *ps)
{
    FILE  *fp;
    int    pat = 0, trkidx = 0, row, i;
    uint   chn;
    UNITRK_ROW  urow[64], grow;
    UNIMOD      *mf = ps->mf;

    char   s128[512],s[20];

    fp = fopen("patdump.dup","w");

    ps->sngpos = ps->patpos = 0;

    for(; pat<mf->numpat; pat++)
    {   for(chn=0; chn<mf->numchn; chn++)
        {   utrk_local_seek(&urow[chn], mf->tracks[mf->patterns[trkidx++]], 0);
        }

        utrk_global_seek(&grow, mf->globtracks[pat], 0);

        fprintf(fp, "\nPattern %d\n\n", pat);
        for(row=0; row<mf->pattrows[pat]; row++)
        {   UE_EFFECT     eff;
            UNITRK_NOTE   note;
            int           effcounter[64], effrows, ec;

            s128[0] = 0;
            //utrk_global_geteffect(&eff, &grow);
            if(utrk_global_geteffect(&eff, &grow))
            {   sprintf(s,"%2.2d ", eff.memchan);
                strcat(s128, s);

                if(eff.memslot)
                {   sprintf(s,"%2.2d ", eff.memslot);
                    strcat(s128, s);
                } else strcat(s128,"-- ");

                if(!(eff.flags & UEF_MEMORY))
                {   sprintf(s,"%2.2d ", eff.effect.effect);
                    strcat(s128, s);
                } else strcat(s128,"-- ");

            } else strcat(s128,"-- -- -- ");
            
            sprintf(s,":%2.2X: ",row);
            strcat(s128,s);
            
            for(chn=0; chn<mf->numchn; chn++)
            {   int        vol,   pan;
                BOOL       vcrud, pcrud;

                if(chn) strcat(s128," | ");
                
                utrk_getnote(&note, &urow[chn]);

                if(note.note)
                {   note.note -= 1;
                    sprintf(s,"%s%u ",notelookup[note.note % 12], note.note / 12);
                    strcat(s128,s);
                } else strcat(s128, "--- ");

                if(note.inst)
                {   sprintf(s,"%3.3u ",note.inst-1);
                    strcat(s128, s);
                } else strcat(s128, "--- ");

                vol   = pan   = 0;
                vcrud = pcrud = 0;
                effcounter[chn] = 0;
                while(utrk_local_geteffect(&eff, &urow[chn]))
                {   BOOL lcrud = 0;
                    //if(eff.effect.framedly == UFD_RUNONCE)
                        switch(eff.effect.effect)
                        {   case UNI_VOLUME:
                                vol = eff.effect.param.u;
                                vcrud = lcrud = 1; 
                            break;

                            case UNI_PANNING:
                                pan = eff.effect.param.s;
                                pcrud = lcrud = 1;
                            break;

                            /*case UNI_VOLSLIDE:
                                if(chn == 2) printlog("Slide: %x %d", row, eff.effect.param.s);
                            break;*/
                        }
                    if(!lcrud) effcounter[chn] = eff.effect.effect;
                }

                /*if(vcrud)
                {   sprintf(s,"%2.2X ",vol);
                    strcat(s128, s);
                } else strcat(s128, "-- ");
                
                if(pcrud)
                {   sprintf(s,"%3.3X ",pan);
                    strcat(s128, s);
                } else strcat(s128, "--- ");*/

                if(effcounter[chn])
                {   sprintf(s,"%2.2d", effcounter[chn]);
                    strcat(s128, s);
                } else strcat(s128, "--");
            }

            strcat(s128,"\n");
            fwrite(s128,1,strlen(s128),fp);

            /*effrows = 0;
            for(chn=0; chn<mf->numchn; chn++)
                if(effcounter[chn] > effrows) effrows = effcounter[chn];
                
            for(ec=0; ec<effrows; ec++)
            {   char ss[30], s2[30];

                strcpy(s128,"    ");
                for(chn=0; chn<mf->numchn; chn++)
                {   int     lo;
                    INT_MOB dat;
                    
                    urow[chn].pos = 0;

                    i = 0;
                    s[0] = ss[0]= 0;
                    if(chn) strcat(s128," | ");

                    while(utrk_local_geteffect(&eff, &urow[chn]))
                    {   // Universal Framedelay!  If the RUNONCE flag is set, then the command is
                        // executed once on the specified tick, otherwise, the command is simply
                        // delayed for the number of ticks specified.
    
                        if((eff.effect.effect == UNI_VOLUME) || (eff.effect.effect == UNI_PANNING)) continue;
                        if(i++ != ec) continue;

                        lo  = eff.effect.framedly & UFD_TICKMASK;
                        dat = eff.effect.param;

                        if(eff.memslot)
                        {   sprintf(ss,"%3.3d",eff.memslot);
                        } else strcpy(ss, "---");

                        switch(eff.effect.effect)
                        {   case UNI_RETRIG:
                                if(eff.flags & UEF_MEMORY)
                                    strcpy(s,"RETRIG -- --");
                                else
                                    sprintf(s,"RETRIG %2.2u %2.2u",dat.hiword.u, dat.loword.u);
                            break;
                        }
                    }
                    sprintf(s2,"%s %11.11s",ss,s);
                    strcat(s128,s2);
                }
                strcat(s128,"\n");
                //fwrite(s128,1,strlen(s128),fp);
            }*/
            for(chn=0; chn<mf->numchn; chn++) utrk_local_nextrow(&urow[chn]);
            utrk_global_nextrow(&grow);
        }
    }

    ps->sngpos = ps->patpos = 0;
    fclose(fp);
}
