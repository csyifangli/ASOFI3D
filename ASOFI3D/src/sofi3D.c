/*------------------------------------------------------------------------
 * Copyright (C) 2017 For the list of authors, see file AUTHORS.
 *
 * This file is part of ASOFI3D.
 *
 * ASOFI3D is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2.0 of the License only.
 *
 * ASOFI3D is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ASOFI3D. See file COPYING and/or
  * <http://www.gnu.org/licenses/gpl-2.0.html>.
--------------------------------------------------------------------------*/
/* ----------------------------------------------------------------------
 * This is program ASOFI3D.
 * Parallel 3-D Viscoelastic Finite Difference Seismic Modelling
 * using the Standard Staggered Grid (SSG)
 *
 *
 * If you want to publish synthetic data calculated with this program please
 * give a reference to the following paper:
 * Bohlen, T., 2002, Parallel 3-D viscoelastic finite-difference seismic modelling,
 * Computers @ Geosciences, Vol. 28, No. 8, 887-889.
 *
 *  ----------------------------------------------------------------------*/

#include "fd.h"
#include "globvar.h"
#include "data_structures.h"
//#include "openacc.h"
int main(int argc, char **argv)
{
    // TODO: this needs to be moved to the json - ask Mahesh
    int RTM_FLAG = 0;
    int ns, nt, nseismograms = 0, nf1, nf2;
    int lsnap, nsnap = 0, lsamp = 0, nlsamp = 0, buffsize;
    int ntr = 0, ntr_loc = 0, ntr_glob = 0, nsrc = 0, nsrc_loc = 0;
    int ishot, nshots;

    double time1 = 0.0, time2 = 0.0, time3 = 0.0, time4 = 0.0;
    double *time_v_update, *time_s_update, *time_s_exchange, *time_v_exchange, *time_timestep;
    int *xb, *yb, *zb, l;

    float ***absorb_coeff = NULL;

    /* Stress tensor. */
    Tensor3d s;
    s.xy = NULL; s.yz = NULL; s.xz = NULL;
    s.xx = NULL; s.yy = NULL; s.zz = NULL;
    /* TODO: define what `r` means. */
    Tensor3d r;
    r.xy = NULL; r.yz = NULL; r.xz = NULL;
    r.xx = NULL; r.yy = NULL; r.zz = NULL;
    /* Velocity vector. */
    Velocity v;
    v.x = NULL; v.y = NULL; v.z = NULL;

    /* Save old spatial derivatives of velocity for Adam-Bashforth method. */
    VelocityDerivativesTensor dv;
    VelocityDerivativesTensor dv_2;
    VelocityDerivativesTensor dv_3;
    VelocityDerivativesTensor dv_4;

    /* Save old derivatives of the stress for Adam-Bashforth method. */
    float ***svx = NULL, ***svy = NULL, ***svz = NULL;
    float ***svx_2 = NULL, ***svy_2 = NULL, ***svz_2 = NULL, ***svx_3 = NULL, ***svy_3 = NULL, ***svz_3 = NULL;
    float ***svx_4 = NULL, ***svy_4 = NULL, ***svz_4 = NULL;

    /* We need these arrays for the time shift for Adam-Bashforth method. */
    float ***shift_s1 = NULL, ***shift_s2 = NULL, ***shift_s3 = NULL;
    float ***shift_v1 = NULL, ***shift_v2 = NULL, ***shift_v3 = NULL, ***shift_v4 = NULL, ***shift_v5 = NULL, ***shift_v6 = NULL, ***shift_v7 = NULL;
    float ***shift_r1 = NULL, ***shift_r2 = NULL, ***shift_r3 = NULL, ***shift_r4 = NULL, ***shift_r5 = NULL, ***shift_r6 = NULL;

    /* We need these arrays for the time shift for Adams-Bashforth method. */
    float ***rxy_2 = NULL, ***ryz_2 = NULL, ***rxz_2 = NULL;
    float ***rxx_2 = NULL, ***ryy_2 = NULL, ***rzz_2 = NULL;
    float ***rxy_3 = NULL, ***ryz_3 = NULL, ***rxz_3 = NULL;
    float ***rxx_3 = NULL, ***ryy_3 = NULL, ***rzz_3 = NULL;
    float ***rxy_4 = NULL, ***ryz_4 = NULL, ***rxz_4 = NULL;
    float ***rxx_4 = NULL, ***ryy_4 = NULL, ***rzz_4 = NULL;

    float ***rho, ***pi, ***u;
    float ***C11, ***C22, ***C33, ***C12, ***C13, ***C23, ***C44, ***C55, ***C66;
    float ***taus = NULL, ***taup = NULL, *eta = NULL;
    float ***C66ipjp, ***C44jpkp, ***C55ipkp, ***tausipjp = NULL, ***tausjpkp = NULL, ***tausipkp = NULL, ***rjp, ***rkp, ***rip;

    float **srcpos = NULL, **srcpos_loc = NULL, **srcpos1 = NULL, **signals = NULL;
    int **recpos = NULL, **recpos_loc = NULL;
    float **sectionvx = NULL, **sectionvy = NULL, **sectionvz = NULL, **sectionp = NULL,
          **sectioncurl = NULL, **sectiondiv = NULL;
    float ***bufferlef_to_rig, ***bufferrig_to_lef;
    float ***buffertop_to_bot, ***bufferbot_to_top;
    float ***bufferfro_to_bac, ***bufferbac_to_fro;

    float ***sbufferlef_to_rig, ***sbufferrig_to_lef;
    float ***sbuffertop_to_bot, ***sbufferbot_to_top;
    float ***sbufferfro_to_bac, ***sbufferbac_to_fro;

    float **seismo_fulldata = NULL;
    int *recswitch = NULL;

    float *K_x = NULL, *alpha_prime_x = NULL, *a_x = NULL, *b_x = NULL, *K_x_half = NULL, *alpha_prime_x_half = NULL, *a_x_half = NULL, *b_x_half = NULL, *K_y = NULL, *alpha_prime_y = NULL, *a_y = NULL, *b_y = NULL, *K_y_half = NULL, *alpha_prime_y_half = NULL, *a_y_half = NULL, *b_y_half = NULL, *K_z = NULL, *alpha_prime_z = NULL, *a_z = NULL, *b_z = NULL, *K_z_half = NULL, *alpha_prime_z_half = NULL, *a_z_half = NULL, *b_z_half = NULL;
    float ***psi_sxx_x = NULL, ***psi_syy_y = NULL, ***psi_szz_z = NULL, ***psi_sxy_y = NULL, ***psi_sxy_x = NULL, ***psi_sxz_x = NULL, ***psi_sxz_z = NULL, ***psi_syz_y = NULL, ***psi_syz_z = NULL, ***psi_vxx = NULL, ***psi_vyy = NULL, ***psi_vzz = NULL, ***psi_vxy = NULL, ***psi_vxz = NULL, ***psi_vyx = NULL, ***psi_vyz = NULL, ***psi_vzx = NULL, ***psi_vzy = NULL;

    MPI_Request *req_send, *req_rec, *sreq_send, *sreq_rec;

    float memdyn, memmodel, memseismograms, membuffer, memcpml = 0.0, memtotal;
    float amon = 0.0, str = 0.0, dip = 0.0, rake = 0.0;
    float fac1, fac2;
    char *buff_addr, ext[10];
    int *stype = NULL, *stype_loc = NULL;
    char buffer[STRING_SIZE], bufferstring[10];
    FILE *fpsrc = NULL;

    /* Initialize MPI environment */
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &NP);
    MPI_Comm_rank(MPI_COMM_WORLD, &MYID);

    // Assign each MPI process to a GPU
   // acc_init(acc_device_nvidia);
   // acc_set_device_num(MYID, acc_device_nvidia);

    setvbuf(stdout, NULL, _IONBF, 0);

    /* Initialize clock for estimating runtime of program. */
    if (MYID == 0)
    {
        time1 = MPI_Wtime();
        clock();
    }

    /* Print program name, version, author etc to stdout. */
    if (MYID == 0)
        info(stdout);
    SOFI3DVERS = 33; /* 3D isotropic elastic */

    /* PE 0 is reading the parameters from the input file, default is ASOFI3D.json */
    if (MYID == 0)
    {
        if (argc > 1)
        {
            strncpy(FILEINP, argv[1], STRING_SIZE);
            fprintf(stderr, " Input parameter filename read from command line : %s. \n\n", FILEINP);
            if (strlen(FILEINP) > STRING_SIZE - 1)
            {
                fprintf(stderr, "\n ASOFI3D cannot handle paths with more than %d characters.\n", STRING_SIZE - 1);
                fprintf(stderr, " Error: ASOFI3D could not read input parameter file name. -> Exit. \n\n");
                return -1;
            }
        }
        else
        {
            strcpy(FILEINP, "ASOFI3D.json");
            fprintf(stderr, " Caution: input parameter filename set to default 'ASOFI3D.json'. \n\n");
        }
        FP = fopen(FILEINP, "r");

        //read json formated input file
        read_par_json(stdout, FILEINP);
        fclose(FP);
    }
    /* PE 0 will broadcast the parameters to all others PEs */
    exchange_par();

    /* Print info on log-files to stdout */
    if (MYID == 0)
        note(stdout);

    /* open log-file (each PE is using different file) */
    /*	fp=stdout; */
    sprintf(ext, ".%i", MYID);
    strcat(LOG_FILE, ext);

    /* nodes MYIDo writes logging info to LOG_FILE or stdout */
    if (MYID == 0)
        switch (LOG)
        {
            case 0:
                FP = fopen("/dev/null", "w"); /* no logging information will be output */
                break;
            case 1:
                FP = stdout; /* logging information will be written to standard output */
                break;
            case 2:
                if ((FP = fopen(LOG_FILE, "w")) == NULL)
                    err(" Opening log-file failed.");
                /* logging information will be written to LOG_FILE */
                break;
        }

    /* all other nodes write logging info to LOG_FILE */
    if (MYID > 0)
        if ((FP = fopen(LOG_FILE, "w")) == NULL)
            err(" Opening log-file failed.");

    fprintf(FP, " This is the log-file generated by PE %d \n\n", MYID);

    /* domain decomposition */
    initproc();

    /* set some time counters */
    NT = (int)ceil(TIME / DT); /* number of timesteps - replaces: NT=iround(TIME/DT); */
    TIME = (NT - 1) * DT;      /* TIME set to true time of the last time step */

    if (NDTSHIFT > NT)
    {
        ns = 0;
    }
    else
    {
        ns = (int)ceil((float)(NT - NDTSHIFT) / (float)NDT); /* number of samples per trace - replaces buggy formula: ns=iround(NT-NDTSHIFT/NDT); */
    }
    lsnap = iround(TSNAP1 / DT); /* first snapshot at this timestep */
    if (((ns < 0) && (MYID == 0)) && (SEISMO > 0))
    {
        /*fprintf(FP," \nSampling interval for seismogram output (DT) : %f \n\n",DT);
	  fprintf(FP," \nSampling interval shift (NDTSHIFT) : %i , TIME : %f NT : %i NDT : %i \n\n",NDTSHIFT,TIME, NT,NDT);*/
        fprintf(FP, " \nSampling rate for seismogram output (NDT) is out of limit : %i \n\n", ns);
        err(" Check Sampling rate for seismogram output (NDT)!");
    }

    /* output of parameters to stdout: */
    if (MYID == 0)
        writepar(FP, ns);

    /* NXG, NYG NZG denote size of the entire (global) grid */
    NXG = NX;
    NYG = NY;
    NZG = NZ;

    /* In the following, NX, MY, NZ denote size of the local grid ! */
    NX = IENDX;
    NY = IENDY;
    NZ = IENDZ;

    /* compute receiver locations within each subgrid and
	   store local receiver coordinates in recpos_loc */
    if (SEISMO)
    {
        fprintf(FP, "\n ------------------ READING RECEIVER PARAMETERS ----------------- \n");
        recpos = receiver(FP, &ntr);
        recswitch = ivector(1, ntr);
        recpos_loc = splitrec(recpos, &ntr_loc, ntr, recswitch);
        ntr_glob = ntr;
        ntr = ntr_loc;
        fprintf(FP,"SEISMO = %d, ntr = %d\n\n", ntr, SEISMO);
        // getchar();
    }

    /* number of seismogram sections which have to be stored in core memory*/
    /* allocation of memory for seismogramm merge */
    switch (SEISMO)
    {
        case 1: /* particle velocities only */
            nseismograms = 3;
            break;
        case 2: /* pressure only */
            nseismograms = 1;
            break;
        case 3: /* curl and div only */
            nseismograms = 2;
            break;
        case 4: /* everything */
            nseismograms = 6;
            break;
        default:
            nseismograms = 0;
            break;
    }

    /*allocate memory for dynamic, static and buffer arrays */
    fac1 = (NZ + FDORDER) * (NY + FDORDER) * (NX + FDORDER);
    fac2 = sizeof(float) * pow(2.0, -20.0);

    if (L > 0)
    { /*viscoelastic case*/
        if (FDORDER_TIME == 2)
        {
            memdyn = 15.0 * fac1 * fac2;
            memmodel = 16.0 * fac1 * fac2;
        }
        else
        {
            if (FDORDER_TIME == 3)
            {
                memdyn = 57 * fac1 * fac2;
                memmodel = 16.0 * fac1 * fac2;
            }
            else
            {
                memdyn = 78 * fac1 * fac2;
                memmodel = 16.0 * fac1 * fac2;
            }
        }
    }
    else
    { /* elastic case*/
        if (FDORDER_TIME == 2)
        {
            memdyn = 9.0 * fac1 * fac2;
            memmodel = 10.0 * fac1 * fac2;
        }
        else
        {
            if (FDORDER_TIME == 3)
            {
                memdyn = 39 * fac1 * fac2;
                memmodel = 10.0 * fac1 * fac2;
            }
            else
            {
                memdyn = 49 * fac1 * fac2;
                memmodel = 10.0 * fac1 * fac2;
            }
        }
    }

    memseismograms = nseismograms * ntr_glob * ns * fac2 + nseismograms * ntr * ns * fac2;
    membuffer = (2.0 * (3.0 * FDORDER / 2 - 1) * (NY * NZ + NX * NZ + NY * NX) + 2.0 * (3.0 * FDORDER / 2 - 2) * (NY * NZ + NX * NZ + NY * NX)) * fac2;
    membuffer = 4.0 * 6.0 * ((NX * NZ) + (NY * NZ) + (NX * NY)) * fac2;
    if (ABS_TYPE == 1)
        memcpml = 2.0 * FW * 6.0 * (NY * NZ + NX * NZ + NY * NX) * fac2 + 24.0 * 2.0 * FW * fac2;
    buffsize = (FDORDER)*4.0 * 6.0 * (max((NX * NZ), max((NY * NZ), (NX * NY)))) * sizeof(MPI_FLOAT);
    memtotal = memdyn + memmodel + memseismograms + membuffer + memcpml + (buffsize * pow(2.0, -20.0));

    if (MYID == 0)
    {
        fprintf(FP, "\n ----------------------------------------------------------------");
        fprintf(FP, "\n ------------------ MEMORY ALLOCATION --------------------------- \n");
        fprintf(FP, "\n **Message from main (printed by PE %d):\n", MYID);
        fprintf(FP, " Size of local grids: NX=%d \t NY=%d \t NZ=%d \n", NX, NY, NZ);
        fprintf(FP, " Each process is now trying to allocate memory for:\n");
        fprintf(FP, " Dynamic variables: \t\t %6.2f MB\n", memdyn);
        fprintf(FP, " Static variables: \t\t %6.2f MB\n", memmodel);
        fprintf(FP, " Seismograms: \t\t\t %6.2f MB\n", memseismograms);
        if (ABS_TYPE == 1)
            fprintf(FP, " CPML memory variables : \t %6.2f MB\n", memcpml);
        fprintf(FP, " Buffer arrays for grid exchange:%6.2f MB\n", membuffer);
        fprintf(FP, " Network Buffer for MPI_Bsend: \t %6.2f MB\n\n", buffsize * pow(2.0, -20.0));
        fprintf(FP, " Total memory required: \t %6.2f MB.\n", memtotal);
        fprintf(FP, " Please note that the memory consumption is only a good estimate! ");
        fprintf(FP, "\n ---------------------------------------------------------------- \n\n");
    }

    /* allocate buffer for buffering messages */
    buff_addr = malloc(buffsize);
    if (!buff_addr)
        err("allocation failure for buffer for MPI_Bsend !");
    MPI_Buffer_attach(buff_addr, buffsize);

    /* allocation for request and status arrays */
    req_send = (MPI_Request *)malloc(REQUEST_COUNT * sizeof(MPI_Request));
    req_rec = (MPI_Request *)malloc(REQUEST_COUNT * sizeof(MPI_Request));
    sreq_send = (MPI_Request *)malloc(REQUEST_COUNT * sizeof(MPI_Request));
    sreq_rec = (MPI_Request *)malloc(REQUEST_COUNT * sizeof(MPI_Request));

    /* allocation for timing arrays used for performance analysis */
    time_v_update = dvector(1, NT);
    time_s_update = dvector(1, NT);
    time_s_exchange = dvector(1, NT);
    time_v_exchange = dvector(1, NT);
    time_timestep = dvector(1, NT);

    l = 1;
    if (ABS_TYPE == 1 && FDORDER == 2)
    {
        l = 2;
    }

    /* memory allocation for dynamic (wavefield) arrays */
    if (POS[2] == 0)
    {
        v.x = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
        v.y = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
        v.z = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

        if (FDORDER_TIME != 2)
        { /* Allocate memory for Adams Bashforth */
            dv.xyyx = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv.yzzy = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv.xzzx = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv.xxyyzz = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv.yyzz = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv.xxzz = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv.xxyy = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

            dv_2.xyyx = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv_2.yzzy = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv_2.xzzx = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv_2.xxyyzz = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv_2.yyzz = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv_2.xxzz = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv_2.xxyy = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

            dv_3.xyyx = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv_3.yzzy = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv_3.xzzx = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv_3.xxyyzz = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv_3.yyzz = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv_3.xxzz = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv_3.xxyy = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

            svx = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            svy = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            svz = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

            svx_2 = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            svy_2 = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            svz_2 = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

            svx_3 = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            svy_3 = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            svz_3 = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

            if (FDORDER_TIME == 4)
            {
                svx_4 = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
                svy_4 = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
                svz_4 = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

                dv_4.xyyx = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
                dv_4.yzzy = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
                dv_4.xzzx = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
                dv_4.xxyyzz = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
                dv_4.yyzz = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
                dv_4.xxzz = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
                dv_4.xxyy = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            }
        }

        s.xy = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
        s.yz = f3tensor(0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
    }

    if (POS[2] > 0)
    {
        v.x = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
        v.y = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
        v.z = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

        if (FDORDER_TIME != 2)
        { /* Allocate memory for Adams Bashforth */
            dv.xyyx = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv.yzzy = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv.xzzx = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv.xxyyzz = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv.yyzz = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv.xxzz = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv.xxyy = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

            dv_2.xyyx = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv_2.yzzy = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv_2.xzzx = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv_2.xxyyzz = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv_2.yyzz = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv_2.xxzz = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv_2.xxyy = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

            dv_3.xyyx = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv_3.yzzy = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv_3.xzzx = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv_3.xxyyzz = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv_3.yyzz = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv_3.xxzz = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            dv_3.xxyy = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

            svx = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            svy = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            svz = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

            svx_2 = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            svy_2 = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            svz_2 = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

            svx_3 = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            svy_3 = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            svz_3 = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

            if (FDORDER_TIME == 4)
            {
                svx_4 = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
                svy_4 = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
                svz_4 = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

                dv_4.xyyx = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
                dv_4.yzzy = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
                dv_4.xzzx = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
                dv_4.xxyyzz = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
                dv_4.yyzz = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
                dv_4.xxzz = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
                dv_4.xxyy = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            }
        }

    s.xy = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
    s.yz = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
    }
    s.xz = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
    s.xx = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
    s.yy = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
    s.zz = f3tensor(1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

    xb = ivector(0, 1);
    yb = ivector(0, 1);
    zb = ivector(0, 1);

    if (L)
    { /* no allocation in case of purely elastic simulation */
        /* memory allocation for dynamic (model) arrays */
        r.xy = f3tensor(1, NY, 1, NX, 1, NZ);
        r.yz = f3tensor(1, NY, 1, NX, 1, NZ);
        r.xz = f3tensor(1, NY, 1, NX, 1, NZ);
        r.xx = f3tensor(1, NY, 1, NX, 1, NZ);
        r.yy = f3tensor(1, NY, 1, NX, 1, NZ);
        r.zz = f3tensor(1, NY, 1, NX, 1, NZ);

        if (FDORDER_TIME != 2)
        { /* Allocate memory for Adams Bashforth */
            rxy_2 = f3tensor(1, NY, 1, NX, 1, NZ);
            ryz_2 = f3tensor(1, NY, 1, NX, 1, NZ);
            rxz_2 = f3tensor(1, NY, 1, NX, 1, NZ);
            rxx_2 = f3tensor(1, NY, 1, NX, 1, NZ);
            ryy_2 = f3tensor(1, NY, 1, NX, 1, NZ);
            rzz_2 = f3tensor(1, NY, 1, NX, 1, NZ);

            rxy_3 = f3tensor(1, NY, 1, NX, 1, NZ);
            ryz_3 = f3tensor(1, NY, 1, NX, 1, NZ);
            rxz_3 = f3tensor(1, NY, 1, NX, 1, NZ);
            rxx_3 = f3tensor(1, NY, 1, NX, 1, NZ);
            ryy_3 = f3tensor(1, NY, 1, NX, 1, NZ);
            rzz_3 = f3tensor(1, NY, 1, NX, 1, NZ);
            if (FDORDER_TIME == 4)
            {
                rxy_4 = f3tensor(1, NY, 1, NX, 1, NZ);
                ryz_4 = f3tensor(1, NY, 1, NX, 1, NZ);
                rxz_4 = f3tensor(1, NY, 1, NX, 1, NZ);
                rxx_4 = f3tensor(1, NY, 1, NX, 1, NZ);
                ryy_4 = f3tensor(1, NY, 1, NX, 1, NZ);
                rzz_4 = f3tensor(1, NY, 1, NX, 1, NZ);
            }
        }

        /* memory allocation for static (model) arrays */
        taus = f3tensor(0, NY + 1, 0, NX + 1, 0, NZ + 1);
        taup = f3tensor(0, NY + 1, 0, NX + 1, 0, NZ + 1);
        tausipjp = f3tensor(1, NY, 1, NX, 1, NZ);
        tausjpkp = f3tensor(1, NY, 1, NX, 1, NZ);
        tausipkp = f3tensor(1, NY, 1, NX, 1, NZ);
        eta = vector(1, L);
    }

    /* memory allocation for static (model) arrays */
    rho = f3tensor(0, NY + 1, 0, NX + 1, 0, NZ + 1);
    pi = f3tensor(0, NY + 1, 0, NX + 1, 0, NZ + 1);
    // adding Cij variables by VK
    C11 = f3tensor(0, NY + 1, 0, NX + 1, 0, NZ + 1);
    C12 = f3tensor(0, NY + 1, 0, NX + 1, 0, NZ + 1);
    C13 = f3tensor(0, NY + 1, 0, NX + 1, 0, NZ + 1);
    C22 = f3tensor(0, NY + 1, 0, NX + 1, 0, NZ + 1);
    C23 = f3tensor(0, NY + 1, 0, NX + 1, 0, NZ + 1);
    C33 = f3tensor(0, NY + 1, 0, NX + 1, 0, NZ + 1);
    C44 = f3tensor(0, NY + 1, 0, NX + 1, 0, NZ + 1);
    C55 = f3tensor(0, NY + 1, 0, NX + 1, 0, NZ + 1);
    C66 = f3tensor(0, NY + 1, 0, NX + 1, 0, NZ + 1);

    // still keeping u = mu and pi = lambda + 2*mu (just in case) ;)
    u = f3tensor(0, NY + 1, 0, NX + 1, 0, NZ + 1);

    absorb_coeff = f3tensor(1, NY, 1, NX, 1, NZ);

    /* averaged material parameters */
    C66ipjp = f3tensor(1, NY, 1, NX, 1, NZ);
    C44jpkp = f3tensor(1, NY, 1, NX, 1, NZ);
    C55ipkp = f3tensor(1, NY, 1, NX, 1, NZ);
    rjp = f3tensor(1, NY, 1, NX, 1, NZ);
    rkp = f3tensor(1, NY, 1, NX, 1, NZ);
    rip = f3tensor(1, NY, 1, NX, 1, NZ);

    /* memory allocation for CPML variables*/
    if (ABS_TYPE == 1)
    {
        K_x = vector(1, 2 * FW);
        alpha_prime_x = vector(1, 2 * FW);
        a_x = vector(1, 2 * FW);
        b_x = vector(1, 2 * FW);
        K_x_half = vector(1, 2 * FW);
        alpha_prime_x_half = vector(1, 2 * FW);
        a_x_half = vector(1, 2 * FW);
        b_x_half = vector(1, 2 * FW);

        K_y = vector(1, 2 * FW);
        alpha_prime_y = vector(1, 2 * FW);
        a_y = vector(1, 2 * FW);
        b_y = vector(1, 2 * FW);
        K_y_half = vector(1, 2 * FW);
        alpha_prime_y_half = vector(1, 2 * FW);
        a_y_half = vector(1, 2 * FW);
        b_y_half = vector(1, 2 * FW);

        K_z = vector(1, 2 * FW);
        alpha_prime_z = vector(1, 2 * FW);
        a_z = vector(1, 2 * FW);
        b_z = vector(1, 2 * FW);
        K_z_half = vector(1, 2 * FW);
        alpha_prime_z_half = vector(1, 2 * FW);
        a_z_half = vector(1, 2 * FW);
        b_z_half = vector(1, 2 * FW);

        psi_sxx_x = f3tensor(1, NY, 1, 2 * FW, 1, NZ);
        psi_sxy_x = f3tensor(1, NY, 1, 2 * FW, 1, NZ);
        psi_sxz_x = f3tensor(1, NY, 1, 2 * FW, 1, NZ);
        psi_syy_y = f3tensor(1, 2 * FW, 1, NX, 1, NZ);
        psi_sxy_y = f3tensor(1, 2 * FW, 1, NX, 1, NZ);
        psi_syz_y = f3tensor(1, 2 * FW, 1, NX, 1, NZ);
        psi_szz_z = f3tensor(1, NY, 1, NX, 1, 2 * FW);
        psi_sxz_z = f3tensor(1, NY, 1, NX, 1, 2 * FW);
        psi_syz_z = f3tensor(1, NY, 1, NX, 1, 2 * FW);

        psi_vxx = f3tensor(1, NY, 1, 2 * FW, 1, NZ);
        psi_vyy = f3tensor(1, 2 * FW, 1, NX, 1, NZ);
        psi_vzz = f3tensor(1, NY, 1, NX, 1, 2 * FW);
        psi_vxy = f3tensor(1, 2 * FW, 1, NX, 1, NZ);
        psi_vxz = f3tensor(1, NY, 1, NX, 1, 2 * FW);
        psi_vyx = f3tensor(1, NY, 1, 2 * FW, 1, NZ);
        psi_vyz = f3tensor(1, NY, 1, NX, 1, 2 * FW);
        psi_vzx = f3tensor(1, NY, 1, 2 * FW, 1, NZ);
        psi_vzy = f3tensor(1, 2 * FW, 1, NX, 1, NZ);
    }

    /* memory allocation for buffer arrays in which the wavefield information which is exchanged between neighboring PEs is stored */

    /* number of wavefield parameters that need to be exchanged - see exchange_v.c */
    nf1 = (3 * FDORDER / 2) - 1;
    nf2 = nf1 - 1;

    bufferlef_to_rig = f3tensor(1, NY, 1, NZ, 1, nf1);
    bufferrig_to_lef = f3tensor(1, NY, 1, NZ, 1, nf2);
    buffertop_to_bot = f3tensor(1, NX, 1, NZ, 1, nf1);
    bufferbot_to_top = f3tensor(1, NX, 1, NZ, 1, nf2);
    bufferfro_to_bac = f3tensor(1, NY, 1, NX, 1, nf1);
    bufferbac_to_fro = f3tensor(1, NY, 1, NX, 1, nf2);

    sbufferlef_to_rig = f3tensor(1, NY, 1, NZ, 1, nf2);
    sbufferrig_to_lef = f3tensor(1, NY, 1, NZ, 1, nf1);
    sbuffertop_to_bot = f3tensor(1, NX, 1, NZ, 1, nf2);
    sbufferbot_to_top = f3tensor(1, NX, 1, NZ, 1, nf1);
    sbufferfro_to_bac = f3tensor(1, NY, 1, NX, 1, nf2);
    sbufferbac_to_fro = f3tensor(1, NY, 1, NX, 1, nf1);

    /* allocate buffer for seismogram output, merged seismogram section of all PEs */
    if (SEISMO)
        seismo_fulldata = fmatrix(1, ntr_glob, 1, ns);

    /* allocate buffer for seismogram output, seismogram section of each PE */
    if (ntr > 0)
    {
        switch (SEISMO)
        {
            case 1: /* particle velocities only */
                sectionvx = fmatrix(1, ntr, 1, ns);
                sectionvy = fmatrix(1, ntr, 1, ns);
                sectionvz = fmatrix(1, ntr, 1, ns);
                break;
            case 2: /* pressure only */
                sectionp = fmatrix(1, ntr, 1, ns);
                break;
            case 3: /* curl and div only */
                sectioncurl = fmatrix(1, ntr, 1, ns);
                sectiondiv = fmatrix(1, ntr, 1, ns);
                break;
            case 4: /* everything */
                sectionvx = fmatrix(1, ntr, 1, ns);
                sectionvy = fmatrix(1, ntr, 1, ns);
                sectionvz = fmatrix(1, ntr, 1, ns);
                sectioncurl = fmatrix(1, ntr, 1, ns);
                sectiondiv = fmatrix(1, ntr, 1, ns);
                sectionp = fmatrix(1, ntr, 1, ns);
                break;
        }
    }
    int irtm;
        for (irtm = 0; irtm <= RTM_FLAG; irtm++)
    {
        if (irtm>0)
        {
            lsnap =  iround(TSNAP1 / DT);
        }
    if (MYID == 0)
        fprintf(FP, " ... memory allocation for PE %d was successfull.\n\n", MYID);

    /* memory for source position definition */
    srcpos1 = fmatrix(1, 6, 1, 1);




    /* Reading source positions from SOURCE_FILE */
    fprintf(FP, "\n ------------------ READING SOURCE PARAMETERS ------------------- \n");
    switch (SRCREC)
    {
        case 0:
            if (MYID == 0)
                err("SRCREC parameter is invalid (SRCREC!=1)! No source parameters specified!");
            break;
        case 1:
            if (MYID == 0)
            {
                fprintf(FP, "\n Reading source parameters from file: %s (ASOFI3D source format)\n", SOURCE_FILE);

                if ((fpsrc = fopen(SOURCE_FILE, "r")) == NULL)
                    err(" Source file could not be opened !");
                while (fgets(buffer, STRING_SIZE, fpsrc))
                {
                    sscanf(buffer, "%s", bufferstring);
                    /* checks if the line contains a '%'character which indicates a comment line,
					and if the reading of a string was successful, which is not the case for an empty line*/
                    if ((strchr(buffer, '#') == 0) && (sscanf(buffer, "%s", bufferstring) == 1))
                        ++(nsrc);
                }
                rewind(fpsrc);
                if ((nsrc) == 0)
                    fprintf(FP, "\n WARNING: Could not determine number of sources parameter sets in input file. Assuming %d.\n", (nsrc = 0));
                else
                    fprintf(FP, " Number of source positions specified in %s : %d \n", SOURCE_FILE, nsrc);
            }

            MPI_Barrier(MPI_COMM_WORLD);
            MPI_Bcast(&nsrc, 1, MPI_INT, 0, MPI_COMM_WORLD);

            stype = ivector(1, nsrc);
            srcpos = sources(fpsrc, &nsrc, stype);

            /*originally, SOURCE_TYPE=stype is defined in the source file, if not, SOURCE_TYPE is taken from the input file */
            /*if (stype==NULL) printf("PE%d: Source type(s) undefined?! \n",MYID);*/

            break;
        case 2:
            if ((PLANE_WAVE_DEPTH > 0))
            {
                if (MYID == 0)
                {
                    /*stype=(int *)malloc(nsrc*sizeof(int));*/ /* for unknown reasons, the pointer does not point to memory that has been allocated by a subroutine this way */

                    /*determining the number of sources in the specified plane normal/tilted to the surface/upper model boundary*/
                    nsrc = (NXG - 2 * FW + 1) * (NZG - 2 * FW + 1);
                    /*fprintf(FP,"\n nsrc= %i with NGX=%i, NYG=%i and FW=%i. \n",nsrc,NXG,NYG,FW);*/
                }

                MPI_Barrier(MPI_COMM_WORLD);
                MPI_Bcast(&nsrc, 1, MPI_INT, 0, MPI_COMM_WORLD);

                stype = ivector(1, nsrc);
                srcpos = pwsources(&nsrc, stype);
            }
            else
            {
                err("SRCREC parameter specifies PLANE_WAVE excitation, but PLANE_WAVE_DEPTH<=0!");
            }
            break;
        /*more source file formats or source file options can be implemented here*/
        default:
            err("SRCREC parameter is invalid (SRCREC!=1 or SRCREC!=2)! No source parameters specified!");
            break;
    }

    /* create model grids check the function readmod*/
    fprintf(FP, "\n ------------------ MODEL CREATION AND OUTPUT-(aniso READMOD=1 needs to be implemented still)---\n");
    if (READMOD)
        readmod(rho, pi, u, taus, taup, eta);
    else
    {
        if (L == 0)
            model_elastic(rho, pi, u, C11, C12, C13, C22, C23, C33, C44, C55, C66, taus, taup, eta); /* elastic modeling, L is specified in input file*/
        else
        {
            model_visco(rho, pi, u, taus, taup, eta); /* viscoelastic modeling, L is specified in input file*/
        }
    }



    fprintf(FP,"\n \n MYID %d rsf %d rsfden %s", MYID,RSF,RSFDEN);


    // Madagascar

    if (RSF) madinput(RSFDEN,rho);
    //mad_elastic(rho, pi, u, C11, C12, C13, C22, C23, C33, C44, C55, C66, taus, taup, eta);

    if (RUN_MULTIPLE_SHOTS)
        nshots = nsrc;
    else
        nshots = 1;
    /*printf("\n ------------------ checkfd by MYID %i -------------------- \n", MYID);*/
    /* check if the FD run will be stable and free of numerical dispersion */
    checkfd(FP, rho, pi, u, taus, taup, eta, srcpos, nsrc, recpos, ntr_glob);

    /* calculate 3-D array for exponential damping of reflections
           at the edges of the numerical mesh (PML-boundary)*/
    /*if(ABS_TYPE==1){
	  absorb_PML(absorb_coeffx, absorb_coeffy, absorb_coeffz);
        }*/

    /* calculate damping coefficients for CPML boundary*/
    if (ABS_TYPE == 1)
    {
        CPML_coeff(K_x, alpha_prime_x, a_x, b_x, K_x_half, alpha_prime_x_half, a_x_half, b_x_half, K_y, alpha_prime_y, a_y, b_y, K_y_half, alpha_prime_y_half, a_y_half, b_y_half, K_z, alpha_prime_z, a_z, b_z, K_z_half, alpha_prime_z_half, a_z_half, b_z_half);
    }

    /* calculate 3-D array for exponential damping of reflections
	   at the edges of the numerical mesh */
    if (ABS_TYPE == 2)
    {
        absorb(absorb_coeff);
    }

    /* For the calculation of the material parameters between gridpoints
	   the parameters have to be averaged. For this, values lying at 0 and NX+1,
	for example, are required on the local grid. These are now copied from the
	neighbouring grids */
    matcopy(rho, pi, u, C11, C12, C13, C22, C23, C33, C44, C55, C66, taus, taup);

    /* spatial averaging of material parameters, i.e. Tau for S-waves, shear modulus, and density */
    av_mat(rho, pi, u, C44, C55, C66, taus, taup, C66ipjp, C44jpkp, C55ipkp, tausipjp, tausjpkp, tausipkp, rjp, rkp, rip);

    MPI_Barrier(MPI_COMM_WORLD);

    if (CHECKPTREAD)
    {
        if (MYID == 0)
        {
            time3 = MPI_Wtime();
            fprintf(FP, " Reading wavefield from check-point file %s \n", CHECKPTFILE);
        }

        read_checkpoint(-1, NX + 2, -1, NY + 2, -1, NZ + 2, &v, &s, &r,
                        psi_sxx_x, psi_sxy_x, psi_sxz_x, psi_sxy_y, psi_syy_y, psi_syz_y, psi_sxz_z, psi_syz_z, psi_szz_z,
                        psi_vxx, psi_vyx, psi_vzx, psi_vxy, psi_vyy, psi_vzy, psi_vxz, psi_vyz, psi_vzz);
        MPI_Barrier(MPI_COMM_WORLD);
        if (MYID == 0)
        {
            time4 = MPI_Wtime();
            fprintf(FP, " finished (real time: %4.2f s).\n", time4 - time3);
        }
    }

    /* comunication initialisation for persistent communication */
    /*comm_ini(bufferlef_to_rig, bufferrig_to_lef,
	    buffertop_to_bot, bufferbot_to_top, bufferfro_to_bac,
	    bufferbac_to_fro, req_send, req_rec);

	comm_ini_s(sbufferlef_to_rig, sbufferrig_to_lef,
	    sbuffertop_to_bot, sbufferbot_to_top, sbufferfro_to_bac,
	    sbufferbac_to_fro, sreq_send, sreq_rec);*/

    /* initialisation of PML and ABS domain */
    if (ABS_TYPE == 1)
    {
        CPML_ini_elastic(xb, yb, zb);
    }

    if (ABS_TYPE == 2)
    {
        xb[0] = 1;
        xb[1] = NX;
        yb[0] = 1;
        yb[1] = NY;
        zb[0] = 1;
        zb[1] = NZ;
    }

    if (MYID == 0)
    {
        time2 = MPI_Wtime();
        fprintf(FP, "\n\n\n **************************************************\n");
        fprintf(FP, " *********** STARTING TIME STEPPING ***************\n");
        fprintf(FP, " **************************************************\n");
        fprintf(FP, " real time before starting time loop: %4.2f s.\n", time2 - time1);
    }

    // for (int irtm = 0; irtm <= RTM_FLAG; irtm++)
    // {
    //if (RSF) madinput(RSFDEN,rho);





    for (ishot = 1; ishot <= nshots; ishot++)
        {
            fprintf(FP, "\n MYID=%d *****  Starting simulation for shot %d of %d  ********** \n", MYID, ishot, nshots);
            for (nt = 1; nt <= 6; nt++)
                srcpos1[nt][1] = srcpos[nt][ishot];
            if (RUN_MULTIPLE_SHOTS)
            {
                /* find this single source positions on subdomains */
                if (nsrc_loc > 0)
                    free_matrix(srcpos_loc, 1, 6, 1, 1);
                if (stype_loc == NULL)
                    stype_loc = ivector(1, nsrc);
                srcpos_loc = splitsrc(srcpos1, &nsrc_loc, 1, stype_loc, stype);
            }
            else
            {
                /* Distribute multiple source positions on subdomains */
                if (stype_loc == NULL)
                    stype_loc = ivector(1, nsrc);
                srcpos_loc = splitsrc(srcpos, &nsrc_loc, nsrc, stype_loc, stype);
            }

            /* calculate wavelet for each source point */
            signals = wavelet(srcpos_loc, nsrc_loc);

            // if (irtm > 0)
            // {
            //     // sustrip <

            //     // free_matrix()
            //     ntr = ntr_glob;
            //     srcpos = fmatrix(1, 6, 1, ntr);
            //     srcpos_loc = fmatrix(1, 6, 1, ntr);
            //     stype = ivector(1, ntr);
            //     stype_loc = ivector(1, ntr);
            //     for (int ii = 1; ii <= ntr; ii++)
            //     {
            //         srcpos_loc[1][ii] = recpos_loc[1][ii];
            //         srcpos_loc[2][ii] = recpos_loc[2][ii];
            //         srcpos_loc[3][ii] = recpos_loc[3][ii];

            //         srcpos[1][ii] = recpos[1][ii]; // x
            //         srcpos[2][ii] = recpos[2][ii]; // y
            //         srcpos[3][ii] = recpos[3][ii]; // z
            //         srcpos[4][ii] = 0.0;           // tshift
            //         srcpos[5][ii] = 1.0;           // fc
            //         srcpos[6][ii] = 5;             // stype??

            //         fprintf(FP, "\n WE ARE INSIDE \n");
            //     }
            //     nsrc = ntr;
            //     nsrc_loc = ntr_loc;

            //     signals = wavelet(srcpos_loc, nsrc_loc);

			// 	fprintf(FP, "nsrc = %d  nsrc_loc = %d",nsrc, nsrc_loc);
            //     // srcpos_loc = splitsrc(srcpos,&nsrc_loc, nsrc, stype_loc, stype);
            // }

            /* output of calculated wavelet for each source point */

            if ((OUTSOURCEWAVELET != 0) && (nsrc_loc > 0))
            {
                char source_signal_file[STRING_SIZE];
                sprintf(source_signal_file, "source_signal.MYID%d.shot%d.su", MYID, ishot);
                fprintf(FP, "\n PE %d outputs source time function in SU format to %s \n ", MYID, source_signal_file);
                output_source_signal(fopen(source_signal_file, "w"), signals, NT, 1);
            }

            /* initialize wavefield with zero */
            if ((L == 1) && (ABS_TYPE == 2) && (CHECKPTREAD == 0))
            {
                zero(1 - FDORDER / 2, NX + FDORDER / 2, 1 - FDORDER / 2, NY + FDORDER / 2, 1 - FDORDER / 2, NZ + FDORDER / 2, &v, &s, &dv,
                     &dv_2, &dv_3, &dv_4,
                     svx, svy, svz, svx_2, svy_2, svz_2, svx_3, svy_3, svz_3, svx_4, svy_4, svz_4, &r, rxx_2, ryy_2, rzz_2, rxy_2, ryz_2, rxz_2, rxx_3, ryy_3, rzz_3, rxy_3, ryz_3, rxz_3, rxx_4, ryy_4, rzz_4, rxy_4, ryz_4, rxz_4);
            }

            if ((L == 0) && (ABS_TYPE == 2) && (CHECKPTREAD == 0))
            {
                zero_elastic(1 - FDORDER / 2, NX + FDORDER / 2, 1 - FDORDER / 2, NY + FDORDER / 2, 1 - FDORDER / 2, NZ + FDORDER / 2,
                             &v, &s,
                             &dv, &dv_2, &dv_3, &dv_4,
                             svx, svy, svz, svx_2, svy_2, svz_2, svx_3, svy_3, svz_3, svx_4, svy_4, svz_4);
            }
            if ((ABS_TYPE == 1) && (CHECKPTREAD == 0))
            {
                zero_elastic_CPML(NX, NY, NZ, &v, &s, &r, psi_sxx_x, psi_sxy_x, psi_sxz_x, psi_sxy_y, psi_syy_y, psi_syz_y, psi_sxz_z, psi_syz_z, psi_szz_z, psi_vxx, psi_vyx, psi_vzx, psi_vxy, psi_vyy, psi_vzy, psi_vxz, psi_vyz, psi_vzz, rxx_2, ryy_2, rzz_2, rxy_2, ryz_2, rxz_2, rxx_3, ryy_3, rzz_3, rxy_3, ryz_3, rxz_3, rxx_4, ryy_4, rzz_4, rxy_4, ryz_4, rxz_4);
            }

            /* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
            /* start of loop over time steps */

            lsamp = NDTSHIFT + 1;
            nlsamp = 1;



//#pragma acc data copyin(vx[ny1-1:ny2+1][nx1-1:nx2+1][nz1-1:nz2+1],vy[ny1-1:ny2+1][nx1-1:nx2+1][nz1-1:nz2+1],vz[ny1-1:ny2+1][nx1-1:nx2+1][nz1-1:nz2+1])

/*


vxyyx, vyzzy, vxzzx, vxxyyzz, vyyzz, vxxzz, vxxyy,
vxyyx_2, vyzzy_2, vxzzx_2, vxxyyzz_2, vyyzz_2, vxxzz_2, vxxyy_2, vxyyx_3, vyzzy_3, vxzzx_3, vxxyyzz_3, vyyzz_3, vxxzz_3, vxxyy_3, vxyyx_4, vyzzy_4, vxzzx_4, vxxyyzz_4, vyyzz_4, vxxzz_4, vxxyy_4

in:
out: sxx, syy, szz, sxy, syz, sxz,*/

//#pragma acc data copyin (C11,C12,C13,C33,C22,C23,C66ipjp,C44jpkp,C55ipkp)
//#pragma acc data copyout(sxy,syz,sxz,sxx,syy,szz)
/*
#pragma acc data copyin(xb[0],xb[1],yb[0],yb[1],zb[0],zb[1],nt)
#pragma acc data copyin(vx[yb[0]-1:yb[1]+1][xb[0]-1:xb[1]+1][zb[0]-1:zb[1]+1])
#pragma acc data copyin(vy[yb[0]-1:yb[1]+1][xb[0]-1:xb[1]+1][zb[0]-1:zb[1]+1])
#pragma acc data copyin(vz[yb[0]-1:yb[1]+1][xb[0]-1:xb[1]+1][zb[0]-1:zb[1]+1])
#pragma acc data copyin(C11[yb[0]-1:yb[1]+1][xb[0]-1:xb[1]+1][zb[0]-1:zb[1]+1])
#pragma acc data copyin(C12[yb[0]-1:yb[1]+1][xb[0]-1:xb[1]+1][zb[0]-1:zb[1]+1])
#pragma acc data copyin(C13[yb[0]-1:yb[1]+1][xb[0]-1:xb[1]+1][zb[0]-1:zb[1]+1])
#pragma acc data copyin(C22[yb[0]-1:yb[1]+1][xb[0]-1:xb[1]+1][zb[0]-1:zb[1]+1])
#pragma acc data copyin(C23[yb[0]-1:yb[1]+1][xb[0]-1:xb[1]+1][zb[0]-1:zb[1]+1])
#pragma acc data copyin(C33[yb[0]-1:yb[1]+1][xb[0]-1:xb[1]+1][zb[0]-1:zb[1]+1])
#pragma acc data copyin(C66ipjp[yb[0]-1:yb[1]+1][xb[0]-1:xb[1]+1][zb[0]-1:zb[1]+1])
#pragma acc data copyin(C44jpkp[yb[0]-1:yb[1]+1][xb[0]-1:xb[1]+1][zb[0]-1:zb[1]+1])
#pragma acc data copyin(C55ipkp[yb[0]-1:yb[1]+1][xb[0]-1:xb[1]+1][zb[0]-1:zb[1]+1])
*/
/*
#pragma acc data copyout(sxx[yb[0]-1:yb[1]+1][xb[0]-1:xb[1]+1][zb[0]-1:zb[1]+1])
#pragma acc data copyout(syy[yb[0]-1:yb[1]+1][xb[0]-1:xb[1]+1][zb[0]-1:zb[1]+1])
#pragma acc data copyout(szz[yb[0]-1:yb[1]+1][xb[0]-1:xb[1]+1][zb[0]-1:zb[1]+1])
#pragma acc data copyout(sxy[yb[0]-1:yb[1]+1][xb[0]-1:xb[1]+1][zb[0]-1:zb[1]+1])
#pragma acc data copyout(syz[yb[0]-1:yb[1]+1][xb[0]-1:xb[1]+1][zb[0]-1:zb[1]+1])
#pragma acc data copyout(sxz[yb[0]-1:yb[1]+1][xb[0]-1:xb[1]+1][zb[0]-1:zb[1]+1])
*/



            for (nt = 1; nt <= NT; nt++)
            {
                time_v_update[nt] = 0.0;
                time_s_update[nt] = 0.0;
                time_v_exchange[nt] = 0.0;
                time_s_exchange[nt] = 0.0;

                /* Check if simulation is still stable */
                if (isnan(v.y[NY / 2][NX / 2][NZ / 2]))
                    err(" Simulation is unstable !"); /* maybe just breaking the loop would be better */

                if (LOG)
                    if ((MYID == 0) && ((nt + (OUTNTIMESTEPINFO - 1)) % OUTNTIMESTEPINFO) == 0)
                    {
                        fprintf(FP, "\n Computing timestep %d of %d \n", nt, NT);
                        time2 = MPI_Wtime();
                    }

                /* update of particle velocities */
                time_v_update[nt] = update_v(xb[0], xb[1], yb[0], yb[1], zb[0], zb[1], nt, &v,
                    &s, rho, rjp, rkp, rip, srcpos_loc, signals, nsrc_loc, absorb_coeff, stype_loc, svx, svy, svz, svx_2, svy_2, svz_2, svx_3, svy_3, svz_3, svx_4, svy_4, svz_4);

                if (ABS_TYPE == 1)
                {
                    update_v_CPML(xb[0], xb[1], yb[0], yb[1], zb[0], zb[1], nt, &v,
                        &s, rho, rjp, rkp, rip, srcpos_loc, signals, nsrc_loc, absorb_coeff, stype_loc, K_x, a_x, b_x, K_x_half, a_x_half, b_x_half, K_y, a_y, b_y, K_y_half, a_y_half, b_y_half, K_z, a_z, b_z, K_z_half, a_z_half, b_z_half, psi_sxx_x, psi_sxy_x, psi_sxz_x, psi_sxy_y, psi_syy_y, psi_syz_y, psi_sxz_z, psi_syz_z, psi_szz_z);
                };

                // Shift spatial derivatives of the stress one time step back.
                if (FDORDER_TIME == 4)
                {
                    shift_s1 = svx_4;
                    svx_4 = svx_3;
                    svx_3 = svx_2;
                    svx_2 = svx;
                    svx = shift_s1;
                    shift_s2 = svy_4;
                    svy_4 = svy_3;
                    svy_3 = svy_2;
                    svy_2 = svy;
                    svy = shift_s2;
                    shift_s3 = svz_4;
                    svz_4 = svz_3;
                    svz_3 = svz_2;
                    svz_2 = svz;
                    svz = shift_s3;
                }
                if (FDORDER_TIME == 3)
                {
                    shift_s1 = svx_3;
                    svx_3 = svx_2;
                    svx_2 = svx;
                    svx = shift_s1;
                    shift_s2 = svy_3;
                    svy_3 = svy_2;
                    svy_2 = svy;
                    svy = shift_s2;
                    shift_s3 = svz_3;
                    svz_3 = svz_2;
                    svz_2 = svz;
                    svz = shift_s3;
                }

                /* exchange values of particle velocities at grid boundaries between PEs */

            time_v_exchange[nt] = exchange_v(nt, &v, bufferlef_to_rig, bufferrig_to_lef, buffertop_to_bot, bufferbot_to_top,
                                             bufferfro_to_bac, bufferbac_to_fro, req_send, req_rec);

                /* update of components of stress tensor */

                /* update NON PML boundaries */
                if (L > 0)
                {
                    time_s_update[nt] = update_s(xb[0], xb[1], yb[0], yb[1], zb[0], zb[1], nt, &v,
                        &s, &r,
                                                 pi, u, C66ipjp, C44jpkp, C55ipkp, taus, tausipjp, tausjpkp, tausipkp, taup, eta,
                        &dv, &dv_2, &dv_3, &dv_4,
                        rxx_2, ryy_2, rzz_2, rxy_2, ryz_2, rxz_2, rxx_3, ryy_3, rzz_3, rxy_3, ryz_3, rxz_3, rxx_4, ryy_4, rzz_4, rxy_4, ryz_4, rxz_4);
                    if (ABS_TYPE == 1)
                        update_s_CPML(xb[0], xb[1], yb[0], yb[1], zb[0], zb[1], nt, &v,
                            &s, &r, pi, u,
                                      C66ipjp, C44jpkp, C55ipkp, taus, tausipjp, tausjpkp, tausipkp, taup, eta, K_x, a_x, b_x, K_x_half, a_x_half,
                                      b_x_half, K_y, a_y, b_y, K_y_half, a_y_half, b_y_half, K_z, a_z, b_z, K_z_half, a_z_half, b_z_half,
                                      psi_vxx, psi_vyx, psi_vzx, psi_vxy, psi_vyy, psi_vzy, psi_vxz, psi_vyz, psi_vzz);
                }
                else
                {
                    time_s_update[nt] = update_s_elastic(xb[0], xb[1], yb[0], yb[1], zb[0], zb[1], nt, &v,
                        &s, &r,
                                                         pi, u, C11, C12, C13, C22, C23, C33, C66ipjp, C44jpkp, C55ipkp, taus, tausipjp, tausjpkp, tausipkp, taup, eta,
                        &dv, &dv_2, &dv_3, &dv_4);
                    if (ABS_TYPE == 1)
                        update_s_CPML_elastic(xb[0], xb[1], yb[0], yb[1], zb[0], zb[1], nt, &v,
                                &s, pi, u,
                                C11, C12, C13, C22, C23, C33,
                                C66ipjp, C44jpkp, C55ipkp, K_x, a_x, b_x, K_x_half, a_x_half,
                                b_x_half, K_y, a_y, b_y, K_y_half, a_y_half, b_y_half, K_z, a_z, b_z, K_z_half, a_z_half, b_z_half,
                                psi_vxx, psi_vyx, psi_vzx, psi_vxy, psi_vyy, psi_vzy, psi_vxz, psi_vyz, psi_vzz);
                }

                // Shift spatial derivatives of velocity one time step back.
                if (FDORDER_TIME == 4)
                {
                    shift_v1 = dv_4.xyyx;
                    dv_4.xyyx = dv_3.xyyx;
                    dv_3.xyyx = dv_2.xyyx;
                    dv_2.xyyx = dv.xyyx;
                    dv.xyyx = shift_v1;
                    shift_v2 = dv_4.yzzy;
                    dv_4.yzzy = dv_3.yzzy;
                    dv_3.yzzy = dv_2.yzzy;
                    dv_2.yzzy = dv.yzzy;
                    dv.yzzy = shift_v2;
                    shift_v3 = dv_4.xzzx;
                    dv_4.xzzx = dv_3.xzzx;
                    dv_3.xzzx = dv_2.xzzx;
                    dv_2.xzzx = dv.xzzx;
                    dv.xzzx = shift_v3;
                    shift_v4 = dv_4.xxyyzz;
                    dv_4.xxyyzz = dv_3.xxyyzz;
                    dv_3.xxyyzz = dv_2.xxyyzz;
                    dv_2.xxyyzz = dv.xxyyzz;
                    dv.xxyyzz = shift_v4;
                    shift_v5 = dv_4.yyzz;
                    dv_4.yyzz = dv_3.yyzz;
                    dv_3.yyzz = dv_2.yyzz;
                    dv_2.yyzz = dv.yyzz;
                    dv.yyzz = shift_v5;
                    shift_v6 = dv_4.xxzz;
                    dv_4.xxzz = dv_3.xxzz;
                    dv_3.xxzz = dv_2.xxzz;
                    dv_2.xxzz = dv.xxzz;
                    dv.xxzz = shift_v6;
                    shift_v7 = dv_4.xxyy;
                    dv_4.xxyy = dv_3.xxyy;
                    dv_3.xxyy = dv_2.xxyy;
                    dv_2.xxyy = dv.xxyy;
                    dv.xxyy = shift_v7;
                    if (L == 1)
                    {
                        shift_r1 = rxy_4;
                        rxy_4 = rxy_3;
                        rxy_3 = rxy_2;
                        rxy_2 = r.xy;
                        r.xy = shift_r1;
                        shift_r2 = ryz_4;
                        ryz_4 = ryz_3;
                        ryz_3 = ryz_2;
                        ryz_2 = r.yz;
                        r.yz = shift_r2;
                        shift_r3 = rxz_4;
                        rxz_4 = rxz_3;
                        rxz_3 = rxz_2;
                        rxz_2 = r.xz;
                        r.xz = shift_r3;
                        shift_r4 = rxx_4;
                        rxx_4 = rxx_3;
                        rxx_3 = rxx_2;
                        rxx_2 = r.xx;
                        r.xx = shift_r4;
                        shift_r5 = ryy_4;
                        ryy_4 = ryy_3;
                        ryy_3 = ryy_2;
                        ryy_2 = r.yy;
                        r.yy = shift_r5;
                        shift_r6 = rzz_4;
                        rzz_4 = rzz_3;
                        rzz_3 = rzz_2;
                        rzz_2 = r.zz;
                        r.zz = shift_r6;
                    }
                }
                if (FDORDER_TIME == 3)
                {
                    shift_v1 = dv_3.xyyx;
                    dv_3.xyyx = dv_2.xyyx;
                    dv_2.xyyx = dv.xyyx;
                    dv.xyyx = shift_v1;
                    shift_v2 = dv_3.yzzy;
                    dv_3.yzzy = dv_2.yzzy;
                    dv_2.yzzy = dv.yzzy;
                    dv.yzzy = shift_v2;
                    shift_v3 = dv_3.xzzx;
                    dv_3.xzzx = dv_2.xzzx;
                    dv_2.xzzx = dv.xzzx;
                    dv.xzzx = shift_v3;
                    shift_v4 = dv_3.xxyyzz;
                    dv_3.xxyyzz = dv_2.xxyyzz;
                    dv_2.xxyyzz = dv.xxyyzz;
                    dv.xxyyzz = shift_v4;
                    shift_v5 = dv_3.yyzz;
                    dv_3.yyzz = dv_2.yyzz;
                    dv_2.yyzz = dv.yyzz;
                    dv.yyzz = shift_v5;
                    shift_v6 = dv_3.xxzz;
                    dv_3.xxzz = dv_2.xxzz;
                    dv_2.xxzz = dv.xxzz;
                    dv.xxzz = shift_v6;
                    shift_v7 = dv_3.xxyy;
                    dv_3.xxyy = dv_2.xxyy;
                    dv_2.xxyy = dv.xxyy;
                    dv.xxyy = shift_v7;
                    if (L == 1)
                    {
                        shift_r1 = rxy_3;
                        rxy_3 = rxy_2;
                        rxy_2 = r.xy;
                        r.xy = shift_r1;
                        shift_r2 = ryz_3;
                        ryz_3 = ryz_2;
                        ryz_2 = r.yz;
                        r.yz = shift_r2;
                        shift_r3 = rxz_3;
                        rxz_3 = rxz_2;
                        rxz_2 = r.xz;
                        r.xz = shift_r3;
                        shift_r4 = rxx_3;
                        rxx_3 = rxx_2;
                        rxx_2 = r.xx;
                        r.xx = shift_r4;
                        shift_r5 = ryy_3;
                        ryy_3 = ryy_2;
                        ryy_2 = r.yy;
                        r.yy = shift_r5;
                        shift_r6 = rzz_3;
                        rzz_3 = rzz_2;
                        rzz_2 = r.zz;
                        r.zz = shift_r6;
                    }
                }

                /* explosive source */
                if (!CHECKPTREAD)
                {
                    psource(nt, &s, srcpos_loc, signals, nsrc_loc, stype_loc);
                    /* eqsource is a implementation of moment tensor points sources. */
                    eqsource(nt, &s, srcpos_loc, signals, nsrc_loc, stype_loc, amon, str, dip, rake);
                }

                /* stress free surface ? */
                if ((FREE_SURF) && (POS[2] == 0))
                {
                    if (L)
                        surface(1, u, pi, taus, taup, eta, &s, &r, &v, K_x, a_x, b_x,
                                K_z, a_z, b_z, psi_vxx, psi_vzz);
                    else
                        surface_elastic(1, u, pi, &s, &v, K_x, a_x, b_x,
                                        K_z, a_z, b_z, psi_vxx, psi_vzz);
                }

                /* exchange values of stress at boundaries between PEs */
                time_s_exchange[nt] = exchange_s(nt, &s,
                        sbufferlef_to_rig, sbufferrig_to_lef, sbuffertop_to_bot, sbufferbot_to_top, sbufferfro_to_bac, sbufferbac_to_fro, sreq_send, sreq_rec);

                /* store amplitudes at receivers in e.g. sectionvx, sectionvz, sectiondiv, ...*/
                if ((SEISMO) && (ntr > 0) && (nt == lsamp))
                {
                    seismo(nlsamp, ntr, recpos_loc, sectionvx, sectionvy, sectionvz,
                           sectiondiv, sectioncurl, sectionp, &v, &s, pi, u);
                    nlsamp++;
                    lsamp += NDT;
                }

                /* save snapshot in file */
                // fprintf(FP,"SNAP = %d, nt = %d, lsnap = %d, TSNAP2 / DT = %f\n\n", SNAP, nt, lsnap, TSNAP2 / DT);
                if ((SNAP) && (nt == lsnap) && (nt <= TSNAP2 / DT))
                {
                    // fprintf(FP,"irtm = %d !!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n\n", irtm);
                    snap(FP, nt, ++nsnap, SNAP_FORMAT, SNAP, &v, &s, u, pi,
                         IDX, IDY, IDZ, 1, 1, 1, NX, NY, NZ);
                    lsnap = lsnap + iround(TSNAPINC / DT);
                }

                if (LOG)
                    if ((MYID == 0) && ((nt + (OUTNTIMESTEPINFO - 1)) % OUTNTIMESTEPINFO) == 0)
                    {
                        time3 = MPI_Wtime();
                        time_timestep[nt] = (time3 - time2);
                        fprintf(FP, " total real time for timestep %d : \t\t %4.2f s.\n", nt, time3 - time2);
                    }

            } /* end of loop over timesteps */
            /*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

            fprintf(FP, "\n\n *********** Finish TIME STEPPING ****************\n");
            fprintf(FP, " **************************************************\n\n");

            /* write seismograms to file(s) */
            if (SEISMO)
            {
                /* saves seismograms portion of each PE individually to file */
                //if (ntr>0) saveseis(FP,sectionvx,sectionvy,sectionvz,sectionp,sectioncurl,sectiondiv,recpos,recpos_loc,ntr,srcpos1,ishot,ns);

                /* merge of seismogram data from all PE and output data collectively */
                switch (SEISMO)
                {
                    case 1: /* particle velocities only */
                        catseis(sectionvx, seismo_fulldata, recswitch, ntr_glob, ns);
                        if (MYID == 0)
                            saveseis_glob(FP, seismo_fulldata, recpos, recpos_loc, ntr_glob, srcpos, ishot, ns, 1);
                        catseis(sectionvy, seismo_fulldata, recswitch, ntr_glob, ns);
                        if (MYID == 0)
                            saveseis_glob(FP, seismo_fulldata, recpos, recpos_loc, ntr_glob, srcpos, ishot, ns, 2);
                        catseis(sectionvz, seismo_fulldata, recswitch, ntr_glob, ns);
                        if (MYID == 0)
                            saveseis_glob(FP, seismo_fulldata, recpos, recpos_loc, ntr_glob, srcpos, ishot, ns, 3);

                        break;
                    case 2: /* pressure only */
                        catseis(sectionp, seismo_fulldata, recswitch, ntr_glob, ns);
                        if (MYID == 0)
                            saveseis_glob(FP, seismo_fulldata, recpos, recpos_loc, ntr_glob, srcpos, ishot, ns, 4);

                        break;
                    case 3: /* curl and div only */
                        catseis(sectiondiv, seismo_fulldata, recswitch, ntr_glob, ns);
                        if (MYID == 0)
                            saveseis_glob(FP, seismo_fulldata, recpos, recpos_loc, ntr_glob, srcpos, ishot, ns, 5);
                        catseis(sectioncurl, seismo_fulldata, recswitch, ntr_glob, ns);
                        if (MYID == 0)
                            saveseis_glob(FP, seismo_fulldata, recpos, recpos_loc, ntr_glob, srcpos, ishot, ns, 6);

                        break;
                    case 4: /* everything */
                        /*fprintf(FP," start merging, ntr= %d : \n",ntr_glob);
				fprintf(stdout,"Message from PE %d\n",MYID);*/
                        catseis(sectionvx, seismo_fulldata, recswitch, ntr_glob, ns);
                        if (MYID == 0)
                            saveseis_glob(FP, seismo_fulldata, recpos, recpos_loc, ntr_glob, srcpos, ishot, ns, 1);
                        catseis(sectionvy, seismo_fulldata, recswitch, ntr_glob, ns);
                        if (MYID == 0)
                            saveseis_glob(FP, seismo_fulldata, recpos, recpos_loc, ntr_glob, srcpos, ishot, ns, 2);
                        catseis(sectionvz, seismo_fulldata, recswitch, ntr_glob, ns);
                        if (MYID == 0)
                            saveseis_glob(FP, seismo_fulldata, recpos, recpos_loc, ntr_glob, srcpos, ishot, ns, 3);
                        catseis(sectionp, seismo_fulldata, recswitch, ntr_glob, ns);
                        if (MYID == 0)
                            saveseis_glob(FP, seismo_fulldata, recpos, recpos_loc, ntr_glob, srcpos, ishot, ns, 4);
                        catseis(sectiondiv, seismo_fulldata, recswitch, ntr_glob, ns);
                        if (MYID == 0)
                            saveseis_glob(FP, seismo_fulldata, recpos, recpos_loc, ntr_glob, srcpos, ishot, ns, 5);
                        catseis(sectioncurl, seismo_fulldata, recswitch, ntr_glob, ns);
                        if (MYID == 0)
                            saveseis_glob(FP, seismo_fulldata, recpos, recpos_loc, ntr_glob, srcpos, ishot, ns, 6);

                        break;
                    default:
                        break;
                }
                fprintf(FP, "\n\n");
            }

            /* output timing information (real times for update and exchange) */
            if (LOG)
                if (MYID == 0)
                    timing(time_v_update, time_s_update, time_s_exchange, time_v_exchange, time_timestep, ishot);

        } /* end of loop over shots */
    }




    if (CHECKPTWRITE)
    {
        if (MYID == 0)
        {
            time3 = MPI_Wtime();
            fprintf(FP, " Saving wavefield to check-point file %s \n", CHECKPTFILE);
        }

        save_checkpoint(-1, NX + 2, -1, NY + 2, -1, NZ + 2, &v, &s, &r,
                        psi_sxx_x, psi_sxy_x, psi_sxz_x, psi_sxy_y, psi_syy_y, psi_syz_y, psi_sxz_z, psi_syz_z, psi_szz_z,
                        psi_vxx, psi_vyx, psi_vzx, psi_vxy, psi_vyy, psi_vzy, psi_vxz, psi_vyz, psi_vzz);
        MPI_Barrier(MPI_COMM_WORLD);
        if (MYID == 0)
        {
            time4 = MPI_Wtime();
            fprintf(FP, " finished (real time: %4.2f s).\n", time4 - time3);
        }
    }

    l = 1;
    if (ABS_TYPE == 1 && FDORDER == 2)
    {
        l = 2;
    }

    /*de-allocation of memory */
    if (POS[2] == 0)
    {
        free_f3tensor(v.x, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
        free_f3tensor(v.y, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
        free_f3tensor(v.z, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
        free_f3tensor(s.xy, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
        free_f3tensor(s.yz, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

        if (FDORDER_TIME != 2)
        {
            free_f3tensor(dv.xyyx, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv.yzzy, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv.xzzx, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv.xxyyzz, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv.yyzz, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv.xxzz, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv.xxyy, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

            free_f3tensor(dv_2.xyyx, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv_2.yzzy, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv_2.xzzx, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv_2.xxyyzz, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv_2.yyzz, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv_2.xxzz, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv_2.xxyy, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

            free_f3tensor(dv_3.xyyx, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv_3.yzzy, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv_3.xzzx, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv_3.xxyyzz, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv_3.yyzz, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv_3.xxzz, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv_3.xxyy, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

            free_f3tensor(svx, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(svy, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(svz, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

            free_f3tensor(svx_2, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(svy_2, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(svz_2, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

            free_f3tensor(svx_3, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(svy_3, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(svz_3, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

            if (FDORDER_TIME == 4)
            {
                free_f3tensor(dv_4.xyyx, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
                free_f3tensor(dv_4.yzzy, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
                free_f3tensor(dv_4.xzzx, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
                free_f3tensor(dv_4.xxyyzz, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
                free_f3tensor(dv_4.yyzz, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
                free_f3tensor(dv_4.xxzz, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
                free_f3tensor(dv_4.xxyy, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

                free_f3tensor(svx_4, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
                free_f3tensor(svy_4, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
                free_f3tensor(svz_4, 0 - FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            }
        }
    }

    if (POS[2] > 0)
    {
        free_f3tensor(v.x, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
        free_f3tensor(v.y, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
        free_f3tensor(v.z, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
        free_f3tensor(s.xy, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
        free_f3tensor(s.yz, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

        if (FDORDER_TIME != 2)
        {
            free_f3tensor(dv.xyyx, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv.yzzy, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv.xzzx, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv.xxyyzz, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv.yyzz, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv.xxzz, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv.xxyy, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

            free_f3tensor(dv_2.xyyx, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv_2.yzzy, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv_2.xzzx, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv_2.xxyyzz, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv_2.yyzz, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv_2.xxzz, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv_2.xxyy, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

            free_f3tensor(dv_3.xyyx, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv_3.yzzy, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv_3.xzzx, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv_3.xxyyzz, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv_3.yyzz, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv_3.xxzz, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(dv_3.xxyy, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

            free_f3tensor(svx, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(svy, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(svz, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

            free_f3tensor(svx_2, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(svy_2, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(svz_2, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

            free_f3tensor(svx_3, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(svy_3, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            free_f3tensor(svz_3, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

            if (FDORDER_TIME == 4)
            {
                free_f3tensor(dv_4.xyyx, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
                free_f3tensor(dv_4.yzzy, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
                free_f3tensor(dv_4.xzzx, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
                free_f3tensor(dv_4.xxyyzz, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
                free_f3tensor(dv_4.yyzz, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
                free_f3tensor(dv_4.xxzz, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
                free_f3tensor(dv_4.xxyy, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

                free_f3tensor(svx_4, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
                free_f3tensor(svy_4, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
                free_f3tensor(svz_4, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
            }
        }
    }

    free_f3tensor(s.xz, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
    free_f3tensor(s.xx, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
    free_f3tensor(s.yy, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);
    free_f3tensor(s.zz, 1 - l * FDORDER / 2, NY + l * FDORDER / 2, 1 - l * FDORDER / 2, NX + l * FDORDER / 2, 1 - l * FDORDER / 2, NZ + l * FDORDER / 2);

    if (ABS_TYPE == 1)
    {
        free_vector(K_x, 1, 2 * FW);
        free_vector(alpha_prime_x, 1, 2 * FW);
        free_vector(a_x, 1, 2 * FW);
        free_vector(b_x, 1, 2 * FW);
        free_vector(K_x_half, 1, 2 * FW);
        free_vector(alpha_prime_x_half, 1, 2 * FW);
        free_vector(a_x_half, 1, 2 * FW);
        free_vector(b_x_half, 1, 2 * FW);

        free_vector(K_y, 1, 2 * FW);
        free_vector(alpha_prime_y, 1, 2 * FW);
        free_vector(a_y, 1, 2 * FW);
        free_vector(b_y, 1, 2 * FW);
        free_vector(K_y_half, 1, 2 * FW);
        free_vector(alpha_prime_y_half, 1, 2 * FW);
        free_vector(a_y_half, 1, 2 * FW);
        free_vector(b_y_half, 1, 2 * FW);

        free_vector(K_z, 1, 2 * FW);
        free_vector(alpha_prime_z, 1, 2 * FW);
        free_vector(a_z, 1, 2 * FW);
        free_vector(b_z, 1, 2 * FW);
        free_vector(K_z_half, 1, 2 * FW);
        free_vector(alpha_prime_z_half, 1, 2 * FW);
        free_vector(a_z_half, 1, 2 * FW);
        free_vector(b_z_half, 1, 2 * FW);

        free_f3tensor(psi_sxx_x, 1, NY, 1, 2 * FW, 1, NZ);
        free_f3tensor(psi_syy_y, 1, 2 * FW, 1, NX, 1, NZ);
        free_f3tensor(psi_szz_z, 1, NY, 1, NX, 1, 2 * FW);
        free_f3tensor(psi_sxy_x, 1, NY, 1, 2 * FW, 1, NZ);
        free_f3tensor(psi_sxy_y, 1, 2 * FW, 1, NX, 1, NZ);
        free_f3tensor(psi_sxz_x, 1, NY, 1, 2 * FW, 1, NZ);
        free_f3tensor(psi_sxz_z, 1, NY, 1, NX, 1, 2 * FW);
        free_f3tensor(psi_syz_y, 1, 2 * FW, 1, NX, 1, NZ);
        free_f3tensor(psi_syz_z, 1, NY, 1, NX, 1, 2 * FW);

        free_f3tensor(psi_vxx, 1, NY, 1, 2 * FW, 1, NZ);
        free_f3tensor(psi_vyy, 1, 2 * FW, 1, NX, 1, NZ);
        free_f3tensor(psi_vzz, 1, NY, 1, NX, 1, 2 * FW);
        free_f3tensor(psi_vxy, 1, 2 * FW, 1, NX, 1, NZ);
        free_f3tensor(psi_vyx, 1, NY, 1, 2 * FW, 1, NZ);
        free_f3tensor(psi_vxz, 1, NY, 1, NX, 1, 2 * FW);
        free_f3tensor(psi_vzx, 1, NY, 1, 2 * FW, 1, NZ);
        free_f3tensor(psi_vyz, 1, NY, 1, NX, 1, 2 * FW);
        free_f3tensor(psi_vzy, 1, 2 * FW, 1, NX, 1, NZ);
    }

    if (L)
    {
        free_f3tensor(r.xx, 1, NY, 1, NX, 1, NZ);
        free_f3tensor(r.yy, 1, NY, 1, NX, 1, NZ);
        free_f3tensor(r.zz, 1, NY, 1, NX, 1, NZ);
        free_f3tensor(r.xy, 1, NY, 1, NX, 1, NZ);
        free_f3tensor(r.yz, 1, NY, 1, NX, 1, NZ);
        free_f3tensor(r.xz, 1, NY, 1, NX, 1, NZ);
        if (FDORDER_TIME > 2)
        {
            free_f3tensor(rxx_2, 1, NY, 1, NX, 1, NZ);
            free_f3tensor(ryy_2, 1, NY, 1, NX, 1, NZ);
            free_f3tensor(rzz_2, 1, NY, 1, NX, 1, NZ);
            free_f3tensor(rxy_2, 1, NY, 1, NX, 1, NZ);
            free_f3tensor(ryz_2, 1, NY, 1, NX, 1, NZ);
            free_f3tensor(rxz_2, 1, NY, 1, NX, 1, NZ);
            free_f3tensor(rxx_3, 1, NY, 1, NX, 1, NZ);
            free_f3tensor(ryy_3, 1, NY, 1, NX, 1, NZ);
            free_f3tensor(rzz_3, 1, NY, 1, NX, 1, NZ);
            free_f3tensor(rxy_3, 1, NY, 1, NX, 1, NZ);
            free_f3tensor(ryz_3, 1, NY, 1, NX, 1, NZ);
            free_f3tensor(rxz_3, 1, NY, 1, NX, 1, NZ);
            if (FDORDER_TIME == 4)
            {
                free_f3tensor(rxx_4, 1, NY, 1, NX, 1, NZ);
                free_f3tensor(ryy_4, 1, NY, 1, NX, 1, NZ);
                free_f3tensor(rzz_4, 1, NY, 1, NX, 1, NZ);
                free_f3tensor(rxy_4, 1, NY, 1, NX, 1, NZ);
                free_f3tensor(ryz_4, 1, NY, 1, NX, 1, NZ);
                free_f3tensor(rxz_4, 1, NY, 1, NX, 1, NZ);
            }
        }
        free_f3tensor(taus, 0, NY + 1, 0, NX + 1, 0, NZ + 1);
        free_f3tensor(taup, 0, NY + 1, 0, NX + 1, 0, NZ + 1);
        free_vector(eta, 1, L);
        free_f3tensor(tausipjp, 1, NY, 1, NX, 1, NZ);
        free_f3tensor(tausjpkp, 1, NY, 1, NX, 1, NZ);
        free_f3tensor(tausipkp, 1, NY, 1, NX, 1, NZ);
    }

    //isotropic parameters releasing

    free_f3tensor(rho, 0, NY + 1, 0, NX + 1, 0, NZ + 1);
    free_f3tensor(pi, 0, NY + 1, 0, NX + 1, 0, NZ + 1);
    free_f3tensor(u, 0, NY + 1, 0, NX + 1, 0, NZ + 1);

    //anisotropic parameters releasing

    free_f3tensor(C11, 0, NY + 1, 0, NX + 1, 0, NZ + 1);
    free_f3tensor(C12, 0, NY + 1, 0, NX + 1, 0, NZ + 1);
    free_f3tensor(C13, 0, NY + 1, 0, NX + 1, 0, NZ + 1);
    free_f3tensor(C22, 0, NY + 1, 0, NX + 1, 0, NZ + 1);
    free_f3tensor(C23, 0, NY + 1, 0, NX + 1, 0, NZ + 1);
    free_f3tensor(C33, 0, NY + 1, 0, NX + 1, 0, NZ + 1);
    free_f3tensor(C44, 0, NY + 1, 0, NX + 1, 0, NZ + 1);
    free_f3tensor(C55, 0, NY + 1, 0, NX + 1, 0, NZ + 1);
    free_f3tensor(C66, 0, NY + 1, 0, NX + 1, 0, NZ + 1);
    free_f3tensor(absorb_coeff, 1, NY, 1, NX, 1, NZ);

    /* averaged material parameters */
    free_f3tensor(C66ipjp, 1, NY, 1, NX, 1, NZ);
    free_f3tensor(C44jpkp, 1, NY, 1, NX, 1, NZ);
    free_f3tensor(C55ipkp, 1, NY, 1, NX, 1, NZ);

    free_f3tensor(rjp, 1, NY, 1, NX, 1, NZ);
    free_f3tensor(rkp, 1, NY, 1, NX, 1, NZ);
    free_f3tensor(rip, 1, NY, 1, NX, 1, NZ);

    if (nsrc_loc > 0)
    {
        free_matrix(signals, 1, nsrc_loc, 1, NT);
        free_matrix(srcpos_loc, 1, 6, 1, nsrc_loc);
    }

    free_f3tensor(bufferlef_to_rig, 1, NY, 1, NZ, 1, nf1);
    free_f3tensor(bufferrig_to_lef, 1, NY, 1, NZ, 1, nf2);
    free_f3tensor(buffertop_to_bot, 1, NX, 1, NZ, 1, nf1);
    free_f3tensor(bufferbot_to_top, 1, NX, 1, NZ, 1, nf2);
    free_f3tensor(bufferfro_to_bac, 1, NY, 1, NX, 1, nf1);
    free_f3tensor(bufferbac_to_fro, 1, NY, 1, NX, 1, nf2);

    free_f3tensor(sbufferlef_to_rig, 1, NY, 1, NZ, 1, nf2);
    free_f3tensor(sbufferrig_to_lef, 1, NY, 1, NZ, 1, nf1);
    free_f3tensor(sbuffertop_to_bot, 1, NX, 1, NZ, 1, nf2);
    free_f3tensor(sbufferbot_to_top, 1, NX, 1, NZ, 1, nf1);
    free_f3tensor(sbufferfro_to_bac, 1, NY, 1, NX, 1, nf2);
    free_f3tensor(sbufferbac_to_fro, 1, NY, 1, NX, 1, nf1);

    /* free memory for global receiver source positions */
    if (SEISMO > 0)
    {
        free_imatrix(recpos, 1, 3, 1, ntr_glob);
    }

    /* free memory for global source positions */
    free_matrix(srcpos, 1, 6, 1, nsrc);

    if ((ntr > 0) && (SEISMO > 0))
    {
        free_imatrix(recpos_loc, 1, 3, 1, ntr);
        free_matrix(seismo_fulldata, 1, ntr_glob, 1, ns);

        switch (SEISMO)
        {
            case 1: /* particle velocities only */
                free_matrix(sectionvx, 1, ntr, 1, ns);
                free_matrix(sectionvy, 1, ntr, 1, ns);
                free_matrix(sectionvz, 1, ntr, 1, ns);
                break;
            case 2: /* pressure only */
                free_matrix(sectionp, 1, ntr, 1, ns);
                break;
            case 3: /* curl and div only */
                free_matrix(sectioncurl, 1, ntr, 1, ns);
                free_matrix(sectiondiv, 1, ntr, 1, ns);
                break;
            case 4: /* everything */
                free_matrix(sectionvx, 1, ntr, 1, ns);
                free_matrix(sectionvy, 1, ntr, 1, ns);
                free_matrix(sectionvz, 1, ntr, 1, ns);
                free_matrix(sectionp, 1, ntr, 1, ns);
                free_matrix(sectioncurl, 1, ntr, 1, ns);
                free_matrix(sectiondiv, 1, ntr, 1, ns);
                break;
        }
    }

    /* free memory for source position definition */
    free_matrix(srcpos1, 1, 6, 1, 1);
    free_ivector(stype, 1, nsrc);
    free_ivector(stype_loc, 1, nsrc);

    /* de-allocate buffer for messages */
    MPI_Buffer_detach(buff_addr, &buffsize);

    MPI_Barrier(MPI_COMM_WORLD);

    /* merge snapshot files created by the PEs into one file */
    /* if ((SNAP) && (MYID==0)) snapmerge(nsnap);*/

    free_ivector(xb, 0, 1);
    free_ivector(yb, 0, 1);
    free_ivector(zb, 0, 1);

    /* free timing arrays */
    free_dvector(time_v_update, 1, NT);
    free_dvector(time_s_update, 1, NT);
    free_dvector(time_s_exchange, 1, NT);
    free_dvector(time_v_exchange, 1, NT);
    free_dvector(time_timestep, 1, NT);

    if (MYID == 0)
    {
        fprintf(FP, "\n **Info from main (written by PE %d): \n", MYID);
        time4 = MPI_Wtime();
        fprintf(FP, " Total real time of program: %4.2f seconds.\n\n", time4 - time1);
        fprintf(FP, " ******************************************************\n");
        fprintf(FP, " **************** ASOFI3D has finished *****************\n");
        fprintf(FP, " ******************************************************\n\n");
    }

    fclose(FP);

    MPI_Finalize();

    return 0;
}