#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "rsl.h"

void replace_ray_gaps_with_null(Sweep *sweep)
{
    /* Check for ray gaps and replace any with null.  A ray gap means that
     * the difference in ray numbers in a pair of consecutive rays is greater
     * than 1.  When such a case is found, nulls are inserted for missing rays.
     */

    int i, j, maxrays, nrays;
    Sweep *newsweep;

    if (sweep == NULL) {
        fprintf(stderr,"replace_ray_gaps_with_null: Null sweep.\n");
        return;
    }

    nrays = sweep->h.nrays;
    /* If last ray of sweep is non-null, and its ray number is same as nrays,
     * then there are no gaps.
     */
    if (sweep->ray[nrays-1] && sweep->ray[nrays-1]->h.ray_num == nrays) return;

    /* Determine expected maximum rays from beamwidth. */
    maxrays = 720;
    if (sweep->h.beam_width > 0.9) maxrays = 360;
    if (sweep->h.nrays > maxrays) maxrays = sweep->h.nrays;
    newsweep = RSL_new_sweep(maxrays);

    /* Copy rays to new sweep based on ray number.  New sweep will contain
     * NULL where rays are missing.
     */
    for (i=0, j=0; i<maxrays; i++)
        if (sweep->ray[j] && sweep->ray[j]->h.ray_num == i+1)
            newsweep->ray[i] = sweep->ray[j++];

    /* Copy rays back to original sweep. */
    for (i=0; i<maxrays; i++) sweep->ray[i] = newsweep->ray[i];

    sweep->h.nrays = maxrays;
    free(newsweep);
}


int get_ray_index(Sweep *sweep, Ray *ray)
{
    /* Find the array index for the given ray in the sweep structure. */

    float target_azimuth, this_azimuth;
    int iray, incr, maxrays, nchk;

    if (ray == NULL) return -1;

    target_azimuth = ray->h.azimuth;

    /* Start with ray_num - 1.  That will normally be a match. */
    iray = ray->h.ray_num - 1;
    if (sweep->ray[iray]->h.azimuth == target_azimuth)
        return iray;

    /* Search for ray with matching azimuth by incrementing ray index forward
     * or backward, depending on azimuth.
     */
    this_azimuth = sweep->ray[iray]->h.azimuth;
    if (this_azimuth < target_azimuth) incr = 1;
    else incr = -1;
    iray+=incr;
    maxrays = sweep->h.nrays;
    for (nchk = 1; nchk <= maxrays; nchk++) {
        if (iray < 0) iray = maxrays - 1;
        if (iray >= maxrays) iray = 0;
        if (sweep->ray[iray]->h.azimuth == target_azimuth) return iray;
        iray+=incr;
    }
    /* Control shouldn't end up here.  Ray came from this sweep, so there
     * has to be a match.
     */
    fprintf(stderr,"get_ray_index: Didn't find matching azimuth, but there "
            "should be one.\n");
    fprintf(stderr,"Current values:\n");
    fprintf(stderr, "target_azimuth: %.2f\n"
            "this_azimuth:   %.2f\n"
            "iray:           %d\n"
            "incr:           %d\n"
            "maxrays:        %d\n"
            "nchk:           %d\n",
            target_azimuth, this_azimuth, iray, incr, maxrays, nchk);
    return -1;
}


int get_azimuth_match(Sweep *sweep1, Sweep *sweep2, int *iray1, int *iray2)
{
    /* Using the azimuth of sweep1->ray[iray1], find the ray in sweep2 with
     * matching azimuth.  The index of matching ray is returned through
     * argument iray2.
     *
     * Return value: function returns 1 for successful match, 0 otherwise.
     */

    Ray *ray2;
    float matchlimit;
    const notfound = 0, found = 1;

    matchlimit = sweep2->h.horz_half_bw;
    /* Assure that matchlimit is wide enough--a problem with pre-Build10. */
    if (sweep2->h.beam_width > .9) matchlimit = 0.5;

    ray2 = RSL_get_closest_ray_from_sweep(sweep2,
            sweep1->ray[*iray1]->h.azimuth, matchlimit);

    /* If no match, try again with a slightly wider limit. */
    if (ray2 == NULL) ray2 = RSL_get_closest_ray_from_sweep(sweep2,
            sweep1->ray[*iray1]->h.azimuth, matchlimit + .04 * matchlimit);
    
    if (ray2 == NULL) {
        *iray2 = -1;
        return notfound;
    }

    *iray2 = get_ray_index(sweep2, ray2);
    return found;
}


static int get_first_azimuth_match(Sweep *dzsweep, Sweep *vrsweep,
        int *dz_ray_index, int *vr_ray_index)
{
    /* Find the DZ ray whose azimuth matches the first ray of VR sweep.
     * Indexes for the matching rays are returned through arguments.
     * Return value: 1 for successful match, 0 otherwise.
     */

    Ray *dz_ray, *vr_ray;
    int iray_dz = 0, iray_vr = 0, match_found = 0;
    const notfound = 0, found = 1;

    /* Get first non-null DZ ray. */
    while (dzsweep->ray[iray_dz] == NULL && iray_dz < dzsweep->h.nrays)
        iray_dz++;
    if (iray_dz == dzsweep->h.nrays) {
        fprintf(stderr,"get_first_azimuth_match: All rays in DZ sweep number %d"
                " are NULL.\nMerge split cut not done for this sweep.\n",
                dzsweep->h.sweep_num);
        return notfound;
    }

    /* Get VR ray with azimuth closest to first DZ ray. */
    
    while (!match_found) {
        if (get_azimuth_match(dzsweep, vrsweep, &iray_dz, &iray_vr)) {
            match_found = 1;
        }
        else {
            /* If still no match, get next non-null DZ ray. */
            iray_dz++;
            while ((dz_ray = dzsweep->ray[iray_dz]) == NULL) {
                iray_dz++;
                if (iray_dz >= dzsweep->h.nrays) {
                    fprintf(stderr,"get_first_azimuth_match: Couldn't find a "
                            "matching azimuth.\nSweeps compared:\n");
                    fprintf(stderr,"DZ: sweep number %d, elev %f\n"
                            "VR: sweep number %d, elev %f\n",
                            dzsweep->h.sweep_num, dzsweep->h.elev,
                            vrsweep->h.sweep_num, vrsweep->h.elev);
                    return notfound;
                }
            }
        }
    }

    *dz_ray_index = iray_dz;
    *vr_ray_index = iray_vr;
    return found;
}


static void remove_excess_rays(Radar *radar, int iswp)
{
    /* This function removes rays in excess of velocity ray count from
     * reflectivity and dual-pol fields in current sweep.
     */
    int volindex[4] = {DZ_INDEX, DR_INDEX, PH_INDEX, RH_INDEX};
    const nvols = 4;
    int i, ivol, j, nrays_vr, nrays_this_field;

    /* This function is only for legacy beam width. */
    if (radar->v[0]->sweep[0]->h.beam_width < .9) return;

    nrays_vr = radar->v[VR_INDEX]->sweep[iswp]->h.nrays;
    for (i = 0; i < nvols; i++) {
        ivol = volindex[i];
        if (radar->v[ivol]) {
            nrays_this_field = radar->v[ivol]->sweep[iswp]->h.nrays;
            if (nrays_this_field > nrays_vr) {
                for (j = nrays_vr; j < nrays_this_field; j++) {
                    RSL_free_ray(radar->v[ivol]->sweep[iswp]->ray[j]);
                    radar->v[ivol]->sweep[iswp]->ray[j] = NULL;
                }
            }
            radar->v[ivol]->sweep[iswp]->h.nrays = nrays_vr;
        }
    }

}


static void reorder_rays(Sweep *dzsweep, Sweep *vrsweep, Sweep *swsweep)
{
    /* Reorder rays of the VR and SW sweeps so that they match by azimuth
     * the rays in the DZ sweep.
     */
    int i, iray_dz, iray_vr, iray_dz_start, ray_num;
    int maxrays;
    int more_rays;
    Sweep *new_vrsweep, *new_swsweep;

    /* Allocate new VR and SW sweeps.  Allocate maximum number of rays based
     * on beam width.
     */

    if (vrsweep != NULL) {
        maxrays = (vrsweep->h.beam_width < 0.8) ? 720 : 360;
        if (vrsweep->h.nrays > maxrays) maxrays = vrsweep->h.nrays;
    }
    else {
        fprintf(stderr,"reorder_rays: Velocity sweep number %d is NULL.\n"
                "Merge split cut not done for this sweep.\n",
                vrsweep->h.sweep_num);
        return;
    }

    replace_ray_gaps_with_null(dzsweep);
    replace_ray_gaps_with_null(vrsweep);
    if (swsweep) replace_ray_gaps_with_null(swsweep);

    /* Get ray indices for first match of DZ and VR azimuths. */
    if (get_first_azimuth_match(dzsweep, vrsweep, &iray_dz, &iray_vr) == 0) {
        fprintf(stderr,"reorder_rays: Merge split cut not done for DZ sweep "
                "number %d and VR sweep %d.\n",
                dzsweep->h.sweep_num, vrsweep->h.sweep_num);
        return;
    }

    /* If DZ and VR azimuths already match, no need to do ray alignment. */
    if (iray_dz == iray_vr) return;

    new_vrsweep = RSL_new_sweep(maxrays);
    memcpy(&new_vrsweep->h, &vrsweep->h, sizeof(Sweep_header));
    if (swsweep) {
        new_swsweep = RSL_new_sweep(maxrays);
        memcpy(&new_swsweep->h, &swsweep->h, sizeof(Sweep_header));
    }

    /* Copy VR rays to new VR sweep such that their azimuths match those of
     * DZ rays at the same indices.  Do the same for SW.
     */

    iray_dz_start = iray_dz;
    ray_num = dzsweep->ray[iray_dz]->h.ray_num;
    more_rays = 1;
    while (more_rays) {
        if (vrsweep->ray[iray_vr]) {
            new_vrsweep->ray[iray_dz] = vrsweep->ray[iray_vr];
            new_vrsweep->ray[iray_dz]->h.ray_num = ray_num;
        }
        if (swsweep && swsweep->ray[iray_vr]) {
            new_swsweep->ray[iray_dz] = swsweep->ray[iray_vr];
            new_swsweep->ray[iray_dz]->h.ray_num = ray_num;
        }
        iray_dz++;
        iray_vr++;
        ray_num++;

        /* Handle sweep wraparound. */
        if (iray_dz == dzsweep->h.nrays) {
            iray_dz = 0;
            ray_num = 1;
            /* Check if azimuth rematch is needed. */
            if (dzsweep->ray[iray_dz] && vrsweep->ray[iray_vr] &&
                fabsf(dzsweep->ray[iray_dz]->h.azimuth -
                    vrsweep->ray[iray_vr]->h.azimuth)
                 > dzsweep->h.horz_half_bw) {
                get_azimuth_match(dzsweep, vrsweep, &iray_dz, &iray_vr);
            }
        }
        if (iray_vr == vrsweep->h.nrays) iray_vr = 0;

        /* If we're back at first match, we're done. */
        if (iray_dz == iray_dz_start) more_rays = 0;
    }

    /* Copy ray pointers in new order back to original sweeps. */
    /* TODO: Can we simply replace the sweeps themselves with the new sweeps? */

    for (i=0; i < maxrays; i++) {
        vrsweep->ray[i] = new_vrsweep->ray[i];
        if (swsweep) swsweep->ray[i] = new_swsweep->ray[i];
    }

    /* TODO:
     * Update nrays in sweep header with ray number of last ray
     * (ray count includes NULL rays).
     */
    vrsweep->h.nrays = maxrays;
    if (swsweep) swsweep->h.nrays = maxrays;
}


void align_this_split_cut(Radar *radar, int iswp)
{
    int vrindex[] = {VR_INDEX, V2_INDEX, V3_INDEX};
    int swindex[] = {SW_INDEX, S2_INDEX, S3_INDEX};
    int ivr, nvrfields;
    Sweep *dzsweep, *vrsweep, *swsweep;

    if (radar->v[DZ_INDEX]->sweep[iswp] == NULL) {
        fprintf(stderr,"align_this_split_cut: DZ sweep at index %d is NULL.\n"
                "Merge split cut not done for this sweep.\n", iswp);
        return;
    }
    
    if (radar->v[DZ_INDEX]->sweep[iswp]->h.beam_width > .9)
        remove_excess_rays(radar, iswp);

    dzsweep = radar->v[DZ_INDEX]->sweep[iswp];
    

    if (radar->h.vcp != 121) nvrfields = 1;
    else {
        if (iswp < 4) nvrfields = 3;
        else nvrfields = 2;
    }

    for (ivr = 0; ivr < nvrfields; ivr++) {
        if (radar->v[vrindex[ivr]]) {
            vrsweep = radar->v[vrindex[ivr]]->sweep[iswp];
        }
        else {
            fprintf(stderr,"align_this_split_cut: No VR at INDEX %d.\n"
                    "No ray alignment done for this VR index at DZ sweep "
                    "index %d.\n", vrindex[ivr], iswp);
            return;
        }
        /* Make sure vrsweep and dzsweep elevs are approximately the same. */
        if (fabsf(dzsweep->h.elev - vrsweep->h.elev) > 0.1) {
            fprintf(stderr,"align_this_split_cut: elevation angles for DZ and "
                    "VR don't match:\n"
                    "radar->v[DZ_INDEX=%d]->sweep[%d]->h.elev = %f\n"
                    "radar->v[VR_INDEX=%d]->sweep[%d]->h.elev = %f\n",
                    DZ_INDEX, iswp, dzsweep->h.elev,
                    VR_INDEX, iswp, vrsweep->h.elev);
            return;
        }
        if (radar->v[swindex[ivr]])
            swsweep = radar->v[swindex[ivr]]->sweep[iswp];
        else
            swsweep = NULL;
        reorder_rays(dzsweep, vrsweep, swsweep);
    }
}


static Radar *copy_radar_for_split_cut_adjustments(Radar *r)
{
  /* Return a copy of the radar structure for modification at split cuts. */

  Radar *r_new;
  int i,j,k;

  r_new = RSL_new_radar(MAX_RADAR_VOLUMES);
  r_new->h = r->h;
  /* Copy the contents of VR and SW volumes, which will be modified. For other
   * fields, just copy pointers.
   */
  for (k=0; k < MAX_RADAR_VOLUMES; k++) {
    if (r->v[k] == NULL) continue;
    switch(k) {
      case VR_INDEX: case SW_INDEX: case V2_INDEX: case S2_INDEX:
      case V3_INDEX: case S3_INDEX:
        r_new->v[k] = RSL_new_volume(r->v[k]->h.nsweeps);
        r_new->v[k]->h = r->v[k]->h;
        for (i=0; i < r->v[k]->h.nsweeps; i++) {
            /* Add new sweeps to volume and copy ray pointers. */
            r_new->v[k]->sweep[i] = RSL_new_sweep(r->v[k]->sweep[i]->h.nrays);
            r_new->v[k]->sweep[i]->h = r->v[k]->sweep[i]->h;
            for (j=0; j < r->v[k]->sweep[i]->h.nrays; j++) {
                r_new->v[k]->sweep[i]->ray[j] = r->v[k]->sweep[i]->ray[j]; 
            }
        }
        break;
      default:
        /* For fields other than VR or SW types, simply copy volume pointer.
         */
        r_new->v[k] = r->v[k];
        break;
    }
  }
  return r_new;
}


Radar *wsr88d_align_split_cut_rays(Radar *radar_in)
{
/* This is the driver function for aligning rays in split cuts by azimuth. */

    int iswp;
    int nsplit, vcp;
    Radar *radar;

    /* If DZ or VR is not present, nothing more to do. */
    if (radar_in->v[DZ_INDEX] == NULL || radar_in->v[VR_INDEX] == NULL)
        return radar_in;

    vcp = radar_in->h.vcp;

    /* How many split cuts are in this VCP? */
    nsplit = 0;
    switch (vcp) {
        case 11: case 21: case 32: case 211: case 221:
            nsplit = 2;
            break;
        case 12: case 31: case 212:
            nsplit = 3;
            break;
        case 121:
            nsplit = 5;
            break;
    }

    if (nsplit == 0) {
        fprintf(stderr, "wsr88d_align_split_cut_rays: Unknown VCP: %d\n", vcp);
        return radar_in;
    }

    /* Make a copy of radar.  We'll modify the copy. */
    radar = copy_radar_for_split_cut_adjustments(radar_in);

    /* Align rays in each split cut. */

    for (iswp = 0; iswp < nsplit; iswp++) {
        if (radar->v[DZ_INDEX]->sweep[iswp] == NULL) {
            fprintf(stderr, "wsr88d_align_split_cut_rays: Encountered "
                    "unexpected NULL DZ sweep\n");
            fprintf(stderr, "iswp (sweep index) = %d\n", iswp);
            return radar;
        }
        align_this_split_cut(radar, iswp);
    }

    return radar;
}
