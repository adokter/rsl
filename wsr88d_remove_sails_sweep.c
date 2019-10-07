#include "rsl.h"

void wsr88d_remove_sails_sweep(Radar *radar)
{
    /* Remove SAILS sweeps.  For VCPs 12 and 212 only. */

    int i, j;
    int sails_loc[4];
    int isails, nsails;

    if (radar->h.vcp != 12 && radar->h.vcp != 212) return;

    for (j=0; j < MAX_RADAR_VOLUMES; j++) {
        if (radar->v[j]) {
            /* Find and save SAILS indices. */
            nsails = 0;
            for (i=1; i < radar->v[j]->h.nsweeps; i++) {
                /* If this sweep's elevation is less than previous sweep,
                   remove this sweep. */
                if (radar->v[j]->sweep[i]->h.elev < 
                        radar->v[j]->sweep[i-1]->h.elev) {
                    sails_loc[nsails++] = i;
                }
            }
            /* Remove sweeps at SAILS indices. */
            if (nsails > 0) {
                for (isails=0; isails < nsails; isails++) {
                    RSL_free_sweep(radar->v[j]->sweep[sails_loc[isails]]);
                    radar->v[j]->sweep[sails_loc[isails]] = NULL;
                }
            }
        } /* if radar->v[j] not NULL */
    } /* for volumes */
    /* Push down the sweeps to remove gaps in radar structure. */
    if (nsails > 0) {
        radar = RSL_prune_radar(radar);
        fprintf(stderr,"Removed %d SAILS sweep%s.\n", nsails,
                (nsails > 1) ? "s" : "");  /* Thanks K&R */
        fprintf(stderr,"Call RSL_keep_sails() before RSL_anyformat_to_radar() "
                "to keep SAILS sweeps.\n");
    }
}
