#ifndef PATH_PLANNER_H
#define PATH_PLANNER_H

#include <uquad_aux_math.h>

typedef enum path_type{
    HOVER = 0,
    STRAIGHT,
    CIRCULAR,
    PATH_TYPE_COUNT
}path_type_t;

typedef struct set_point{
    uquad_mat_t *x;
    uquad_mat_t *w;
}set_point_t;

typedef struct path_planner{
    path_type_t pt;
    set_point_t *sp;
    uquad_mat_t *A;
    uquad_mat_t *Acirc;
    uquad_mat_t *B;
    double Ts;
    uquad_mat_t *Qhov;
    uquad_mat_t *Rhov;
    uquad_mat_t *Qrec;
    uquad_mat_t *Rrec;
    uquad_mat_t *Qcirc;
    uquad_mat_t *Rcirc;
    uquad_mat_t *Q;
    uquad_mat_t *R;
    uquad_mat_t *K;
}path_planner_t;

path_planner_t *pp_init(void);

int pp_update_setpoint(path_planner_t *pp, uquad_mat_t *x);

void pp_deinit(path_planner_t *pp);

int pp_update_K(path_planner_t *pp);

int pp_lqr(uquad_mat_t *K, uquad_mat_t *phi, uquad_mat_t *gamma, uquad_mat_t *Q, uquad_mat_t *R);
int pp_disc(uquad_mat_t *phi,uquad_mat_t *gamma,uquad_mat_t *A,uquad_mat_t *B, double Ts);
#endif //PATH_PLANNER_H

int pp_lin_model(uquad_mat_t *A, uquad_mat_t *B, path_type_t pt, set_point_t *sp);
