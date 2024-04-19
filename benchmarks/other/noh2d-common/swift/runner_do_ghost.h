#pragma once

void runner_do_ghost(Tree *leaf, const hydro_props *hydro_properties, HYDRO_STATS_T *hydro_stats);

void do_density(Tree *leaf) {
    float r2, hi, hj, hig2, hjg2, dx[3] = {0};
    struct part *pi, *pj;

    const float a = 1.0f; // SWIFT parameter, ignored by the Minimal SPH scheme
    const float H = 1.0f; // SWIFT parameter, ignored by the Minimal SPH scheme

    if (leaf->items.empty()) [[unlikely]] return;

    auto activeRegion = Tree::Geo::Region(leaf->range.low - 0.001, leaf->range.high + 0.001);
    auto *activeRegionParent = leaf->getRegionParent(activeRegion, true);
    auto activeRegionIter = Tree::InsideRegionLeafsIterator(activeRegionParent, activeRegion);

    auto *activeCell = (Tree *) nullptr;
    while ((activeCell = activeRegionIter.next().value_or(nullptr))) {
        for (auto localIdx = 0; localIdx < leaf->items.size(); localIdx++) {
            pi = &leaf->items[localIdx];
            hi = pi->h;
            auto hig = hi * kernel_gamma;
            hig2 = hig * hig;

            if (!activeCell->range.overlaps({pi->_x - hig, pi->_x + hig})) continue;

            for (auto activeIdx = 0; activeIdx < activeCell->items.size(); activeIdx++) {
                pj = &activeCell->items[activeIdx];
                r2 = 0.0f;

                for (int k = 0; k < Geo::D::N; k++) {
                    dx[k] = pi->_x[k] - pj->_x[k];
                    r2 += dx[k] * dx[k];
                }

                auto hjg = pj->h * kernel_gamma;
                hjg2 = hjg * hjg;

                /* Hit or miss? */
                if ((r2 > 0) & (r2 < hig2 | r2 < hjg2)) {
                    /* Interact */
                    runner_iact_nonsym_density(r2, dx, hi, pj->h, pi, pj, a, H);
                }
            }
        }
    }
}

void do_density(Tree *leaf, int *pids, int pidCount) {
    float r2, hi, hj, hig2, hjg2, dx[3] = {0};
    struct part *pi, *pj;

    const float a = 1.0f; // SWIFT parameter, ignored by the Minimal SPH scheme
    const float H = 1.0f; // SWIFT parameter, ignored by the Minimal SPH scheme

    if (leaf->items.empty()) [[unlikely]] return;

    auto activeRegion = Tree::Geo::Region(leaf->range.low - 0.001, leaf->range.high + 0.001);
    auto *activeRegionParent = leaf->getRegionParent(activeRegion, true);
    auto activeRegionIter = Tree::InsideRegionLeafsIterator(activeRegionParent, activeRegion);

    auto *activeCell = (Tree *) nullptr;
    while ((activeCell = activeRegionIter.next().value_or(nullptr))) {
        for (auto pidIdx = 0; pidIdx < pidCount; pidIdx++) {
            pi = &leaf->items[pids[pidIdx]];
            hi = pi->h;
            auto hig = hi * kernel_gamma;
            hig2 = hig * hig;

            if (!activeCell->range.overlaps({pi->_x - hig, pi->_x + hig})) continue;

            for (auto activeIdx = 0; activeIdx < activeCell->items.size(); activeIdx++) {
                pj = &activeCell->items[activeIdx];
                r2 = 0.0f;

                for (int k = 0; k < Geo::D::N; k++) {
                    dx[k] = pi->_x[k] - pj->_x[k];
                    r2 += dx[k] * dx[k];
                }

                auto hjg = pj->h * kernel_gamma;
                hjg2 = hjg * hjg;

                /* Hit or miss? */
                if ((r2 > 0) & (r2 < hig2 | r2 < hjg2)) {
                    /* Interact */
                    runner_iact_nonsym_density(r2, dx, hi, pj->h, pi, pj, a, H);
                }
            }
        }
    }
}

void runner_do_ghost(Tree *leaf, const hydro_props *hydro_properties, HYDRO_STATS_T *hydro_stats) {
    const float hydro_h_max = hydro_properties->h_max;
    const float hydro_h_min = hydro_properties->h_min;
    const float eps = hydro_properties->h_tolerance;
    const float hydro_eta_dim =
            pow_dimension(hydro_properties->eta_neighbours);
    const int use_mass_weighted_num_ngb =
            hydro_properties->use_mass_weighted_num_ngb;
    const int max_smoothing_iter = hydro_properties->max_smoothing_iterations;
    int redo = 0, count = 0;

    /* Running value of the maximal smoothing length */
    float h_max = hydro_stats->h_max;
    float h_max_active = hydro_stats->h_max_active;

    /* Recurse? */
    if (false) {} else {

        /* Init the list of active particles that have to be updated and their
         * current smoothing lengths. */
        int *pid = NULL;
        float *h_0 = NULL;
        float *left = NULL;
        float *right = NULL;
        if ((pid = (int *)malloc(sizeof(int) * leaf->items.size())) == NULL)
            printf("Can't allocate memory for pid.");
        if ((h_0 = (float *)malloc(sizeof(float) * leaf->items.size())) == NULL)
            printf("Can't allocate memory for h_0.");
        if ((left = (float *)malloc(sizeof(float) * leaf->items.size())) == NULL)
            printf("Can't allocate memory for left.");
        if ((right = (float *)malloc(sizeof(float) * leaf->items.size())) == NULL)
            printf("Can't allocate memory for right.");
        for (int k = 0; k < leaf->items.size(); k++) {
                pid[count] = k;
                h_0[count] = leaf->items[k].h;
                left[count] = 0.f;
                right[count] = hydro_h_max;
                ++count;
        }

        /* While there are particles that need to be updated... */
        for (int num_reruns = 0; count > 0 && num_reruns < max_smoothing_iter;
             num_reruns++) {

            /* Reset the redo-count. */
            redo = 0;

            /* Loop over the remaining active parts in this cell. */
            for (int i = 0; i < count; i++) {

                /* Get a direct pointer on the part. */
                struct part *p = &leaf->items[pid[i]];
                struct xpart *xp = &leaf->items[pid[i]].xpart;

#ifdef SWIFT_DEBUG_CHECKS
                /* Is this part within the timestep? */
        if (!part_is_active(p, e)) error("Ghost applied to inactive particle");
#endif

                /* Get some useful values */
                const float h_init = h_0[i];
                const float h_old = p->h;
                const float h_old_dim = pow_dimension(h_old);
                const float h_old_dim_minus_one = pow_dimension_minus_one(h_old);

                float h_new;
                int has_no_neighbours = 0;

                if (p->density.wcount < 1.e-5 * kernel_root) { /* No neighbours case */

                    /* Flag that there were no neighbours */
                    has_no_neighbours = 1;

                    /* Double h and try again */
                    h_new = 2.f * h_old;

                } else {

                    /* Finish the density calculation */
                    hydro_end_density(p, &COSMO);

                    /* Are we using the alternative definition of the
                       number of neighbours? */
                    if (use_mass_weighted_num_ngb) {
#if defined(GIZMO_MFV_SPH) || defined(GIZMO_MFM_SPH) || defined(SHADOWFAX_SPH)
                        error(
                "Can't use alternative neighbour definition with this scheme!");
#else
                        const float inv_mass = 1.f / hydro_get_mass(p);
                        p->density.wcount = p->rho * inv_mass;
                        p->density.wcount_dh = p->density.rho_dh * inv_mass;
#endif
                    }

                    /* Compute one step of the Newton-Raphson scheme */
                    const float n_sum = p->density.wcount * h_old_dim;
                    const float n_target = hydro_eta_dim;
                    const float f = n_sum - n_target;
                    const float f_prime =
                            p->density.wcount_dh * h_old_dim +
                            hydro_dimension * p->density.wcount * h_old_dim_minus_one;

                    /* Improve the bisection bounds */
                    if (n_sum < n_target)
                        left[i] = MAX(left[i], h_old);
                    else if (n_sum > n_target)
                        right[i] = MAX(right[i], h_old);

#ifdef SWIFT_DEBUG_CHECKS
                    /* Check the validity of the left and right bounds */
          if (left[i] > right[i])
            error("Invalid left (%e) and right (%e)", left[i], right[i]);
#endif

                    /* Skip if h is already h_max and we don't have enough neighbours */
                    /* Same if we are below h_min */
                    if (((p->h >= hydro_h_max) && (f < 0.f)) ||
                        ((p->h <= hydro_h_min) && (f > 0.f))) {

                        /* We have a particle whose smoothing length is already set (wants
                         * to be larger but has already hit the maximum OR wants to be
                         * smaller but has already reached the minimum). So, just tidy up
                         * as if the smoothing length had converged correctly  */

#ifdef EXTRA_HYDRO_LOOP

                        /* As of here, particle gradient variables will be set. */
            /* The force variables are set in the extra ghost. */

            /* Compute variables required for the gradient loop */
            hydro_prepare_gradient(p, xp, cosmo, hydro_props, pressure_floor);
            mhd_prepare_gradient(p, xp, cosmo, hydro_props);

            /* The particle gradient values are now set.  Do _NOT_
               try to read any particle density variables! */

            /* Prepare the particle for the gradient loop over neighbours
             */
            hydro_reset_gradient(p);
            mhd_reset_gradient(p);

#else

                        /* As of here, particle force variables will be set. */

                        /* Compute variables required for the force loop */
                        hydro_prepare_force(p, xp, &COSMO, &HYDRO_PROPS, nullptr,
                                            0.0, 0.0);

                        /* The particle force values are now set.  Do _NOT_
                           try to read any particle density variables! */

                        /* Prepare the particle for the force loop over neighbours */
                        hydro_reset_acceleration(p);

#endif /* EXTRA_HYDRO_LOOP */

                        /* Ok, we are done with this particle */
                        continue;
                    }

                    /* Normal case: Use Newton-Raphson to get a better value of h */

                    /* Avoid floating point exception from f_prime = 0 */
                    h_new = h_old - f / (f_prime + std::numeric_limits<float>::min());

                    /* Be verbose about the particles that struggle to converge */
                    if (num_reruns > max_smoothing_iter - 10) {

                        printf(
                                "Smoothing length convergence problem: iter=%d p->id=%lld "
                                "h_init=%12.8e h_old=%12.8e h_new=%12.8e f=%f f_prime=%f "
                                "n_sum=%12.8e n_target=%12.8e left=%12.8e right=%12.8e\n",
                                num_reruns, p->id, h_init, h_old, h_new, f, f_prime, n_sum,
                                n_target, left[i], right[i]);
                    }

#ifdef SWIFT_DEBUG_CHECKS
                    if (((f > 0.f && h_new > h_old) || (f < 0.f && h_new < h_old)) &&
              (h_old < 0.999f * hydro_props->h_max))
            error(
                "Smoothing length correction not going in the right direction");
#endif

                    /* Safety check: truncate to the range [ h_old/2 , 2h_old ]. */
                    h_new = MIN(h_new, 2.f * h_old);
                    h_new = MAX(h_new, 0.5f * h_old);

                    /* Verify that we are actually progrssing towards the answer */
                    h_new = MAX(h_new, left[i]);
                    h_new = MIN(h_new, right[i]);
                }

                /* Check whether the particle has an inappropriate smoothing length
                 */
                if (fabsf(h_new - h_old) > eps * h_old) {

                    /* Ok, correct then */

                    /* Case where we have been oscillating around the solution */
                    if ((h_new == left[i] && h_old == right[i]) ||
                        (h_old == left[i] && h_new == right[i])) {

                        /* Bisect the remaining interval */
                        p->h = pow_inv_dimension(
                                0.5f * (pow_dimension(left[i]) + pow_dimension(right[i])));

                    } else {

                        /* Normal case */
                        p->h = h_new;
                    }

                    /* If within the allowed range, try again */
                    if (p->h < hydro_h_max && p->h > hydro_h_min) {

                        /* Flag for another round of fun */
                        pid[redo] = pid[i];
                        h_0[redo] = h_0[i];
                        left[redo] = left[i];
                        right[redo] = right[i];
                        redo += 1;

                        /* Re-initialise everything */
                        hydro_init_part(p, nullptr);

                        /* Off we go ! */
                        continue;

                    } else if (p->h <= hydro_h_min) {

                        /* Ok, this particle is a lost cause... */
                        p->h = hydro_h_min;

                    } else if (p->h >= hydro_h_max) {

                        /* Ok, this particle is a lost cause... */
                        p->h = hydro_h_max;

                        /* Do some damage control if no neighbours at all were found */
                        if (has_no_neighbours) {
                            assert(false)
                            hydro_part_has_no_neighbours(p, xp, &COSMO);
                        }

                    } else {
                        printf(
                                "Fundamental problem with the smoothing length iteration "
                                "logic.");
                    }
                }

                /* We now have a particle whose smoothing length has converged */

                /* Check if h_max has increased */
                h_max = MAX(h_max, p->h);
                h_max_active = MAX(h_max_active, p->h);



                /* As of here, particle force variables will be set. */

                /* Compute variables required for the force loop */
                hydro_prepare_force(p, xp, &COSMO, &HYDRO_PROPS, nullptr, 0.0,
                                    0.0);

                /* The particle force values are now set.  Do _NOT_
                   try to read any particle density variables! */

                /* Prepare the particle for the force loop over neighbours */
                hydro_reset_acceleration(p);

            }

            /* We now need to treat the particles whose smoothing length had not
             * converged again */

            /* Re-set the counter for the next loop (potentially). */
            count = redo;
            if (count > 0) {
                do_density(leaf, pid, count);
            }
        }

        if (count) {
            printf(
                    "Smoothing length failed to converge for the following gas "
                    "particles:\n");
            for (int i = 0; i < count; i++) {
                struct part *p = &leaf->items[pid[i]];
                printf("ID: %lld, h: %g, wcount: %g\n", p->id, p->h, p->density.wcount);
            }

            printf("Smoothing length failed to converge on %i particles.\n", count);
        }

        /* Be clean */
        free(left);
        free(right);
        free(pid);
        free(h_0);
    }

    /* Update h_max */
    hydro_stats->h_max = h_max;
    hydro_stats->h_max_active = h_max_active;

#ifdef SWIFT_DEBUG_CHECKS
    for (int i = 0; i < c->hydro.count; ++i) {
    const struct part *p = &c->hydro.parts[i];
    const float h = c->hydro.parts[i].h;
    if (part_is_inhibited(p, e)) continue;

    if (h > c->hydro.h_max)
      error("Particle has h larger than h_max (id=%lld)", p->id);
    if (part_is_active(p, e) && h > c->hydro.h_max_active)
      error("Active particle has h larger than h_max_active (id=%lld)", p->id);
  }
#endif


}
